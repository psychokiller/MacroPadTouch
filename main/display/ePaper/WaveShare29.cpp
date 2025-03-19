#include "WaveShare29.h"

WaveShare29::WaveShare29()
{
    gpio_set_direction(DISPLAY_BUSY, GPIO_MODE_INPUT);
    gpio_set_direction(DISPLAY_RST, GPIO_MODE_OUTPUT);
    gpio_set_direction(SPI_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(SPI_CLK, GPIO_MODE_OUTPUT);
    gpio_set_direction(SPI_CS, GPIO_MODE_OUTPUT);
    gpio_set_direction(SPI_MOSI, GPIO_MODE_OUTPUT);

    spi_device_interface_config_t spi_config = {
        .clock_speed_hz = 20000,
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

void WaveShare29::reset()
{
    gpio_set_level(DISPLAY_RST, 1); // HIGH
    vTaskDelay(pdMS_TO_TICKS(20));

    gpio_set_level(DISPLAY_RST, 0); // LOW
    vTaskDelay(pdMS_TO_TICKS(10));

    gpio_set_level(DISPLAY_RST, 1); // HIGH
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_LOGI("EPD_29", "RESET");
}

void WaveShare29::send_command(uint8_t command)
{
    gpio_set_level(SPI_DC, 0);
    gpio_set_level(SPI_CS, 0);
    SPI_WRITE(command, spi_handle);
    gpio_set_level(SPI_CS, 1);
}

void WaveShare29::send_data(uint8_t data)
{
    gpio_set_level(SPI_DC, 1);
    gpio_set_level(SPI_CS, 0);
    SPI_WRITE(data, spi_handle);
    gpio_set_level(SPI_CS, 1);
}

void WaveShare29::send_data2(uint8_t *data, uint32_t len)
{
    gpio_set_level(SPI_DC, 1);
    gpio_set_level(SPI_CS, 0);
    SPI_WRITE_N(data, len, spi_handle);
    gpio_set_level(SPI_CS, 1);
}

void WaveShare29::read_busy()
{
    while (true)
    {
        if (gpio_get_level(DISPLAY_BUSY) == 0)
            break;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    ESP_LOGI("EPD_29", "READ BUSY");
}

void WaveShare29::set_window(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end)
{
    send_command(0x44); // SET_RAM_X_ADDRESS_START_END_POSITION
    // send_data(0x00);
    // send_data(0x15);
    send_data((x_start >> 3) & 0xFF);
    send_data((x_end >> 3) & 0xFF);

    send_command(0x45); // SET_RAM_Y_ADDRESS_START_END_POSITION
    // send_data(0x00);
    // send_data(0x00);
    
    // send_data(0x00);
    // send_data(0x128);

    send_data(y_start & 0xFF);
    send_data((y_start >> 8) & 0xFF);
    send_data(y_end & 0xFF);
    send_data((y_end >> 8) & 0xFF);
}

void WaveShare29::set_cursor(uint16_t x, uint16_t y)
{
    send_command(0x4E); // SET_RAM_X_ADDRESS_COUNTER
    send_data(x & 0xFF);
    // send_data(0x00);

    send_command(0x4F); // SET_RAM_Y_ADDRESS_COUNTER
    // send_data(0x00);
    // send_data(0x00);

    send_data(y & 0xFF);
    send_data((y >> 8) & 0xFF);
}

void WaveShare29::turn_display_on()
{
    send_command(0x22); // Display Update Control
    send_data(0xF7); // was 0xc7 -> 0xF7
    send_command(0x20); // Activate Display Update Sequence
    read_busy();
    ESP_LOGI("EPD_29", "TURN DISPLAY ON");
}

void WaveShare29::turn_display_on_partial()
{
    send_command(0x22); // Display Update Control
    send_data(0x0f);    // fast:0x0c, quality:0x0f, 0xcf
    send_command(0x20); // Activate Display Update Sequence
    read_busy();
}

void WaveShare29::init() {
    init(FULL);
}

void WaveShare29::init(display_refresh_mode mode)
{
    if (mode == FULL)
    {
        reset();
        vTaskDelay(pdMS_TO_TICKS(100));
        read_busy();

        send_command(0x12); // SWRESET
        read_busy();

        send_command(0x01); // Driver output control
        send_data(0x27);
        send_data(0x01);
        send_data(0x00);

        send_command(0x11); // data entry mode
        send_data(0x03);

        set_window(0, 0, width-1, height-1);

        // send_command(0x22);
        // send_data(0xb1);
        // send_command(0x20);
        // read_busy();

        send_command(0x21); //  Display update control
        send_data(0x00);
        send_data(0x80);

        set_cursor(0, 0);

        read_busy();
        // LutByHost(WS_20_30);
        ESP_LOGI("EPD_29", "FULL INIT");
    }
    else if (mode == PARTIAL)
    {
        // Reset
        gpio_set_level(DISPLAY_RST, 0);
        vTaskDelay(pdMS_TO_TICKS(1));
        gpio_set_level(DISPLAY_RST, 1);
        vTaskDelay(pdMS_TO_TICKS(2));

        LUT(_WF_PARTIAL_2IN9);

        send_command(0x37); 
        send_data(0x00);  
        send_data(0x00);  
        send_data(0x00);  
        send_data(0x00); 
        send_data(0x00);  
        send_data(0x40);  
        send_data(0x00);  
        send_data(0x00);   
        send_data(0x00);  
        send_data(0x00);

        send_command(0x3C); // BorderWavefrom
        send_data(0x80);

        send_command(0x22); 
	    send_data(0xC0);   
	    send_command(0x20); 
	    read_busy();  

        set_window(0, 0, width, height);
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

        set_window(0, 0, width, height);
        set_cursor(0, 0);

        // send_command(0x22); // Load temperature value
        // send_data(0xB1);
        // send_command(0x20);
        // read_busy();

        // send_command(0x1A); // Write to temperature register
        // send_data(0x64);
        // send_data(0x00);

        // send_command(0x22); // Load temperature value
        // send_data(0x91);
        // send_command(0x20);
        read_busy();
    }

    // ESP_LOGI("EPD_29", "INIT");
}

uint16_t WaveShare29::get_screen_width_bytes()
{
    return (width % 8 == 0) ? (width / 8) : (width / 8 + 1);
}

uint16_t WaveShare29::get_screen_size_bytes()
{
    return get_screen_width_bytes() * height;
}

void WaveShare29::clear(display_color color)
{
    send_command(0x24);
    for (uint16_t j = 0; j < total_display_size_bytes ; j++)
    {
        send_data(color);  
    }

    turn_display_on();
    ESP_LOGI("EPD_29", "CLEAR with 0x%X", color);
}

void WaveShare29::display(uint8_t *image)
{
    send_command(0x24);
    for (uint16_t i = 0 ; i < total_display_size_bytes; i ++){
        send_data(image[i]);
    }

    turn_display_on();
}

void WaveShare29::display_base(uint8_t *image)
{
    send_command(0x24);
    for (uint16_t i = 0 ; i < total_display_size_bytes; i ++){
            send_data(image[i]);
    }
    turn_display_on();
}

void WaveShare29::display_partial(uint8_t *image)
{
    init(PARTIAL);

    send_command(0x24); // Write Black and White image to RAM
    send_data2(image, total_display_size_bytes);
    turn_display_on_partial();
}

void WaveShare29::sleep()
{
    send_command(0x10); // enter deep sleep
    send_data(0x01);
    vTaskDelay(pdMS_TO_TICKS(100));
}

void WaveShare29::LUT(uint8_t* lut) {
    uint8_t count;
	send_command(0x32);
	for(count=0; count<153; count++) 
		send_data(lut[count]); 
	read_busy();
}

void WaveShare29::LutByHost(uint8_t* lut) {
    LUT((uint8_t *)lut);			//lut
	send_command(0x3f);
	send_data(*(lut+153));
	send_command(0x03);	// gate voltage
	send_data(*(lut+154));
	send_command(0x04);	// source voltage
	send_data(*(lut+155));	// VSH
	send_data(*(lut+156));	// VSH2
	send_data(*(lut+157));	// VSL
	send_command(0x2c);		// VCOM
	send_data(*(lut+158));
}

WaveShare29::~WaveShare29() {};