#include "carrier.h"
#include "esphome/core/log.h"

namespace esphome {
namespace carrier {

static const char *TAG = "carrier.climate";

static const uint8_t NBITS = 48;
static const uint16_t NREPEAT = 3;

static const uint32_t HEADER_MARK_US = 8400;
static const uint32_t HEADER_SPACE_US = 4150;
static const uint32_t BIT_MARK_US = 550;
static const uint32_t BIT_ONE_SPACE_US = 1540;
static const uint32_t BIT_ZERO_SPACE_US = 500;
static const uint32_t FOOTER_MARK_US = 500;
static const uint32_t FOOTER_SPACE_US = 500;
static const uint32_t LOOP_DELAY_US = 130000;

/* Carrier IR Data format */
static const uint64_t DEFAULT_DATA = 0x0000c00080100000ULL;

static const uint64_t COUNT1_SHIFT = 44;
static const uint64_t COUNT1_MASK = (0x3ULL << COUNT1_SHIFT);

static const uint64_t COUNT2_SHIFT = 4;
static const uint64_t COUNT2_MASK = (0xfULL << COUNT2_SHIFT);

static const uint64_t COUNT3_SHIFT = 0;
static const uint64_t COUNT3_MASK = (0xfULL << COUNT3_SHIFT);

static const uint64_t MODE_SHIFT = 38;
static const uint64_t MODE_MASK = (0x3ULL << MODE_SHIFT);

static const uint64_t LEFT_FAN_SHIFT = 32;
static const uint64_t LEFT_FAN_MASK = (0x1ULL << LEFT_FAN_SHIFT);

static const uint64_t FAN_NUM_SHIFT = 39;
static const uint64_t FAN_NUM_MASK = (0x3ULL << FAN_NUM_SHIFT);

static const uint64_t SPEED_SHIFT = 24;
static const uint64_t SPEED_MASK = (0x1fULL << SPEED_SHIFT);

static const uint64_t TEMP_SHIFT = 16;
static const uint64_t TEMP_MASK = (0xfULL << TEMP_SHIFT);

static const uint64_t POWER_SHIFT = 15;
static const uint64_t POWER_MASK = (0x1ULL << POWER_SHIFT);

static const uint64_t RIGHT_FAN_SHIFT = 14;
static const uint64_t RIGHT_FAN_MASK = (0x1ULL << RIGHT_FAN_SHIFT);


/*
  Mode    : 1(Cooling), 2(Air Cleaning), 3(Dehumidification)
  Temp    : min 17(1) ~ max 30(e)
  Speed   : min(0) ~ max(18)
  Fan Num : min 1(0) ~ max 3(2)
*/


void CarrierClimate::transmit_state() {
  uint64_t remote_state = DEFAULT_DATA;

  switch (this->mode) {
    case climate::CLIMATE_MODE_COOL:
    case climate::CLIMATE_MODE_AUTO:
      remote_state &= ~(POWER_MASK & MODE_MASK);
	  remote_state |= (1ULL << POWER_SHIFT) | (1ULL << MODE_SHIFT); /* Power on with Cooling. */
      break;

    case climate::CLIMATE_MODE_OFF:
      remote_state &= ~POWER_MASK; /* Power off */
	  break;

    default:
      break;
  }

  if (this->mode != climate::CLIMATE_MODE_OFF) {
    auto temp = (uint8_t) roundf(clamp(this->target_temperature, CARRIER_TEMP_MIN, CARRIER_TEMP_MAX));
	remote_state &= ~TEMP_MASK;
    remote_state |= (temp-CARRIER_TEMP_MIN+1) << TEMP_SHIFT;
  }

  ESP_LOGV(TAG, "Sending carrier code: 0x%04X-0x%08X", (remote_state>>32), (remote_state&0xffffffff));

  auto transmit = this->transmitter_->transmit();
  auto data = transmit.get_data();

  data->set_carrier_frequency(38000);

  // Header
  data->mark(HEADER_MARK_US);
  data->space(HEADER_SPACE_US);

  // Data
  for (uint8_t i = 0; i < NBITS; i++) {
	uint64_t mask = (1ULL<<(NBITS-i-1));
    data->mark(BIT_MARK_US);
    data->space((remote_state & mask) ? BIT_ONE_SPACE_US : BIT_ZERO_SPACE_US);
  }

  // Footer
  data->mark(FOOTER_MARK_US);
  data->space(FOOTER_SPACE_US);  // Pause before repeating

  transmit.set_send_times(NREPEAT);
  transmit.set_send_wait(LOOP_DELAY_US);
  transmit.perform();

}

bool CarrierClimate::on_receive(remote_base::RemoteReceiveData data) {
  uint64_t remote_state = 0;
  uint8_t  power, mode, temp, speed, fan_num, left_fan, right_fan;
  uint8_t  count1, count2, count3;

  if (!data.expect_item(HEADER_MARK_US, HEADER_SPACE_US))
     return false;

  for (uint8_t i = 0; i < NBITS; i++) {
    remote_state <<= 1UL;
    if (data.expect_item(BIT_MARK_US, BIT_ONE_SPACE_US)) {
	  remote_state |= 1UL;
    } else if (data.expect_item(BIT_MARK_US, BIT_ZERO_SPACE_US)) {
	  remote_state |= 0UL;
    } else {
	  return false;
    }
  }

  if (!data.expect_mark(FOOTER_MARK_US))
    return false;

  ESP_LOGV(TAG, "Receive carrier code: 0x%04X-0x%08X", (remote_state>>32), (remote_state&0xffffffff));

  power     = (remote_state & POWER_MASK) >> POWER_SHIFT;
  mode      = (remote_state & MODE_MASK) >> MODE_SHIFT;
  temp      = (remote_state & TEMP_MASK) >> TEMP_SHIFT;
  speed     = (remote_state & SPEED_MASK) >> SPEED_SHIFT;
  fan_num   = (remote_state & FAN_NUM_MASK) >> FAN_NUM_SHIFT;
  left_fan  = (remote_state & LEFT_FAN_MASK) >> LEFT_FAN_SHIFT;
  right_fan = (remote_state & RIGHT_FAN_MASK) >> RIGHT_FAN_SHIFT;

  count1    = (remote_state & COUNT1_MASK) >> COUNT1_SHIFT;
  count2    = (remote_state & COUNT2_MASK) >> COUNT2_SHIFT;
  count3    = (remote_state & COUNT3_MASK) >> COUNT3_SHIFT;

  ESP_LOGV(TAG, "Receive carrier decode: power %d, mode %d, temp %d, speed %d, fan_num %d, left_fan %d, right_fan %d count (%d,%d,%d)",
    power, mode, temp, speed, fan_num, left_fan, right_fan, count1, count2, count3);

  return true;
}

}  // namespace carrier
}  // namespace esphome
