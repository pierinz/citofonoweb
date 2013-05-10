#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>

#ifdef lights
#include "libdoor.h"
#endif

#ifndef CONFPATH
    #define CONFPATH "conf/badge_daemon.conf"
#endif

#ifndef confline
    #define confline 255
#endif

#ifndef confdef
    #define confdef 55
#endif

#ifndef confval
    #define confval 200
#endif

#ifndef keylen
    #define keylen 20
#endif

#ifndef loglen
    #define loglen 150
#endif

char *source, *helper, separator[2], *logfile;
short verbose=0;
short light=0;
short statusled=17;
short debounce=1;

pthread_t thr_source, thr_helper;
int psource[2], phelperIN[2], phelperOUT[2];
int spid,hpid;
FILE* flog;
/* Mutex */
pthread_mutex_t mutex;

int loop=1;

size_t trimwhitespace(char *out, size_t len, const char *str){
    const char *end;
    size_t out_size;

    
    if(len == 0)
      return 0;

    /* Trim leading space */
    while(isspace(*str)) str++;

    if(*str == 0)  /* All spaces? */
    {
      *out = 0;
      return 1;
    }

    /* Trim trailing space */
    end = str + strlen(str) - 1;
    while(end > str && isspace(*end)) end--;
    end++;

    /* Set output size to minimum of trimmed string length and buffer size minus 1 */
    out_size = (end - str) < len-1 ? (end - str) : len-1;

    /* Copy trimmed string and add null terminator */
    memcpy(out, str, out_size);
    out[out_size] = 0;

    return out_size;
}

void logmessage(char *message){
    char buffer[22];
    time_t rawtime;
    struct tm * timeinfo;
    
    time (&rawtime);
    timeinfo = localtime (&rawtime);
    strftime(buffer,22,"%m/%d/%y %H:%M:%S - ",timeinfo);

    /* Assicurati che nessuno scriva sul log contemporaneamente */
    pthread_mutex_lock(&mutex);
    fputs(buffer,flog);
    fputs(message,flog);
    fputs("\n",flog);
    fflush(flog);
    pthread_mutex_unlock(&mutex);
}

void rotate(){
    pthread_mutex_lock(&mutex);
    fclose(flog);
    while(access(logfile,F_OK)){
        /* Wait for file deletion */
        usleep(20000);
    }
    flog=fopen(logfile,"a");
    pthread_mutex_unlock(&mutex);
}

void clean(){
    fclose(flog);
    free(logfile);
}

void fatal(char* message){
    logmessage(message);
    logmessage("Fatal error - program terminated.");
    exit(1);
    clean();
}

void loadConf(){
    FILE* fp;
    char *line,*def,*val;

    fp=fopen(CONFPATH,"r");
    if (!fp){
        fprintf(stderr,"File %s:\n",CONFPATH);
        perror("Error opening configuration: ");
        exit(1);
    }
    
    line=calloc(1,confline*sizeof(char));
    def=calloc(1,confdef*sizeof(char));
    val=calloc(1,confdef*sizeof(char));
    
    while(fgets(line,255,fp)){
        sscanf(line,"%s%s",def,val);
        if (strcmp(def,"source")==0){
            /* must be large enough to contain "val" */
            source=calloc(1,strlen(val)+1);
            strcpy(source,val);
        }
        if (strcmp(def,"helper")==0){
            /* must be large enough to contain "val" */
            helper=calloc(1,strlen(val)+1);
            strcpy(helper,val);
        }
        if (strcmp(def,"logfile")==0){
            /* must be large enough to contain "val" */
            logfile=calloc(1,strlen(val)+1);
            strcpy(logfile,val);
        }
        if (strcmp(def,"verbose")==0){
            verbose=atoi(val);
        }
        if (strcmp(def,"light")==0){
            light=atoi(val);
        }
        if (strcmp(def,"statusled")==0){
            statusled=atoi(val);
        }
        if (strcmp(def,"debounce")==0){
            debounce=atoi(val);
        }
        memset(val,0,confdef*sizeof(char));
        memset(def,0,confdef*sizeof(char));
    }
    free(val);
    free(def);
    free(line);
    fclose(fp);

    if (verbose > 1){
        fprintf(stderr,"Configuration loaded.\n");
    }
}

void *tSource(){
    char *buffer,*oldbuffer, *msg;
    time_t now,before;
    FILE* pipesource;

    spid=fork();
    if(spid==0){
        /* Child process*/
        close(STDOUT_FILENO);
        close(STDIN_FILENO);
        dup2(psource[1], STDOUT_FILENO);
        close(psource[0]);
        close(psource[1]);

        if (execlp(source,"source",NULL)<0){
            perror("source: execlp: ");
            _exit(1);
        }
    }
    else if (spid>0){
        /* Parent process */
        free(source);
        
        close(psource[1]); /* These are being used by the child */

        buffer=calloc(1,sizeof(char)*keylen);
        oldbuffer=calloc(1,sizeof(char)*keylen);
        oldbuffer=strcpy(oldbuffer,"\0");
        before=0;
        time(&now);
        
        pipesource=fdopen(psource[0],"r");
        while (loop && fgets(buffer,keylen,pipesource)){
            strtok(buffer,"\n");
            
            if (verbose > 2){
                fprintf(stderr,"Raw data: %s\n",buffer);
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
            fprintf(stderr,"Source process terminated.\n");
        }
        logmessage("Source process terminated.");
    }
    else{
        perror("fork: ");
        loop=0;
    }
    return 0;
}


void *tHelper(){
    char *buffer;
    time_t now;
    FILE* pipehelper;

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

        if (execlp(helper,"helper",NULL)<0){
            perror("helper: execlp: ");
            _exit(1);
        }
    }
    else if (hpid>0){
        /* Parent process */
        free(helper);
        
        /* These are being used by the child */
        close(phelperIN[1]);
        close(phelperOUT[0]);

        buffer=calloc(1,sizeof(char)*loglen);
        time(&now);
        
        pipehelper=fdopen(phelperIN[0],"r");

        while (loop && fgets(buffer,loglen,pipehelper)){
            strtok(buffer,"\n");
            time(&now);
            if (verbose > 0){
                fprintf(stderr,"Log: %s\n",buffer);
            }
            logmessage(buffer);
        }
        fclose(pipehelper);
        free(buffer);
        /* Pipe closed by signal handler */
        if (verbose > 0){
            fprintf(stderr,"Helper process terminated.\n");
        }
        logmessage("Helper process terminated.");
    }
    else{
        perror("fork: ");
        loop=0;
    }
    return 0;
}

void signal_handler(int signum){
    if ((signum==SIGTERM) || (signum==SIGINT) || (signum==SIGQUIT)){
        kill(spid,15);
        kill(hpid,15);
        //Then we continue the cleanup in SIGCHLD
    }
    else if (signum==SIGCHLD){
        loop=0;
        kill(spid,15);
        kill(hpid,15);
        waitpid(spid,NULL,0);
        waitpid(hpid,NULL,0);
    }
    else if (signum==SIGUSR1){
        rotate();
    }
}

int main (int argc, char *argv[]){
    struct sigaction sig_h;
    
    loadConf();

    pthread_mutex_lock(&mutex);
    flog=fopen(logfile,"a");
    if (!flog){
        perror("fopen: ");
        exit(1);
    }
    pthread_mutex_unlock(&mutex);
    
    #ifdef lights
    if (light)
        pin_on(statusled);
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

    /* Crea thread autenticazione */
    pthread_create(&thr_helper,NULL,tHelper,NULL);

    /* Attendi che i threads terminino */
    pthread_join(thr_source,NULL);
    pthread_join(thr_helper,NULL);
    
    logmessage("Program terminated.");
    clean();
    #ifdef lights
    if (light)
        pin_off(statusled);
    #endif
    return EXIT_SUCCESS;
}