/**
 * @file console.h
 * @brief Console related functions (simulate UART)
 *
 * @author Nathan Olff
 * @date August 10, 2016
 */

#ifndef LOWAPP_SIMU_CONSOLE_H_
#define LOWAPP_SIMU_CONSOLE_H_

#include "board.h"

/**
 * Signal used to stop console thread when SIGINT is received by the main
 * thread.
 */
#define SIGNAL_CONSOLE_END	SIGUSR1

int8_t cmd_response(uint8_t* data, uint16_t length);
void* console_handler(void* arg);
int8_t start_thread_cmd();

#endif /* LOWAPP_SIMU_CONSOLE_H_ */
