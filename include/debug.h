#ifndef DEBUG_H
#define DEBUG_H

#define DEBUG 1
#define dprint(str, ...) if (DEBUG) fprintf(stderr, #str"\n", __VA_ARGS__);

#endif
