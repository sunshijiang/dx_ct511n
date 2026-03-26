# DX-CT511N ESPHome External Component

This repository now contains an ESPHome external component for the `DX-CT511N` 4G/GNSS module.

Current implementation focus:

- preserve MQTT server and account settings as YAML config
- preserve UART pin and baud configuration through native ESPHome `uart:`
- provide UART-based 4G + MQTT connection sequence
- provide MQTT publish and subscribe support
- provide long MQTT publish support through `AT+MPUBEX`
- provide GNSS power switch using `AT+MGPSC`
- provide diagnostics for signal quality, modem responses, and NMEA output
- provide automations for MQTT messages, JSON payloads, NMEA sentences, and ready state

The three PDF files are still kept in the repo, but this model cannot read PDF inputs directly in this session, so the implementation is based on:

- your existing `uart_async_rxtxtasks_main.c` prototype
- the AT command flow already present in that file
- ESPHome built-in component structure/conventions
- the normalized notes in `docs/ct511n_uart_mqtt_reference.md`

If you want full parity with every command from the UART application guide, the next step is to provide the key command tables as text. This session cannot directly read the PDF or image inputs.

The current command assumptions extracted from your text notes are documented in `docs/ct511n_uart_mqtt_reference.md` so you can review and correct them.

## Example

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/sunshijiang/dx_ct511n
    components: [dx_ct511n]

uart:
  id: uart_ct511n
  tx_pin: GPIO21
  rx_pin: GPIO20
  baud_rate: 115200

dx_ct511n:
  id: modem
  uart_id: uart_ct511n
  apn: cmnbiot
  broker: 157.3233.fun
  port: 1883
  client_id: 4g_car
  username: 4g_car
  password: "191910"
  subscribe_topics:
    - /topic/led
    - /topic/gps
  on_json_message:
    then:
      - logger.log:
          format: "JSON from module: %s"
          args: [payload.c_str()]
  on_nmea_sentence:
    then:
      - logger.log:
          format: "NMEA: %s"
          args: [sentence.c_str()]

  gps:
    name: CT511N GPS Power
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
  gnss_sentence:
    name: CT511N Last NMEA
  mqtt_connected:
    name: CT511N MQTT Connected
  network_connected:
    name: CT511N Network Connected

interval:
  - interval: 60s
    then:
      - dx_ct511n.publish:
          id: modem
          topic: /topic/heartbeat
          payload: '{"state":"alive"}'
      - dx_ct511n.publish_long:
          id: modem
          topic: /topic/long_payload
          payload: '{"message":"this can use MPUBEX for longer payloads"}'
```

`username` and `password` are optional. If either is provided, the component generates the extended
`AT+MCONFIG="client_id","username","password",0,0` form. Otherwise it uses the simplified
`AT+MCONFIG="client_id"` form from the quick-start notes.

`mqtt_version` defaults to `4`. If your broker only works with the module using the older dialect,
set:

```yaml
dx_ct511n:
  mqtt_version: 3
```

Incoming subscribed MQTT payloads are now unescaped before being exposed through `on_mqtt_message` and
`last_payload`, so normal JSON matching in YAML is easier.

## Full YAML Example

See `docs/full_yaml_example.md` for a full example with:

- local external component path
- MQTT credentials
- LED remote control via subscribed MQTT topic
- DHT11 temperature/humidity reporting
- LED status publishing

## 4G-Only Usage

If you do not want Wi-Fi at all and only want the ESP32 to communicate through the CT511N module,
use the 4G-only example in `docs/4g_only_yaml_example.md`.

In that mode:

- remove `wifi:`
- remove `api:`
- remove Wi-Fi based `ota:`
- rely on CT511N MQTT topics for remote control and telemetry

If you want a git-based example that pins the collaboration branch directly, see
`docs/esphome_codex_branch_example.yaml` and set `ref: codex`.
