#include "carrier.h"
#include "esphome/core/log.h"

namespace esphome {
namespace carrier {

#define FAN_NUM_ONE     4ULL
#define FAN_NUM_TWO     6ULL
#define FAN_NUN_THREE   7ULL

#define FAN_MODE_COOLING      0x04ULL
#define FAN_MODE_CLEANING     0x08ULL
#define FAN_MODE_DEHUMIDITY   0x0AULL

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
static const uint64_t COUNT_SHIFT = 44;
static const uint64_t COUNT_MASK = (0x3ULL << COUNT_SHIFT);

static const uint64_t LEFT_FAN_SHIFT = 32;
static const uint64_t LEFT_FAN_MASK = (0x1ULL << LEFT_FAN_SHIFT);

static const uint64_t RIGHT_FAN_SHIFT = 14;
static const uint64_t RIGHT_FAN_MASK = (0x1ULL << RIGHT_FAN_SHIFT);

static const uint64_t FAN_NUM_SHIFT = 29;
static const uint64_t FAN_NUM_MASK = (0x7ULL << FAN_NUM_SHIFT);

static const uint64_t SPEED_SHIFT = 24;
static const uint64_t SPEED_MASK = (0x1fULL << SPEED_SHIFT);

static const uint64_t TEMP_SHIFT = 16;
static const uint64_t TEMP_MASK = (0xfULL << TEMP_SHIFT);

static const uint64_t POWER_SHIFT = 15;
static const uint64_t POWER_MASK = (0x1ULL << POWER_SHIFT);

static const uint64_t MODE_SHIFT = 36;
static const uint64_t MODE_MASK = (0xfULL << MODE_SHIFT);

static const uint64_t PMV1_SHIFT = 11;
static const uint64_t PMV2_SHIFT = 20;
static const uint64_t PMV_MASK = ((1ULL << PMV1_SHIFT) | (1ULL << PMV2_SHIFT));

static const uint64_t CHECKSUM_SHIFT = 0;
static const uint64_t CHECKSUM_MASK = (0xffULL << CHECKSUM_SHIFT);

static const uint64_t DEFAULT_FAN_SPEED = 18ULL;
static const uint64_t AUTO_FAN_SPEED = 31ULL;

/*
  Temp    : min 17(1) ~ max 30(14) 
  Speed   : min(5) ~ max(18), PMV(Auto) 31
*/
static const uint64_t DEFAULT_DATA = 0x0000c00000100000ULL | (FAN_MODE_COOLING << MODE_SHIFT) | (FAN_NUN_THREE <<FAN_NUM_SHIFT);
static uint64_t remote_state = DEFAULT_DATA;

static void parse_data(uint64_t data) {
#if 0
  uint8_t  power, mode, pmv1, pmv2, temp, speed, fan_num, left_fan, right_fan;
  uint8_t  count, checksum;

  power     = (data & POWER_MASK) >> POWER_SHIFT;
  mode      = (data & MODE_MASK) >> MODE_SHIFT;
  pmv1      = (data & PMV_MASK) >> PMV1_SHIFT;
  pmv2      = (data & PMV_MASK) >> PMV2_SHIFT;

  temp      = (data & TEMP_MASK) >> TEMP_SHIFT;
  speed     = (data & SPEED_MASK) >> SPEED_SHIFT;
  fan_num   = (data & FAN_NUM_MASK) >> FAN_NUM_SHIFT;
  left_fan  = (data & LEFT_FAN_MASK) >> LEFT_FAN_SHIFT;
  right_fan = (data & RIGHT_FAN_MASK) >> RIGHT_FAN_SHIFT;

  count    = (data & COUNT_MASK) >> COUNT_SHIFT;
  checksum    = (data & CHECKSUM_MASK) >> CHECKSUM_SHIFT;

  ESP_LOGD(TAG, "Parse Data: power %d, mode %d, pmv(%d-%d), count %d, checksum 0x%02x", power, mode, pmv1, pmv2, count, checksum);
  ESP_LOGD(TAG, "            temp %d, speed %d, fan_num %d, left_fan %d, right_fan %d", temp, speed, fan_num, left_fan, right_fan);
#else
  int bit_shift;
  uint8_t  checksum, cal_checksum = 0;

  checksum = (data & CHECKSUM_MASK) >> CHECKSUM_SHIFT;

  for(bit_shift=8; bit_shift<NBITS; bit_shift+=8)
  {
    cal_checksum += (data >> bit_shift) & 0xff;
  }

  cal_checksum &= 0xff;

  ESP_LOGD(TAG, "Checksum: actual (0x%02x) calculate (0x%02x)", checksum, cal_checksum);
#endif
}

void CarrierClimate::transmit_state() {
  static uint64_t count=0;
  uint8_t bit_shift, checksum = 0;

  /* Power on/off */
  switch (this->mode) {
    case climate::CLIMATE_MODE_COOL:
    case climate::CLIMATE_MODE_AUTO:
      remote_state &= ~(POWER_MASK);
      remote_state |= (1ULL << POWER_SHIFT);
      break;

    case climate::CLIMATE_MODE_OFF:
      remote_state &= ~(POWER_MASK);
      break;
    default:
      break;
  }

  /* Fan control */
  switch (this->mode) {
    case climate::CLIMATE_MODE_COOL:
      remote_state &= ~(SPEED_MASK | LEFT_FAN_MASK | RIGHT_FAN_MASK | PMV_MASK);
      remote_state |= (DEFAULT_FAN_SPEED << SPEED_SHIFT) | (1ULL << LEFT_FAN_SHIFT) | (1ULL << RIGHT_FAN_SHIFT) | (1ULL << PMV2_SHIFT);
      break;

    case climate::CLIMATE_MODE_AUTO:
      remote_state &= ~(SPEED_MASK | LEFT_FAN_MASK | RIGHT_FAN_MASK | PMV_MASK);
      remote_state |= (AUTO_FAN_SPEED << SPEED_SHIFT) | (1ULL << PMV1_SHIFT);
      break;

    default:
      break;
  }
  
  /* Tempreture */
  if (this->mode != climate::CLIMATE_MODE_OFF) {
    auto temp = (uint64_t) roundf(clamp(this->target_temperature, CARRIER_TEMP_MIN, CARRIER_TEMP_MAX));

	remote_state &= ~TEMP_MASK;
    remote_state |= (temp-CARRIER_TEMP_MIN+1ULL) << TEMP_SHIFT;
  }

  /* Counter */
  count = (count + 1) & 0x03;
  remote_state &= ~(COUNT_MASK);
  remote_state |= (count << COUNT_SHIFT);

  /* Checksum */
  for(bit_shift=8; bit_shift<NBITS; bit_shift+=8)
  {
    checksum += (remote_state >> bit_shift) & 0xff;
  }
  checksum &= 0xff;

  remote_state &= ~(CHECKSUM_MASK);
  remote_state |= (checksum << CHECKSUM_SHIFT);

  ESP_LOGD(TAG, "Sending carrier code: 0x%04x%08x", (uint32_t)(remote_state>>32), (uint32_t)(remote_state&0xffffffffULL));
  parse_data(remote_state);

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
  uint64_t receive_data = 0;


  if (!data.expect_item(HEADER_MARK_US, HEADER_SPACE_US))
     return false;

  for (uint8_t i = 0; i < NBITS; i++) {
    receive_data <<= 1UL;
    if (data.expect_item(BIT_MARK_US, BIT_ONE_SPACE_US)) {
	  receive_data |= 1UL;
    } else if (data.expect_item(BIT_MARK_US, BIT_ZERO_SPACE_US)) {
	  receive_data |= 0UL;
    } else {
	  return false;
    }
  }

  if (!data.expect_mark(FOOTER_MARK_US))
    return false;

  ESP_LOGD(TAG, "Receive carrier code: 0x%04x%08x", (uint32_t)(receive_data>>32), (uint32_t)(receive_data&0xffffffffULL));
  parse_data(receive_data);

  return true;
}

}  // namespace carrier
}  // namespace esphome
