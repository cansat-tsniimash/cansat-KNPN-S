/*
 * bmp280.h
 *
 *  Created on: 25 февр. 2026 г.
 *      Author: Anton
 */


#ifndef BMP280_BMP280_H_
#define BMP280_BMP280_H_

#include "bme280.h"

typedef struct bme280_bus_s
{
	uint8_t ADDR;
	I2C_HandleTypeDef *hi2c1;

}bmp280_bus_t;


void bmp280_delay_us(uint32_t period, void *intf_ptr);
BME280_INTF_RET_TYPE bmp280_write_reg (uint8_t reg_addr, const uint8_t *reg_data, uint32_t len,void *intf_ptr);
BME280_INTF_RET_TYPE bmp280_read_reg (uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr);


#endif /* BMP280_BMP280_H_ */
