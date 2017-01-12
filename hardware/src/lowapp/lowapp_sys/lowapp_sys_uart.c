/**
 * @file lowapp_sys_uart.c
 * @brief Implementation of the UART related functions for LoWAPP
 *
 * @author Nathan Olff
 * @date October 20, 2016
 */
#include "board.h"
#include "uart.h"
#include "uart-board.h"

/**
 * @addtogroup lowapp_hardware_sys
 * @{
 */
/**
 * @addtogroup lowapp_hardware_sys_uart LoWAPP Hardware System UART Interface
 * @brief System UART Middle Layer With Semtech UART Functions
 * @{
 */

/** Size of the AT buffer */
#define AT_BUFFER_SIZE	256

/** Buffer used to store incoming UART data */
uint8_t ATBufferR[AT_BUFFER_SIZE];
/** Buffer used to store outgoing UART data */
uint8_t ATBufferT[AT_BUFFER_SIZE];

/** New line characters used for cmdResponse */
const uint8_t newLineCharacters[] = "\r\n";

/**
 * Initialise UART for AT commands
 *
 * @param BaudRate Baud rate for the UART communication
 */
void AtModeInit( uint32_t BaudRate )
{
	FifoInit(&Uart1.FifoTx, ATBufferT, AT_BUFFER_SIZE);
	FifoInit(&Uart1.FifoRx, ATBufferR, AT_BUFFER_SIZE);
	UartInit( &Uart1, 1, UART_TX, UART_RX );
	UartConfig(&Uart1, RX_TX, BaudRate, UART_8_BIT, UART_1_STOP_BIT, NO_PARITY, NO_FLOW_CTRL);
}

/**
 * DeInitializes the UART and the pins
 */
void AtModeDeInit( void )
{
	UartDeInit( &Uart1 );
}

/**
 * Transmit data over serial line
 *
 * @see LOWAPP_SYS_IF#SYS_cmdResponse
 */
int8_t cmd_response(uint8_t* data, uint16_t length){
	if(UartPutBuffer(&Uart1, data, length) == 0) {
		// Add end of line character
		UartPutBuffer(&Uart1, (uint8_t*)newLineCharacters, 2);
		return 0;
	}
	else {
		return -1;
	}
}

/** @} */
/** @} */
