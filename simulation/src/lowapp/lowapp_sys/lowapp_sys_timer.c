/**
 * @file lowapp_sys_timer.c
 * @brief Basic timer management (for TIMEOUT and CADTIMEOUT)
 *
 * @author Nathan Olff
 * @date August 10, 2016
 */
#include "lowapp_sys_timer.h"
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <pthread.h>


/**
 * @addtogroup lowapp_simu
 * @{
 */
/**
 * @addtogroup lowapp_simu_timers LoWAPP Simulation Timers
 * @brief Linux timers simulating hardware timers
 * @{
 */

/** One shot timer id */
timer_t timer_id;
/** One shot timer 2 id */
timer_t timer2_id;
/** Repetitive timer id */
timer_t timer_repet_id;

/** Callback for one shot timer */
void (*timerCb)(void) = NULL;
/** Callback for one shot timer 2 */
void (*timer2Cb)(void) = NULL;
/** Callback for repetitive timer */
void (*timerRepetCb)(void) = NULL;

/**
 * Get epoch time in ms
 *
 * @return Time in ms
 */
uint64_t get_time_ms() {
	uint64_t milli;
	struct timespec spec;
	clock_gettime(CLOCK_MONOTONIC, &spec);
	milli = spec.tv_sec*1000 + round(spec.tv_nsec/1000000);
	return milli;
}

/**
 * Get epoch time in us
 *
 * @return Time in ms
 */
uint64_t get_time_us() {
	uint64_t milli;
	struct timespec spec;
	clock_gettime(CLOCK_MONOTONIC, &spec);
	milli = spec.tv_sec*1000000 + round(spec.tv_nsec/1000);
	return milli;
}

/**
 * Example of timer callback function
 * @param ts Current time
 */
void timer_callback(uint64_t ts) {
	printf("It is %ld\r\n", ts);
}

/**
 * @name One shot timer
 * @{
 */
/**
 * Handler for the Linux timer signal, calling the timerCb callback
 * @param sig Signal received
 */
void timer_handler(sigval_t sig) {
	timerCb();
	pthread_exit(NULL);
}

/**
 * Initialise the one shot timer with its handler
 *
 * @param callback Callback to call when the timer times out
 */
void init_timer1(void (*callback)(void)) {
	struct sigevent sev;

	/* Create the timer */
	sev.sigev_notify = SIGEV_THREAD;	// Handle timer in a thread
	sev.sigev_signo = SIGONESHOT;
	sev.sigev_notify_attributes = NULL;
	sev.sigev_notify_function = timer_handler;

	sev.sigev_value.sival_ptr = &timer_id;

	timerCb = callback;

	timer_create(CLOCK_MONOTONIC, &sev, &timer_id);
}


/**
 * Delete the timer at the end of the program to free resources
 */
void clean_timer1() {
	set_timer1(0);
	timer_delete(timer_id);
}


/**
 * Request to set a callback to be called after timems ms
 *
 * ! Because of the way Linux timer work, we need an intermediate function
 * called timer_handler that itself calls our callback.
 * The callback parameter of set_timer is therefore NOT used here.
 *
 * @param timems Time after which the timer should send its signal
 */
void set_timer1(uint32_t timems) {
	struct itimerspec its;

	/* Convert into s and ns */
	its.it_value.tv_sec = timems / 1000;
	its.it_value.tv_nsec = (timems % 1000)*1000000;
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;

	/* Arm the timer */
	timer_settime(timer_id, 0, &its, NULL);
}

/**
 * Disarm the timer
 */
void cancel_timer1() {
	set_timer1(0);
}

/** @} */

/**
 * @name Second one shot timer
 * @{
 */

/**
 * Handler for the Linux timer signal, calling the timer2Cb callback
 * @param sig Signal received
 */
void timer2_handler(sigval_t sig) {
	timer2Cb();
	pthread_exit(NULL);
}

/**
 * Initialise the one shot timer 2 with its handler
 */
void init_timer2(void (*callback)(void)) {
	struct sigevent sev;

	/* Create the timer */
	sev.sigev_notify = SIGEV_THREAD;	// Handle timer in a thread
	sev.sigev_signo = SIGONESHOT2;
	sev.sigev_notify_attributes = NULL;
	sev.sigev_notify_function = timer2_handler;

	timer2Cb = callback;

	sev.sigev_value.sival_ptr = &timer2_id;
	timer_create(CLOCK_MONOTONIC, &sev, &timer2_id);
}


/**
 * Delete the timer 2 at the end of the program to free resources
 */
void clean_timer2() {
	set_timer1(0);
}


/**
 * Request to set a callback to be called after timems ms
 *
 * ! Because of the way Linux timer work, we need an intermediate function
 * called timer_handler that itself calls our callback.
 * The callback parameter of set_timer is therefore NOT used here.
 *
 * @param timems Time after which the timer should send its signal
 */
void set_timer2(uint32_t timems) {
	struct itimerspec its;

	/* Convert into s and ns */
	its.it_value.tv_sec = timems / 1000;
	its.it_value.tv_nsec = (timems % 1000)*1000000;
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;

	/* Arm the timer */
	timer_settime(timer2_id, 0, &its, NULL);
}

/**
 * Disarm the timer
 *
 */
void cancel_timer2() {
	return set_timer2(0);
}

/** @} */

/**
 * @name Repetitive timer
 * @{
 */

/**
 * Handler for the Linux timer repetitive signal
 *
 * @param sig Signal received
 */
void timer_repet_handler(sigval_t sig) {
	timerRepetCb();
	pthread_exit(NULL);
}

/**
 * Initialise the repetitive timer with its handler
 */
void init_repet_timer(void (*callback)(void)) {
	struct sigevent sev;

	/* Create the timer */
	sev.sigev_notify = SIGEV_THREAD;	// Handle timer in a thread
	sev.sigev_signo = SIGREPET;
	sev.sigev_notify_attributes = NULL;
	sev.sigev_notify_function = timer_repet_handler;

	timerRepetCb = callback;

	sev.sigev_value.sival_ptr = &timer_repet_id;
	timer_create(CLOCK_MONOTONIC, &sev, &timer_repet_id);
}

/**
 * Delete the timer at the end of the program to free resources
 */
void clean_repet_timer() {
	timer_delete(timer_repet_id);
}

/**
 * Request to set a callback to be called every timems ms
 *
 * ! Because of the way Linux timer work, we need an intermediate function
 * called timer_handler that itself calls our callback.
 * The callback parameter of set_timer is therefore NOT used here.
 *
 * @param timems Time after which the timer should send its signal
 */
void set_repet_timer(uint32_t timems) {
	struct itimerspec its;

	/* Convert into s and ns */
	its.it_value.tv_sec = timems / 1000;
	its.it_value.tv_nsec = (timems % 1000)*1000000;
	its.it_interval.tv_sec = its.it_value.tv_sec;
	its.it_interval.tv_nsec = its.it_value.tv_nsec;

	/* Arm the timer */
	timer_settime(timer_repet_id, 0, &its, NULL);
}

/**
 * Disarm the repetitive timer
 *
 * @retval 0 On success
 * @retval -1 Otherwise
 */
void cancel_repet_timer() {
	return set_repet_timer(0);
}

/** @} */
/** @} */
/** @} */
