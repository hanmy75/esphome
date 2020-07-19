#include "carrier_protocol.h"
#include "esphome/core/log.h"

namespace esphome {
namespace remote_base {

static const char *TAG = "remote.carrier";

static const uint8_t NBITS = 48;
static const uint32_t HEADER_HIGH_US = 8400;
static const uint32_t HEADER_LOW_US = 4170;
static const uint32_t BIT_HIGH_US = 546;
static const uint32_t BIT_ONE_LOW_US = 1540;
static const uint32_t BIT_ZERO_LOW_US = 490;
static const uint32_t FOOTER_HIGH_US = 546;
static const uint32_t FOOTER_LOW_US = 546;

void CarrierProtocol::encode(RemoteTransmitData *dst, const CarrierData &data) {
  const uint32_t *pdata;
  uint32_t mask;
  dst->set_carrier_frequency(38000);
  dst->reserve( 4 + NBITS * 2u);

  dst->item(HEADER_HIGH_US, HEADER_LOW_US);

  for (int i = (NBITS - 1); i >=0; i--) {
    pdata = (i>=32) ? &data.data1 : &data.data2;
    mask  = (i>=32) ? 1<<(i-32) : 1<<i;

    if (*pdata & mask)
      dst->item(BIT_HIGH_US, BIT_ONE_LOW_US);
    else
      dst->item(BIT_HIGH_US, BIT_ZERO_LOW_US);
  }

  dst->item(FOOTER_HIGH_US, FOOTER_LOW_US);
}

optional<CarrierData> CarrierProtocol::decode(RemoteReceiveData src) {
  uint32_t *pdata;
  CarrierData out{
      .data1 = 0,
      .data2 = 0
  };
  if (!src.expect_item(HEADER_HIGH_US, HEADER_LOW_US))
    return {};

  for (uint8_t i = 0; i < NBITS; i++) {
    pdata = (i<32) ? &out.data1 : &out.data2;
    *pdata <<= 1UL;
    if (src.expect_item(BIT_HIGH_US, BIT_ONE_LOW_US)) {
      *pdata |= 1UL;
    } else if (src.expect_item(BIT_HIGH_US, BIT_ZERO_LOW_US)) {
      *pdata |= 0UL;
    } else {
      return {};
    }
  }

  if (!src.expect_mark(FOOTER_HIGH_US))
    return {};
  return out;
}
void CarrierProtocol::dump(const CarrierData &data) { ESP_LOGD(TAG, "Received Carrier: data=0x%08x-0x%08x", data.data1 , data.data2); }

}  // namespace remote_base
}  // namespace esphome
