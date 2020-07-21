#pragma once

#include "esphome/components/climate_ir/climate_ir.h"

namespace esphome {
namespace carrier {

// Temperature
const uint8_t CARRIER_TEMP_MIN = 17;  // Celsius
const uint8_t CARRIER_TEMP_MAX = 30;  // Celsius

class CarrierClimate : public climate_ir::ClimateIR {
 public:
  CarrierClimate() : climate_ir::ClimateIR(CARRIER_TEMP_MIN, CARRIER_TEMP_MAX) {}

 protected:
  /// Transmit via IR the state of this climate controller.
  void transmit_state() override;
  /// Handle received IR Buffer
  bool on_receive(remote_base::RemoteReceiveData data) override;
};

}  // namespace carrier
}  // namespace esphome
