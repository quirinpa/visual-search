#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUG
#define dprint(str, ...); fprintf(stderr, str"\n", __VA_ARGS__);
#define dputs(str); fputs(str"\n", stderr);
#else
#define dprint(str, ...); ;
#define dputs(str); ;
#endif

#endif

