/*
 * api_linked_checksum_list.c
 *
 *  Created on: Mar 25, 2016
 *      Author: santiago
 */

#include "api_list.h"

/**
 * Initializer
 */
void list_init(list_t *list) {
	list->head = NULL;
	list->size = 0;
}

/**
 * Destructor
 * @note: Once destroyed, although its in the same state as the
 * #Constructor, its advisable to init it again, in case this
 * mutates in the future.
 */
void list_destroy(list_t *list) {
	node_t *node = list->head;
	node_t *nextNode;

	while(node != NULL) {
		nextNode = node->next;

		free(node);

		node = nextNode;
	}

	list->head = NULL;
	list->size = 0;
}

/**
 * Get element at index position
 *
 * @returns 0 if IndexOutOfBounds or the data in the given index.
 */
unsigned long list_get(list_t *list, int index) {
	if(index >= list->size || index < 0)
		return 0;

	node_t *node = list->head;

	for(int i = 0 ; i < index ; i++)
		node = node->next;

	return node->data;
}

/**
 * Append at the end of the list an element
 */
void list_add(list_t *list, unsigned long num) {
	node_t *node = list->head;

	node_t *newNode = malloc(sizeof(node_t));
    newNode->data = num;
    newNode->next = NULL;

	if(node != NULL) {
		while(node->next != NULL)
			node = node->next;

	    node->next = newNode;
	} else {
		list->head = newNode;
	}

    list->size++;
}

/**
 * @return size of the list
 */
int list_count(list_t *list) {
	return list->size;
}
