#include <stdlib.h>
#include "list.h"

void
list_append(list_t *list, void  *value) {
	listnode_t *newnode = (listnode_t*) malloc(sizeof(listnode_t));

	{
		listnode_t **last_ref = &list->last, *last = *last_ref;

		if (last)
			last->next = newnode;
		else
			list->first = newnode;

		*last_ref = newnode;
	}

	newnode->value = value;
	newnode->next = NULL;
}

void* list_serve(list_t *list) {
	listnode_t **first_ref = &list->first, *first = *first_ref;

	if (first) {
		void *res = first->value;
		*first_ref = first->next;
		if (first == list->last)
			list->last = first->next;
		free(first);
		return res;
	}

	return NULL;
}

list_t *new_list(void *first_value) {
	list_t *list = (list_t *) malloc(sizeof(list_t));
	listnode_t *first_node;
	
	if (first_value) {
		first_node = (listnode_t*) malloc(sizeof(listnode_t));
		first_node->next = NULL; 
		first_node->value = first_value;
	} else first_node = NULL;

	list->first = first_node;
	list->last = first_node;

	return list;
}
