#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace ballu_ac {

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