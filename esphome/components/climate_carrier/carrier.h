#pragma once

#include "esphome/components/climate_ir/climate_ir.h"

namespace esphome {
namespace carrier {

// Temperature
const uint8_t CARRIER_TEMP_MIN = 17;  // Celsius
const uint8_t CARRIER_TEMP_MAX = 30;  // Celsius

class CarrierClimate : public climate_ir::ClimateIR {
 public:
  CarrierClimate() : climate_ir::ClimateIR(CARRIER_TEMP_MIN, CARRIER_TEMP_MAX, 1.0f, true, true,
                              {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM,
                               climate::CLIMATE_FAN_HIGH},
                              {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_BOTH, climate::CLIMATE_SWING_VERTICAL, 
                               climate::CLIMATE_SWING_HORIZONTAL}) {}
  void set_threshold(uint8_t threshold);

 protected:
  uint8_t threshold_;
  /// Transmit via IR the state of this climate controller.
  void transmit_state() override;
  /// Current temperature changed.
  void on_state() override;
  /// Handle received IR Buffer
  bool on_receive(remote_base::RemoteReceiveData data) override;
};

}  // namespace carrier
}  // namespace esphome
