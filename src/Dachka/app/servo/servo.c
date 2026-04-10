/*
 * servo.c
 *
 *  Created on: 10 апр. 2026 г.
 *      Author: Anton
 */

#include "stm32f1xx_hal.h"
#include "math.h"

extern TIM_HandleTypeDef htim3;

#define TICK_MIN 25
#define TICK_MIN 125

#define K 39
#define B 2775

void Servo_Init(void)
{
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
	Servo_Angle(0);
}


void ServoTialTick(uint16_t tickes)
{



}





int32_t ServoTickAngle(uint16_t servo_tick)
{

}
