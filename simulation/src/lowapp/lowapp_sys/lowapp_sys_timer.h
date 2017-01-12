/**
 * @file lowapp_sys_timer.h
 * @brief Basic timer management (for TIMEOUT and CADTIMEOUT)
 *
 * @author Nathan Olff
 * @date August 10, 2016
 */

#ifndef LOWAPP_SYS_TIMER_H_
#define LOWAPP_SYS_TIMER_H_

#include <signal.h>
#include <stdint.h>


/** SIgnal number for one shot timer */
#define SIGONESHOT	SIGRTMIN
/** SIgnal number for one shot timer 2 */
#define SIGONESHOT2	SIGRTMIN+2
/** Signal number for repetitive timer */
#define SIGREPET	SIGRTMIN+1

void init_timer1(void (*callback)(void));
void init_timer2(void (*callback)(void));
uint64_t get_time_ms();
uint64_t get_time_us();
void timer_callback(uint64_t ts);
void set_timer1(uint32_t timems);
void clean_timer1();
void cancel_timer1();
void set_timer2(uint32_t timems);
void clean_timer2();
void cancel_timer2();
void set_repet_timer(uint32_t timems);
void init_repet_timer(void (*callback)(void));
void cancel_repet_timer();
void clean_repet_timer();

#endif /* LOWAPP_SIMU_TIMER_H_ */
