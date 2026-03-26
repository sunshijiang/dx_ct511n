# 4G-Only YAML Example

This example removes Wi-Fi, captive portal, native API, and OTA over Wi-Fi.

Use this mode when the ESP32 only talks through the `DX-CT511N` module over UART and all remote data exchange is done through the module's MQTT connection.

Important limitations:

- Home Assistant native `api:` is not used in this mode
- standard ESPHome OTA over Wi-Fi is not used in this mode
- device telemetry and remote control should be implemented through `dx_ct511n.publish`, `dx_ct511n.publish_long`, and subscribed MQTT topics
- serial `logger:` can still be used for local debugging

```yaml
substitutions:
  device_name: ct511n_4g_only

  mqtt_broker: "broker.emqx.io"
  mqtt_port: "1883"
  mqtt_client_id: "4G TEST"
  mqtt_username: "your_user"
  mqtt_password: "your_pass"
  mqtt_apn: "cmnbiot"

  topic_led_cmd: "/topic/led"
  topic_led_status: "/topic/led/status"
  topic_sensor: "/topic/sensor"
  topic_gps_cmd: "/topic/gps"

esphome:
  name: ${device_name}
  friendly_name: ${device_name}

esp32:
  board: esp32-c3-devkitm-1
  framework:
    type: esp-idf

logger:

external_components:
  - source:
      type: local
      path: D:/esphome/esphome/components/dx_ct511n/components
    components: [dx_ct511n]

uart:
  id: uart_ct511n
  tx_pin: GPIO21
  rx_pin: GPIO20
  baud_rate: 115200

dx_ct511n:
  id: modem
  uart_id: uart_ct511n
  apn: ${mqtt_apn}
  broker: ${mqtt_broker}
  port: ${mqtt_port}
  client_id: ${mqtt_client_id}
  username: ${mqtt_username}
  password: ${mqtt_password}
  keepalive: 60
  subscribe_topics:
    - ${topic_led_cmd}
    - ${topic_gps_cmd}

  signal_quality:
    name: CT511N CSQ
  rssi_dbm:
    name: CT511N RSSI
  bit_error_rate:
    name: CT511N BER
  last_response:
    name: CT511N Last Response
  last_topic:
    name: CT511N Last Topic
  last_payload:
    name: CT511N Last Payload
  mqtt_connected:
    name: CT511N MQTT Connected
  network_connected:
    name: CT511N Network Connected
  gps:
    id: gps_power
    name: CT511N GPS Power

  on_ready:
    then:
      - script.execute: publish_led_state
      - script.execute: publish_sensor_state

  on_mqtt_message:
    then:
      - lambda: |-
          if (topic == "${topic_led_cmd}") {
            auto apply_led = [&](auto *sw, const char *key) {
              std::string on_pat = std::string("\"") + key + "\":\"on\"";
              std::string off_pat = std::string("\"") + key + "\":\"off\"";

              if (payload.find(on_pat) != std::string::npos) {
                sw->turn_on();
              } else if (payload.find(off_pat) != std::string::npos) {
                sw->turn_off();
              }
            };

            apply_led(id(led1_sw), "led1");
            apply_led(id(led2_sw), "led2");
            apply_led(id(led3_sw), "led3");
            id(publish_led_state).execute();
          }

          if (topic == "${topic_gps_cmd}") {
            if (payload.find("\"gps_switch\":\"on\"") != std::string::npos) {
              id(gps_power).turn_on();
            } else if (payload.find("\"gps_switch\":\"off\"") != std::string::npos) {
              id(gps_power).turn_off();
            }
          }

switch:
  - platform: gpio
    id: led1_sw
    name: LED1
    pin: GPIO2
    restore_mode: ALWAYS_OFF
    on_turn_on:
      - script.execute: publish_led_state
    on_turn_off:
      - script.execute: publish_led_state

  - platform: gpio
    id: led2_sw
    name: LED2
    pin: GPIO3
    restore_mode: ALWAYS_OFF
    on_turn_on:
      - script.execute: publish_led_state
    on_turn_off:
      - script.execute: publish_led_state

  - platform: gpio
    id: led3_sw
    name: LED3
    pin: GPIO4
    restore_mode: ALWAYS_OFF
    on_turn_on:
      - script.execute: publish_led_state
    on_turn_off:
      - script.execute: publish_led_state

sensor:
  - platform: dht
    pin: GPIO10
    model: DHT11
    temperature:
      id: dht_temperature
      name: DHT11 Temperature
    humidity:
      id: dht_humidity
      name: DHT11 Humidity
    update_interval: 30s

script:
  - id: publish_led_state
    mode: restart
    then:
      - dx_ct511n.publish:
          id: modem
          topic: ${topic_led_status}
          payload: !lambda |-
            char out[128];
            snprintf(out, sizeof(out),
                     "{\"led1\":\"%s\",\"led2\":\"%s\",\"led3\":\"%s\"}",
                     id(led1_sw).state ? "on" : "off",
                     id(led2_sw).state ? "on" : "off",
                     id(led3_sw).state ? "on" : "off");
            return std::string(out);

  - id: publish_sensor_state
    mode: restart
    then:
      - if:
          condition:
            lambda: |-
              return !isnan(id(dht_temperature).state) && !isnan(id(dht_humidity).state);
          then:
            - dx_ct511n.publish:
                id: modem
                topic: ${topic_sensor}
                payload: !lambda |-
                  char out[128];
                  snprintf(out, sizeof(out),
                           "{\"temperature\":%.2f,\"humidity\":%.2f}",
                           id(dht_temperature).state,
                           id(dht_humidity).state);
                  return std::string(out);

interval:
  - interval: 30s
    then:
      - script.execute: publish_sensor_state
```
