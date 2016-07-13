#ifndef AVL_H
#define AVL_H

#include <stdio.h>

/* DEBUG MACRO transforms node's key to a text representation it writes
 * to FILE *f. Having this defined changes the function definitions of
 * avl_insert, etc. So comment this for more efficient implementations */
/* #define AVL_DEBUG(item, f) fprintf(f, "%.0f", (double)*(float*)item) */

typedef struct avl_st {
	void *data, *key;
	struct avl_st *parent, *left, *right;
	signed char balance;
} avl_t;

avl_t *avl_min(register avl_t *);
void avl_free(avl_t *);

#include "bool.h"
typedef bool_t (*avl_compare_gfp_t)(void*, void*);

#ifdef AVL_DEBUG
void avl_to_html(avl_t *, FILE *);
avl_t *avl_insert( avl_t*, void*, void*, avl_compare_gfp_t, FILE*);
#else
avl_t *avl_insert( avl_t*, void*, void*, avl_compare_gfp_t);
#endif

#endif
