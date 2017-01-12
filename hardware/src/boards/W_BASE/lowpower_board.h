/*
Description: WWDG general functions implementation

Author: Boulet Benjamin
*/

#ifndef __LOWPOWSER_BOARD_H__
#define __LOWPOWSER_BOARD_H__

#include <stdio.h>
#include <stdint.h>
#include "stdbool.h"
#include "stm32l1xx.h"
#include "utilities.h"

#define UART_ON 	true
#define	UART_OFF 	false

void SystemLowPower_Config( bool UartOn );
void SystemDeepLowPower_Config(void);
void SystemWakeUp_Config( bool UartOn );
void EnterSleepMode( bool fullSleep );

#endif // __LOWPOWSER_BOARD_H__
