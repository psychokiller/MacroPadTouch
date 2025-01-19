#ifndef _Utils_H_
#define _Utils_H_

#include <stdio.h>
#include "driver/i2c_master.h"
#include "esp_system.h"
#include "esp_check.h"
#include "driver/spi_master.h"
#include <string.h>
#include "driver/gpio.h"
#include "Config.h"

void convert_registers_to_byte_array(uint16_t slave_register, uint8_t *output);

uint8_t I2C_WRITE(uint16_t , uint8_t* , uint8_t, i2c_master_dev_handle_t*);
uint8_t I2C_READ(uint16_t , uint8_t* , uint8_t , i2c_master_dev_handle_t*);
void SPI_WRITE(uint8_t, spi_device_handle_t);
void SPI_WRITE_N(uint8_t*, uint32_t, spi_device_handle_t);

#endif