/*
 * print.c
 *
 *  Created on: Feb 20, 2026
 *      Author: Anton
 */
#include "stm32f1xx.h"
extern UART_HandleTypeDef huart2;



int _write(int file, char *ptr, int len)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, 100);
  return len;
}
