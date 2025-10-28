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

TouchPoint Icnt86x::transform_coordinates(const TouchPoint& tp, MIRROR_IMAGE mirror, const Display& display) {
    TouchPoint transformed = tp;
    
    switch (mirror) {
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
            transformed.x = tp.y;
            transformed.y = display.get_width() - tp.x;
            break;
    }
    
    return transformed;
}

TouchPoint Icnt86x::scan(i2c_master_dev_handle_t *i2c_dev_handle)
{
    uint8_t buff[100] = {0};

    // 1. Read STATUS BYTE (0x1001)
    I2C_READ(baseCoordinates_address, buff, 1, i2c_dev_handle);

    if (buff[0] == 0x00) 
    {
        // NO TOUCH path: Use the read-to-clear function and delay.
        I2C_CLEAR_REGISTER(baseCoordinates_address, i2c_dev_handle);
        vTaskDelay(pdMS_TO_TICKS(1)); 

        if (touch != NULL) return *touch;
        return TouchPoint();
    }
    else // TOUCH DETECTED (buff[0] > 0)
    {
        ESP_LOGI(TAG, "TOUCH WAS DETECTED");
        if (touch == NULL)
            touch = new TouchPoint();

        uint8_t touchCount = buff[0]; 

        if (touchCount > 5 || touchCount < 1)
        {
            ESP_LOGI(TAG, "Touch Count was wrong (%d). Clearing register.", touchCount);
            I2C_CLEAR_REGISTER(baseCoordinates_address, i2c_dev_handle);
            return *touch;
        }

        // 2. Read All Coordinate Data (0x1002) - 7 bytes per touch point
        uint8_t bytes_to_read = touchCount * 7; 
        I2C_READ(firstCoordinates_address, buff, bytes_to_read, i2c_dev_handle); 

        // 3. Clear the status register (Read-to-clear mechanism)
        I2C_CLEAR_REGISTER(baseCoordinates_address, i2c_dev_handle);

        // 4. Parse the data (7 bytes per point)
        for (int i = 0; i < touchCount; i++)
        {
            // Extract raw 16-bit values from the buffer
            // Data is Little-Endian (LSB, MSB)
            uint16_t raw_x_val = ((uint16_t)buff[2 + 7 * i] << 8) + buff[1 + 7 * i]; 
            uint16_t raw_y_val = ((uint16_t)buff[4 + 7 * i] << 8) + buff[3 + 7 * i];
            
            // --- Coordinate Validity Check ---
            // If the raw data is outside a plausible range (e.g., 4096x4096), skip it.
            // This filters out the 65411/0xFF83 garbage values.
            if (raw_x_val > 4096 || raw_y_val > 4096) {
                 ESP_LOGE(TAG, "Garbage coordinates filtered: x=%u, y=%u", raw_x_val, raw_y_val);
                 continue; 
            }

            // --- CRITICAL FIX: Swap X and Y ---
            // The chip's raw X is mapped to the display's Y (vertical axis) and vice versa.
            touch->x = raw_y_val; // Chip Y becomes screen X
            touch->y = raw_x_val; // Chip X becomes screen Y

            // Pressure/Size (8-bit)
            touch->size = buff[5 + 7 * i]; 

            // Track/Event ID (8-bit)
            touch->track_id = buff[6 + 7 * i]; 

            ESP_LOGI(TAG, "TOUCH DETECTED trackId: %d, x: %d, y: %d, s: %d", touch->track_id, touch->x, touch->y, touch->size);
        }
        
        return *touch;
    }
}

// TouchPoint Icnt86x::scan(i2c_master_dev_handle_t *i2c_dev_handle)
// {
//     // Check the actual interrupt pin state first (e.g., TOUCH_INT is pulled low on touch)
//     if (gpio_get_level(TOUCH_INT) != 0) // Assuming active-low interrupt
//     {
//         // No interrupt signal (pin is high or inactive). Clear the register just in case.
//         I2C_CLEAR_REGISTER(baseCoordinates_address, i2c_dev_handle);
//         vTaskDelay(pdMS_TO_TICKS(1)); 
        
//         // If there was no touch signal, return the last known good point or a default point.
//         if (touch != NULL) return *touch;
//         return TouchPoint();
//     }
    
//     // --- If the INT pin IS LOW, proceed with the I2C read/clear sequence ---
//     uint8_t buff[100] = {0};
    
//     // 1. Read STATUS BYTE (0x1001)
//     I2C_READ(baseCoordinates_address, buff, 1, i2c_dev_handle);

//     // If the register is 0x00, something is wrong, but the INT line is active. Clear and log.
//     if (buff[0] == 0x00) 
//     {
//         ESP_LOGE(TAG, "INT active but 0x1001 is 0x00. Clearing anyway.");
//         I2C_CLEAR_REGISTER(baseCoordinates_address, i2c_dev_handle);
//         vTaskDelay(pdMS_TO_TICKS(1));
//         if (touch != NULL) return *touch;
//         return TouchPoint();
//     }
    
//     // ... (rest of the touch-detected logic) ...
//     {
//         ESP_LOGI(TAG, "TOUCH WAS DETECTED");
//         if (touch == NULL) touch = new TouchPoint();

//         uint8_t touchCount = buff[0]; 

//         if (touchCount > 5 || touchCount < 1)
//         {
//             ESP_LOGI(TAG, "Touch Count was wrong (%d). Clearing register.", touchCount);
//             I2C_CLEAR_REGISTER(baseCoordinates_address, i2c_dev_handle);
//             return *touch;
//         }

//         // 2. Read All Coordinate Data (0x1002) - 7 bytes per touch point
//         uint8_t bytes_to_read = touchCount * 7; 
//         I2C_READ(firstCoordinates_address, buff, bytes_to_read, i2c_dev_handle); 

//         // 3. Clear the status register
//         I2C_CLEAR_REGISTER(baseCoordinates_address, i2c_dev_handle);

//         // 4. Parse the data (7 bytes per point)
//         for (int i = 0; i < touchCount; i++)
//         {
//             touch->x = ((uint16_t)buff[2 + 7 * i] << 8) + buff[1 + 7 * i]; 
//             touch->y = ((uint16_t)buff[4 + 7 * i] << 8) + buff[3 + 7 * i];
//             touch->size = buff[5 + 7 * i]; 
//             touch->track_id = buff[6 + 7 * i]; 

//             ESP_LOGI(TAG, "TOUCH DETECTED trackId: %d, x: %d, y: %d, s: %d", touch->track_id, touch->x, touch->y, touch->size);
//         }
        
//         return *touch;
//     }
// }

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