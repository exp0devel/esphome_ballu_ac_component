#include "ballu_ac.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ballu_ac {

static const char *const TAG = "ballu_ac";

void BalluAC::setup() {}

climate::ClimateTraits BalluAC::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supports_current_temperature(true);
  traits.set_supports_two_point_target_temperature(false);
  traits.set_visual_min_temperature(16);
  traits.set_visual_max_temperature(32);
  traits.set_visual_temperature_step(0.5);
  traits.add_supported_mode(climate::CLIMATE_MODE_OFF);
  traits.add_supported_mode(climate::CLIMATE_MODE_COOL);
  traits.add_supported_mode(climate::CLIMATE_MODE_HEAT);
  traits.add_supported_mode(climate::CLIMATE_MODE_DRY);
  traits.add_supported_mode(climate::CLIMATE_MODE_FAN_ONLY);
  traits.add_supported_fan_mode(climate::CLIMATE_FAN_AUTO);
  traits.add_supported_fan_mode(climate::CLIMATE_FAN_LOW);
  traits.add_supported_fan_mode(climate::CLIMATE_FAN_MEDIUM);
  traits.add_supported_fan_mode(climate::CLIMATE_FAN_HIGH);
  return traits;
}

void BalluAC::loop() {
  if (millis() - this->last_poll_ > this->period_) {
    this->last_poll_ = millis();
    uint8_t ping[] = {0x7E, 0x00};
    this->write_array(ping, sizeof(ping));
  }

  std::vector<uint8_t> data;
  while (this->available()) {
    data.push_back(this->read());
  }
  if (!data.empty())
    this->parse_frame_(data);
}

void BalluAC::send_command_(uint8_t cmd, const uint8_t *payload, size_t len) {
  std::vector<uint8_t> frame;
  frame.reserve(len + 3);
  frame.push_back(0x7E);
  frame.push_back(cmd);
  for (size_t i = 0; i < len; ++i) frame.push_back(payload[i]);
  uint8_t crc = 0;
  for (size_t i = 0; i < frame.size(); ++i) crc += frame[i];
  frame.push_back(crc);
  this->write_array(frame.data(), frame.size());
  ESP_LOGV(TAG, "Sent command 0x%02X", cmd);
}

void BalluAC::parse_frame_(const std::vector<uint8_t> &data) {
  if (data.empty()) return;
  if (data[0] != 0x7E) {
    ESP_LOGD(TAG, "Not a Ballu frame (no 0x7E)");
    return;
  }
  if (data.size() < 8) return;

  uint8_t crc = 0;
  for (size_t i = 0; i < data.size() - 1; i++) crc += data[i];
  if (crc != data.back()) {
    ESP_LOGD(TAG, "CRC mismatch");
    return;
  }

  uint8_t temp_bcd = data[4];
  float temp = ((temp_bcd >> 4) & 0x0F) * 10 + (temp_bcd & 0x0F);
  this->current_temperature = temp;

  uint8_t mode = data[5] & 0x07;
  switch (mode) {
    case 0x01: this->mode = climate::CLIMATE_MODE_COOL; break;
    case 0x02: this->mode = climate::CLIMATE_MODE_HEAT; break;
    case 0x03: this->mode = climate::CLIMATE_MODE_DRY; break;
    case 0x04: this->mode = climate::CLIMATE_MODE_FAN_ONLY; break;
    default: break;
  }

  uint8_t fan = data[6] & 0x03;
  if (fan == 0x00) this->fan_mode = climate::CLIMATE_FAN_AUTO;
  else if (fan == 0x01) this->fan_mode = climate::CLIMATE_FAN_LOW;
  else if (fan == 0x02) this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
  else if (fan == 0x03) this->fan_mode = climate::CLIMATE_FAN_HIGH;

  this->target_temperature = temp;
  this->publish_state();
}

void BalluAC::control(const climate::ClimateCall &call) {
  auto new_mode = this->mode;
  auto new_fan_mode = this->fan_mode;
  float new_target = this->target_temperature;
  bool changed = false;

  if (call.get_mode().has_value()) {
    new_mode = *call.get_mode();
    changed = true;
  }
  if (call.get_fan_mode().has_value()) {
    new_fan_mode = *call.get_fan_mode();
    changed = true;
  }
  if (call.get_target_temperature().has_value()) {
    new_target = *call.get_target_temperature();
    changed = true;
  }
  if (!changed) return;

  if (new_target < 16.0f) new_target = 16.0f;
  if (new_target > 32.0f) new_target = 32.0f;
  uint8_t target_int = static_cast<uint8_t>(new_target + 0.5f);
  uint8_t temp_bcd = ((target_int / 10) << 4) | (target_int % 10);

  uint8_t mode_bits = 0x00;
  switch (new_mode) {
    case climate::CLIMATE_MODE_COOL: mode_bits = 0x01; break;
    case climate::CLIMATE_MODE_HEAT: mode_bits = 0x02; break;
    case climate::CLIMATE_MODE_DRY: mode_bits = 0x03; break;
    case climate::CLIMATE_MODE_FAN_ONLY: mode_bits = 0x04; break;
    default: break;
  }

  uint8_t fan_bits = 0x00;
  switch (new_fan_mode) {
    case climate::CLIMATE_FAN_LOW: fan_bits = 0x01; break;
    case climate::CLIMATE_FAN_MEDIUM: fan_bits = 0x02; break;
    case climate::CLIMATE_FAN_HIGH: fan_bits = 0x03; break;
    default: fan_bits = 0x00; break;
  }

  bool power_on = new_mode != climate::CLIMATE_MODE_OFF;
  uint8_t payload[5] = {0x00, power_on ? 0x01 : 0x00, temp_bcd, mode_bits, fan_bits};
  this->send_command_(0x01, payload, sizeof(payload));

  this->mode = new_mode;
  this->fan_mode = new_fan_mode;
  this->target_temperature = new_target;
  this->publish_state();
}

}  // namespace ballu_ac
}  // namespace esphome