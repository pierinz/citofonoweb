#define _GNU_SOURCE
#include "common.h"
#include "badge_logger_common.h"

char *tmpf, *queue;
char *reporthandler, *params;
char *uploader;

int qsize=1500, msize;
uint16_t *start, *current;
int interval=30;
short verbose=0;
short loop=1;
short getseconds=0;

int fd=0;
int hpid=0;

void loadConf(char *conffile){
    FILE* fp;
    char line[confline], def[confdef], val[confval];

    fp=fopen(conffile,"r");
	free(conffile);
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
        if (strcmp(def,"queuefile")==0){
            asprintf(&tmpf,"%s",val);
			continue;
        }
		if (strcmp(def,"queuesize")==0){
			if (atoi(val) > 65535){
				fprintf(stderr, "Your queue size is too long. Max queue size: 65535\n");
				exit(1);
			}
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
		if (strcmp(def,"params")==0){
            asprintf(&params,"%s",val);
			continue;
        }
		if (strcmp(def,"uploader")==0){
            asprintf(&uploader,"%s",val);
			continue;
        }
    }
    fclose(fp);

	if (params == NULL){
		fprintf(stderr, "Error: 'params' not found in configuration.\n");
		exit(1);
	}
	if (tmpf == NULL){
		fprintf(stderr, "Error: queue file not found in configuration.\n");
		exit(1);
	}
	if (uploader == NULL){
		asprintf(&uploader, " ");
	}
	if (reporthandler == NULL){
		asprintf(&uploader, " ");
	}

    if (verbose > 1){
        fprintf(stderr,"Configuration loaded.\n");
    }
}

char** argv_from_string(char *args) {
    int i, spaces = 0, argc = 0, len = strlen(args);
    char **argv;

    for (i = 0; i < len; i++)
        if (isspace(args[i]))
            spaces++;

    /* add 1 for cmd, 1 for NULL and 1 as spaces will be one short */
    argv = (char**) malloc ( (spaces + 3) * sizeof(char*) );
    argv[argc++] = args;
    
    for ( i = 0; i < len; i++ ) {
        if ( isspace(args[i]) ) {
            args[i] = '\0';
            if ( i + 1 < len )
              argv[argc++] = args + i + 1;
        }
    }

    argv[argc] = (char*)NULL;
    return argv;
}

void feedback(char* param){
	char *command;
	char **args;
	int pid=0;

	pid=fork();
	if (pid == 0){
		if (strlen(reporthandler) < 2){
			return;
		}

		if (asprintf(&command, "%s %s", reporthandler, param) < 0){
			perror("asprintf: ");
		}

		args=argv_from_string(command);
        if (execvp(args[0], args) < 0){
            perror("Source -> execvp: ");
            _exit(1);
        }
		free(command);
	}
	else if (pid > 0){
		return;
	}
	else{
		perror("fork");
	}
}

void runUploader(){
	char **args;

	hpid=fork();
	if (hpid == 0){
		args=argv_from_string(uploader);
        if (execvp(args[0], args) < 0){
            perror("Source -> execvp: ");
            _exit(1);
        }
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
	char *param, elem[elsize], buftime[22];
	size_t *n;

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
				printf("Usage: badge_logger [ -f configuration file ] [ -h ]\n"
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
	pages=((sizeof(char) * (qsize + 1) * elsize) + (sysconf(_SC_PAGE_SIZE) - 1) ) / sysconf(_SC_PAGE_SIZE);
	msize=pages * sysconf(_SC_PAGE_SIZE);

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

	start = (uint16_t*) (queue + ((qsize) * elsize * sizeof(char)));
	current = (uint16_t*) (queue + ((qsize) * elsize * sizeof(char))) + sizeof(uint16_t);

	if (new > 0){
		*start=0;
		*current=0;
		msync(queue, msize, MS_SYNC);
	}

	/* Catch exit signals */
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

    /* Catch child termination signal */
    sig_h.sa_handler=signal_handler;
    sig_h.sa_flags=SA_NODEFER | SA_RESTART;
    sigaction(SIGCHLD,&sig_h,NULL);

	if (verbose){
		printf("Queued elements: %d\n", abs(*current - *start) / elsize);
	}
	/* Start uploader */
	if (strlen(uploader) > 2)
		runUploader();

	n=malloc(sizeof(int));
	*n=0;

	printf("Ready to accept data.\n");
	fflush(stdout);
	while (loop && (getline(&param, n, stdin) > 0)){
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
			printf("Got param: %s, index: %d %d\n", param, *start, *current);
			fflush(stdout);
		}

		/* Report to user (if configured) */
		feedback(param);

		if (pushData(elem) < 0){
			printf("Too many queued elements. Waiting the parser. Do not stop this program or all further elements will be lost.\n");
			fflush(stdout);
			retry=1;
			while (pushData(elem) < 0){
				printf("Attempt %d\n",retry);
				fflush(stdout);
				retry++;
				sleep(interval);
			}
			fflush(stdout);
		}
		else{
			printf("%s has been registered.\n", elem);
			fflush(stdout);
		}

		if (verbose){
			fprintf(stderr,"%d %d\n", *start, *current);
		}
		free(param);
		param=NULL;
	}

	/* Free param on getline error */
	if (param != NULL)
		free(param);

	free(n);
	free(tmpf);
	free(reporthandler);
	free(uploader);
	free(params);
	exit(0);
}
