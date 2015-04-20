#include "f_lock.h"

/* Exclusive lock */
void f_elock(int fd){
	struct flock fl;
	
	fl.l_type   = F_WRLCK;  /* F_RDLCK, F_WRLCK, F_UNLCK    */
	fl.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END */
	fl.l_start  = 0;        /* Offset from l_whence         */
	fl.l_len    = 0;        /* length, 0 = to EOF           */
	fl.l_pid    = getpid(); /* our PID                      */
	
	/* Lock file */
	if (fcntl(fd, F_SETLKW, &fl)==-1){  /* F_GETLK, F_SETLK, F_SETLKW */
		perror("fcntl");
		exit(1);
	}
}

/* Shared lock */
void f_shlock(int fd){
	struct flock fl;
	
	fl.l_type   = F_RDLCK;  /* F_RDLCK, F_WRLCK, F_UNLCK    */
	fl.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END */
	fl.l_start  = 0;        /* Offset from l_whence         */
	fl.l_len    = 0;        /* length, 0 = to EOF           */
	fl.l_pid    = getpid(); /* our PID                      */
	
	/* Lock file */
	if (fcntl(fd, F_SETLKW, &fl)==-1){  /* F_GETLK, F_SETLK, F_SETLKW */
		perror("fcntl");
		exit(1);
	}
}

/* Remove lock */
void f_unlock(int fd){
	struct flock fl;
	
	fl.l_type   = F_UNLCK;  /* F_RDLCK, F_WRLCK, F_UNLCK    */
	fl.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END */
	fl.l_start  = 0;        /* Offset from l_whence         */
	fl.l_len    = 0;        /* length, 0 = to EOF           */
	fl.l_pid    = getpid(); /* our PID                      */
	
	/* Unlock file */
	if (fcntl(fd, F_SETLK, &fl)==-1){  /* F_GETLK, F_SETLK, F_SETLKW */
		perror("fcntl");
		exit(1);
	}
}
