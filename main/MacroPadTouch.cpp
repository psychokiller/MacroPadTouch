#include <stdio.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "touch/GT1151.h"
#include "touch/ICNT86X.h"
#include "touch/TouchPoint.h"
#include "Config.h"
#include "Utils.h"
#include "display/ePaper/WaveShare213.h"
#include "display/ePaper/WaveShare29.h"
#include "driver/spi_master.h"
#include "paint/GUI_Paint.h"
#include "graphics/ImageData.h"
#include "fonts/fonts.h"
#include "graphics/Button.h"
#include "ui/UiManager.h"
#include <string>
#include<vector>

i2c_master_dev_handle_t *i2c_dev_handle, i2c_dev;

TouchPoint tp;

void setup_i2c_configuration(TouchDriver*);
void clear_screen(uint8_t *BlankDisplayImage, Display &display, display_color color);

extern "C" void app_main(void)
{

    TouchDriver* touchDriver = new Gt1151();
    // TouchDriver* touchDriver = new Icnt86x();
    setup_i2c_configuration(touchDriver);
    touchDriver->init(i2c_dev_handle);
    vTaskDelay(pdMS_TO_TICKS(500));

    WaveShare213 display = WaveShare213();
    // WaveShare29 display = WaveShare29();

    display.init();
    display.clear(BLACK);

    // Create a new image cache
    uint8_t *BlankDisplayImage;
    uint16_t Imagesize = display.get_screen_size_bytes(); // should be Display Width * Height (in bytes not pixels)
    if ((BlankDisplayImage = (uint8_t *)malloc(Imagesize)) == NULL)
    {
        printf("Failed to apply for black memory...\r\n");
        while (1)
            ;
    }

    clear_screen(BlankDisplayImage, display, WHITE);

    const uint16_t number_of_buttons = 6;

    UiManager* ui = new UiManager(display);
    std::vector<Button*> buttons;
    

    // Button** buttons = new Button*[number_of_buttons];

    for (int i = 0; i < number_of_buttons ; i ++) {
        const char* btn_header = ("KOKO" + std::to_string(i)).c_str();
        ESP_LOGI("Main", "Btn Header: %s", btn_header);
        buttons.push_back(new Button(const_cast<char*>( btn_header), &Font20, BLACK, WHITE));
    }

    ui->draw(buttons);

    // Paint_DrawString_EN(0,0, "KOKOKOKO", &Font24, WHITE, BLACK);
    // Paint_DrawRectangle(0,0,50,50,BLACK,DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    // Paint_DrawBitMap(gImage_2in9);

    display.display(BlankDisplayImage);
    
    while (true)
    {
        tp = touchDriver->scan(i2c_dev_handle);
        
        // use tp
    }
}



void clear_screen(uint8_t *BlankDisplayImage, Display &display, display_color color)
{
    Paint_NewImage(BlankDisplayImage, display.get_width(), display.get_height(), ROTATE_90, color);
    Paint_Clear(color);
}

void setup_i2c_configuration(TouchDriver* td)
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

    i2c_device_config_t slave_dev_cnfg = td->get_device_config();

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
