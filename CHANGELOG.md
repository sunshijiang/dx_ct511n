# Changelog

## Unreleased

### Added

- initial ESPHome external component skeleton under `components/dx_ct511n`
- YAML-configurable MQTT broker, port, client ID, APN, keepalive, and topic subscription list
- UART-based MQTT connection sequence using CT511N AT commands
- MQTT publish action `dx_ct511n.publish`
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

### Notes

- `AT+MPUBEX` long-message mode is not implemented yet
- exact downlink UART message framing still needs confirmation from the full manual
