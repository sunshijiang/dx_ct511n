# DX-CT511N Project Handoff

This file summarizes the project goals, implementation status, constraints, and next steps so the work can be continued by another AI or developer.

## Project Goal

Build an ESPHome external component for the `DX-CT511N` module, mainly focused on:

- 4G connectivity over UART
- MQTT connection, publish, subscribe, and remote control
- GNSS power control and NMEA reception
- preserving MQTT server and UART pin configuration in ESPHome YAML

The design target is based on:

- the unfinished prototype `uart_async_rxtxtasks_main.c`
- user-provided notes extracted from `DX-CT511&CT511N_串口UART_应用指导.pdf`
- ESPHome external component conventions

## Current Repository Structure

- `components/dx_ct511n/__init__.py`
- `components/dx_ct511n/dx_ct511n.h`
- `components/dx_ct511n/dx_ct511n.cpp`
- `docs/ct511n_uart_mqtt_reference.md`
- `docs/project_handoff.md`
- `README.md`
- `CHANGELOG.md`

## Current Component Capabilities

### Implemented

- UART-based setup flow:
  - `AT`
  - `ATE1`
  - `AT+QICSGP`
  - `AT+NETOPEN`
  - `AT+MCONFIG`
  - `AT+MIPSTART`
  - `AT+MCONNECT`
  - `AT+MSUB`
- MQTT publish action: `dx_ct511n.publish`
- Raw AT action: `dx_ct511n.send_at`
- Reconnect action: `dx_ct511n.reconnect`
- Unsubscribe action: `dx_ct511n.unsubscribe`
- Disconnect action: `dx_ct511n.disconnect`
- Close action: `dx_ct511n.close`
- Periodic signal query using `AT+CSQ`
- GNSS power control using `AT+MGPSC`
- MQTT topic callback trigger `on_mqtt_message`
- JSON and NMEA callback triggers
- YAML-configurable broker, port, APN, keepalive, subscribe topics, client ID, username, password

### Current YAML Style

The component is currently implemented as a single main platform under `dx_ct511n:`.

It is not yet split into standard ESPHome child platforms such as:

- `sensor: - platform: dx_ct511n`
- `text_sensor: - platform: dx_ct511n`
- `binary_sensor: - platform: dx_ct511n`
- `switch: - platform: dx_ct511n`

So all entities must currently be configured directly under the `dx_ct511n:` block.

## Important User Requirements Already Stated

- preserve MQTT server configuration in YAML
- preserve UART pin configuration in YAML
- allow a 4G-only deployment mode without Wi-Fi configuration
- allow remote MQTT control of LED outputs
- support temperature and humidity reporting
- push every meaningful repository change to GitHub so the user can pull and compile immediately
- maintain a markdown reference of project decisions so another AI can continue the work

## Important Technical Notes

### MCONFIG behavior

Current implementation logic:

- if no username/password are set:

```text
AT+MCONFIG="client_id"
```

- if username or password is set:

```text
AT+MCONFIG="client_id","username","password",0,0
```

This was added because the user requested MQTT username/password support.

### MQTT downlink payload format

The user observed `last_payload` values like:

```text
{\"led1\":\"on\",\"led2\":\"on\",\"led3\":\"on\"}
```

This suggests the UART downlink payload was being exposed with escaped quotes.

Impact:

- remote control messages do arrive
- YAML matching logic may fail unless payload is normalized first

This was already improved by unescaping payload before exposing it to YAML callbacks.

### MQTT async result handling

Field logs showed that CT511N can reply with immediate `OK` and only later emit the real result as:

```text
+MCONNECT: FAILURE
+MSUB: FAILURE
+MPUB: FAILURE
```

The component was updated to wait for these async result lines before treating connect, subscribe, and publish as successful.

### ESPHome compile issues already fixed

- fixed `TimePeriod` codegen bug by passing `.total_milliseconds`

## Known Gaps / Next Tasks

### High Priority

1. Improve README example so it matches the actual current component structure

### Medium Priority

2. Add GNSS query support:
   - `AT+MGPSC?`
   - `AT+GPSMODE`
   - `AT+GPSST`
3. Confirm exact success/error responses for `MUNSUB`, `MDISCONNECT`, and `MIPCLOSE`
4. Confirm whether `MPUBEX` also emits a final async success/failure line distinct from short publish

### Nice To Have

5. Refactor into standard ESPHome child platforms (`sensor.py`, `text_sensor.py`, etc.)
6. Add built-in JSON parsing helpers for LED/GPS remote control

## Example User Scenario In Progress

The user wants a complete ESPHome YAML that includes:

- CT511N modem config
- optional 4G-only deployment without Wi-Fi
- LED1/LED2/LED3 GPIO switches
- DHT11 temperature and humidity sensor
- publishing sensor data over CT511N MQTT
- receiving MQTT commands to control LEDs remotely
- GPS power control

## Local Development Note

The user has switched to a local external component path for stability, instead of using Git-based external component cache.

## Codex Branch Note

The repository also has a `codex` branch with a more advanced example flow, including:

- an additional example YAML
- runtime subscription management ideas
- richer topic naming conventions

When comparing branches, use the `codex` branch as a reference for future improvements, but keep the main branch examples aligned with the currently supported component API.

## Files Not Pushed As Source Material

These existed locally but were intentionally not included in code commits:

- PDF manuals
- `uart_async_rxtxtasks_main.c`

They are still useful as offline reference material.
