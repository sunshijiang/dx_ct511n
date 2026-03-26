#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace esphome {
namespace dx_ct511n {

class DXCT511NComponent;

class DXCT511NGPSSwitch : public switch_::Switch, public Parented<DXCT511NComponent> {
 protected:
  void write_state(bool state) override;
};

class DXCT511NComponent : public PollingComponent, public uart::UARTDevice {
 public:
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  void setup() override;
  void dump_config() override;
  void loop() override;
  void update() override;

  void set_broker(const std::string &broker) { this->broker_ = broker; }
  void set_port(uint16_t port) { this->port_ = port; }
  void set_client_id(const std::string &client_id) { this->client_id_ = client_id; }
  void set_username(const std::string &username) { this->username_ = username; }
  void set_password(const std::string &password) { this->password_ = password; }
  void set_apn(const std::string &apn) { this->apn_ = apn; }
  void set_keepalive(uint16_t keepalive) { this->keepalive_ = keepalive; }
  void set_clean_session(bool clean_session) { this->clean_session_ = clean_session; }
  void set_command_timeout(uint32_t timeout_ms) { this->command_timeout_ms_ = timeout_ms; }
  void set_reconnect_interval(uint32_t interval_ms) { this->reconnect_interval_ms_ = interval_ms; }
  void add_subscribe_topic(const std::string &topic, uint8_t qos = 0);

  SUB_SENSOR(signal_quality)
  SUB_SENSOR(rssi_dbm)
  SUB_SENSOR(bit_error_rate)
  SUB_TEXT_SENSOR(last_response)
  SUB_TEXT_SENSOR(last_topic)
  SUB_TEXT_SENSOR(last_payload)
  SUB_TEXT_SENSOR(gnss_sentence)
  SUB_BINARY_SENSOR(mqtt_connected)
  SUB_BINARY_SENSOR(network_connected)
  SUB_SWITCH(gps)

  template<typename F> void add_on_ready_callback(F &&callback) {
    this->ready_callback_.add(std::forward<F>(callback));
  }
  template<typename F> void add_on_mqtt_message_callback(F &&callback) {
    this->mqtt_message_callback_.add(std::forward<F>(callback));
  }
  template<typename F> void add_on_json_message_callback(F &&callback) {
    this->json_message_callback_.add(std::forward<F>(callback));
  }
  template<typename F> void add_on_nmea_sentence_callback(F &&callback) {
    this->nmea_sentence_callback_.add(std::forward<F>(callback));
  }

  void publish_message(const std::string &topic, const std::string &payload, uint8_t qos = 0, bool retain = false);
  void publish_long_message(const std::string &topic, const std::string &payload, uint8_t qos = 0,
                            bool retain = false);
  void request_subscribe(const std::string &topic, uint8_t qos = 0);
  void send_raw_at(const std::string &command, uint32_t timeout_ms = 0);
  void request_reconnect();
  void request_gps_power(bool state);
  void request_unsubscribe(const std::string &topic_filter);
  void request_disconnect();
  void request_close();

 protected:
  enum class SetupStep {
    STEP_IDLE,
    STEP_AT,
    STEP_ATE1,
    STEP_APN,
    STEP_NETOPEN,
    STEP_MCONFIG,
    STEP_MIPSTART,
    STEP_MCONNECT,
    STEP_SUBSCRIBE,
    STEP_READY,
  };

  enum class CommandKind {
    KIND_SETUP,
    KIND_PUBLISH,
    KIND_PUBLISH_LONG_PROMPT,
    KIND_PUBLISH_LONG_RESULT,
    KIND_CSQ,
    KIND_RAW_AT,
    KIND_GPS_POWER,
    KIND_MSUB,
    KIND_MUNSUB,
    KIND_MDISCONNECT,
    KIND_MIPCLOSE,
  };

  struct Subscription {
    std::string topic;
    uint8_t qos{0};
  };

  struct CommandRequest {
    std::string command;
    std::vector<std::string> expects;
    uint32_t timeout_ms{0};
    CommandKind kind{CommandKind::KIND_RAW_AT};
    bool desired_state{false};
    std::string data_payload;
  };

  void process_line_(const std::string &line);
  void process_json_stream_char_(char c);
  void process_idle_();
  void start_setup_command_();
  void send_command_(const CommandRequest &request);
  void finish_pending_(bool success);
  CommandRequest make_subscribe_request_(const Subscription &subscription, CommandKind kind) const;
  void upsert_subscription_(const std::string &topic, uint8_t qos);
  void remove_subscription_(const std::string &topic);
  int find_subscription_index_(const std::string &topic) const;
  void schedule_reconnect_();
  void set_network_connected_(bool connected);
  void set_mqtt_connected_(bool connected);
  void set_gps_enabled_(bool enabled);
  void handle_message_line_(const std::string &line);
  void handle_json_payload_(const std::string &payload);
  void handle_nmea_sentence_(const std::string &sentence);
  std::string unescape_mqtt_payload_(const std::string &payload) const;
  void parse_csq_(const std::string &response);
  bool response_contains_expected_(const std::string &response) const;
  bool is_error_response_(const std::string &response) const;
  std::string escape_at_string_(const std::string &input) const;

  std::string broker_;
  uint16_t port_{1883};
  std::string client_id_;
  std::string username_;
  std::string password_;
  std::string apn_{"cmnbiot"};
  uint16_t keepalive_{60};
  bool clean_session_{true};
  uint32_t command_timeout_ms_{5000};
  uint32_t reconnect_interval_ms_{30000};
  std::vector<Subscription> subscribe_topics_;

  SetupStep step_{SetupStep::STEP_IDLE};
  size_t subscribe_index_{0};
  bool network_connected_{false};
  bool mqtt_connected_{false};
  bool gps_enabled_{false};
  bool ready_emitted_{false};
  uint32_t retry_count_{0};
  uint32_t max_retries_{3};
  uint32_t reconnect_at_{0};

  std::vector<CommandRequest> queue_;
  bool pending_active_{false};
  CommandRequest pending_;
  std::string pending_response_;
  uint32_t pending_started_{0};

  std::string line_buffer_;
  std::string json_buffer_;
  bool json_collecting_{false};
  int json_depth_{0};

  CallbackManager<void()> ready_callback_;
  CallbackManager<void(std::string, std::string)> mqtt_message_callback_;
  CallbackManager<void(std::string)> json_message_callback_;
  CallbackManager<void(std::string)> nmea_sentence_callback_;
};

class DXCT511NReadyTrigger : public Trigger<> {
 public:
  explicit DXCT511NReadyTrigger(DXCT511NComponent *parent) {
    parent->add_on_ready_callback([this]() { this->trigger(); });
  }
};

class DXCT511NMQTTMessageTrigger : public Trigger<std::string, std::string> {
 public:
  explicit DXCT511NMQTTMessageTrigger(DXCT511NComponent *parent) {
    parent->add_on_mqtt_message_callback(
        [this](const std::string &topic, const std::string &payload) { this->trigger(topic, payload); });
  }
};

class DXCT511NJSONMessageTrigger : public Trigger<std::string> {
 public:
  explicit DXCT511NJSONMessageTrigger(DXCT511NComponent *parent) {
    parent->add_on_json_message_callback([this](const std::string &payload) { this->trigger(payload); });
  }
};

class DXCT511NNMEASentenceTrigger : public Trigger<std::string> {
 public:
  explicit DXCT511NNMEASentenceTrigger(DXCT511NComponent *parent) {
    parent->add_on_nmea_sentence_callback([this](const std::string &sentence) { this->trigger(sentence); });
  }
};

template<typename... Ts> class DXCT511NPublishAction : public Action<Ts...> {
 public:
  explicit DXCT511NPublishAction(DXCT511NComponent *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(std::string, topic)
  TEMPLATABLE_VALUE(std::string, payload)
  TEMPLATABLE_VALUE(uint8_t, qos)
  TEMPLATABLE_VALUE(bool, retain)

  void play(const Ts &...x) override {
    this->parent_->publish_message(this->topic_.value(x...), this->payload_.value(x...), this->qos_.value(x...),
                                   this->retain_.value(x...));
  }

 protected:
  DXCT511NComponent *parent_;
};

template<typename... Ts> class DXCT511NPublishLongAction : public Action<Ts...> {
 public:
  explicit DXCT511NPublishLongAction(DXCT511NComponent *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(std::string, topic)
  TEMPLATABLE_VALUE(std::string, payload)
  TEMPLATABLE_VALUE(uint8_t, qos)
  TEMPLATABLE_VALUE(bool, retain)

  void play(const Ts &...x) override {
    this->parent_->publish_long_message(this->topic_.value(x...), this->payload_.value(x...), this->qos_.value(x...),
                                        this->retain_.value(x...));
  }

 protected:
  DXCT511NComponent *parent_;
};

template<typename... Ts> class DXCT511NSubscribeAction : public Action<Ts...> {
 public:
  explicit DXCT511NSubscribeAction(DXCT511NComponent *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(std::string, topic)
  TEMPLATABLE_VALUE(uint8_t, qos)

  void play(const Ts &...x) override {
    this->parent_->request_subscribe(this->topic_.value(x...), this->qos_.value(x...));
  }

 protected:
  DXCT511NComponent *parent_;
};

template<typename... Ts> class DXCT511NSendATAction : public Action<Ts...> {
 public:
  explicit DXCT511NSendATAction(DXCT511NComponent *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(std::string, command)
  TEMPLATABLE_VALUE(uint32_t, timeout_ms)

  void play(const Ts &...x) override {
    this->parent_->send_raw_at(this->command_.value(x...), this->timeout_ms_.value(x...));
  }

 protected:
  DXCT511NComponent *parent_;
};

template<typename... Ts> class DXCT511NReconnectAction : public Action<Ts...> {
 public:
  explicit DXCT511NReconnectAction(DXCT511NComponent *parent) : parent_(parent) {}
  void play(const Ts &...x) override { this->parent_->request_reconnect(); }

 protected:
  DXCT511NComponent *parent_;
};

template<typename... Ts> class DXCT511NUnsubscribeAction : public Action<Ts...> {
 public:
  explicit DXCT511NUnsubscribeAction(DXCT511NComponent *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(std::string, topic_filter)

  void play(const Ts &...x) override { this->parent_->request_unsubscribe(this->topic_filter_.value(x...)); }

 protected:
  DXCT511NComponent *parent_;
};

template<typename... Ts> class DXCT511NDisconnectAction : public Action<Ts...> {
 public:
  explicit DXCT511NDisconnectAction(DXCT511NComponent *parent) : parent_(parent) {}
  void play(const Ts &...x) override { this->parent_->request_disconnect(); }

 protected:
  DXCT511NComponent *parent_;
};

template<typename... Ts> class DXCT511NCloseAction : public Action<Ts...> {
 public:
  explicit DXCT511NCloseAction(DXCT511NComponent *parent) : parent_(parent) {}
  void play(const Ts &...x) override { this->parent_->request_close(); }

 protected:
  DXCT511NComponent *parent_;
};

}  // namespace dx_ct511n
}  // namespace esphome
