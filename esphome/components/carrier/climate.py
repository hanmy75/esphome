import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate_ir
from esphome.const import CONF_ID, CONF_THRESHOLD

AUTO_LOAD = ['climate_ir']

carrier_ns = cg.esphome_ns.namespace('carrier')
CarrierClimate = carrier_ns.class_('CarrierClimate', climate_ir.ClimateIR)

CONFIG_SCHEMA = climate_ir.CLIMATE_IR_WITH_RECEIVER_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(CarrierClimate),
    cv.Optional(CONF_THRESHOLD, default=3): cv.uint8_t,
})


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield climate_ir.register_climate_ir(var, config)

    cg.add(var.set_threshold(config[CONF_THRESHOLD]))

