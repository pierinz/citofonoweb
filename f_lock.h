/* Include guard */
#ifndef F_LOCK_H
#define	F_LOCK_H

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

void f_elock(int fd);

void f_shlock(int fd);

void f_unlock(int fd);

#endif	/* F_LOCK_H */
