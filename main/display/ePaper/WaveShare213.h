#ifndef _WaveShare213_H_
#define _WaveShare213_H_

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
#include "../DisplayColors.h"
#include "../DisplayRefreshModes.h"
#include "Display.h"

class WaveShare213: public Display
{
public:
    WaveShare213();
    ~WaveShare213();

    void clear(display_color color);

    void turn_display_on();
    void turn_display_on_fast();
    void turn_display_on_partial();
    void turn_display_on_partial_wait();
    void sleep();

    void init();
    void init(display_refresh_mode mode);
    void display(uint8_t *image);
    void display_fast(uint8_t *image);
    void display_base(uint8_t *image);
    void display_partial(uint8_t *image);
    void display_partial_wait(uint8_t *image);
    void set_window(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end);
    void set_cursor(uint16_t x, uint16_t y);
    uint16_t get_screen_size_bytes();

private:
    uint16_t get_screen_width_bytes();
    static const uint16_t spi_clock_speed = 40000;
    static const uint16_t width = 122; // Portrait orientation
    static const uint16_t height = 250; // Portrait orientation
};

#endif