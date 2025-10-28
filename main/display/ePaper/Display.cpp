#include "Display.h"

Display::Display() : width(0), height(0), SPI_CLK_SPEED(0) {};
Display::~Display(){};

Display::Display(uint16_t width, uint16_t height, uint16_t spi_clk_speed) : width(width), height(height), SPI_CLK_SPEED(spi_clk_speed) {
    init_display_pin_configuration();
}

void Display::init_display_pin_configuration() {
    gpio_set_direction(DISPLAY_BUSY, GPIO_MODE_INPUT);
    gpio_set_direction(DISPLAY_RST, GPIO_MODE_OUTPUT);
    gpio_set_direction(SPI_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(SPI_CLK, GPIO_MODE_OUTPUT);
    gpio_set_direction(SPI_CS, GPIO_MODE_OUTPUT);
    gpio_set_direction(SPI_MOSI, GPIO_MODE_OUTPUT);

    spi_device_interface_config_t spi_config = {
        .clock_speed_hz = SPI_CLK_SPEED,
        .spics_io_num = SPI_CS,
        .queue_size = 1
    };

    spi_bus_config_t bus_config = {
        .mosi_io_num = SPI_MOSI,
        .sclk_io_num = SPI_CLK
    };

    spi_bus_initialize(DISPLAY_HOST, &bus_config, SPI_DMA_CH_AUTO);

    spi_bus_add_device(DISPLAY_HOST, &spi_config, &spi_handle);

    gpio_set_level(SPI_CS, 1);  // HIGH
    gpio_set_level(SPI_CLK, 0); // low
}

uint16_t Display::get_spi_clk_speed() {
    return this->SPI_CLK_SPEED;
}
uint16_t Display::get_width() const {
    return this->width;
}
uint16_t Display::get_height() const {
    return this->height;
}

uint16_t Display::get_total_display_size_bytes() {
    return (width * height) / 8;
}

void Display::reset() {
    gpio_set_level(DISPLAY_RST, 1); // HIGH
    vTaskDelay(pdMS_TO_TICKS(20));

    gpio_set_level(DISPLAY_RST, 0); // LOW
    vTaskDelay(pdMS_TO_TICKS(10));

    gpio_set_level(DISPLAY_RST, 1); // HIGH
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_LOGI("EPD_29", "RESET");
}

void Display::send_command(uint8_t command){
    gpio_set_level(SPI_DC, 0);
    gpio_set_level(SPI_CS, 0);
    SPI_WRITE(command, spi_handle);
    gpio_set_level(SPI_CS, 1);
}

void Display::send_data(uint8_t data) {
    gpio_set_level(SPI_DC, 1);
    gpio_set_level(SPI_CS, 0);
    SPI_WRITE(data, spi_handle);
    gpio_set_level(SPI_CS, 1);
}

void Display::send_data2(uint8_t *data, uint32_t len)
{
    gpio_set_level(SPI_DC, 1);
    gpio_set_level(SPI_CS, 0);
    SPI_WRITE_N(data, len, spi_handle);
    gpio_set_level(SPI_CS, 1);
}

void Display::read_busy()
{
    while (true)
    {
        if (gpio_get_level(DISPLAY_BUSY) == 0)
            break;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_LOGI("EPD", "READ BUSY");
}
