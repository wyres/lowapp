/**
 * @file lowapp_shared_res.h
 * @brief Protection for access to shared resources
 *
 * @author Nathan Olff
 * @date August 25, 2016
 */

#ifndef LOWAPP_SHARED_RES_H_
#define LOWAPP_SHARED_RES_H_

void wakeup_sm();
void reset_device();

int lock_wakeUp();
int unlock_wakeUp();
int lock_eventQ();
int unlock_eventQ();
int lock_coldEventQ();
int unlock_coldEventQ();
int lock_atcmd();
int unlock_atcmd();

void init_mutexes();
void clean_mutex();

#endif
