# DX-CT511N UART MQTT Reference

This markdown file is a normalized reference derived from the user-provided notes from
`DX-CT511&CT511N_串口UART_应用指导.pdf`.

It is intended as a working reference for validating the ESPHome external component in
`components/dx_ct511n`.

## UART Default Parameters

- baud rate: `115200`
- data bits: `8`
- parity: `none`
- stop bits: `1`

## MQTT Quick Start Flow

The simplified MQTT flow in the notes is:

1. Configure APN

```text
AT+QICSGP=1,1,"cmnbiot","",""
```

2. Open mobile data network

```text
AT+NETOPEN
```

Expected success indication:

```text
+NETOPEN: SUCCESS
```

3. Set MQTT client ID

```text
AT+MCONFIG="4G TEST"
```

4. Configure MQTT broker address, port and version

```text
AT+MIPSTART="broker.emqx.io",1883,4
```

5. Connect to MQTT broker

```text
AT+MCONNECT=1,60
```

6. Subscribe topic

```text
AT+MSUB="phone",0
```

7. Publish short payload

```text
AT+MPUB="4G",0,0,"hello world"
```

8. Publish long payload

```text
AT+MPUBEX="4G",0,0,20
```

Notes from the provided text:

- after `AT+MPUBEX`, the module enters data transmission mode
- the module returns prompt `>`
- data must be sent within 10 seconds, otherwise it exits data mode and returns `ERROR`
- any format data can be sent in this mode
- actual transmitted data length must exactly match `<msgLen>`
- if the sent payload is shorter than `<msgLen>`, the module returns `ERROR`

9. Unsubscribe topic

```text
AT+MUNSUB="phone"
```

10. Disconnect MQTT

```text
AT+MDISCONNECT
```

11. Release MQTT resources

```text
AT+MIPCLOSE
```

## Important Clarifications Extracted From Notes

- the quick-start commands are described as the simplified form
- full syntax and behavior still need to be verified against section `5. AT命令详解`
- `AT+MIPSTART` is described as configuring MQTT server address, port and version
- `AT+MCONNECT=1,60` appears to use connection index `1` and keepalive `60`
- `AT+MSUB` uses topic plus QoS
- `AT+MPUB` uses topic, QoS, retain, payload
- `AT+MPUBEX` is the long payload variant with explicit payload length

## AT Command List Mentioned In User Notes

The following commands were listed in the notes, even though not all details were provided:

### Basic and UART

- `AT`
- `ATE<mode>`
- `ATI`
- `AT+IPR`
- `AT+RESET`
- `AT+FLOWCTRL`

### SIM / Network / Time / Power

- `AT+ICCID`
- `AT+CEREG?`
- `AT+CSQ`
- `AT+CCLK`
- `AT+CTZR`
- `AT+SYSSLEEP`
- `AT+CSCLK`
- `AT+QICSGP`
- `AT+NETOPEN`
- `AT+NETCLOSE`

### TCP/IP

- `AT+CIPOPEN`
- `AT+CIPSEND`
- `AT+CIPCLOSE`
- `AT+CIPMODE`
- `ATO`
- `AT+MCIPCFG`
- `AT+MDNSGIP`
- `AT+MPING`

### MQTT

- `AT+MCONFIG`
- `AT+MIPSTART`
- `AT+MCONNECT`
- `AT+MPUB`
- `AT+MPUBEX`
- `AT+MSUB`
- `AT+MUNSUB`
- `AT+MDISCONNECT`
- `AT+MIPCLOSE`
- `AT+MQTTSTATU`
- `AT+MQTTSSL`
- `AT+MQTTMIX`

### HTTP

- `AT$HTTPOPEN`
- `AT$HTTPCLOSE`
- `AT$HTTPPARA`
- `AT$HTTPACTION`
- `AT$HTTPDATA`
- `AT$HTTPSEND`
- `AT$HTTPDATAEX`
- `AT$HTTPRQH`

### GNSS

- `AT+MGPSC`
- `AT+GPSMODE`
- `AT+GPSST`
- `AT+AGNSSGET`
- `AT+AGNSSSET`

## Current Mapping Into ESPHome Component

The current external component intentionally focuses on MQTT-centered behavior first:

- `AT+QICSGP`
- `AT+NETOPEN`
- `AT+MCONFIG`
- `AT+MIPSTART`
- `AT+MCONNECT`
- `AT+MSUB`
- `AT+MPUB`
- `AT+CSQ`
- `AT+MGPSC`

## Known Gaps To Validate Later

- confirm whether `AT+QICSGP` should use `1,1` or `1,2` for the target SIM/operator
- confirm final `AT+MCONFIG` syntax beyond client ID only
- confirm exact success/failure responses for `AT+MCONNECT`, `AT+MSUB`, `AT+MUNSUB`, `AT+MDISCONNECT`, `AT+MIPCLOSE`
- confirm how downlink MQTT messages are formatted on UART
- add `AT+MPUBEX` long-payload handling once the exact UART interaction is verified
- add `AT+MQTTSTATU` based state probing if needed

## Validation Checklist For User Review

Please verify these items against your original manual:

1. Is `AT+MCONFIG` quick form limited to only client ID, or should username/password also be part of the default design?
2. Is MQTT version `4` the expected default for `AT+MIPSTART` on CT511N?
3. Is the APN mode in `AT+QICSGP` definitely `1,1` for your target deployment?
4. What exact UART text is emitted for incoming subscribed MQTT messages?
5. What exact response does `AT+MGPSC?` return?
6. Should `AT+GPSMODE` and `AT+GPSST` be added in the first implementation batch?
