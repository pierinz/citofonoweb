#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <signal.h>

#include <linux/input.h>
#define EV_PRESSED 1
#define EV_RELEASED 0
#define EV_REPEAT 2

#ifndef CONFPATH
    #define CONFPATH "conf/hid_read.conf"
#endif

short verbose=0;
char* devname=NULL;
int max_retry=3;
int timeout=10;

short loop=1;
int retry=-1;

/* 0 = scancode, 1 = keycode */
int mode=0;
int outmode=0;

/* KBDUS means US Keyboard Layout. This is a scancode table
*  used to layout a standard US keyboard. I have left some
*  comments in to give you an idea of what key is what, even
*  though I set it's array index to 0. You can change that to
*  whatever you want using a macro, if you wish! */
unsigned char kbdus[128] =
{
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
 '\'', '`',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', ',', '.', '/',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};

/* Clean */
void clean(){
    free(devname);
}

void loadConf(){
    FILE* fp;
    char line[255],def[55],val[200];

    fp=fopen(CONFPATH,"r");
    if (!fp){
        fprintf(stderr,"File %s:\n",CONFPATH);
        perror("Error opening configuration: ");
        exit(1);
    }
    while(fgets(line,255,fp)){
        sscanf(line,"%s%s",def,val);
        if (strcmp(def,"device")==0){
            /* "devname" must be large enough to contain "val" */
            devname=malloc(sizeof(val));
            strcpy(devname,val);
        }
        if (strcmp(def,"verbose")==0)
            verbose=atoi(val);
        if (strcmp(def,"max_retry")==0)
            max_retry=atoi(val);
        if (strcmp(def,"timeout")==0)
            timeout=atoi(val);
        if (strcmp(def,"mode")==0){
            if (strcmp(val,"scancode")==0)
                mode=0;
            else if (strcmp(val,"keycode")==0)
                mode=1;
            else
                mode=2;
        }
        if (strcmp(def,"outmode")==0){
            if (strcmp(val,"line")==0)
                outmode=0;
            else
                outmode=1;
        }
    }
    fclose(fp);
    
    if (verbose > 1){
        fprintf(stderr,"Configuration loaded.\n");
    }
}

int prepareDev(char* devname){
    int fd = sizeof(struct input_event);
    char name[256] = "Unknown";
    
    /* Open Device */
    if ((fd = open (devname, O_RDONLY)) == -1){
        fprintf(stderr,"%s is not a valid device.\n", devname);
        perror("error: ");
        if (retry > -1 && retry < max_retry){
        /* If the device disappeared, try to recover */
            retry++;
            fprintf(stderr,"Trying to recover...\n");
            usleep(timeout);
            return prepareDev(devname);
        }
        clean();
        exit(1);
    }

    if (verbose > 0){
        /* Print Device Name */
        ioctl (fd, EVIOCGNAME (sizeof (name)), name);
        fprintf (stderr,"Reading From : %s (%s)\n", devname, name);
    }

    /* Uso esclusivo */
    ioctl (fd, EVIOCGRAB, 1);
    
    if (retry > -1){
        /* The device is now usable */
        retry=0;
        fprintf(stderr,"Crash counter set to 0.\n");
    }
    
    return fd;
}

void signal_handler(int signum){
    if ((signum==SIGTERM) || (signum==SIGINT) || (signum==SIGQUIT)){
        loop=0;
    }
}

int main (int argc, char *argv[]){
    int fd;
    int yalv;
    /* how many bytes were read */
    size_t rb;
    /* the events (up to 64 at once) */
    struct input_event ev[64];
    struct sigaction sig_h;
    /* Entire line */
    char* key;
    /* Buffer */
    char buffer[2];
    
    /* Cattura segnali di uscita */
    sig_h.sa_handler=signal_handler;
    sig_h.sa_flags=0;
    /* Signals blocked during the execution of the handler. */
    sigemptyset(&sig_h.sa_mask);
    sigaddset(&sig_h.sa_mask, SIGINT);
    sigaddset(&sig_h.sa_mask, SIGTERM);
    sigaddset(&sig_h.sa_mask, SIGQUIT);
    
    sigaction(SIGQUIT,&sig_h,NULL);
    sigaction(SIGINT,&sig_h,NULL);
    sigaction(SIGTERM,&sig_h,NULL);
    
    /* Load settings from file */
    loadConf();
        
    /* Setup check */
    if (devname == NULL){
        if (argv[1] == NULL){
            fprintf(stderr,"Please specify the path to the dev event interface device\n");
            clean();
            exit (1);
        }
	devname = argv[1];
    }
    
    /* Open device and grab it */
    fd=prepareDev(devname);

    
    /* Initialize key */
    key=calloc(1,sizeof(char));
    
    while (loop){
        /* Read events from device */
        rb=read(fd,ev,sizeof(struct input_event)*64);

        /* Check for errors */
        if (rb == -1){
            if (errno==EINTR){
                break;
            }
            else{
                retry++;
                perror("read error: ");
                usleep(timeout);
                fd=prepareDev(devname);
                continue;
            }
        }
        else if (rb < (int) sizeof(struct input_event)){
            retry++;
            perror("evtest: short read");
            usleep(timeout);
            fd=prepareDev(devname);
            continue;
        }
        
        /* Parse events */
        for (yalv = 0; yalv < (int) (rb / sizeof(struct input_event)); yalv++){
            if (EV_KEY == ev[yalv].type){
                /* Stampa solo al keyup */
                if (ev[yalv].value==EV_RELEASED){
                    if (outmode == 0){
                        if (kbdus[ev[yalv].code] == '\n'){
                            printf("%s\n",key);
                            fflush(stdout);
                            /* Clean old key */
                            free(key);
                            key=calloc(1,sizeof(char));
                        }
                        else{
                            key=realloc(key,(strlen(key)+2)*sizeof(char));
                            buffer[0]=kbdus[ev[yalv].code];
                            buffer[1]='\0';
                            strcat(key,buffer);
                        }
                    }
                    else{
                        if (mode==0)
                            printf("%3d\n", ev[yalv].code);
                        else if (mode==1)
                            printf("%3d\n", kbdus[ev[yalv].code]);
                        else
                            printf("%3c\n", kbdus[ev[yalv].code]);
                        fflush(stdout);

                        if (verbose > 1){
                            fprintf(stderr,"%d | %c | %d\n", ev[yalv].code,kbdus[ev[yalv].code],kbdus[ev[yalv].code]);
                        }
                    }
                }
            }
        }
    }
    free(key);
    close(fd);
    clean();
    return 0;
}
