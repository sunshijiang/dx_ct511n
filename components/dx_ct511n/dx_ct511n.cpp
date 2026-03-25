#include "dx_ct511n.h"

#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#include <cstdio>

namespace esphome {
namespace dx_ct511n {

static const char *const TAG = "dx_ct511n";

static const char ASCII_CR = 0x0D;
static const char ASCII_LF = 0x0A;

void DXCT511NGPSSwitch::write_state(bool state) { this->parent_->request_gps_power(state); }

void DXCT511NComponent::setup() {
  this->step_ = SetupStep::STEP_AT;
  this->subscribe_index_ = 0;
  this->retry_count_ = 0;
  this->ready_emitted_ = false;
  this->set_network_connected_(false);
  this->set_mqtt_connected_(false);
}

void DXCT511NComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "DX-CT511N:");
  ESP_LOGCONFIG(TAG, "  Broker: %s:%u", this->broker_.c_str(), this->port_);
  ESP_LOGCONFIG(TAG, "  Client ID: %s", this->client_id_.c_str());
  ESP_LOGCONFIG(TAG, "  APN: %s", this->apn_.c_str());
  ESP_LOGCONFIG(TAG, "  Keepalive: %u", this->keepalive_);
  ESP_LOGCONFIG(TAG, "  Subscribe topics: %u", static_cast<unsigned>(this->subscribe_topics_.size()));
  LOG_SENSOR("  ", "Signal Quality", this->signal_quality_sensor_);
  LOG_SENSOR("  ", "RSSI dBm", this->rssi_dbm_sensor_);
  LOG_SENSOR("  ", "Bit Error Rate", this->bit_error_rate_sensor_);
  LOG_TEXT_SENSOR("  ", "Last Response", this->last_response_text_sensor_);
  LOG_TEXT_SENSOR("  ", "Last Topic", this->last_topic_text_sensor_);
  LOG_TEXT_SENSOR("  ", "Last Payload", this->last_payload_text_sensor_);
  LOG_TEXT_SENSOR("  ", "GNSS Sentence", this->gnss_sentence_text_sensor_);
  LOG_BINARY_SENSOR("  ", "MQTT Connected", this->mqtt_connected_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Network Connected", this->network_connected_binary_sensor_);
}

void DXCT511NComponent::loop() {
  while (this->available()) {
    uint8_t byte;
    if (!this->read_byte(&byte))
      break;

    const char c = static_cast<char>(byte);
    if (c != ASCII_CR) {
      this->process_json_stream_char_(c);
    }

    if (c == ASCII_CR)
      continue;

    if (c == ASCII_LF) {
      if (!this->line_buffer_.empty()) {
        this->process_line_(this->line_buffer_);
        this->line_buffer_.clear();
      }
      continue;
    }

    if (this->line_buffer_.size() < 1024) {
      this->line_buffer_.push_back(c);
    } else {
      this->line_buffer_.clear();
    }
  }

  if (this->pending_active_) {
    if (millis() - this->pending_started_ > this->pending_.timeout_ms) {
      ESP_LOGW(TAG, "Command timeout: %s", this->pending_.command.c_str());
      this->finish_pending_(false);
    }
    return;
  }

  this->process_idle_();
}

void DXCT511NComponent::update() {
  if (!this->mqtt_connected_) {
    return;
  }
  if (this->signal_quality_sensor_ != nullptr || this->rssi_dbm_sensor_ != nullptr ||
      this->bit_error_rate_sensor_ != nullptr) {
    this->queue_.push_back(
        CommandRequest{"AT+CSQ", {"+CSQ:", "OK"}, this->command_timeout_ms_, CommandKind::KIND_CSQ});
  }
}

void DXCT511NComponent::process_line_(const std::string &line) {
  ESP_LOGV(TAG, "RX: %s", line.c_str());
  if (this->last_response_text_sensor_ != nullptr) {
    this->last_response_text_sensor_->publish_state(line);
  }

  if (line.rfind("$", 0) == 0) {
    this->handle_nmea_sentence_(line);
  }

  if (line.find("+MGPSC:") != std::string::npos) {
    this->set_gps_enabled_(line.find("1") != std::string::npos);
  }

  if (this->pending_active_) {
    if (!this->pending_response_.empty()) {
      this->pending_response_.append("\n");
    }
    this->pending_response_.append(line);

    if (this->response_contains_expected_(this->pending_response_)) {
      this->finish_pending_(true);
      return;
    }
    if (this->is_error_response_(line) && !this->response_contains_expected_(line)) {
      this->finish_pending_(false);
      return;
    }
  }

  this->handle_message_line_(line);
}

void DXCT511NComponent::process_json_stream_char_(char c) {
  if (!this->json_collecting_) {
    if (c == '{') {
      this->json_collecting_ = true;
      this->json_depth_ = 1;
      this->json_buffer_.clear();
      this->json_buffer_.push_back(c);
    }
    return;
  }

  if (this->json_buffer_.size() >= 1024) {
    this->json_collecting_ = false;
    this->json_depth_ = 0;
    this->json_buffer_.clear();
    return;
  }

  this->json_buffer_.push_back(c);
  if (c == '{')
    this->json_depth_++;
  if (c == '}')
    this->json_depth_--;

  if (this->json_depth_ == 0) {
    this->handle_json_payload_(this->json_buffer_);
    this->json_collecting_ = false;
    this->json_buffer_.clear();
  }
}

void DXCT511NComponent::process_idle_() {
  if (this->reconnect_at_ != 0 && millis() < this->reconnect_at_) {
    return;
  }
  if (this->reconnect_at_ != 0 && millis() >= this->reconnect_at_) {
    this->reconnect_at_ = 0;
    this->step_ = SetupStep::STEP_AT;
  }

  if (this->step_ != SetupStep::STEP_READY) {
    this->start_setup_command_();
    return;
  }

  if (!this->queue_.empty()) {
    auto request = this->queue_.front();
    this->queue_.erase(this->queue_.begin());
    this->send_command_(request);
  }
}

void DXCT511NComponent::start_setup_command_() {
  CommandRequest request;
  request.kind = CommandKind::KIND_SETUP;
  request.timeout_ms = this->command_timeout_ms_;

  switch (this->step_) {
    case SetupStep::STEP_AT:
      request.command = "AT";
      request.expects = {"OK"};
      break;
    case SetupStep::STEP_ATE1:
      request.command = "ATE1";
      request.expects = {"OK"};
      break;
    case SetupStep::STEP_APN:
      request.command = "AT+QICSGP=1,1,\"" + this->apn_ + "\",\"\",\"\"";
      request.expects = {"OK"};
      break;
    case SetupStep::STEP_NETOPEN:
      request.command = "AT+NETOPEN";
      request.timeout_ms = 10000;
      request.expects = {"OK", "+NETOPEN: SUCCESS", "ERROR: 902"};
      break;
    case SetupStep::STEP_MCONFIG:
      request.command = "AT+MCONFIG=\"" + this->client_id_ + "\"";
      request.expects = {"OK"};
      break;
    case SetupStep::STEP_MIPSTART:
      request.command = "AT+MIPSTART=\"" + this->broker_ + "\"," + to_string(this->port_) + ",4";
      request.timeout_ms = 10000;
      request.expects = {"OK", "ALREADY", "CONNECT"};
      break;
    case SetupStep::STEP_MCONNECT:
      request.command = "AT+MCONNECT=1," + to_string(this->keepalive_);
      request.timeout_ms = 8000;
      request.expects = {"OK", "CONNECTED", "SUCCESS", "ALREADY"};
      break;
    case SetupStep::STEP_SUBSCRIBE:
      if (this->subscribe_index_ >= this->subscribe_topics_.size()) {
        this->step_ = SetupStep::STEP_READY;
        this->set_mqtt_connected_(true);
        return;
      }
      request.command = "AT+MSUB=\"" + this->subscribe_topics_[this->subscribe_index_] + "\",0";
      request.expects = {"OK", "SUCCESS", "ALREADY"};
      break;
    case SetupStep::STEP_READY:
    case SetupStep::STEP_IDLE:
      return;
  }

  this->send_command_(request);
}

void DXCT511NComponent::send_command_(const CommandRequest &request) {
  this->pending_ = request;
  this->pending_active_ = true;
  this->pending_response_.clear();
  this->pending_started_ = millis();
  ESP_LOGD(TAG, "TX: %s", request.command.c_str());
  this->write_str(request.command.c_str());
  this->write_byte(ASCII_CR);
  this->write_byte(ASCII_LF);
}

void DXCT511NComponent::finish_pending_(bool success) {
  auto kind = this->pending_.kind;
  auto command = this->pending_.command;
  auto desired_state = this->pending_.desired_state;
  auto response = this->pending_response_;

  this->pending_active_ = false;
  this->pending_response_.clear();

  if (kind == CommandKind::KIND_SETUP) {
    if (!success) {
      if (++this->retry_count_ >= this->max_retries_) {
        ESP_LOGW(TAG, "Setup failed at step %d, scheduling reconnect", static_cast<int>(this->step_));
        this->schedule_reconnect_();
      }
      return;
    }

    this->retry_count_ = 0;
    switch (this->step_) {
      case SetupStep::STEP_AT:
        this->step_ = SetupStep::STEP_ATE1;
        break;
      case SetupStep::STEP_ATE1:
        this->step_ = SetupStep::STEP_APN;
        break;
      case SetupStep::STEP_APN:
        this->step_ = SetupStep::STEP_NETOPEN;
        break;
      case SetupStep::STEP_NETOPEN:
        this->set_network_connected_(true);
        this->step_ = SetupStep::STEP_MCONFIG;
        break;
      case SetupStep::STEP_MCONFIG:
        this->step_ = SetupStep::STEP_MIPSTART;
        break;
      case SetupStep::STEP_MIPSTART:
        this->step_ = SetupStep::STEP_MCONNECT;
        break;
      case SetupStep::STEP_MCONNECT:
        this->subscribe_index_ = 0;
        this->step_ = SetupStep::STEP_SUBSCRIBE;
        break;
      case SetupStep::STEP_SUBSCRIBE:
        this->subscribe_index_++;
        break;
      case SetupStep::STEP_READY:
      case SetupStep::STEP_IDLE:
        break;
    }
    return;
  }

  if (kind == CommandKind::KIND_CSQ && success) {
    this->parse_csq_(response);
    return;
  }

  if (kind == CommandKind::KIND_GPS_POWER && success) {
    this->set_gps_enabled_(desired_state);
    if (this->gps_switch_ != nullptr) {
      this->gps_switch_->publish_state(desired_state);
    }
    return;
  }

  if (kind == CommandKind::KIND_MDISCONNECT && success) {
    this->set_mqtt_connected_(false);
    return;
  }

  if (kind == CommandKind::KIND_MIPCLOSE && success) {
    this->set_network_connected_(false);
    this->set_mqtt_connected_(false);
    return;
  }

  if (kind == CommandKind::KIND_PUBLISH && !success) {
    ESP_LOGW(TAG, "Publish failed for command: %s", command.c_str());
    this->set_mqtt_connected_(false);
    this->step_ = SetupStep::STEP_MCONNECT;
  }
}

void DXCT511NComponent::schedule_reconnect_() {
  this->pending_active_ = false;
  this->queue_.clear();
  this->set_network_connected_(false);
  this->set_mqtt_connected_(false);
  this->subscribe_index_ = 0;
  this->step_ = SetupStep::STEP_AT;
  this->reconnect_at_ = millis() + this->reconnect_interval_ms_;
  this->retry_count_ = 0;
}

void DXCT511NComponent::set_network_connected_(bool connected) {
  this->network_connected_ = connected;
  if (this->network_connected_binary_sensor_ != nullptr) {
    this->network_connected_binary_sensor_->publish_state(connected);
  }
}

void DXCT511NComponent::set_mqtt_connected_(bool connected) {
  const bool changed = this->mqtt_connected_ != connected;
  this->mqtt_connected_ = connected;
  if (this->mqtt_connected_binary_sensor_ != nullptr) {
    this->mqtt_connected_binary_sensor_->publish_state(connected);
  }
  if (connected && changed && !this->ready_emitted_) {
    this->ready_emitted_ = true;
    this->ready_callback_.call();
  }
  if (!connected) {
    this->ready_emitted_ = false;
  }
}

void DXCT511NComponent::set_gps_enabled_(bool enabled) {
  this->gps_enabled_ = enabled;
  if (this->gps_switch_ != nullptr) {
    this->gps_switch_->publish_state(enabled);
  }
}

void DXCT511NComponent::handle_message_line_(const std::string &line) {
  const auto json_start = line.find('{');
  const auto json_end = line.rfind('}');
  if (json_start != std::string::npos && json_end != std::string::npos && json_end > json_start) {
    this->handle_json_payload_(line.substr(json_start, json_end - json_start + 1));
  }

  if (line.find("+MSUB:") == std::string::npos)
    return;

  std::string topic;
  std::string payload;
  size_t first = line.find('"');
  if (first != std::string::npos) {
    size_t second = line.find('"', first + 1);
    if (second != std::string::npos) {
      topic = line.substr(first + 1, second - first - 1);
      size_t third = line.find('"', second + 1);
      size_t fourth = line.rfind('"');
      if (third != std::string::npos && fourth != std::string::npos && fourth > third) {
        payload = line.substr(third + 1, fourth - third - 1);
      }
    }
  }

  if (!topic.empty()) {
    if (this->last_topic_text_sensor_ != nullptr) {
      this->last_topic_text_sensor_->publish_state(topic);
    }
    if (!payload.empty() && this->last_payload_text_sensor_ != nullptr) {
      this->last_payload_text_sensor_->publish_state(payload);
    }
    this->mqtt_message_callback_.call(topic, payload);
  }
}

void DXCT511NComponent::handle_json_payload_(const std::string &payload) {
  if (this->last_payload_text_sensor_ != nullptr) {
    this->last_payload_text_sensor_->publish_state(payload);
  }
  this->json_message_callback_.call(payload);
}

void DXCT511NComponent::handle_nmea_sentence_(const std::string &sentence) {
  if (this->gnss_sentence_text_sensor_ != nullptr) {
    this->gnss_sentence_text_sensor_->publish_state(sentence);
  }
  this->nmea_sentence_callback_.call(sentence);
}

void DXCT511NComponent::parse_csq_(const std::string &response) {
  const auto pos = response.find("+CSQ:");
  if (pos == std::string::npos)
    return;

  int csq = -1;
  int ber = -1;
  if (sscanf(response.c_str() + pos, "+CSQ: %d,%d", &csq, &ber) != 2) {
    return;
  }

  int rssi_dbm = -999;
  if (csq >= 0 && csq <= 31) {
    rssi_dbm = -113 + 2 * csq;
  }

  if (this->signal_quality_sensor_ != nullptr) {
    this->signal_quality_sensor_->publish_state(csq);
  }
  if (this->rssi_dbm_sensor_ != nullptr) {
    this->rssi_dbm_sensor_->publish_state(rssi_dbm);
  }
  if (this->bit_error_rate_sensor_ != nullptr) {
    this->bit_error_rate_sensor_->publish_state(ber);
  }
}

bool DXCT511NComponent::response_contains_expected_(const std::string &response) const {
  for (const auto &token : this->pending_.expects) {
    if (!token.empty() && response.find(token) != std::string::npos) {
      return true;
    }
  }
  return false;
}

bool DXCT511NComponent::is_error_response_(const std::string &response) const {
  return response.find("ERROR") != std::string::npos || response.find("+CME ERROR") != std::string::npos ||
         response.find("+CMS ERROR") != std::string::npos;
}

std::string DXCT511NComponent::escape_at_string_(const std::string &input) const {
  std::string out;
  out.reserve(input.size() + 8);
  for (char c : input) {
    if (c == '\\' || c == '"') {
      out.push_back('\\');
      out.push_back(c);
      continue;
    }
    if (static_cast<unsigned char>(c) < 0x20 && c != '\t') {
      continue;
    }
    out.push_back(c);
  }
  return out;
}

void DXCT511NComponent::publish_message(const std::string &topic, const std::string &payload, uint8_t qos,
                                        bool retain) {
  const auto escaped = this->escape_at_string_(payload);
  this->queue_.push_back(CommandRequest{"AT+MPUB=\"" + topic + "\"," + to_string(qos) + "," +
                                            to_string(retain ? 1 : 0) + ",\"" + escaped + "\"",
                                        {"OK"}, this->command_timeout_ms_, CommandKind::KIND_PUBLISH});
}

void DXCT511NComponent::send_raw_at(const std::string &command, uint32_t timeout_ms) {
  auto actual_timeout = timeout_ms == 0 ? this->command_timeout_ms_ : timeout_ms;
  this->queue_.push_back(CommandRequest{command, {"OK"}, actual_timeout, CommandKind::KIND_RAW_AT});
}

void DXCT511NComponent::request_reconnect() { this->schedule_reconnect_(); }

void DXCT511NComponent::request_gps_power(bool state) {
  this->queue_.push_back(CommandRequest{"AT+MGPSC=" + to_string(state ? 1 : 0), {"OK"}, this->command_timeout_ms_,
                                        CommandKind::KIND_GPS_POWER, state});
}

void DXCT511NComponent::request_unsubscribe(const std::string &topic_filter) {
  this->queue_.push_back(CommandRequest{"AT+MUNSUB=\"" + topic_filter + "\"",
                                        {"OK", "SUCCESS"}, this->command_timeout_ms_, CommandKind::KIND_MUNSUB});
}

void DXCT511NComponent::request_disconnect() {
  this->queue_.push_back(
      CommandRequest{"AT+MDISCONNECT", {"OK", "SUCCESS"}, this->command_timeout_ms_, CommandKind::KIND_MDISCONNECT});
}

void DXCT511NComponent::request_close() {
  this->queue_.push_back(
      CommandRequest{"AT+MIPCLOSE", {"OK", "SUCCESS"}, this->command_timeout_ms_, CommandKind::KIND_MIPCLOSE});
}

}  // namespace dx_ct511n
}  // namespace esphome
