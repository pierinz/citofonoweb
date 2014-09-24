#define _GNU_SOURCE
#include "common.h"
#include <pthread.h>

#define VERSION "0.3"

/* Default configuration directory path (if not specified with -f)*/
#ifndef CONFPATH
    #define CONFPATH "conf"
#endif

/* Max length of log line */
#ifndef loglen
    #define loglen 150
#endif

char *source, *helper, separator[2];
short verbose=0;
short debounce=1;

pthread_t thr_source, thr_helper;
int psource[2], phelperIN[2], phelperOUT[2];
int spid,hpid;
/* Set to 0 if someone requested the process to terminate */
int errorstatus=1;

#ifdef SYSTEMD_ONLY
#define NO_LOGFILE
#endif

#ifdef MK_PIDFILE
char *pidfile;
#endif

#ifndef NO_LOGFILE
char *logfile;
FILE* flog;
#endif
/* Mutex */
pthread_mutex_t mutex;

int loop=1;

void version(){
	printf("badge_daemon, version %s (compiled %s, %s)\n", VERSION, __TIME__, __DATE__);
    printf("(C) 2012-2014 Gabriele Martino\n");
    printf("Website: https://github.com/pierinz/citofonoweb\n");
    printf("\nCompiled options:\n");
#ifdef SYSTEMD_ONLY
	printf("-DSYSTEMD_ONLY ");
#endif
#ifdef MK_PIDFILE
	printf("-DMK_PIDFILE ");
#endif
#ifdef NO_LOGFILE
	printf("-DNO_LOGFILE ");
#endif
	printf("-DCONFPATH %s -Dloglen %d -Dconfline %d -Dconfdef %d -Dconfval %d -Dkeylen %d\n",
			CONFPATH, loglen, confline, confdef, confval, keylen);
	fflush(stdout);
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

void logmessage(char *message){
    char buffer[22];
    time_t rawtime;
    struct tm * timeinfo;
    
    time (&rawtime);
    timeinfo = localtime (&rawtime);
    strftime(buffer,22,"%d/%m/%y %H:%M:%S - ",timeinfo);

#ifndef NO_LOGFILE
    /* Other threads shouldn't write logfile concurrently */
    pthread_mutex_lock(&mutex);
    fputs(buffer,flog);
    fputs(message,flog);
    fputs("\n",flog);
    fflush(flog);
    pthread_mutex_unlock(&mutex);
#else
	printf("%s%s\n",buffer,message);
	fflush(stdout);
#endif
}

void rotate(){
#ifndef NO_LOGFILE
    pthread_mutex_lock(&mutex);
    fclose(flog);
    while(access(logfile,F_OK)==0){
        /* Wait for file deletion */
        usleep(20000);
    }
    flog=fopen(logfile,"a");
    pthread_mutex_unlock(&mutex);
	fprintf(stderr,"Logfile rotated.");
    logmessage("Logfile rotated.");
#else
	logmessage("rotate failed - this program was compiled without logfile support.");
#endif
}

void clean(){
#ifndef NO_LOGFILE
    if (flog)
        fclose(flog);
	free(logfile);
#endif
#ifdef MK_PIDFILE
	unlink(pidfile);
	free(pidfile);
#endif
    free(source);
    free(helper);
}

void fatal(char* message){
    fprintf(stderr,"%s\n",message);
    logmessage(message);

    fprintf(stderr,"Fatal error - program terminated.\n");
    logmessage("Fatal error - program terminated.");

    clean();
    exit(1);
}

void loadConf(char *conffile){
    FILE* fp;
    char line[confline], def[confdef], val[confval];

    fp=fopen(conffile,"r");
    if (!fp){
        fprintf(stderr,"File %s:\n",conffile);
        perror("Error opening configuration: ");
        exit(1);
    }
    
    while(fgets(line,(confline - 1),fp)){
		/* Delete previous value */
		def[0]='\0';
		val[0]='\0';

        sscanf(line,"%s %[^\n]",def,val);
        if (strcmp(def,"source")==0){
            /* must be large enough to contain "val" */
            source=calloc(1,strlen(val)+1);
            strcpy(source,val);
			continue;
        }
        if (strcmp(def,"helper")==0){
            /* must be large enough to contain "val" */
            helper=calloc(1,strlen(val)+1);
            strcpy(helper,val);
			continue;
        }
		#ifndef NO_LOGFILE
        if (strcmp(def,"logfile")==0){
            /* must be large enough to contain "val" */
            logfile=calloc(1,strlen(val)+1);
            strcpy(logfile,val);
			continue;
        }
		#endif
		#ifdef MK_PIDFILE
		if (strcmp(def,"pidfile")==0){
            /* must be large enough to contain "val" */
            pidfile=calloc(1,strlen(val)+1);
            strcpy(pidfile,val);
			continue;
        }
		#endif
        if (strcmp(def,"verbose")==0){
            verbose=atoi(val);
			continue;
        }
        if (strcmp(def,"debounce")==0){
            debounce=atoi(val);
			continue;
        }
    }
    fclose(fp);

    if (verbose > 1){
        fprintf(stderr,"Configuration loaded.\n");
    }
}

void *tSource(){
    char *buffer,*oldbuffer, *msg;
    time_t now,before;
    FILE* pipesource;
	char** args;

    spid=fork();
    if(spid==0){
        /* Child process*/
        close(STDOUT_FILENO);
        close(STDIN_FILENO);
        dup2(psource[1], STDOUT_FILENO);
        close(psource[0]);
        close(psource[1]);

        args=argv_from_string(source);
        if (execvp(args[0],args)<0){
            perror("Source -> execvp: ");
            _exit(1);
        }
    }
    else if (spid>0){
        /* Parent process */
        free(source);
        source=NULL;
        
        close(psource[1]); /* These are being used by the child */

        buffer=calloc(1,sizeof(char)*keylen);
        oldbuffer=calloc(1,sizeof(char)*keylen);
        oldbuffer=strcpy(oldbuffer,"\0");
        before=0;
        time(&now);
        
        pipesource=fdopen(psource[0],"r");
        
        if (verbose > 1){
            fprintf(stderr,"Source -> ready.\n");
        }
        logmessage("Source -> ready.");
        
        while (loop && fgets(buffer,keylen,pipesource)){
            strtok(buffer,"\n");
            
            if (verbose > 1){
                fprintf(stderr,"Source -> raw data: %s\n",buffer);
            }
            
            time(&now);
            if (strcmp(buffer,oldbuffer)!=0 && (before+debounce) <= now){
                if (verbose > 1){
                    fprintf(stderr,"Got badge %s from source\n",buffer);
                }
                
                msg=calloc(1,sizeof(char)*(strlen(buffer)+1+22));
                sprintf(msg,"Got badge %s from source",buffer);
                logmessage(msg);
                free(msg);
                
                strcat(buffer,"\n");
                /* Send key to helper via pipe */
                if (write(phelperOUT[1],buffer,sizeof(char)*(strlen(buffer))) < 0){
                    perror("write: ");
                    break;
                }
                
                strcpy(buffer,oldbuffer);
                before=now;
            }
        }
        fclose(pipesource);
        close(phelperOUT[1]);
        free(buffer);
        free(oldbuffer);
        /* Pipe closed by signal handler */
        
        if (verbose > 0){
            fprintf(stderr,"Source -> process terminated.\n");
        }
        logmessage("Source -> process terminated.");
    }
    else{
        perror("Source -> fork: ");
        loop=0;
    }
    return 0;
}


void *tHelper(){
    char *buffer;
    time_t now;
    FILE* pipehelper;
	char** args;

    hpid=fork();
    if(hpid==0){
        /* Child process*/
        close(STDOUT_FILENO);
        close(STDIN_FILENO);
        dup2(phelperOUT[0], STDIN_FILENO);
        dup2(phelperIN[1], STDOUT_FILENO);

        close(phelperIN[0]);
        close(phelperOUT[1]);
        close(phelperIN[1]);
        close(phelperOUT[0]);

        args=argv_from_string(helper);
        if (execvp(args[0],args)<0){
            perror("Source -> execvp: ");
            _exit(1);
        }
    }
    else if (hpid>0){
        /* Parent process */
        free(helper);
        helper=NULL;
        
        /* These are being used by the child */
        close(phelperIN[1]);
        close(phelperOUT[0]);

        buffer=calloc(1,sizeof(char)*loglen);
        time(&now);
        
        pipehelper=fdopen(phelperIN[0],"r");
        
        if (verbose > 1){
            fprintf(stderr,"Helper -> ready.\n");
        }
        logmessage("Helper -> ready to parse data.");
        
        while (loop && fgets(buffer,loglen,pipehelper)){
            strtok(buffer,"\n");
            time(&now);
            if (verbose > 1){
                fprintf(stderr,"Helper -> logging %s\n",buffer);
            }
            logmessage(buffer);
        }
        fclose(pipehelper);
        free(buffer);
        /* Pipe closed by signal handler */
        if (verbose > 0){
            fprintf(stderr,"Helper -> process terminated.\n");
        }
        logmessage("Helper -> process terminated.");
    }
    else{
        perror("Helper -> fork: ");
        loop=0;
    }
    return 0;
}

void signal_handler(int signum){
    char *buf;
    if ((signum==SIGTERM) || (signum==SIGINT) || (signum==SIGQUIT)){
		errorstatus=0;
        if (asprintf(&buf,"Caught signal %d, shutting down...",signum) < 0){
            perror("asprintf: ");
        }
        else{
            fprintf(stderr,"%s\n",buf);
            logmessage(buf);
            free(buf);
        }
        kill(spid,15);
        kill(hpid,15);
        /* Then we continue the cleanup in SIGCHLD */
    }
    else if (signum==SIGCHLD){
        loop=0;
        kill(spid,15);
        kill(hpid,15);
        waitpid(spid,NULL,0);
        waitpid(hpid,NULL,0);
    }
    else if (signum==SIGUSR1){
		#ifndef NO_LOGFILE
        if (asprintf(&buf,"Caught signal %d, rotating log file...",signum) < 0){
            perror("asprintf: ");
            clean();
            exit(1);
        }
        else{
            fprintf(stderr,"%s\n",buf);
            logmessage(buf);
            free(buf);
        }
		#endif
        rotate();
    }
}

int main (int argc, char *argv[]){
    struct sigaction sig_h;
	int c;
	#ifdef MK_PIDFILE
	FILE* pidf;
	#endif
	char *conffile=NULL;
	
	/* Load settings from commandline */
    while ((c = getopt (argc, argv, "f:hv")) != -1){
        switch (c){
            case 'f':
                if (asprintf(&conffile,"%s",optarg)<0){
					perror("asprintf");
					exit(1);
				}
                break;
            case 'h':
                printf("Usage: badge_daemon [ -f configuration file ] [ -h ] [ -v ]\n"
                    "\n"
                    "-f FILE\t\tLoad configuration from FILE (badge_daemon.conf if not specified)\n"
                    "-h\t\tPrint this message and exit \n"
					"-v\t\tPrint version information and exit\n\n"
                );
                exit (1);
			case 'v':
				version();
				exit (1);
        }
    }
	if (!conffile){
		if (asprintf(&conffile,"%s/%s",CONFPATH,"badge_daemon.conf")<0){
			perror("asprintf");
			exit(1);
		}
	}
	
    loadConf(conffile);
	free(conffile);

	#ifdef MK_PIDFILE
	pidf=fopen(pidfile,"w");
	if (!pidf){
		perror("pidfile: fopen: ");
        clean();
        exit(1);
	}
	fprintf(pidf,"%d\n",getpid());
	fclose(pidf);
	#endif
	
	#ifndef NO_LOGFILE
    flog=fopen(logfile,"a");
    if (!flog){
        perror("logfile: fopen: ");
        clean();
        exit(1);
    }
	#endif

    logmessage("Daemon started.");
    
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
    sig_h.sa_flags=SA_NODEFER;
    sigaction(SIGCHLD,&sig_h,NULL);
    
    if (pipe(psource) < 0){
        perror("pipe:");
        fatal("Error opening source pipe");
    }

    if (pipe(phelperIN) < 0){ /* Where the parent is going to write to */
        perror("pipe:");
        fatal("Error opening helper pipe 1");
    }
    if (pipe(phelperOUT) < 0){ /* From where parent is going to read */
        perror("pipe:");
        fatal("Error opening helper pipe 1");
    }
    
    /* Crea thread di controllo coda */
    pthread_create(&thr_source,NULL,tSource,NULL);
    if (verbose>1){
        fprintf(stderr,"Source thread started.\n");
    }
    logmessage("Source thread started.");

    /* Crea thread autenticazione */
    pthread_create(&thr_helper,NULL,tHelper,NULL);
    if (verbose>1){
        fprintf(stderr,"Helper thread started.\n");
    }
    logmessage("Helper thread started.");

    /* Attendi che i threads terminino */
    pthread_join(thr_source,NULL);
    pthread_join(thr_helper,NULL);
    
    logmessage("Program terminated.");
    clean();

    return errorstatus;
}