#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include "../DisplayColors.h"
#include "../DisplayRefreshModes.h"
#include <stdio.h>
#include "Config.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_check.h"
#include "esp_log.h"
#include "Utils.h"
#include "soc/soc.h"
#include "soc/gpio_periph.h"

class Display {
public:
    Display();
    Display(uint16_t, uint16_t, uint16_t);
    ~Display();
    spi_device_handle_t spi_handle;
    
    virtual void clear(display_color color) = 0;
    virtual void turn_display_on() = 0;
    virtual void sleep() = 0;
    
    virtual void init(display_refresh_mode mode) = 0;
    virtual void init() = 0;
    virtual void display(uint8_t *image) = 0;

    void read_busy();
    void reset();
    void send_command(uint8_t command);
    void send_data(uint8_t data);
    void send_data2(uint8_t *data, uint32_t len);
    
    uint16_t get_width() const;
    uint16_t get_height() const;
    uint16_t get_spi_clk_speed();
    uint16_t get_total_display_size_bytes();

private:
    uint16_t width;
    uint16_t height;
    uint16_t SPI_CLK_SPEED;
    void init_display_pin_configuration();
};

#endif