/**
 * @file lowapp_sys_io.c
 * @brief System level IO related functions for LoWAPP simulation
 * 
 * @date December 6, 2016
 * @author Nathan Olff
 */

#include "lowapp_sys_io.h"

/**
 * Transmit data over serial line (or console output for the simulation)
 *
 * @param data Data to be sent (as a string)
 * @param length Length of the data to print
 * @return 0
 */
int8_t cmd_response(uint8_t* data, uint16_t length){
	fwrite(data, length, 1, stdout);
	fwrite("\n", 1, 1, stdout);
	fflush(stdout);
	return 0;
}
