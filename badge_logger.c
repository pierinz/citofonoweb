#define _GNU_SOURCE
#include "common.h"
#include <sys/mman.h>
#include <sys/fcntl.h>
#include "f_lock.h"

#ifndef elsize
#define elsize 64
#endif

char *datahandler, *params;
char *reporthandler;
char *tmpf, *queue;

int qsize=1500;
int start, current, msize;
int interval=30;
short verbose=0;
short loop=1;
short getseconds=0;

int fd=0;
int hpid=0, fpid=0;

void loadConf(char *conffile){
    FILE* fp;
    char line[confline], def[confdef], val[confval];

    fp=fopen(conffile,"r");
    if (!fp){
        fprintf(stderr,"File %s:\n",conffile);
        perror("Error opening configuration: ");
        exit(1);
    }

    while(fgets(line,(confline-1),fp)){
		/* Delete previous value */
		def[0]='\0';
		val[0]='\0';

        sscanf(line,"%s %[^\n]",def,val);

		if (strcmp(def,"verbose")==0){
            verbose=atoi(val);
			continue;
        }
        if (strcmp(def,"datahandler")==0){
            asprintf(&datahandler,"%s",val);
			continue;
        }
        if (strcmp(def,"params")==0){
            /* must be large enough to contain "val" */
            asprintf(&params,"%s",val);
			continue;
        }
        if (strcmp(def,"queuefile")==0){
            asprintf(&tmpf,"%s",val);
			continue;
        }
		if (strcmp(def,"queuesize")==0){
            qsize=atoi(val);
			continue;
        }
		if (strcmp(def,"getseconds")==0){
            getseconds=atoi(val);
			continue;
        }
		if (strcmp(def,"interval")==0){
            interval=atoi(val);
			continue;
        }
		if (strcmp(def,"reporthandler")==0){
            asprintf(&reporthandler,"%s",val);
			continue;
        }
    }
    fclose(fp);

    if (verbose > 1){
        fprintf(stderr,"Configuration loaded.\n");
    }
}

void feedback(char* param){
	char *command;

	if (strlen(reporthandler) < 2){
		return;
	}

	if (asprintf(&command, "%s %s", reporthandler, param) < 0){
		perror("asprintf: ");
	}

	if (system(command) != 0){
		printf("Unexpected error while reporting feedback\n");
		fflush(stdout);
	}
	free(command);
}

int pushData(char* param){
	int qfree;

	if ((current+elsize) % (qsize*elsize) == start){
		return -1;
	}

	f_elock(fd);
	sprintf(queue+current,"%s",param);
	current=(current+elsize) % (qsize*elsize);

	sprintf(queue+((qsize+1)*elsize),"%d|%d",start,current);

	msync(queue,msize,MS_SYNC);
	qfree=(qsize*elsize) - abs(start - current);
	if (qfree == qsize*elsize){
		qfree=0;
	}
	f_unlock(fd);
	return qfree;
}

/* Copy the last element to result and removes from queue */
int popData(char** result){
	if (abs(current-start) == 0){
		return 0;
	}

	f_elock(fd);
	if (result != NULL){
		sprintf(*result,"%s", queue+start);
	}

	start=(start+elsize) % (qsize*elsize);
	sprintf(queue+((qsize+1)*elsize),"%d|%d",start,current);
	msync(queue,msize,MS_SYNC);
	f_unlock(fd);
	return abs(current - start);
}

/* Copy the last element to result without removing from the queue */
int pickData(char** result){
	if (abs(current-start) == 0){
		return 0;
	}

	f_elock(fd);
	sprintf(*result,"%s", queue+start);

	f_unlock(fd);
	return abs(current - start);
}

int sendData(char param[keylen]){
	char* command;
	int success=-1;

	asprintf(&command,"%s %s",datahandler,param);
	if (verbose > 0){
		printf("Running save command: %s\n", command);
		fflush(stdout);
	}
	success=system(command);	
	free(command);

	if (success == 0){
		return 1;
	}
	else{
		printf("Error: the datahandler exited with status %d. Will retry later.\n", success);
		fflush(stdout);
		return -1;
	}
}

int emptyQueue(){
	char* element;
	int i=0, total, queued;

	total=abs(current-start);
	queued=total;
	if (total > 0){
		element=malloc(sizeof(char)*elsize);
		while (pickData(&element) > 0){
			if (verbose){
				printf("Got element %s from queue\n",element);
				fflush(stdout);
			}
			if (sendData(element) < 1){
				/* Stop on first error and retry in the future */
				printf("%d elements sent before server vanished.\n",i);
				fflush(stdout);
				break;
			}
			else{
				queued=popData(NULL);
			}
			i++;
		}
		free(element);
	}

	printf("Queued elements: %d/%d elements sent.\n", i, total / elsize);
	fflush(stdout);
	return queued;
}

void runHelper(){
	int retry=0;

	if (hpid)
		return;

	hpid=fork();

	if (hpid == 0){
		retry=1;
		printf("Parsing all queued elements in %d seconds\n", interval);
		fflush(stdout);
		sleep(interval);
		while (emptyQueue() > 0){
			printf("An error occurred. Attempt %d to parse data in %d seconds\n", retry, interval*retry);
			retry++;
			if (interval*retry < 900){
				sleep(interval*retry);
			}
			else{
				sleep(900);
			}

			if (!loop){
				_exit(0);
			}
			fflush(stdout);
		}
		printf("All elements parsed\n");
		_exit(0);
	}
	else if(hpid > 0){
		return;
	}
	else{
		perror("fork:");
	}
}

void signal_handler(int signum){
	int pid;

    if ((signum==SIGTERM) || (signum==SIGINT) || (signum==SIGQUIT)){
		printf("Caught signal %d, shutting down...\n", signum);
        loop=0;
		if (getpid()==hpid){
			kill(hpid, SIGTERM);
		}
		fclose(stdin);
    }
    else if (signum==SIGCHLD){
        pid=wait(NULL);
		if (pid == hpid){
			printf("background data loader %d has terminated.\n", pid);
			hpid=0;
		}

		else if (pid > 0){
			printf("helper %d has terminated.\n", pid);
		}
	}
	fflush(stdout);
}

int main (int argc, char *argv[]){
    struct sigaction sig_h;
	int c,pages,retry;
	char *conffile=NULL;
	char param[keylen],elem[elsize], buftime[22];

	short new=0;
	time_t rawtime;
	struct tm * timeinfo;

	/* Load settings from commandline */
    while ((c = getopt (argc, argv, "f:h")) != -1){
        switch (c){
            case 'f':
                if (asprintf(&conffile,"%s",optarg)<0){
					perror("asprintf");
					exit(1);
				}
                break;
            case 'h':
				printf("Usage: badge_daemon [ -f configuration file ] [ -h ]\n"
                    "\n"
                    "-f FILE\t\tLoad configuration from FILE\n"
                    "-h\t\tShow this message\n\n"
                );
                exit(1);
        }
    }
	if (!conffile){
		fprintf(stderr,"Configuration file missing.\n");
		exit(1);
	}

	loadConf(conffile);
	free(conffile);

	fd=open(tmpf, O_RDWR);
	if (fd < 0 && errno==ENOENT){
		/* First run: create file and set flag */
		fd=open(tmpf, O_RDWR | O_CREAT, S_IRWXU);
		new=1;
	}
	if (fd < 0){
		perror("fopen");
		exit(1);
	}

	/* Mmapped memory must be aligned to page size */
	pages=1+(((sizeof(char)*(qsize+1)*elsize)-1)/sysconf(_SC_PAGE_SIZE));
	msize=pages*sysconf(_SC_PAGE_SIZE);

	/* Enlarge the file to the defined size */
	if (ftruncate(fd,msize)<0){
		perror("ftruncate");
		exit(1);
	}
	/* Allocate the "shared memory" */
	queue=(char*) mmap(NULL, msize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

	if (queue==MAP_FAILED){
		perror("mmap");
		exit(1);
	}

	if (new > 0){
		start=0;
		current=0;
	}
	else{
		fprintf(stderr,"%s\n",queue+((qsize+1)*elsize));
		sscanf(queue+((qsize+1)*elsize),"%d|%d",&start,&current);
	}

	/* Cattura segnali di uscita */
    sig_h.sa_handler=signal_handler;
    sig_h.sa_flags=0;
    /* Signals blocked during the execution of the handler. */
    sigemptyset(&sig_h.sa_mask);
    sigaddset(&sig_h.sa_mask, SIGINT);
    sigaddset(&sig_h.sa_mask, SIGTERM);
    sigaddset(&sig_h.sa_mask, SIGQUIT);
    sigaddset(&sig_h.sa_mask, SIGUSR1);

    sigaction(SIGQUIT,&sig_h,NULL);
    sigaction(SIGINT,&sig_h,NULL);
    sigaction(SIGTERM,&sig_h,NULL);
    sigaction(SIGUSR1,&sig_h,NULL);

    /* Cattura segnale child process concluso */
    sig_h.sa_handler=signal_handler;
    sig_h.sa_flags=SA_NODEFER | SA_RESTART;
    sigaction(SIGCHLD,&sig_h,NULL);

	if (verbose){
		printf("Queued elements: %d\n", abs(current-start) / elsize);
	}
	if (abs(current-start) > 0){
		runHelper();
	}

	printf("Ready to accept data.\n");
	fflush(stdout);
	while (loop && fgets(param,keylen,stdin)){
		/* Remove trailing \n */
		strtok(param,"\n");

		time (&rawtime);
		timeinfo = localtime (&rawtime);
		if (getseconds){
			strftime(buftime,22,"%Y%m%d%H%M%S",timeinfo);
		}
		else{
			strftime(buftime,22,"%Y%m%d%H%M00",timeinfo);
		}

		sprintf(elem, params, buftime, param);

		if (verbose > 1){
			printf("Got param: %s, index: %d %d\n",param, start, current);
			fflush(stdout);
		}

		/* Report to user (if configured) */
		feedback(param);
		
		if (sendData(elem) < 0){
			if (pushData(elem) < 0){
				printf("Too many queued elements. Waiting the parser. Do not stop this program or all further elements will be lost.\n");
				fflush(stdout);
				retry=1;
				while (sendData(elem) < 0){
					printf("Attempt %d\n",retry);
					fflush(stdout);
					retry++;
					sleep(interval);
				}
				printf("The parser is working! Sending all queued elements\n");
				fflush(stdout);
				emptyQueue();
			}
			else{
				runHelper();
			}
		}
		else{
			emptyQueue();
		}
		if (verbose){
			fprintf(stderr,"%d %d\n",start, current);
		}
	}

	free(tmpf);
	free(datahandler);
	free(reporthandler);
	free(params);
	exit(0);
}
