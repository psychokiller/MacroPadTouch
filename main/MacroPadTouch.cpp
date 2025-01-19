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
#include "display/ePaper/WaveShare213.h"
#include "driver/spi_master.h"
#include "paint/GUI_Paint.h"
#include "graphics/ImageData.h"

i2c_master_dev_handle_t *i2c_dev_handle, i2c_dev;

extern i2c_device_config_t get_device_config();
extern TouchPoint scan(i2c_master_dev_handle_t *);
TouchPoint tp;

void setup_i2c_configuration();

extern "C" void app_main(void)
{
    setup_i2c_configuration();

    WaveShare213 display = WaveShare213();

    display.init(FULL);
    display.clear(BLACK);

    // Create a new image cache
    uint8_t *BlackImage;
    uint16_t Imagesize = 16 * 255;
    if ((BlackImage = (uint8_t *)malloc(Imagesize)) == NULL)
    {
        printf("Failed to apply for black memory...\r\n");
        while (1)
            ;
    }

    Paint_NewImage(BlackImage, WIDTH, HEIGHT, 90, WHITE);
    Paint_Clear(WHITE);

    display.init(FAST);
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);
    Paint_DrawBitMap(gImage_2in13);

    display.display_fast(BlackImage);
    vTaskDelay(pdMS_TO_TICKS(1000));

    while (true)
    {
        tp = scan(i2c_dev_handle);
        // use tp
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
        .flags = {
            .enable_internal_pullup = true}};

    i2c_master_bus_handle_t bus_handle;
    i2c_new_master_bus(&master_config, &bus_handle);

    i2c_device_config_t slave_dev_cnfg = get_device_config();

    i2c_dev_handle = &i2c_dev;

    i2c_master_bus_add_device(bus_handle, &slave_dev_cnfg, i2c_dev_handle);

    // configure the TOUCH_INT pin
    gpio_config_t touch_interrupt_pin_config = {
        .pin_bit_mask = TOUCH_INT,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };

    gpio_config(&touch_interrupt_pin_config);

    // configure the TOUCH_RST pin
    gpio_config_t reset_pin_config = {
        .pin_bit_mask = TOUCH_RST,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    gpio_config(&reset_pin_config);
}
