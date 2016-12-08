/**
 * @file lowapp_shared_res.c
 * @brief Protection for access to shared resources
 *
 * @author Nathan Olff
 * @date August 25, 2016
 */
#include "lowapp_shared_res.h"

#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

/**
 * @addtogroup lowapp_simu
 * @{
 */
/**
 * @addtogroup lowapp_simu_shared_resources LoWAPP Simulation Shared Resources Access
 * @brief Protection for access to shared resources between threads
 * @{
 */

/** Mutex protecting the standard event queue */
pthread_mutex_t mutex_eventQ;
/** Mutex protecting the cold event queue */
pthread_mutex_t mutex_coldEventQ;
/** Mutex protecting the at command queue */
pthread_mutex_t mutex_atcmd;

/** Pthread condition used to signal the radio thread */
pthread_cond_t cond_wakeup;
/** Mutex protecting the event queue for waking up the  */
pthread_mutex_t mutex_wakeup;

extern bool reboot;

/**
 * Wake up the state machine
 */
void wakeup_sm() {
	lock_wakeUp();
	pthread_cond_signal(&cond_wakeup);
	unlock_wakeUp();
}

/**
 * Simulate device reset
 */
void reset_device() {
	lock_wakeUp();
	reboot = true;
	pthread_cond_signal(&cond_wakeup);
	unlock_wakeUp();
}
/**
 * Initialise all mutexes
 */
void init_mutexes() {
	pthread_mutex_init ( &mutex_wakeup, NULL);
	pthread_mutex_init ( &mutex_atcmd, NULL);
	pthread_mutex_init ( &mutex_coldEventQ, NULL);
	pthread_mutex_init ( &mutex_eventQ, NULL);
}
/**
 * Lock the standard event queue mutex
 */
int lock_wakeUp() {
	return pthread_mutex_lock(&mutex_wakeup);
}
/**
 * Unlock the standard event queue mutex
 */
int unlock_wakeUp() {
	return pthread_mutex_unlock(&mutex_wakeup);
}
/**
 * Lock the standard event queue mutex
 */
int lock_eventQ() {
	return pthread_mutex_lock(&mutex_eventQ);
}
/**
 * Unlock the standard event queue mutex
 */
int unlock_eventQ() {
	return pthread_mutex_unlock(&mutex_eventQ);
}
/**
 * Lock the cold event queue mutex
 */
int lock_coldEventQ() {
	return pthread_mutex_lock(&mutex_coldEventQ);
}
/**
 * Unlock the cold event queue mutex
 */
int unlock_coldEventQ() {
	return pthread_mutex_unlock(&mutex_coldEventQ);
}
/**
 * Lock the at command queue
 */
int lock_atcmd() {
	return pthread_mutex_lock(&mutex_atcmd);
}
/**
 * Unlock the at command queue
 */
int unlock_atcmd() {
	return pthread_mutex_unlock(&mutex_atcmd);
}

/**
 * Clean mutex resources
 */
void clean_mutex() {
    pthread_mutex_destroy(&mutex_eventQ);
    pthread_mutex_destroy(&mutex_coldEventQ);
    pthread_mutex_destroy(&mutex_atcmd);
    pthread_mutex_destroy(&mutex_wakeup);
}

/** @} */
/** @} */
