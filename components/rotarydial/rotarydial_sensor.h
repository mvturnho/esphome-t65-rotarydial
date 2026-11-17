#pragma once

#include <cstdint>
#include <string>
#include <utility>

#include "esphome/components/sensor/sensor.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"

namespace esphome::rotarydial {

/// Reads a pulse-dialing rotary dial (e.g. the PTT T65).
///
/// - trigger pin: the off-normal contact. LOW while the dial is turned away from
///   its rest position, HIGH when it is back at rest (use `inverted: true` on the
///   pin if your wiring is the other way around).
/// - pulse pin: the pulse contact. Every rising edge while dialing is one pulse;
///   1 pulse = digit 1 ... 10 pulses = digit 0.
///
/// Every completed digit is published as the sensor state. When no new digit has
/// been dialed for `timeout` ms, the on_numbers callbacks receive all digits dialed
/// since the last clear_numbers() as a string.
class RotaryDialComponent : public sensor::Sensor, public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_trigger_pin(InternalGPIOPin *pin) { this->trigger_pin_ = pin; }
  void set_pulse_pin(InternalGPIOPin *pin) { this->pulse_pin_ = pin; }
  void set_timeout_value(uint32_t timeout_ms) { this->dial_timeout_ = timeout_ms; }

  /// All digits dialed since the last clear_numbers(), e.g. "0612345678".
  const std::string &get_numbers() const { return this->numbers_; }

  /// Forget all dialed digits and cancel a pending on_numbers callback.
  void clear_numbers();

  template<typename F> void add_on_numbers_callback(F &&callback) {
    this->numbers_callback_.add(std::forward<F>(callback));
  }

 protected:
  void start_digit_();
  void finish_digit_();

  InternalGPIOPin *trigger_pin_{nullptr};
  InternalGPIOPin *pulse_pin_{nullptr};
  uint32_t dial_timeout_{3000};

  uint8_t pulse_count_{0};
  bool trigger_last_{true};
  bool pulse_last_{false};

  std::string numbers_;
  LazyCallbackManager<void(const std::string &)> numbers_callback_;
};

template<typename... Ts> class ClearNumbersAction : public Action<Ts...>, public Parented<RotaryDialComponent> {
 public:
  void play(const Ts &.../*x*/) override { this->parent_->clear_numbers(); }
};

}  // namespace esphome::rotarydial
