/*
 * lsm6ds3.c
 *
 *  Created on: Mar 4, 2026
 *      Author: Anton
 */
#include "I2C_crutch/i2c-crutch.h"
#include "stm32f1xx.h"
#include "lsm6ds3.h"

int32_t lsm_write_reg (void *handle,  uint8_t SUB, const uint8_t *data, uint16_t len)
{

	lsm_data_t *ptr = (lsm_data_t *)handle;
	HAL_StatusTypeDef hal_res;
	uint8_t LSM_MACCIV [2] = {SUB, *data};
	for( int i = 0; i < len; i++ )
	{
		LSM_MACCIV [0] = SUB + i;
		LSM_MACCIV [1] = data[i];
		hal_res = HAL_I2C_Master_Transmit(ptr->hi2c, ptr->ADDR, LSM_MACCIV, 2, 100);
		if (hal_res != HAL_OK)
		{
			if (hal_res == HAL_BUSY)
				I2C_ClearBusyFlagErratum(ptr->hi2c, 100);
			return hal_res;
		}

	}
	return HAL_OK;
}

int32_t lsm_read_reg (void *handle, uint8_t SUB, uint8_t *data, uint16_t len)
{
	lsm_data_t *ptr = (lsm_data_t *)handle;
	HAL_StatusTypeDef hal_res;
	hal_res = HAL_I2C_Master_Transmit(ptr->hi2c, ptr->ADDR, &SUB, 1, 100);
	if (hal_res != HAL_OK)
	{
		if (hal_res == HAL_BUSY)
			I2C_ClearBusyFlagErratum(ptr->hi2c, 100);
		return hal_res;
	}
	hal_res = HAL_I2C_Master_Receive(ptr->hi2c,ptr->ADDR, data, len, 150);
	if (hal_res != HAL_OK)
	{
		if (hal_res == HAL_BUSY)
			I2C_ClearBusyFlagErratum(ptr->hi2c, 100);
		return hal_res;
	}
	return HAL_OK;
}
