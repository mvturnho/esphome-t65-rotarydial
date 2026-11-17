#include "rotarydial_sensor.h"

#include <cinttypes>

#include "esphome/core/log.h"

namespace esphome::rotarydial {

static const char *const TAG = "rotarydial";

/// Scheduler name of the "number complete" timeout (must have static lifetime).
static const char *const DIAL_TIMEOUT = "dial_timeout";

/// A digit is 1..10 pulses; 10 pulses means digit 0.
static constexpr uint8_t MAX_PULSES = 10;

void RotaryDialComponent::setup() {
  this->trigger_pin_->setup();
  this->pulse_pin_->setup();
  // Start from the real pin levels so a dial that is not at rest during boot
  // (or inverted wiring) does not register a phantom digit.
  this->trigger_last_ = this->trigger_pin_->digital_read();
  this->pulse_last_ = this->pulse_pin_->digital_read();
}

void RotaryDialComponent::dump_config() {
  LOG_SENSOR("", "Rotary Dial", this);
  LOG_PIN("  Trigger Pin: ", this->trigger_pin_);
  LOG_PIN("  Pulse Pin: ", this->pulse_pin_);
  ESP_LOGCONFIG(TAG, "  Timeout: %" PRIu32 " ms", this->dial_timeout_);
}

void RotaryDialComponent::loop() {
  const bool trigger = this->trigger_pin_->digital_read();
  const bool pulse = this->pulse_pin_->digital_read();

  if (trigger != this->trigger_last_) {
    this->trigger_last_ = trigger;
    if (!trigger) {
      this->start_digit_();
    } else {
      this->finish_digit_();
    }
  }

  // Count rising edges of the pulse contact while the dial is off its rest position.
  if (!trigger && pulse && !this->pulse_last_ && this->pulse_count_ < UINT8_MAX) {
    this->pulse_count_++;
  }
  this->pulse_last_ = pulse;
}

void RotaryDialComponent::start_digit_() {
  this->pulse_count_ = 0;
  // The user is still dialing: the number is not complete yet.
  this->cancel_timeout(DIAL_TIMEOUT);
}

void RotaryDialComponent::finish_digit_() {
  const uint8_t pulses = this->pulse_count_;
  this->pulse_count_ = 0;

  if (pulses == 0) {
    ESP_LOGD(TAG, "Dial returned without pulses, ignoring");
  } else if (pulses > MAX_PULSES) {
    ESP_LOGW(TAG, "Ignoring digit with %u pulses (max %u); check wiring or contact bounce",
             static_cast<unsigned>(pulses), static_cast<unsigned>(MAX_PULSES));
  } else {
    const uint8_t digit = pulses % 10;
    this->numbers_.push_back(static_cast<char>('0' + digit));
    ESP_LOGD(TAG, "Dialed digit %u, number so far: %s", static_cast<unsigned>(digit), this->numbers_.c_str());
    this->publish_state(digit);
  }

  if (this->numbers_.empty())
    return;

  this->set_timeout(DIAL_TIMEOUT, this->dial_timeout_, [this]() {
    ESP_LOGD(TAG, "Number complete: %s", this->numbers_.c_str());
    this->numbers_callback_.call(this->numbers_);
  });
}

void RotaryDialComponent::clear_numbers() {
  this->cancel_timeout(DIAL_TIMEOUT);
  this->numbers_.clear();
}

}  // namespace esphome::rotarydial
