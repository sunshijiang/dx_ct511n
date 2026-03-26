# Changelog

## Unreleased

### Added

- initial ESPHome external component skeleton under `components/dx_ct511n`
- YAML-configurable MQTT broker, port, client ID, APN, keepalive, and topic subscription list
- UART-based MQTT connection sequence using CT511N AT commands
- MQTT publish action `dx_ct511n.publish`
- long MQTT publish action `dx_ct511n.publish_long`
- raw command action `dx_ct511n.send_at`
- reconnect action `dx_ct511n.reconnect`
- unsubscribe action `dx_ct511n.unsubscribe`
- MQTT disconnect action `dx_ct511n.disconnect`
- MQTT close action `dx_ct511n.close`
- diagnostics for CSQ, RSSI, BER, last response, topic, payload, and NMEA sentence
- markdown reference `docs/ct511n_uart_mqtt_reference.md`

### Changed

- aligned `AT+QICSGP` default quick-start form to `1,1`
- aligned `AT+MIPSTART` default MQTT version to `4`
- changed `AT+MCONFIG` default generation to simplified client-ID form
- changed `AT+MCONFIG` generation to use username/password form when credentials are configured
- unescaped incoming subscribed MQTT payload before exposing it to automations and diagnostics
- changed MQTT connect/subscribe/publish handling to wait for async `SUCCESS` or `FAILURE` result lines
- added configurable `mqtt_version` for `AT+MIPSTART`

### Notes

- exact downlink UART message framing still needs confirmation from the full manual
