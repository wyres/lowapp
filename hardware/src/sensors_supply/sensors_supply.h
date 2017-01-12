/*
Description: MODE AT general functions implementation

Author: Boulet Benjamin
*/

#ifndef __SENSORS_SUPPLY_H__
#define __SENSORS_SUPPLY_H__

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "stm32l1xx.h"
#include "utilities.h"
#include "board.h"

#define UART_SUPPLY_PIN 	PA_1 //PB_14
#define I2C_SUPPLY_PIN 		PA_12

/*!
 * \brief Prototype functions.
 */
void UartSensorOn( void );
void UartSensorOff( void );
void I2cSensorOn( void );
void I2cSensorOff( void );

#endif // __SENSORS_SUPPLY_H__
