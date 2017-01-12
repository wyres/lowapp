/**
 * @file lowapp_sys_uart.h
 * @brief Implementation of the UART related functions for LoWAPP
 *
 * @author Nathan Olff
 * @date October 20, 2016
 */
#ifndef LOWAPP_SYS_UART_H_
#define LOWAPP_SYS_UART_H_

#include "lowapp_sys.h"

void AtModeInit( uint32_t BaudRate );
int8_t cmd_response(uint8_t* data, uint16_t length);

#endif
