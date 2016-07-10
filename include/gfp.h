#ifndef GFP_H
#define GFP_H

typedef void*(*access_gfp_t)(void*);
typedef int(*compare_gfp_t)(void*,void*);
typedef void(*process_gfp_t)(void*);

/* useful for map() type of functions, */
/* typedef void*(*transform_gfp_t)(void*, void*); */

#include <stdio.h>
typedef void(*save_gfp_t)(void*,FILE*);
typedef void*(*load_gfp_t)(FILE*);

#endif
