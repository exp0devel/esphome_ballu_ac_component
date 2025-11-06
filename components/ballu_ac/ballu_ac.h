// Custom ESPHome component for Ballu-based air conditioners (forked from AUX)
#pragma once

#include <stdarg.h>
#include <cinttypes>

#include "esphome.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

#ifndef USE_ARDUINO
using String = std::string;
#define F(string_literal) (string_literal)
#endif

namespace esphome {
namespace ballu_ac {

static const char *const TAG = "BalluAC";

using climate::ClimateFanMode;
using climate::ClimateMode;
using climate::ClimatePreset;
using climate::ClimateSwingMode;
using climate::ClimateTraits;

// Ballu AC Class
class BalluAC : public climate::Climate, public uart::UARTDevice, public Component {
 public:
  void setup() override;
  void loop() override;
  void control(const climate::ClimateCall &call) override;
  climate::ClimateTraits traits() override;

  void set_ballu_mode(bool mode) { ballu_mode_ = mode; }

 protected:
  void parse_frame_(const std::vector<uint8_t> &data);
  void send_command_(uint8_t cmd, const uint8_t *payload, size_t len);

  bool ballu_mode_{false};
  uint32_t last_poll_{0};
};

}  // namespace ballu_ac
}  // namespace esphome