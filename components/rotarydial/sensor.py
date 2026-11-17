import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation, pins
from esphome.automation import maybe_simple_id
from esphome.components import sensor
from esphome.const import CONF_ID, CONF_TIMEOUT

from . import rotarydial_ns

CONF_TRIGGER_PIN = "trigger_pin"
CONF_PULSE_PIN = "pulse_pin"
CONF_ON_NUMBERS = "on_numbers"

ICON_PHONE_DIAL = "mdi:phone-dial"

RotaryDialComponent = rotarydial_ns.class_(
    "RotaryDialComponent", sensor.Sensor, cg.Component
)
ClearNumbersAction = rotarydial_ns.class_(
    "ClearNumbersAction",
    automation.Action,
    cg.Parented.template(RotaryDialComponent),
)

CONFIG_SCHEMA = cv.All(
    # build_callback_automation() and register_action(synchronous=...) were
    # introduced in ESPHome 2026.4.0.
    cv.require_esphome_version(2026, 4, 0),
    sensor.sensor_schema(
        RotaryDialComponent,
        icon=ICON_PHONE_DIAL,
        accuracy_decimals=0,
    )
    .extend(
        {
            cv.Required(CONF_TRIGGER_PIN): pins.internal_gpio_input_pin_schema,
            cv.Required(CONF_PULSE_PIN): pins.internal_gpio_input_pin_schema,
            cv.Optional(
                CONF_TIMEOUT, default="3s"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_ON_NUMBERS): automation.validate_automation({}),
        }
    )
    .extend(cv.COMPONENT_SCHEMA),
)


@automation.register_action(
    "rotarydial.clear_numbers",
    ClearNumbersAction,
    maybe_simple_id(
        {
            cv.GenerateID(): cv.use_id(RotaryDialComponent),
        }
    ),
    synchronous=True,
)
async def rotarydial_clear_numbers_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)

    trigger_pin = await cg.gpio_pin_expression(config[CONF_TRIGGER_PIN])
    cg.add(var.set_trigger_pin(trigger_pin))
    pulse_pin = await cg.gpio_pin_expression(config[CONF_PULSE_PIN])
    cg.add(var.set_pulse_pin(pulse_pin))

    cg.add(var.set_timeout_value(config[CONF_TIMEOUT]))

    for conf in config.get(CONF_ON_NUMBERS, []):
        await automation.build_callback_automation(
            var, "add_on_numbers_callback", [(cg.std_string, "x")], conf
        )
