#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "argtable3/argtable3.h"

static const char *TAG = "Touch_Driver";

// Pin definitions
#define I2C_SDA GPIO_NUM_21
#define I2C_SCL GPIO_NUM_22
#define TOUCH_RST GPIO_NUM_16
#define TOUCH_INT GPIO_NUM_17
#define I2C_FREQ 400000
#define I2C_PORT I2C_NUM_0

#define GT1151N_ADDR 0x14
#define PRODUCT_ID_REG 0x8140
#define COORD_ADDR 0x814E
#define MAX_TOUCH_POINTS 10
#define CONFIG_REG 0x8047
#define COMMAND 0x8040


// Basic structure to hold a point data
// based on the GT1151N Data sheet
// But likely similar to the ICNT86X
struct TouchPoint
{                                // 8 bytes
    uint8_t status_and_track_id; // 1 byte
    uint16_t x_coordinate;       // 2 bytes LOW byte and HIGH byte
    uint16_t y_coordinate;       // 2 bytes LOW byte and HIGH byte
    uint8_t point_width;         // the width of the point detected
    uint8_t point_height;        // the height of the point detected
    uint8_t reserved_unused;     // RESERVED byte after every touch point without anything useful
};

// get value of a bit in a byte
uint8_t get_bit(uint8_t byte, uint8_t bit_position)
{
    if (bit_position > 7)
        return 0; // Error check for valid position
    return (byte >> bit_position) & 1;
}

// I2C registers are 16 bits values (e.g. 0x8047) which means
// that we can't hold its value in uint8_t anymore
// this method will return an array of 2 uint8_t bytes
// which can be used instead
void convert_registers_to_byte_array(uint16_t slave_register, uint8_t *output)
{
    if (output != NULL)
    {
        output[0] = (uint8_t)(slave_register >> 8);   // MSB
        output[1] = (uint8_t)(slave_register & 0xFF); // LSB
    }
}

// This is a reset procedure based 
// on both Touch sensors Datasheets
void reset_touch_device()
{
    gpio_set_level(TOUCH_RST, 1); // HIGH
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(TOUCH_RST, 0); // LOW
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(TOUCH_RST, 1); // HIGH
    vTaskDelay(pdMS_TO_TICKS(100));
}

esp_err_t touch_read_product(i2c_master_dev_handle_t handle)
{
    uint8_t buff[11];
    uint8_t prod_reg[2];

    convert_registers_to_byte_array(PRODUCT_ID_REG, prod_reg);
    uint8_t checksum = 0;

    if (handle != NULL)
    {
        i2c_master_transmit_receive(handle, prod_reg, sizeof(prod_reg), buff, sizeof(buff), -1);

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

void touch_init(i2c_master_dev_handle_t handle)
{
    reset_touch_device();
    touch_read_product(handle);
    vTaskDelay(pdMS_TO_TICKS(100));
}

void app_main(void)
{
    // define I2C bus configuration
    i2c_master_bus_config_t master_config = {
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .flags.enable_internal_pullup = true};

    i2c_master_bus_handle_t bus_handle;
    i2c_new_master_bus(&master_config, &bus_handle);

    // define the configuration for the I2C device
    // in this case it is the Touch Sensor GT1151N/ICNT86X
    i2c_device_config_t slave_dev_cnfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = GT1151N_ADDR,
        .scl_speed_hz = I2C_FREQ,
        .flags.disable_ack_check = false};

    i2c_master_dev_handle_t dev_handle;
    i2c_master_bus_add_device(bus_handle, &slave_dev_cnfg, &dev_handle);

    // configure the TOUCH_INT pin
    gpio_config_t touch_interrupt_pin_config = {
        .pull_up_en = true,
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_NEGEDGE,
        .pin_bit_mask = TOUCH_INT};

    gpio_config(&touch_interrupt_pin_config);

    // configure the TOUCH_RST pin
    gpio_config_t reset_pin_config = {
        .pull_up_en = true,
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = TOUCH_RST};

    gpio_config(&reset_pin_config);

    /**
     * The I2C in ESP32 works by sending starting the Send
     * by sending the Register Address followed by the Data
     * in the same array 
     * (e.g. regs[0], regs[1] are the Register address, regs[2] is the data command to be sent)
     * The device address is prepended by the dev_handler passed into the `i2c_master_` methods
     */
    uint8_t regs[3];
    convert_registers_to_byte_array(COMMAND, regs);
    regs[2] = 0x00; // send 0x00 to the Touch controller to enable reading the touch points

    i2c_master_transmit(dev_handle, regs, sizeof(regs), -1); // send command
    
    convert_registers_to_byte_array(0x8041, regs);
    i2c_master_transmit(dev_handle, regs, sizeof(regs), -1); // send 0x00 to the data register
    vTaskDelay(pdMS_TO_TICKS(100));

    convert_registers_to_byte_array(COORD_ADDR, regs); // send command to the coordinates registers
    regs[2] = 0x00; // send the 0x00 to the register to enable reading the coordinates of the touch points
    uint8_t buff[100] = {0x00};

    while (true)
    {
        // we transmit the 0x00 to the read coord registers, and recieve in the buff
        i2c_master_transmit_receive(dev_handle, regs, sizeof(regs), buff, sizeof(buff), -1);

        uint8_t number_of_touch_points = buff[0] & 0x0F; // the 4-bit of the first response byte contains the number of touch points

        if (number_of_touch_points == 0) // as per all documentations, if there are nothing being read, we should wait 1ms
        {
            vTaskDelay(pdMS_TO_TICKS(1)); // this should be doing 1ms delay
            continue; // skip the rest as there are not read touch points
        }

        for (int i = 0; i < number_of_touch_points; i++) // loop over the touch points
        {
            uint16_t x = (buff[2 + 8 * i] << 8) + buff[3 + 8 * i]; // combine the lowest byte and upper byte to have an accumulative value for X
            uint16_t y = (buff[4 + 8 * i] << 8) + buff[5 + 8 * i]; // combine the lowest byte and upper byte to have an accumulative value for y
            ESP_LOGI(TAG, "touch Data: X %d, Y %d, ", x, y); // log the X & Y positions
        }

        i2c_master_transmit(dev_handle, regs, sizeof(regs), -1); // send 0x00 once we've read the data as per the datasheet
    }
}
