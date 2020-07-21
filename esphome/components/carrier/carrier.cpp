#include "carrier.h"
#include "esphome/core/log.h"

namespace esphome {
namespace carrier {

static const char *TAG = "carrier.climate";

void CarrierClimate::transmit_state() {
  uint32_t remote_state;

  switch (this->mode) {
    case climate::CLIMATE_MODE_COOL:
      break;
    case climate::CLIMATE_MODE_HEAT:
      break;
    case climate::CLIMATE_MODE_AUTO:
      break;
    case climate::CLIMATE_MODE_OFF:
    default:
      remote_state = 0;
      break;
  }
  if (this->mode != climate::CLIMATE_MODE_OFF) {
    auto temp = (uint8_t) roundf(clamp(this->target_temperature, 0, 100));
  }

  ESP_LOGV(TAG, "Sending carrier code: 0x%02X", remote_state);

  auto transmit = this->transmitter_->transmit();
  auto data = transmit.get_data();

  data->set_carrier_frequency(38000);

  transmit.perform();
}

bool CarrierClimate::on_receive(remote_base::RemoteReceiveData data) {
  return false;
}

}  // namespace carrier
}  // namespace esphome
