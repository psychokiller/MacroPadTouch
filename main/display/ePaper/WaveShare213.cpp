#include "WaveShare213.h"

WaveShare213::WaveShare213()
{
    gpio_set_direction(DISPLAY_BUSY, GPIO_MODE_INPUT);
    gpio_set_direction(DISPLAY_RST, GPIO_MODE_OUTPUT);
    gpio_set_direction(SPI_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(SPI_CLK, GPIO_MODE_OUTPUT);
    gpio_set_direction(SPI_CS, GPIO_MODE_OUTPUT);
    gpio_set_direction(SPI_MOSI, GPIO_MODE_OUTPUT);

    spi_device_interface_config_t spi_config = {
        .clock_speed_hz = 40000,
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
};

void WaveShare213::reset()
{
    gpio_set_level(DISPLAY_RST, 1); // HIGH
    vTaskDelay(pdMS_TO_TICKS(20));

    gpio_set_level(DISPLAY_RST, 0); // LOW
    vTaskDelay(pdMS_TO_TICKS(2));

    gpio_set_level(DISPLAY_RST, 1); // HIGH
    vTaskDelay(pdMS_TO_TICKS(20));
    ESP_LOGI("EPD", "RESET");
}

void WaveShare213::send_command(uint8_t command)
{
    gpio_set_level(SPI_DC, 0);
    gpio_set_level(SPI_CS, 0);
    SPI_WRITE(command, spi_handle);
    gpio_set_level(SPI_CS, 1);
}

void WaveShare213::send_data(uint8_t data)
{
    gpio_set_level(SPI_DC, 1);
    gpio_set_level(SPI_CS, 0);
    SPI_WRITE(data, spi_handle);
    gpio_set_level(SPI_CS, 1);
}

void WaveShare213::send_data2(uint8_t *data, uint32_t len)
{
    gpio_set_level(SPI_DC, 1);
    gpio_set_level(SPI_CS, 0);
    SPI_WRITE_N(data, len, spi_handle);
    gpio_set_level(SPI_CS, 1);
}

void WaveShare213::read_busy()
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

void WaveShare213::set_window(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end)
{
    send_command(0x44); // SET_RAM_X_ADDRESS_START_END_POSITION
    send_data((x_start >> 3) & 0xFF);
    send_data((x_end >> 3) & 0xFF);

    send_command(0x45); // SET_RAM_Y_ADDRESS_START_END_POSITION
    send_data(y_start & 0xFF);
    send_data((y_start >> 8) & 0xFF);
    send_data(y_end & 0xFF);
    send_data((y_end >> 8) & 0xFF);
}

void WaveShare213::set_cursor(uint16_t x, uint16_t y)
{
    send_command(0x4E); // SET_RAM_X_ADDRESS_COUNTER
    send_data(x & 0xFF);

    send_command(0x4F); // SET_RAM_Y_ADDRESS_COUNTER
    send_data(y & 0xFF);
    send_data((y >> 8) & 0xFF);
}

void WaveShare213::turn_display_on()
{
    send_command(0x22); // Display Update Control
    send_data(0xf7);
    send_command(0x20); // Activate Display Update Sequence
    read_busy();
    ESP_LOGI("EPD", "TURN DISPLAY ON");
}

void WaveShare213::turn_display_on_fast()
{
    send_command(0x22); // Display Update Control
    send_data(0xc7);    // fast:0x0c, quality:0x0f, 0xcf
    send_command(0x20); // Activate Display Update Sequence
    read_busy();
}

void WaveShare213::turn_display_on_partial()
{
    send_command(0x22); // Display Update Control
    send_data(0xff);    // fast:0x0c, quality:0x0f, 0xcf
    send_command(0x20); // Activate Display Update Sequence
}

void WaveShare213::turn_display_on_partial_wait()
{
    send_command(0x22); // Display Update Control
    send_data(0xff);    // fast:0x0c, quality:0x0f, 0xcf
    send_command(0x20); // Activate Display Update Sequence
    read_busy();
}

void WaveShare213::init(display_refresh_mode mode)
{
    if (mode == FULL)
    {
        reset();

        read_busy();
        send_command(0x12); // SWRESET
        read_busy();

        send_command(0x01); // Driver output control
        send_data(0xF9);
        send_data(0x00);
        send_data(0x00);
        // send_data(0x27);
        // send_data(0x01);
        // send_data(0x00);

        send_command(0x11); // data entry mode
        send_data(0x03);

        set_window(0, 0, WIDTH, HEIGHT);
        set_cursor(0, 0);

        send_command(0x3C); // BorderWavefrom
        send_data(0x05);

        send_command(0x21); //  Display update control
        send_data(0x00);
        send_data(0x80);

        send_command(0x18); // Read built-in temperature sensor
        send_data(0x80);

        read_busy();
        ESP_LOGI("EPD", "FULL INIT");
    }
    else if (mode == PARTIAL)
    {
        // Reset
        gpio_set_level(DISPLAY_RST, 0);
        vTaskDelay(pdMS_TO_TICKS(1));
        gpio_set_level(DISPLAY_RST, 1);

        send_command(0x3C); // BorderWavefrom
        send_data(0x80);

        send_command(0x01); // Driver output control
        send_data(0xF9);
        send_data(0x00);
        send_data(0x00);

        send_command(0x11); // data entry mode
        send_data(0x03);

        set_window(0, 0, WIDTH, HEIGHT);
        set_cursor(0, 0);
    }
    else
    {
        reset();
        send_command(0x12); // SWRESET
        read_busy();

        send_command(0x18); // Read built-in temperature sensor
        send_data(0x80);

        send_command(0x11); // data entry mode
        send_data(0x03);

        set_window(0, 0, WIDTH, HEIGHT);
        set_cursor(0, 0);

        send_command(0x22); // Load temperature value
        send_data(0xB1);
        send_command(0x20);
        read_busy();

        send_command(0x1A); // Write to temperature register
        send_data(0x64);
        send_data(0x00);

        send_command(0x22); // Load temperature value
        send_data(0x91);
        send_command(0x20);
        read_busy();
    }

    ESP_LOGI("EPD", "INIT");
}

uint16_t WaveShare213::get_screen_width_bytes()
{
    return (WIDTH % 8 == 0) ? (WIDTH / 8) : (WIDTH / 8 + 1);
}

void WaveShare213::clear(display_color color)
{
    uint16_t Width, Height;
    Width = get_screen_width_bytes();
    Height = HEIGHT;

    send_command(0x24);
    for (uint16_t j = 0; j < Height; j++)
    {
        for (uint16_t i = 0; i < Width; i++)
        {
            send_data(color);
        }
    }

    turn_display_on();
    ESP_LOGI("EPD", "CLEAR with %X", color);
}

void WaveShare213::display(uint8_t *image)
{
    uint16_t Width, Height;
    Width = get_screen_width_bytes();
    Height = HEIGHT;

    send_command(0x24);
    for (uint16_t j = 0; j < Height; j++)
    {
        for(uint16_t i = 0 ; i < Width; i++){
            send_data(image[i + j * Width]);
        }
    }

    turn_display_on();
}

void WaveShare213::display_fast(uint8_t *image)
{
    uint16_t Width, Height;
    Width = get_screen_width_bytes();
    Height = HEIGHT;

    send_command(0x24);
    for (uint16_t j = 0; j < Height; j++)
    {
        for (uint16_t i = 0; i < Width; i++)
        {
            send_data(image[i + j * Width]);
        }
    }

    turn_display_on_fast();
}

void WaveShare213::display_base(uint8_t *image)
{
    uint16_t Width, Height;
    Width = get_screen_width_bytes();
    Height = HEIGHT;

    send_command(0x24);
    for (uint16_t j = 0; j < Height; j++)
    {
        for (uint16_t i = 0; i < Width; i++)
        {
            send_data(image[i + j * Width]);
        }
    }
    turn_display_on();
}

void WaveShare213::display_partial(uint8_t *image)
{
    uint16_t Width, Height;
    Width = get_screen_width_bytes();
    Height = HEIGHT;

    init(PARTIAL);

    send_command(0x24); // Write Black and White image to RAM
    send_data2(image, Width * Height);
    turn_display_on_partial();
}

void WaveShare213::display_partial_wait(uint8_t *image)
{
    uint16_t Width, Height;
    Width = get_screen_width_bytes();
    Height = HEIGHT;

    read_busy();

    init(PARTIAL);
    send_command(0x24); // Write Black and White image to RAM
    send_data2(image, Width * Height);
    turn_display_on_partial_wait();
}

void WaveShare213::sleep()
{
    send_command(0x10); // enter deep sleep
    send_data(0x01);
    vTaskDelay(pdMS_TO_TICKS(100));
}

WaveShare213::~WaveShare213() {};