# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.0.0] - 2026-11-17

First release as a standalone repository. The component can now be loaded with
`external_components` straight from GitHub.

### Added

- Repository layout for `external_components` (`components/rotarydial/`).
- README with wiring, configuration, and migration notes, plus a complete
  example configuration in `examples/`.
- Build tests for ESP32 (Arduino and ESP-IDF), ESP8266, and the host platform,
  and a GitHub Actions workflow that compiles them against ESPHome 2026.4.0 and
  the latest release.
- `dump_config` now logs both pins and the timeout.
- Debug log messages for every digit, every completed number, and ignored dial
  movements.
- The sensor uses the `mdi:phone-dial` icon by default.

### Changed

- **Requires ESPHome 2026.4.0 or newer.** The component now uses the current
  ESPHome APIs:
  - `on_numbers` is registered with `build_callback_automation`, so the separate
    `NumbersTrigger` class is gone.
  - Callbacks are stored in a `LazyCallbackManager<void(const std::string &)>`.
  - `ClearNumbersAction::play()` takes `const Ts &...`.
  - `rotarydial.clear_numbers` is registered with `synchronous=True`.
  - The scheduler timeout uses a static `const char *` name.
  - The code uses the `esphome::rotarydial` nested namespace.
- `timeout` now counts from the **end** of the last digit instead of its start.
  The number is reported only after the dial has been idle for `timeout`, even
  for long digits such as `0`.
- `timeout` defaults to `3s` in the schema. It no longer accepts a lambda: that
  was accepted before, but it broke compilation.
- `get_numbers()` now returns `const std::string &` instead of a copy of a
  `std::vector<int>`.
- The component counts pulses only while the dial is away from its rest
  position.
- The component reads the initial pin levels at boot instead of assuming them.

### Fixed

- `rotarydial.clear_numbers` now cancels an `on_numbers` that has not fired yet.
  Before, putting down the handset while dialing published an empty number a
  few seconds later, which overwrote states such as "Hangup".
- A dial movement without pulses no longer adds a phantom `0`.
- More than 10 pulses is now ignored with a warning instead of being turned
  into `0`.
- The pulse counter is initialized. Before, its first value was undefined.
- The Python class now derives from `sensor.Sensor`, so the dial's `id` can be
  used where a sensor is expected.

### Removed

- The `trigger_id` option of `on_numbers`. It was never needed in
  configurations.
- Unused code: `dialTimeout()`, `pickup_old_`, and the `binary_sensor` include.

## [0.1.0] - 2023-01-08

- First version, a custom component in `custom_components/rotarydial`.

[Unreleased]: https://github.com/mvturnho/esphome-t65-rotarydial/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/mvturnho/esphome-t65-rotarydial/releases/tag/v1.0.0
