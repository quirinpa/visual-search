#include <stdlib.h>

#include "fifo.h"
fifo_t *new_fifo() {
	fifo_t *fifo = (fifo_t*) malloc(sizeof(fifo_t));
	/* fifo->count = 0; */
	fifo->top = NULL;
	return fifo;
}

void fifo_push(fifo_t *fifo, void *value) {
	fifo_node_t *node = (fifo_node_t*) malloc(sizeof(fifo_node_t));
	node->value = value;
	node->next = fifo->top;
	fifo->top = node;
	/* fifo->count++; */
}

void *fifo_pop(fifo_t *fifo) {
	fifo_node_t *top = fifo->top;

	if (top) {
		void *v = top->value;
		fifo->top = top->next;
		free(top);
		return v;
	}

	return NULL;
}
