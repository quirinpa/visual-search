#ifndef FIFO_H
#define FIFO_H

typedef struct fifo_node_st {
	void *value;
	struct fifo_node_st *next;
} fifo_node_t;

#include<stddef.h>
typedef struct {
	fifo_node_t *top;
	/* size_t count; */
} fifo_t;


fifo_t *new_fifo(void);
void fifo_push(fifo_t *, void *);
void *fifo_pop(fifo_t *);

#endif
