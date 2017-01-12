/**
 * @file lowapp_sys_timer.h
 * @brief Implementation of the Timer related functions for LoWAPP
 *
 * @author Nathan Olff
 * @date October 21, 2016
 */
#ifndef LOWAPP_SYS_TIMER_H_
#define LOWAPP_SYS_TIMER_H_

#include "lowapp_sys.h"
#include "timer.h"

int init_timers();
void clear_timer();
void init_timer1(void (*callback)(void));
void init_timer2(void (*callback)(void));
void init_timer_cad(void (*callback)(void));
void set_timer1(uint32_t timems);
void set_timer2(uint32_t timems);
void set_timer_cad(uint32_t timems);
void cancel_timer1();
void cancel_timer2();
void cancel_timer_cad();

uint64_t get_time_ms();

#endif
