/**
 * @file lowapp_utils_list.c
 * @brief Functions to manage linked lists
 *
 * @author Nathan Olff
 * @date November 10, 2016
 */
#include "lowapp_utils_list.h"
#include <stdlib.h>
#ifdef SIMU
	#include "lowapp_shared_res.h"
#endif

/**
 * @addtogroup lowapp_core
 * @{
 */
/**
 * @addtogroup lowapp_core_utils
 * @{
 */

/**
 * @addtogroup lowapp_core_utils_linkedlist LoWAPP Core Utility Linked List
 * @brief Linked List management
 * @{
 */

/**
 * Get the number of elements in a linked list
 * @param list List to check
 * @return The number of elements in the linked list
 */
uint8_t get_size_list(LL_T* list) {
	return list->count;
}

/**
 * Add an element to a list
 * @param list List to which the element will be added
 * @param d Element to add
 * @param time Time of the element (activity or transmission for example)
 */
void add_to_list(LL_T* list, uint16_t d, uint64_t time) {
	LL_EL_T *element = (LL_EL_T*) malloc(sizeof(LL_EL_T));
	if(get_size_list(list) == 0) {
		list->head = element;
		list->tail = element;
		element->prev = NULL;
	}
	else {
		list->tail->next = element;
		element->prev = list->tail;
	}
	element->next = NULL;
	element->data = d;
	element->time = time;
	list->totalData += d;
	list->tail = element;
	list->count++;
}

/**
 * Remove head of the list
 *
 * @param list List from which the head should be removed
 * @retval -1 If the list was empty
 * @retval 0 On success
 */
int8_t pop_head(LL_T* list) {
	uint8_t size = get_size_list(list);
	LL_EL_T* tmp;
	if(size == 0) {
		return -1;
	}
	tmp = list->head;
	/* In case there is only one element, tmp->next will be NULL */
	list->head = tmp->next;
	list->count--;
	list->totalData -= tmp->data;
	//free(tmp);	// This is generating stack trace errors
	tmp = NULL;
	return 0;
}

/**
 * Get the head of the list
 *
 * @param list List from which the element should be retrieved
 * @param d Element retrieved from the head of the list
 * @param time Time of the element
 * @retval -1 If the list was empty
 * @retval 0 On success
 */
int8_t get_head(LL_T* list, uint16_t* d, uint64_t* time) {
	uint8_t size = get_size_list(list);
	if(size == 0) {
		return -1;
	}
	*d = list->head->data;
	*time = list->head->time;
	return 0;
}


/** @} */
/** @} */
/** @} */
