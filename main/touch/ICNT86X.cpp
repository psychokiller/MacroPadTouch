#include "Config.h"
#include "ICNT86X.h"

Icnt86x::Icnt86x() : TouchDriver() {};
Icnt86x::~Icnt86x() {};

// This is a reset procedure based
// on both Touch sensors Datasheets
void Icnt86x::reset()
{
    gpio_set_level(TOUCH_RST, 1); // HIGH
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(TOUCH_RST, 0); // LOW
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(TOUCH_RST, 1); // HIGH
    vTaskDelay(pdMS_TO_TICKS(100));
}

esp_err_t Icnt86x::read_product_id(i2c_master_dev_handle_t *handle)
{
    uint8_t buff[4] = {0};
    uint8_t prod_reg[2];

    convert_registers_to_byte_array(productId_reg, prod_reg);

    if (handle != NULL)
    {
        esp_err_t result = i2c_master_transmit_receive(*handle, prod_reg, sizeof(prod_reg), buff, sizeof(buff), -1);

        ESP_LOGI(TAG, "IC version: %x %x, FW Version: %x %x", buff[0], buff[1], buff[2], buff[3]);

        return ESP_OK;
    }
    else
        return ESP_ERR_INVALID_STATE;
}

void Icnt86x::init(i2c_master_dev_handle_t *handle)
{
    reset();
    read_product_id(handle);
    vTaskDelay(pdMS_TO_TICKS(100));
}

TouchPoint Icnt86x::transform_coordinates(const TouchPoint &tp, MIRROR_IMAGE mirror, const Display &display)
{
    TouchPoint transformed = tp;

    switch (mirror)
    {
    case MIRROR_NONE:
        // No transformation needed
        break;
    case MIRROR_HORIZONTAL:
        transformed.x = display.get_width() - tp.x;
        break;
    case MIRROR_VERTICAL:
        transformed.y = display.get_height() - tp.y;
        break;
    case MIRROR_ORIGIN:
        // The most common case based on the current codebase
        transformed.x = display.get_height() - tp.x;
        transformed.y = display.get_width() - tp.y;
        break;
    }

    return transformed;
}

TouchPoint Icnt86x::scan(i2c_master_dev_handle_t *i2c_dev_handle)
{
    uint8_t buff[100] = {0};
    uint8_t mask[1] = {0x00};

    // 1. Read STATUS BYTE (0x1001)
    I2C_READ(baseCoordinates_address, buff, 1, i2c_dev_handle);

    if (buff[0] == 0x00)
    {
        I2C_WRITE(baseCoordinates_address, mask, 1, i2c_dev_handle);
        vTaskDelay(pdMS_TO_TICKS(1));

        return TouchPoint();
    }
    else // TOUCH DETECTED (buff[0] > 0)
    {
        if (touch == NULL)
            touch = new TouchPoint();

        uint8_t touchCount = buff[0];

        if (touchCount > 5 || touchCount < 1)
        {
            ESP_LOGI(TAG, "Touch Count was wrong (%d). Clearing register.", touchCount);
            I2C_WRITE(baseCoordinates_address, mask, 1, i2c_dev_handle);
            vTaskDelay(pdMS_TO_TICKS(1));

            return TouchPoint();
        }

        // 2. Read All Coordinate Data (0x1002) - 7 bytes per touch point
        uint8_t bytes_to_read = touchCount * 7;
        I2C_READ(firstCoordinates_address, buff, bytes_to_read, i2c_dev_handle);

        I2C_WRITE(baseCoordinates_address, mask, 1, i2c_dev_handle);

        // 4. Parse the data (7 bytes per point)
        for (int i = 0; i < touchCount; i++)
        {
            // Extract raw 16-bit values from the buffer
            // Data is Little-Endian (LSB, MSB)
            uint16_t raw_x_val = ((uint16_t)buff[2 + 7 * i] << 8) + buff[1 + 7 * i];
            uint16_t raw_y_val = ((uint16_t)buff[4 + 7 * i] << 8) + buff[3 + 7 * i];
            touch->x = raw_x_val;
            touch->y = raw_y_val;
            // Pressure/Size (8-bit)
            touch->size = buff[5 + 7 * i];
            // Track/Event ID (8-bit)
            touch->track_id = buff[6 + 7 * i];
        }
        return *touch;
    }
    return *touch;
}
