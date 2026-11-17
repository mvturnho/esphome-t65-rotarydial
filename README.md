# ESPHome Rotary Dial (T65)

[![CI](https://github.com/mvturnho/esphome-t65-rotarydial/actions/workflows/ci.yml/badge.svg)](https://github.com/mvturnho/esphome-t65-rotarydial/actions/workflows/ci.yml)

An [ESPHome](https://esphome.io) external component that reads the rotary
(pulse) dial of an old telephone, such as the Dutch PTT **T65**, and turns it
into something Home Assistant can use.

- Every dialed digit is published as a **sensor** value (`0`–`9`).
- When nothing has been dialed for a configurable `timeout`, the **`on_numbers`**
  automation fires with the full number dialed so far as a string (for example
  `"0612345678"`).
- The **`rotarydial.clear_numbers`** action starts a new number, typically when
  the handset is lifted or put back.

## Requirements

- ESPHome **2026.4.0** or newer. The component is tested against 2026.4.0 and
  the latest release.
- An ESP32 (Arduino or ESP-IDF framework) or ESP8266. Any platform with internal
  GPIO pins should work.
- Two free GPIO pins for the dial, plus usually a third one for the hook switch.

## Installation

Add the component to your device configuration with
[`external_components`](https://esphome.io/components/external_components/):

```yaml
external_components:
  - source: github://mvturnho/esphome-t65-rotarydial@v1.0.0
    components: [rotarydial]
```

Pin a release tag (`@v1.0.0`) so that a change in this repository never breaks
your device unexpectedly. Use `@main` to follow the latest development version.
The available versions are listed in [CHANGELOG.md](CHANGELOG.md).

## Wiring

A rotary dial has two contacts:

| Contact | Behavior | Connect to |
| --- | --- | --- |
| **Off-normal contact** | Closed while the dial is turned away from its rest position | `trigger_pin` |
| **Pulse contact** | Normally closed. It opens once per pulse while the dial runs back | `pulse_pin` |

The usual wiring connects each contact between a GPIO pin and GND and enables
the internal pull-up:

```text
            ┌──────────── GPIO17 (trigger_pin, pull-up)
  off-normal│
            └──────────── GND

            ┌──────────── GPIO16 (pulse_pin, pull-up)
  pulse     │
            └──────────── GND
```

The component expects these signal levels:

- `trigger_pin` is **LOW while dialing** and HIGH when the dial is at rest.
- `pulse_pin` makes a **rising edge for every pulse**: 1 pulse is digit 1 and
  10 pulses is digit 0.

If your dial or wiring produces the opposite levels, add `inverted: true` to
that pin.

## Configuration

```yaml
text_sensor:
  - platform: template
    id: dialed_number
    name: Dialed number

sensor:
  - platform: rotarydial
    id: dial
    name: Dial
    timeout: 3s
    trigger_pin:
      number: GPIO17
      mode:
        input: true
        pullup: true
    pulse_pin:
      number: GPIO16
      mode:
        input: true
        pullup: true
    on_numbers:
      then:
        - text_sensor.template.publish:
            id: dialed_number
            state: !lambda "return x;"

binary_sensor:
  - platform: gpio
    name: Handset lifted
    pin:
      number: GPIO23
      mode:
        input: true
        pullup: true
    on_press:
      - rotarydial.clear_numbers: dial
    on_release:
      - rotarydial.clear_numbers: dial
```

A complete device configuration is in
[examples/t65-dial-phone.yaml](examples/t65-dial-phone.yaml).

### Configuration variables

| Option | Type | Default | Description |
| --- | --- | --- | --- |
| `trigger_pin` | [Pin schema](https://esphome.io/guides/configuration-types/#pin-schema) | **required** | The pin connected to the off-normal contact. It must be an internal GPIO pin. |
| `pulse_pin` | [Pin schema](https://esphome.io/guides/configuration-types/#pin-schema) | **required** | The pin connected to the pulse contact. It must be an internal GPIO pin. |
| `timeout` | [Time](https://esphome.io/guides/configuration-types/#time) | `3s` | How long to wait after the last digit before `on_numbers` fires. |
| `on_numbers` | [Automation](https://esphome.io/automations/) | | Runs when the number is complete. The variable `x` (`std::string`) holds all digits dialed since the last `rotarydial.clear_numbers`. |
| `id` | [ID](https://esphome.io/guides/configuration-types/#id) | | Use this ID in actions and lambdas. |

All other options of a [sensor](https://esphome.io/components/sensor/) are also
available, such as `name`, `icon`, `filters`, `on_value` and `internal`. The
sensor uses the `mdi:phone-dial` icon and 0 decimals by default.

### Action: `rotarydial.clear_numbers`

This action forgets all digits dialed so far and cancels an `on_numbers` that
has not fired yet. Call it when the handset is lifted or put back, so that every
call starts with an empty number.

```yaml
on_press:
  - rotarydial.clear_numbers: dial

# or, in the long form:
on_press:
  - rotarydial.clear_numbers:
      id: dial
```

### Lambda access

```cpp
// All digits dialed since the last clear, as a std::string
std::string number = id(dial).get_numbers();
```

## How it works

```text
trigger_pin  ‾‾‾‾\______________________/‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
pulse_pin    ____________/‾\_/‾\_/‾\_______________________________________________
                         1   2   3        │                        │
                                 digit 3 published          timeout expires:
                                 to the sensor              on_numbers("…3")
```

1. When `trigger_pin` goes LOW, a new digit starts and a pending `on_numbers`
   is postponed.
2. While the dial runs back, every rising edge on `pulse_pin` counts as one
   pulse.
3. When `trigger_pin` goes HIGH again, the pulse count becomes a digit. The
   component publishes the digit as the sensor state and appends it to the
   number.
4. If no new digit starts within `timeout`, `on_numbers` fires with the whole
   number. The number keeps growing until `rotarydial.clear_numbers` is called.

The component ignores a dial movement without pulses, for example a dial that
was released before reaching the finger stop. It also ignores more than 10
pulses and logs a warning in that case.

## Troubleshooting

- **No digits at all.** Check the `dump_config` output in the logs to verify
  the pins. Then check the signal levels from the [Wiring](#wiring) section.
  Most problems are solved with `inverted: true` on one of the pins.
- **Digits are off by one, or you see "Ignoring digit with N pulses".** This
  usually comes from contact bounce or a wrong pin level on the pulse contact.
  Try the pull-up and `inverted` settings, or add a small capacitor (about
  100 nF) across the pulse contact.
- **Missed pulses.** The component samples the pins in the main loop. A
  component that blocks the loop for a long time can make it miss pulses
  (a pulse is about 40–60 ms). Look for "took a long time" warnings in the log.
- Set the logger to `DEBUG` (the default level) to see every digit and the
  complete number in the log.

## Upgrading from the 2023 custom component

Older configurations copied the files to `custom_components/rotarydial`. To
upgrade:

1. Delete the local `custom_components/rotarydial` folder.
2. Add the `external_components` block from [Installation](#installation).
3. Update ESPHome to 2026.4.0 or newer.

The YAML options are the same as before. The behavior changed in these places:

- `timeout` now counts from the **end** of the last digit instead of its start.
  For the same `timeout`, `on_numbers` therefore fires up to about a second
  later than before.
- `rotarydial.clear_numbers` also cancels an `on_numbers` that has not fired
  yet. Before, a handset put down while dialing could still report an empty
  number afterwards.
- `get_numbers()` returns a `std::string` instead of a `std::vector<int>`.

[CHANGELOG.md](CHANGELOG.md) lists every change.

## Development

The `tests/` folder has build configurations that use this checkout as a local
external component:

```bash
pip install esphome
esphome config  tests/test.esp32-ard.yaml
esphome compile tests/test.host.yaml   # quick native build, no toolchain download needed
```

CI compiles every test configuration for ESP32 (Arduino and ESP-IDF), ESP8266
and the host platform. It does this against the minimum supported ESPHome
version and the latest release, and repeats it weekly to catch breaking changes
in ESPHome early.

## License

[MIT](LICENSE)
