/**
 * @file lowapp_shared_res.c
 * @brief Protection for access to shared resources
 *
 * @author Nathan Olff
 * @date October 20, 2016
 */
#include "lowapp_shared_res.h"

/**
 * Wake up the state machine
 * 
 * (Only used in simulation)
 */
void wakeup_sm() {

}

/**
 * Simulate device reset
 *
 * (Only used in simulation)
 */
void reset_device() {

}

/**
 * Lock the standard event queue
 * Conflicts are avoided in the critical section by disabling interrupts
 */
void lock_eventQ() {
	__disable_irq();
}
/**
 * Unlock the standard event queue
 */
void unlock_eventQ(){
	__enable_irq();
}
/**
 * Lock the cold event queue
 *
 * Conflicts are avoided in the critical section by disabling interrupts
 */
void lock_coldEventQ() {
	__disable_irq();
}
/**
 * Unlock the cold event queue
 */
void unlock_coldEventQ(){
	__enable_irq();
}
/**
 * Lock the at command queue
 *
 * Conflicts are avoided in the critical section by disabling interrupts
 */
void lock_atcmd(){
	__disable_irq();
}
/**
 * Unlock the at command queue
 */
void unlock_atcmd(){
	__enable_irq();
}

/**
 * Initialise all mutexes
 *
 * (Only used in simulation)
 */
void init_mutexes(){

}

/**
 * Clean mutex resources
 *
 * (Only used in simulation)
 */
void clean_mutex(){

}
