#ifndef _Config_H_
#define _Config_H_

typedef enum {
    GT1151,
    ICNT86X
} supported_touch_sensors;

// Pin definitions
// I2C pins
#define I2C_SDA GPIO_NUM_21
#define I2C_SCL GPIO_NUM_22
#define I2C_FREQ 400000
#define I2C_PORT I2C_NUM_0

// Touch display pins
#define TOUCH_RST GPIO_NUM_16
#define TOUCH_INT GPIO_NUM_17

// Display pins
#define DISPLAY_RST  GPIO_NUM_26
#define DISPLAY_BUSY GPIO_NUM_25

// SPI pins
#define SPI_MOSI GPIO_NUM_14
#define SPI_CLK GPIO_NUM_13
#define SPI_CS  GPIO_NUM_15
#define SPI_DC  GPIO_NUM_27

// Touch driver definition
#define USE_TOUCH_DRIVER GT1151

// Display SPI Host
#define DISPLAY_HOST SPI3_HOST

#endif