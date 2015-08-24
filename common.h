/* Include guard */
#ifndef B_COMMON_H
#define B_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>

/* Max configuration line length (including null character) */
#ifndef confline
#define confline 256
#endif

/* Max length of configuration directive name (including null character) */
#ifndef confdef
#define confdef 56
#endif

/* Max length of configuration directive value (including null character) */
#ifndef confval
#define confval 201
#endif

/* Max length of key (the output line from source program) (including null character) */
#ifndef keylen
#define keylen 20
#endif

#endif
