/*
 * phororesistor.h
 *
 *  Created on: Apr 11, 2026
 *      Author: Anton
 */

#ifndef FOTORESICTOR_PHORORESISTOR_H_
#define FOTORESICTOR_PHORORESISTOR_H_

#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include <stm32f1xx.h>

int megalux(ADC_HandleTypeDef* hadc, float *minilux);

#endif /* FOTORESICTOR_PHORORESISTOR_H_ */
