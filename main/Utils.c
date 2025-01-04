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

uint8_t I2C_WRITE(uint16_t register_address, uint8_t* write_buffer, uint8_t len, i2c_master_dev_handle_t* dev_handle) {
    uint8_t wbuf[50]={(register_address>>8)&0xff, register_address&0xff};
	for(uint8_t i=0; i<len; i++) {
		wbuf[i+2] = write_buffer[i];
	}
    return i2c_master_transmit(*dev_handle, wbuf, len+2 , -1); // send command
}

uint8_t I2C_READ(uint16_t register_address, uint8_t* read_buffer, uint8_t len, i2c_master_dev_handle_t* dev_handle) {
    uint8_t *rbuf = read_buffer;
    I2C_WRITE(register_address, 0,0, dev_handle);

    esp_err_t result = i2c_master_receive(*dev_handle, rbuf, len, -1);
    // ESP_LOGI(TAG, "RESULT: %d", result);
    return result;
}

