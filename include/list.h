#ifndef LIST_H
#define LIST_H

typedef struct listnode_st {
	void *value;
	struct listnode_st *next;
} listnode_t;

typedef struct {
	listnode_t *first, *last;
} list_t;

void list_append(list_t *, void *);
void *list_serve(list_t *);
list_t *new_list(void *);

#endif
