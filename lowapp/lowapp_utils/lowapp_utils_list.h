/**
 * @file lowapp_utils_list.h
 * @brief Functions to manage linked lists
 *
 * @author Nathan Olff
 * @date November 10, 2016
 */

#ifndef LOWAPP_UTILS_LIST_H_
#define LOWAPP_UTILS_LIST_H_

#include "lowapp_core.h"
#include "lowapp_log.h"

/**
 * @addtogroup lowapp_core
 * @{
 */
/**
 * @addtogroup lowapp_core_utils
 * @{
 */
/**
 * @addtogroup lowapp_core_utils_linkedlist
 * @{
 */

/**
 * Linked list structure for duty cycle management
 */
typedef struct LL {
	/** Head of the linked list */
	struct LL_EL *head;
	/** Tail of the linked list */
	struct LL_EL *tail;
	/** Number of element in the list */
	uint8_t count;
	/** Sum of all the data stored in the list */
	uint64_t totalData;
}LL_T;

/**
 * Linked list element for duty cycle management
 */
typedef struct LL_EL {
	/** Pointer to the next element of the list */
 	struct LL_EL *next;
 	/** Pointer to the previous element of the list */
 	struct LL_EL *prev;
 	/** Data stored in the element of the list */
 	uint16_t data;
 	/** Time linked to the data */
 	uint64_t time;
} LL_EL_T;


void add_to_list(LL_T* list, uint16_t d, uint64_t time);
uint8_t get_size_list(LL_T* list);
int8_t pop_head(LL_T* list);
int8_t get_head(LL_T* list, uint16_t* d, uint64_t* time);




/** @} */
/** @} */
/** @} */

#endif
