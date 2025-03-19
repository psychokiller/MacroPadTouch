#include "Config.h"
#include "ICNT86X.h"


Icnt86x::Icnt86x(): TouchDriver() {};
Icnt86x::~Icnt86x() { };

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
        esp_err_t result =  i2c_master_transmit_receive(*handle, prod_reg, sizeof(prod_reg), buff, sizeof(buff), -1);
        ESP_LOGE(TAG, "Result: %d", result);

        ESP_LOGI(TAG, "IC version: %x %x\r\n, FW Version: %x %x", buff[0], buff[1], buff[2], buff[3]);

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

TouchPoint Icnt86x::scan(i2c_master_dev_handle_t *i2c_dev_handle)
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
            touch.track_id = buff[6 + 7 * i];
            touch.x = ((uint16_t)buff[2 + 7 * i] << 8) + buff[1 + 7 * i];
            touch.y = ((uint16_t)buff[4 + 7 * i] << 8) + buff[3 + 7 * i];
            touch.size = buff[5 + 7 * i];

            ESP_LOGI(TAG, "TOUCH WAS DETECTED trackId: %d, x: %d, y: %d, s: %d", touch.track_id, touch.x, touch.y, touch.size);
        }
        return touch;
    }
    return touch;
}

i2c_device_config_t Icnt86x::get_device_config()
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