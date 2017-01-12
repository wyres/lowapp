/**
 * @file lowapp_sys_io.h
 * @brief System level IO related functions for LoWAPP simulation
 * 
 * @date December 6, 2016
 * @author Nathan Olff
 */

#ifndef SRC_LOWAPP_LOWAPP_SYS_LOWAPP_SYS_IO_H_
#define LOWAPP_SYS_IO_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/syscall.h>

int8_t cmd_response(uint8_t* data, uint16_t length);


#endif /* SRC_LOWAPP_LOWAPP_SYS_LOWAPP_SYS_IO_H_ */
