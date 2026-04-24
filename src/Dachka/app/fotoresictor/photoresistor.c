/*
 * photoresistor.c
 *
 *  Created on: Apr 11, 2026
 *      Author: Anton
 */


#include "fotoresictor/phororesistor.h"
#include "math.h"

#define RESISTOR (10000.0)

int megalux(ADC_HandleTypeDef* hadc, float *minilux) {
    HAL_StatusTypeDef rc = HAL_ADC_Start(hadc);
    if(rc != HAL_OK) return rc;

    rc = HAL_ADC_PollForConversion(hadc, 100);
    if(rc != HAL_OK) return rc;

    uint32_t adc_val = HAL_ADC_GetValue(hadc);
    float voltage = adc_val / 4095.0f * 3.3f;

    *minilux = voltage;

    rc = HAL_ADC_Stop(hadc);
    return rc;
}
