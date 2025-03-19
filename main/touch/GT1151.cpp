#include "touch/GT1151.h"
#include "Config.h"

Gt1151::Gt1151(): TouchDriver() {};
Gt1151::~Gt1151() { };

// This is a reset procedure based
// on both Touch sensors Datasheets
void Gt1151::reset()
{
    gpio_set_level(TOUCH_RST, 1); // HIGH
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(TOUCH_RST, 0); // LOW
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(TOUCH_RST, 1); // HIGH
    vTaskDelay(pdMS_TO_TICKS(100));
}

esp_err_t Gt1151::read_product_id(i2c_master_dev_handle_t *handle)
{
    uint8_t buff[11];
    uint8_t prod_reg[2];

    convert_registers_to_byte_array(productId_reg, prod_reg);
    uint8_t checksum = 0;

    if (handle != NULL)
    {
        i2c_master_transmit_receive(*handle, prod_reg, sizeof(prod_reg), buff, sizeof(buff), -1);

        for (int i = 0; i < sizeof(buff); i++)
        {
            checksum += buff[i];
        }

        uint32_t mask_id, patch_id, sensor_id;
        char product_id[5] = {0};
        mask_id = (uint32_t)((buff[7] << 16) | (buff[8] << 8) | buff[9]);
        patch_id = (uint32_t)((buff[4] << 16) | (buff[5] << 8) | buff[6]);
        memcpy(product_id, buff, 4);
        sensor_id = buff[10] & 0x0F;
        ESP_LOGI(
            TAG, "IC version: GT%s_%06" PRIX32 "(Patch)_%04" PRIX32 "(Mask)_%02" PRIX32 "(SensorID)",
            product_id, patch_id, mask_id >> 8, sensor_id);

        return ESP_OK;
    }
    else
        return ESP_ERR_INVALID_STATE;
}

void Gt1151::init(i2c_master_dev_handle_t *handle)
{
    reset();
    read_product_id(handle);
    vTaskDelay(pdMS_TO_TICKS(100));
}

TouchPoint Gt1151::scan(i2c_master_dev_handle_t *i2c_dev_handle)
{
    uint8_t buff[100];
    uint8_t mask[1] = {0x00};
    

    I2C_READ(baseCoordinates_address, buff, 1, i2c_dev_handle);
    if ((buff[0] & 0x80) == 0x00)
    {
        I2C_WRITE(baseCoordinates_address, mask, 1, i2c_dev_handle);
        vTaskDelay(pdMS_TO_TICKS(1));
        // ESP_LOGI(TAG, " NO TOUCH WAS DETECTED \r\n");
        return touch;
    }
    else
    {
        ESP_LOGI(TAG, "TOUCH WAS DETECTED \r\n");
        uint8_t touchCount = buff[0] & 0x0f;
        if (touchCount > 5 || touchCount < 1)
        {
            ESP_LOGI(TAG, "Touch Count was wrong read again....");
            I2C_WRITE(baseCoordinates_address, mask, 1, i2c_dev_handle);
            return touch;
        }
        I2C_READ(firstCoordinates_address, &buff[1], touchCount * 8, i2c_dev_handle);
        I2C_WRITE(baseCoordinates_address, mask, 1, i2c_dev_handle);
        for (int i = 0; i < touchCount; i++)
        {
            touch.track_id = buff[1 + 8 * i];
            touch.x = ((uint16_t)buff[3 + 8 * i] << 8) + buff[2 + 8 * i];
            touch.y = ((uint16_t)buff[5 + 8 * i] << 8) + buff[4 + 8 * i];
            touch.size = ((uint16_t)buff[7 + 8 * i] << 8) + buff[6 + 8 * i];

            ESP_LOGI(TAG, "TOUCH WAS DETECTED trackId: %d, x: %d, y: %d, s: %d", touch.track_id, touch.x, touch.y, touch.size);
        }
        return touch;
    }
    return touch;
}

i2c_device_config_t Gt1151::get_device_config()
{
    // define the configuration for the I2C device
    // in this case it is the Touch Sensor GT1151N/ICNT86X
    i2c_device_config_t slave_dev_cnfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = chip_address,
        .scl_speed_hz = I2C_FREQ,
        .flags = {
            .disable_ack_check = false}};

    return slave_dev_cnfg;
}