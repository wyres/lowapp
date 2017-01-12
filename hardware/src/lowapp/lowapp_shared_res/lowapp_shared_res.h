#ifndef LOWAPP_SIMU_THREAD_PROTECTION_H_
#define LOWAPP_SIMU_THREAD_PROTECTION_H_

#include "board.h"


void wakeup_sm();
void reset_device();

void lock_wakeUp();
void unlock_wakeUp();
void lock_eventQ();
void unlock_eventQ();
void lock_coldEventQ();
void unlock_coldEventQ();
void lock_atcmd();
void unlock_atcmd();

void init_mutexes();
void clean_mutex();

#endif
