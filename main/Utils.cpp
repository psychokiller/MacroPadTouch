#include "Utils.h"

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

uint8_t I2C_WRITE(uint16_t register_address, uint8_t *write_buffer, uint8_t len, i2c_master_dev_handle_t *i2c_dev_handle)
{
    uint8_t wbuf[50] = {(uint8_t)((register_address >> 8) & 0xff), (uint8_t)(register_address & 0xff)};
    for (uint8_t i = 0; i < len; i++)
    {
        wbuf[i + 2] = write_buffer[i];
    }
    return i2c_master_transmit(*i2c_dev_handle, wbuf, len + 2, -1); // send command
}

uint8_t I2C_READ(uint16_t register_address, uint8_t *read_buffer, uint8_t len, i2c_master_dev_handle_t *i2c_dev_handle)
{
    uint8_t *rbuf = read_buffer;
    I2C_WRITE(register_address, 0, 0, i2c_dev_handle);

    esp_err_t result = i2c_master_receive(*i2c_dev_handle, rbuf, len, -1);
    return result;
}

void SPI_WRITE(uint8_t data, spi_device_handle_t handle)
{
    
    gpio_set_level(SPI_CS, 0);
    
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    
    t.flags = SPI_TRANS_USE_TXDATA;
    t.tx_buffer = NULL;
    t.tx_data[0] = data;
    t.length = 8;

    spi_device_polling_transmit(handle, &t);

    gpio_set_level(SPI_CS, 1);
}

void SPI_WRITE_N(uint8_t *data, uint32_t len, spi_device_handle_t handle)
{
    for (uint32_t i = 0; i < len; i++)
    {
        SPI_WRITE(data[i], handle);
    }
}