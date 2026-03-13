/*
 * lis3mdl.h
 *
 *  Created on: Mar 7, 2026
 *      Author: Anton
 */

#ifndef LIS3MDL_LIS3MDL_H_
#define LIS3MDL_LIS3MDL_H_

#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stm32f1xx.h>
#include "delay/dwt_delay.h"

#include "lis3mdl_reg.h"

typedef struct lis3mdl_data
{
	uint8_t addr;
	I2C_HandleTypeDef *hi2c1;

}lis3mdl_data_t;

int32_t lis3mdl_read(void* handle, uint8_t reg_addr, uint8_t* data_ptr, uint16_t len);
int32_t lis3mdl_write(void* handle, uint8_t reg_addr, const uint8_t *data_ptr, uint16_t len);

#endif /* LIS3MDL_LIS3MDL_H_ */
