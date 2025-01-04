#ifndef _Config_H_
#define _Config_H_

typedef enum {
    GT1151,
    ICNT86X
} supported_touch_sensors;

// Pin definitions
#define I2C_SDA GPIO_NUM_21
#define I2C_SCL GPIO_NUM_22
#define TOUCH_RST GPIO_NUM_16
#define TOUCH_INT GPIO_NUM_17
#define I2C_FREQ 400000
#define I2C_PORT I2C_NUM_0

#define USE_TOUCH_DRIVER GT1151

#endif