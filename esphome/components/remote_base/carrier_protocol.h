#pragma once

#include "esphome/core/component.h"
#include "remote_base.h"

namespace esphome {
namespace remote_base {

struct CarrierData {
  uint32_t data1;
  uint32_t data2;

  bool operator==(const CarrierData &rhs) const { return (data1 == rhs.data1) && (data2 == rhs.data2); }
};

class CarrierProtocol : public RemoteProtocol<CarrierData> {
 public:
  void encode(RemoteTransmitData *dst, const CarrierData &data) override;
  optional<CarrierData> decode(RemoteReceiveData src) override;
  void dump(const CarrierData &data) override;
};

DECLARE_REMOTE_PROTOCOL(Carrier)

template<typename... Ts> class CarrierAction : public RemoteTransmitterActionBase<Ts...> {
 public:
  TEMPLATABLE_VALUE(uint32_t, data1)
  TEMPLATABLE_VALUE(uint32_t, data2)
  void encode(RemoteTransmitData *dst, Ts... x) override {
    CarrierData data{};
    data.data1 = this->data1_.value(x...);
    data.data2 = this->data2_.value(x...);
    CarrierProtocol().encode(dst, data);
  }
};

}  // namespace remote_base
}  // namespace esphome
