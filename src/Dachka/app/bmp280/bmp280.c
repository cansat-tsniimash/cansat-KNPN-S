/*
 * bmp280.c
 *
 *  Created on: 25 февр. 2026 г.
 *      Author: Anton
 */
#include "stm32f1xx.h"
#include "bmp280.h"
#include "delay/dwt_delay.h"

extern I2C_HandleTypeDef hi2c1;

#define BMP280_ADDR (0x76 << 1)



BME280_INTF_RET_TYPE bmp280_read_reg (uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
	bmp280_bus_t *ptr = (bmp280_bus_t *)intf_ptr;
	//(*ptr).ADDR
	//ptr->ADDR

	HAL_StatusTypeDef hal_res;
	hal_res = HAL_I2C_Master_Transmit(ptr->hi2c1, ptr->ADDR, &reg_addr, 1, 100);
	if (hal_res != HAL_OK)
		return hal_res;

	hal_res = HAL_I2C_Master_Receive(ptr->hi2c1, ptr->ADDR, reg_data, len, 150);
	if (hal_res != HAL_OK)
		return hal_res;

	return HAL_OK;
}

BME280_INTF_RET_TYPE bmp280_write_reg (uint8_t reg_addr, const uint8_t *reg_data, uint32_t len,void *intf_ptr)
{
	bmp280_bus_t *ptr = (bmp280_bus_t *)intf_ptr;
	uint8_t BMP_MASSIV [2] = {reg_addr, reg_data[0]};

	for( int i = 0; i < len; i++ )
	{
		BMP_MASSIV [0] = reg_addr + i;
		BMP_MASSIV [1] = reg_data[i];
		HAL_StatusTypeDef hal_res = HAL_I2C_Master_Transmit(ptr->hi2c1, ptr->ADDR, BMP_MASSIV, 2, 100);
		if (hal_res != HAL_OK)
			return hal_res;
		return HAL_OK;
	}
}

void bmp280_delay_us(uint32_t period, void *intf_ptr)
{
	dwt_delay_us(period);
}

















