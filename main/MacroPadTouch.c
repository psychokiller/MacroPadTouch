#include <stdio.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#ifdef GT1151
    #include "touch/GT1151.h"
#endif

#include "touch/TouchPoint.h"
#include "Config.h"
#include "Utils.h"

static const char *TAG = "Touch_Driver";
i2c_master_dev_handle_t *dev_handle, i2c_dev_handle;
TouchPoint *old_touch_point, touch_point;

extern i2c_device_config_t get_device_config();
extern TouchPoint scan();

void setup_i2c_configuration();


void app_main(void)
{
    setup_i2c_configuration();

    while (true)
    {
        scan(dev_handle);
    }
}

void setup_i2c_configuration()
{
    // define I2C bus configuration
    i2c_master_bus_config_t master_config = {
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .flags.enable_internal_pullup = true};

    i2c_master_bus_handle_t bus_handle;
    i2c_new_master_bus(&master_config, &bus_handle);

    i2c_device_config_t slave_dev_cnfg = get_device_config();

    dev_handle = &i2c_dev_handle;

    i2c_master_bus_add_device(bus_handle, &slave_dev_cnfg, dev_handle);

    // configure the TOUCH_INT pin
    gpio_config_t touch_interrupt_pin_config = {
        .pull_up_en = true,
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_NEGEDGE,
        .pin_bit_mask = TOUCH_INT};

    gpio_config(&touch_interrupt_pin_config);

    // configure the TOUCH_RST pin
    gpio_config_t reset_pin_config = {
        .pull_up_en = true,
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = TOUCH_RST};

    gpio_config(&reset_pin_config);
}
