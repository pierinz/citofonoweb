#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>

/* Max configuration line length */
#ifndef confline
    #define confline 255
#endif

/* Max length of configuration directive name */
#ifndef confdef
    #define confdef 55
#endif

/* Max length of configuration directive value */
#ifndef confval
    #define confval 200
#endif

/* Max length of key (the output line from source program) */
#ifndef keylen
    #define keylen 20
#endif