from esphome import automation
import esphome.codegen as cg
from esphome.components import binary_sensor, sensor, switch, text_sensor, uart
import esphome.config_validation as cv
from esphome.const import (
    CONF_BROKER,
    CONF_ID,
    CONF_PASSWORD,
    CONF_PAYLOAD,
    CONF_PORT,
    CONF_QOS,
    CONF_RETAIN,
    CONF_TIMEOUT,
    CONF_TOPIC,
    CONF_TRIGGER_ID,
    CONF_USERNAME,
    DEVICE_CLASS_CONNECTIVITY,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
)

AUTO_LOAD = ["sensor", "text_sensor", "binary_sensor", "switch"]
DEPENDENCIES = ["uart"]
MULTI_CONF = True

CONF_APN = "apn"
CONF_BIT_ERROR_RATE = "bit_error_rate"
CONF_CLIENT_ID = "client_id"
CONF_COMMAND = "command"
CONF_CLEAN_SESSION = "clean_session"
CONF_GNSS_SENTENCE = "gnss_sentence"
CONF_GPS = "gps"
CONF_KEEPALIVE = "keepalive"
CONF_LAST_PAYLOAD = "last_payload"
CONF_LAST_RESPONSE = "last_response"
CONF_LAST_TOPIC = "last_topic"
CONF_MQTT_CONNECTED = "mqtt_connected"
CONF_NETWORK_CONNECTED = "network_connected"
CONF_ON_JSON_MESSAGE = "on_json_message"
CONF_ON_MQTT_MESSAGE = "on_mqtt_message"
CONF_ON_NMEA_SENTENCE = "on_nmea_sentence"
CONF_ON_READY = "on_ready"
CONF_RECONNECT_INTERVAL = "reconnect_interval"
CONF_RSSI_DBM = "rssi_dbm"
CONF_SIGNAL_QUALITY = "signal_quality"
CONF_SUBSCRIBE_TOPICS = "subscribe_topics"
CONF_TOPIC_FILTER = "topic_filter"

dx_ct511n_ns = cg.esphome_ns.namespace("dx_ct511n")
DXCT511NComponent = dx_ct511n_ns.class_(
    "DXCT511NComponent", cg.PollingComponent, uart.UARTDevice
)
DXCT511NGPSSwitch = dx_ct511n_ns.class_(
    "DXCT511NGPSSwitch", switch.Switch, cg.Parented.template(DXCT511NComponent)
)

DXCT511NReadyTrigger = dx_ct511n_ns.class_(
    "DXCT511NReadyTrigger", automation.Trigger.template()
)
DXCT511NMQTTMessageTrigger = dx_ct511n_ns.class_(
    "DXCT511NMQTTMessageTrigger",
    automation.Trigger.template(cg.std_string, cg.std_string),
)
DXCT511NJSONMessageTrigger = dx_ct511n_ns.class_(
    "DXCT511NJSONMessageTrigger", automation.Trigger.template(cg.std_string)
)
DXCT511NNMEASentenceTrigger = dx_ct511n_ns.class_(
    "DXCT511NNMEASentenceTrigger", automation.Trigger.template(cg.std_string)
)

DXCT511NPublishAction = dx_ct511n_ns.class_("DXCT511NPublishAction", automation.Action)
DXCT511NSendATAction = dx_ct511n_ns.class_("DXCT511NSendATAction", automation.Action)
DXCT511NReconnectAction = dx_ct511n_ns.class_(
    "DXCT511NReconnectAction", automation.Action
)
DXCT511NUnsubscribeAction = dx_ct511n_ns.class_(
    "DXCT511NUnsubscribeAction", automation.Action
)
DXCT511NDisconnectAction = dx_ct511n_ns.class_(
    "DXCT511NDisconnectAction", automation.Action
)
DXCT511NCloseAction = dx_ct511n_ns.class_("DXCT511NCloseAction", automation.Action)


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(DXCT511NComponent),
            cv.Required(CONF_BROKER): cv.string_strict,
            cv.Optional(CONF_PORT): cv.port,
            cv.Required(CONF_CLIENT_ID): cv.string_strict,
            cv.Optional(CONF_USERNAME): cv.string,
            cv.Optional(CONF_PASSWORD): cv.string,
            cv.Optional(CONF_APN): cv.string_strict,
            cv.Optional(CONF_KEEPALIVE): cv.int_range(min=10, max=3600),
            cv.Optional(CONF_CLEAN_SESSION): cv.boolean,
            cv.Optional(CONF_RECONNECT_INTERVAL): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_TIMEOUT): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_SUBSCRIBE_TOPICS): cv.ensure_list(cv.string_strict),
            cv.Optional(CONF_SIGNAL_QUALITY): sensor.sensor_schema(
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon="mdi:signal",
            ),
            cv.Optional(CONF_RSSI_DBM): sensor.sensor_schema(
                unit_of_measurement="dBm",
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon="mdi:wifi-strength-2",
            ),
            cv.Optional(CONF_BIT_ERROR_RATE): sensor.sensor_schema(
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon="mdi:pulse",
            ),
            cv.Optional(CONF_LAST_RESPONSE): text_sensor.text_sensor_schema(
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon="mdi:console-line",
            ),
            cv.Optional(CONF_LAST_TOPIC): text_sensor.text_sensor_schema(
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon="mdi:format-list-bulleted",
            ),
            cv.Optional(CONF_LAST_PAYLOAD): text_sensor.text_sensor_schema(
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon="mdi:code-json",
            ),
            cv.Optional(CONF_GNSS_SENTENCE): text_sensor.text_sensor_schema(
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon="mdi:satellite-variant",
            ),
            cv.Optional(CONF_MQTT_CONNECTED): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_CONNECTIVITY,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_NETWORK_CONNECTED): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_CONNECTIVITY,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_GPS): switch.switch_schema(
                DXCT511NGPSSwitch,
                icon="mdi:crosshairs-gps",
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_ON_READY): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(DXCT511NReadyTrigger)}
            ),
            cv.Optional(CONF_ON_MQTT_MESSAGE): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(
                        DXCT511NMQTTMessageTrigger
                    )
                }
            ),
            cv.Optional(CONF_ON_JSON_MESSAGE): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(
                        DXCT511NJSONMessageTrigger
                    )
                }
            ),
            cv.Optional(CONF_ON_NMEA_SENTENCE): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(
                        DXCT511NNMEASentenceTrigger
                    )
                }
            ),
        }
    )
    .extend(cv.polling_component_schema("30s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "dx_ct511n", require_tx=True, require_rx=True
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    cg.add(var.set_broker(config[CONF_BROKER]))
    cg.add(var.set_port(config.get(CONF_PORT, 1883)))
    cg.add(var.set_client_id(config[CONF_CLIENT_ID]))
    cg.add(var.set_username(config.get(CONF_USERNAME, "")))
    cg.add(var.set_password(config.get(CONF_PASSWORD, "")))
    cg.add(var.set_apn(config.get(CONF_APN, "cmnbiot")))
    cg.add(var.set_keepalive(config.get(CONF_KEEPALIVE, 60)))
    cg.add(var.set_clean_session(config.get(CONF_CLEAN_SESSION, True)))
    cg.add(var.set_command_timeout(config.get(CONF_TIMEOUT, cv.TimePeriod(seconds=5))))
    cg.add(
        var.set_reconnect_interval(
            config.get(CONF_RECONNECT_INTERVAL, cv.TimePeriod(seconds=30))
        )
    )

    for topic in config.get(CONF_SUBSCRIBE_TOPICS, []):
        cg.add(var.add_subscribe_topic(topic))

    if conf := config.get(CONF_SIGNAL_QUALITY):
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_signal_quality_sensor(sens))
    if conf := config.get(CONF_RSSI_DBM):
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_rssi_dbm_sensor(sens))
    if conf := config.get(CONF_BIT_ERROR_RATE):
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_bit_error_rate_sensor(sens))

    if conf := config.get(CONF_LAST_RESPONSE):
        sens = await text_sensor.new_text_sensor(conf)
        cg.add(var.set_last_response_text_sensor(sens))
    if conf := config.get(CONF_LAST_TOPIC):
        sens = await text_sensor.new_text_sensor(conf)
        cg.add(var.set_last_topic_text_sensor(sens))
    if conf := config.get(CONF_LAST_PAYLOAD):
        sens = await text_sensor.new_text_sensor(conf)
        cg.add(var.set_last_payload_text_sensor(sens))
    if conf := config.get(CONF_GNSS_SENTENCE):
        sens = await text_sensor.new_text_sensor(conf)
        cg.add(var.set_gnss_sentence_text_sensor(sens))

    if conf := config.get(CONF_MQTT_CONNECTED):
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(var.set_mqtt_connected_binary_sensor(sens))
    if conf := config.get(CONF_NETWORK_CONNECTED):
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(var.set_network_connected_binary_sensor(sens))

    if conf := config.get(CONF_GPS):
        gps_switch = await switch.new_switch(conf)
        await cg.register_parented(gps_switch, config[CONF_ID])
        cg.add(var.set_gps_switch(gps_switch))

    for conf in config.get(CONF_ON_READY, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)

    for conf in config.get(CONF_ON_MQTT_MESSAGE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(
            trigger, [(cg.std_string, "topic"), (cg.std_string, "payload")], conf
        )

    for conf in config.get(CONF_ON_JSON_MESSAGE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.std_string, "payload")], conf)

    for conf in config.get(CONF_ON_NMEA_SENTENCE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.std_string, "sentence")], conf)


PUBLISH_ACTION_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(DXCT511NComponent),
        cv.Required(CONF_TOPIC): cv.templatable(cv.string_strict),
        cv.Required(CONF_PAYLOAD): cv.templatable(cv.string),
        cv.Optional(CONF_QOS): cv.templatable(cv.int_range(min=0, max=2)),
        cv.Optional(CONF_RETAIN): cv.templatable(cv.boolean),
    }
)


@automation.register_action(
    "dx_ct511n.publish",
    DXCT511NPublishAction,
    PUBLISH_ACTION_SCHEMA,
)
async def dx_ct511n_publish_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    topic = await cg.templatable(config[CONF_TOPIC], args, cg.std_string)
    payload = await cg.templatable(config[CONF_PAYLOAD], args, cg.std_string)
    if topic is not None:
        cg.add(var.set_topic(topic))
    if payload is not None:
        cg.add(var.set_payload(payload))
    if CONF_QOS in config:
        qos = await cg.templatable(config[CONF_QOS], args, cg.uint8)
        if qos is not None:
            cg.add(var.set_qos(qos))
    else:
        cg.add(var.set_qos(0))
    if CONF_RETAIN in config:
        retain = await cg.templatable(config[CONF_RETAIN], args, bool)
        if retain is not None:
            cg.add(var.set_retain(retain))
    else:
        cg.add(var.set_retain(False))
    return var


SEND_AT_ACTION_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(DXCT511NComponent),
        cv.Required(CONF_COMMAND): cv.templatable(cv.string_strict),
        cv.Optional(CONF_TIMEOUT): cv.positive_time_period_milliseconds,
    }
)


@automation.register_action(
    "dx_ct511n.send_at",
    DXCT511NSendATAction,
    SEND_AT_ACTION_SCHEMA,
)
async def dx_ct511n_send_at_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    command = await cg.templatable(config[CONF_COMMAND], args, cg.std_string)
    if command is not None:
        cg.add(var.set_command(command))
    cg.add(var.set_timeout_ms(config.get(CONF_TIMEOUT, cv.TimePeriod())))
    return var


@automation.register_action(
    "dx_ct511n.reconnect",
    DXCT511NReconnectAction,
    cv.Schema({cv.GenerateID(): cv.use_id(DXCT511NComponent)}),
)
async def dx_ct511n_reconnect_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)


UNSUBSCRIBE_ACTION_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(DXCT511NComponent),
        cv.Required(CONF_TOPIC_FILTER): cv.templatable(cv.string_strict),
    }
)


@automation.register_action(
    "dx_ct511n.unsubscribe",
    DXCT511NUnsubscribeAction,
    UNSUBSCRIBE_ACTION_SCHEMA,
)
async def dx_ct511n_unsubscribe_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    topic_filter = await cg.templatable(config[CONF_TOPIC_FILTER], args, cg.std_string)
    if topic_filter is not None:
        cg.add(var.set_topic_filter(topic_filter))
    return var


@automation.register_action(
    "dx_ct511n.disconnect",
    DXCT511NDisconnectAction,
    cv.Schema({cv.GenerateID(): cv.use_id(DXCT511NComponent)}),
)
async def dx_ct511n_disconnect_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)


@automation.register_action(
    "dx_ct511n.close",
    DXCT511NCloseAction,
    cv.Schema({cv.GenerateID(): cv.use_id(DXCT511NComponent)}),
)
async def dx_ct511n_close_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)
