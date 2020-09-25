import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import spi
from esphome.const import CONF_ID

DEPENDENCIES = ['spi']
AUTO_LOAD = ['sensor']
MULTI_CONF = True

CONF_MCP3008 = 'mcp3008'

mcp3008_ns = cg.esphome_ns.namespace('mcp3008')
MCP3008 = mcp3008_ns.class_('MCP3008', cg.Component, spi.SPIDevice)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(MCP3008),
}).extend(spi.spi_device_schema(cs_pin_required=True))


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield spi.register_spi_device(var, config)
