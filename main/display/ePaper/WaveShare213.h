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


// Display resolution
#define WIDTH 122
#define HEIGHT 250

// Display refresh/redraw modes
typedef enum
{
    FULL,
    PARTIAL,
    FAST
} display_refresh_mode;



class WaveShare213
{
public:
    WaveShare213();
    ~WaveShare213();
    uint8_t width;
    uint8_t height;
    spi_device_handle_t spi_handle;
    void clear(display_color color);

    void read_busy();
    void turn_display_on();
    void turn_display_on_fast();
    void turn_display_on_partial();
    void turn_display_on_partial_wait();
    void sleep();
    void reset();

    void init(display_refresh_mode mode);
    void display(uint8_t *image);
    void display_fast(uint8_t *image);
    void display_base(uint8_t *image);
    void display_partial(uint8_t *image);
    void display_partial_wait(uint8_t *image);
    void send_command(uint8_t command);
    void send_data(uint8_t data);
    void send_data2(uint8_t *data, uint32_t len);
    void set_window(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end);
    void set_cursor(uint16_t x, uint16_t y);

private:
    uint16_t get_screen_width_bytes();
};

#endif