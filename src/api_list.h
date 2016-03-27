/*
 * api_linked_list.h
 *
 *  Created on: Mar 25, 2016
 *      Author: santiago
 */

#ifndef API_LIST_H_
#define API_LIST_H_

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/**
 * Simple node struct for storing unsigned longs
 * @param data is the data (daaah)
 * @param next is a pointer to the next element of the "list"
 */
typedef struct node_t {
    unsigned long data;
    struct node_t *next;
} node_t;

/**
 * Simple list struct for storing
 * @param head is the first element of the list
 * @param size is the size of the list
 */
typedef struct list_t {
	node_t *head;
	int size;
} list_t;

//Methods (Its really simple)
void list_init(list_t *list);
void list_destroy(list_t *list);
unsigned long list_get(list_t *list, int index);
void list_add(list_t *list, unsigned long num);
int list_count(list_t *list);

#endif /* API_LIST_H_ */
