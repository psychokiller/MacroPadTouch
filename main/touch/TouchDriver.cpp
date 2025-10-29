#include "TouchDriver.h"

TouchDriver::TouchDriver()
{
    touch = NULL;
}

i2c_device_config_t TouchDriver::get_device_config()
{
    // define the configuration for the I2C device
    // in this case it is the Touch Sensor GT1151N/ICNT86X
    i2c_device_config_t slave_dev_cnfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = get_chip_address(),
        .scl_speed_hz = I2C_FREQ,
        .flags = {
            .disable_ack_check = false}};

    return slave_dev_cnfg;
}
