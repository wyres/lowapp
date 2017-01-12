/**
 * @file lowapp_sys_timer.c
 * @brief Implementation of the Timer related functions for LoWAPP
 *
 * @author Nathan Olff
 * @date October 21, 2016
 */
#include "board.h"

/**
 * @addtogroup lowapp_hardware_sys
 * @{
 */
/**
 * @addtogroup lowapp_hardware_sys_timer LoWAPP Hardware System Timer Interface
 * @brief System Timer Middle Layer With Semtech timer Functions
 * @{
 */

/** Timer 1 used in the state machine */
TimerEvent_t lowapp_timer_sm1;
/** Timer 2 used in the state machine */
TimerEvent_t lowapp_timer_sm2;
/** Repetitive timer for CAD interval */
TimerEvent_t lowapp_timer_cad;

/** Callback for one shot timer 1 */
void (*lowapp_timer_sm1_cb)(void) = NULL;
/** Callback for one shot timer 2 */
void (*lowapp_timer_sm2_cb)(void) = NULL;
/** Callback for repetitive timer */
void (*lowapp_timer_cad_cb)(void) = NULL;


/**
 * Initialise the one shot timer with its handler
 */
void init_timers() {
	/* Init timers */
	TimerInit(&lowapp_timer_sm1, lowapp_timer_sm1_cb);
	TimerInit(&lowapp_timer_sm2, lowapp_timer_sm2_cb);
	TimerInit(&lowapp_timer_cad, lowapp_timer_cad_cb);
}

/**
 * Stop and delete all the timers
 */
void clear_timer() {
	TimerStop(&lowapp_timer_sm1);
	TimerStop(&lowapp_timer_sm2);
	TimerStop(&lowapp_timer_cad);
}

/**
 * Initialise timer 1
 *
 * @param callback Callback to call when the signal is received
 */
void init_timer1(void (*callback)(void)) {
	lowapp_timer_sm1_cb = callback;
	TimerInit(&lowapp_timer_sm1, lowapp_timer_sm1_cb);
}

/**
 * Request to set a callback to be called after timems ms using timer 1
 *
 * @param timems Time after which the timer should send its signal
 * @retval 0 On success
 * @retval -1 Otherwise
 */
void set_timer1(uint32_t timems) {
	TimerSetValue(&lowapp_timer_sm1, timems);
	TimerStart(&lowapp_timer_sm1);
}

/**
 * Initialise timer 2
 *
 * @param callback Callback to call when the signal is received
 */
void init_timer2(void (*callback)(void)) {
	lowapp_timer_sm2_cb = callback;
	TimerInit(&lowapp_timer_sm2, lowapp_timer_sm2_cb);
}

/**
 * Request to set a callback to be called after timems ms using timer 2
 *
 * @param timems Time after which the timer should send its signal
 */
void set_timer2(uint32_t timems) {
	TimerSetValue(&lowapp_timer_sm2, timems);
	TimerStart(&lowapp_timer_sm2);
}

/**
 * Initialise repetitive timer
 *
 * @param callback Callback to call when the signal is received
 */
void init_timer_cad(void (*callback)(void)) {
	lowapp_timer_cad_cb = callback;
	TimerInit(&lowapp_timer_cad, lowapp_timer_cad_cb);
}

/**
 * Request to set a callback to be called after timems ms using timer cad
 *
 * @param timems Time after which the timer should send its signal
 */
void set_timer_cad(uint32_t timems) {
	TimerSetValue(&lowapp_timer_cad, timems);
	TimerStart(&lowapp_timer_cad);
}

/**
 * Disarm the timer 1
 */
void cancel_timer1() {
	TimerStop(&lowapp_timer_sm1);
}

/**
 * Disarm the timer 2
 */
void cancel_timer2() {
	TimerStop(&lowapp_timer_sm2);
}

/**
 * Disarm the timer cad
 */
void cancel_timer_cad() {
	TimerStop(&lowapp_timer_cad);
}

/**
 * Get time in ms
 *
 * @return Time in ms
 */
uint64_t get_time_ms() {
	return (uint64_t)TimerGetCurrentTime();
}

/** @} */
/** @} */
