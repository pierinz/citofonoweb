/* Include guard */
#ifndef BADGE_LOGGER_COMMON_H
#define	BADGE_LOGGER_COMMON_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <sys/mman.h>
#include <sys/fcntl.h>
#include "f_lock.h"

#ifndef elsize
#define elsize 64
#endif

int pushData(char* param);

/* Copy the last element to result and removes from queue */
int popData(char** result);

/* Copy the last element to result without removing from the queue */
int pickData(char** result);


#ifdef	__cplusplus
}
#endif

#endif	/* BADGE_LOGGER_COMMON_H */
