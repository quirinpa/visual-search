#ifndef DEBUG_H
#define DEBUG_H

#define DEBUG 0
#define dprint(str, ...) if (DEBUG) fprintf(stderr, str"\n", __VA_ARGS__);

#endif
