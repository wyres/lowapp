/**
 * @file lowapp_if.h
 * @brief Interface functions of the LoWAPP core callable from the application
 * @date June 29, 2016
 *
 * This file contains the configuration the functions callable from
 * the application.
 *
 * @author Brian Wyld
 * @author Nathan Olff
 */
#ifndef LOWAPP_CORE_IF_H_
#define LOWAPP_CORE_IF_H_

#include "lowapp_sys.h"

int8_t lowapp_init(LOWAPP_SYS_IF_T* sys_fns);
int8_t lowapp_atcmd(uint8_t* cmdrequest, uint16_t size);
void lowapp_atcmderror();
uint8_t lowapp_process();

#endif /* LOWAPP_CORE_IF_H_ */
