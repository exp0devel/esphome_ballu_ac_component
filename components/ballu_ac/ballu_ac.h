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

 protected:
  void parse_frame_(const std::vector<uint8_t> &data);
  void send_command_(uint8_t cmd, const uint8_t *payload, size_t len);

  uint32_t last_poll_{0};
  uint32_t period{7000};  // Default 7s poll
};

}  // namespace ballu_ac
}  // namespace esphome