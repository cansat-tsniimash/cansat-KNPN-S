/*
 * lis3mdl.c
 *
 *  Created on: Mar 7, 2026
 *      Author: Anton
 */


#include "../lis3mdl/lis3mdl.h"
#include "lis3mdl_reg.h"

#include "../I2C_crutch/i2c-crutch.h"



int32_t lis3mdl_read(void* handle, uint8_t reg_addr, uint8_t* data_ptr, uint16_t len)
{
	lis3mdl_data_t *ptr = (lis3mdl_data_t *)handle;
	HAL_StatusTypeDef res = HAL_I2C_Master_Transmit(ptr->hi2c1, ptr->addr, &reg_addr, 1, 100);
    if (res != HAL_OK) {
        I2C_ClearBusyFlagErratum(ptr->hi2c1, 100);
        return 1;
    }
    res = HAL_I2C_Master_Receive(ptr->hi2c1, ptr->addr, data_ptr, len, 100);
    if (res != HAL_OK) {
    	I2C_ClearBusyFlagErratum(ptr->hi2c1, 100);
        return 1;
    }
    return 0;
}
int32_t lis3mdl_write(void* handle, uint8_t reg_addr, const uint8_t *data_ptr, uint16_t len)
{
	lis3mdl_data_t *ptr = (lis3mdl_data_t *)handle;
	uint8_t lis[2];
    for (uint16_t i = 0; i < len; i++) {
        lis[0] = reg_addr + i;
        lis[1] = data_ptr[i];
        HAL_StatusTypeDef res = HAL_I2C_Master_Transmit(ptr->hi2c1, ptr->addr, lis, 2, 100);
        if (res != HAL_OK) {
            I2C_ClearBusyFlagErratum(ptr->hi2c1, 100);
            return 1;
        }
    }
    return 0;
}
