/**
 * @file lowapp_sys_impl.h
 * @brief Implementation of the system level functions for LoWAPP
 *
 * @author Nathan Olff
 * @date October 20, 2016
 */
#ifndef LOWAPP_SYS_IMPL_H_
#define LOWAPP_SYS_IMPL_H_

#include "lowapp_inc.h"

void register_sys_functions(LOWAPP_SYS_IF_T *lowappSys);
int8_t get_config(const uint8_t* key, uint8_t* value);
int8_t set_config(const uint8_t* key, const uint8_t* val);

#endif
