#define _GNU_SOURCE
#include "badge_logger_common.h"
extern int qsize;
extern uint16_t *start, *current, msize;
extern char* queue;
extern int fd;

int pushData(char* param) {
	int qfree;

	if ((*current + elsize) % (qsize * elsize) == *start) {
		return -1;
	}

	f_elock(fd);
	sprintf(queue + *current, "%s", param);
	*current = (*current + elsize) % (qsize * elsize);

	msync(queue, msize, MS_SYNC);
	qfree = (qsize * elsize) - abs(*start - *current);
	if (qfree == qsize * elsize) {
		qfree = 0;
	}
	f_unlock(fd);
	return qfree;
}

/* Copy the last element to result and removes from queue */
int popData(char** result) {
	if (abs(*current - *start) == 0) {
		return 0;
	}

	f_elock(fd);
	if (result != NULL) {
		sprintf(*result, "%s", queue + *start);
	}

	*start = (*start + elsize) % (qsize * elsize);
	msync(queue, msize, MS_SYNC);
	f_unlock(fd);
	return abs(*current - *start);
}

/* Copy the last element to result without removing from the queue */
int pickData(char** result) {
	if (abs(*current - *start) == 0) {
		return 0;
	}

	f_elock(fd);
	sprintf(*result, "%s", queue + *start);

	f_unlock(fd);
	return abs(*current - *start);
}
