#ifndef _I2C_TEST_H
#define _I2C_TEST_H

#include <stdbool.h>
#include <stdint.h>

extern void i2c_handleTask(void);
extern bool i2c_write(uint8_t deviceAddress, uint16_t address, uint8_t *buff, uint16_t length);
extern bool i2c_read(uint8_t deviceAddress, uint16_t address, uint8_t *buff, uint16_t length);

#endif
