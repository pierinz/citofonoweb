/*
 * Usage: hid_read [options] device
 * read data from HID devices.
    -m X            Device mode (scancode|keycode)
    -o X            Output (char|line)
	-s X			Skip first X chars
	-l X			Print only first X chars (can be used with -s)
    -v              Be verbose
    -vv             Be more verbose
    -d				Show debug messages
    -t X            If the device disappears, wait X µs before retry
    -r              If the device disappears, retry X times before crashing
    -h              Show this message
*/

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

short verbose=0;
short debug=0;
char* devname;
int max_retry=3;
int timeout=10;

short loop=1;
int retry=-1;

/* 0 = scancode, 1 = keycode */
int mode=0;
/* 0 = line, 1 = char */
int outmode=0;

/* Start from the beginning */
int start=0;
/* Max length */
int length=0;

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
	0,
	0,
	0,
	0,
	0,
	0,
	'\n'	/* Enter key */
};

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

int main (int argc, char *argv[]){
    int fd;
    int yalv;
    /* how many bytes were read */
    size_t rb;
    /* the events (up to 64 at once) */
    struct input_event ev[64];
    /* Entire line */
    char* key;
    /* Buffer */
    char buffer[2];
    int c;
    
    /* Load settings from commandline */
    while ((c = getopt (argc, argv, "r:t:m:o:s:l:vdh")) != -1){
        switch (c){
            case 'r':
                max_retry = atoi(optarg);
                break;
            case 't':
                timeout = atoi(optarg);
                break;
            case 'm':
                if (strcmp(optarg,"scancode")==0)
                    mode=0;
                else if (strcmp(optarg,"keycode")==0)
                    mode=1;
                else
                    mode=2;
                break;
            case 'o':
                if (strcmp(optarg,"line")==0)
                    outmode=0;
                else
                    outmode=1;
                break;
			case 's':
                start = atoi(optarg);
                break;
			case 'l':
                length = atoi(optarg);
                break;
            case 'v':
                verbose++;
                break;
			case 'd':
                debug++;
                break;
            case 'h':
                printf("Usage: hid_read [options] device\n"
                    "read data from HID devices.\n\n"
                    "-m X\t\tDevice mode (scancode|keycode)\n"
                    "-o X\t\tOutput (char|line)\n"
					"-s X\t\tSkip first X chars\n"
					"-l X\t\tPrint only first X chars (can be used with -s)\n"
                    "-v\t\tBe verbose\n"
                    "-vv\t\tBe more verbose\n"
					"-d\t\tShow debug messages\n"
                    "-t X\t\tIf the device disappears, wait X µs before retry\n"
                    "-r\t\tIf the device disappears, retry X times before crashing\n"
                    "-h\t\tShow this message\n\n"
                );
                exit (1);
        }
    }
    /* The next argument is the device name */
    devname=argv[optind];
    
    /* Setup check */
    if (devname == NULL){
        fprintf(stderr,"Please specify the path to the dev event interface device\n");
        exit(1);
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
					if (debug > 0){
						fprintf(stderr,"debug: key released: %d | %c | %d\n", ev[yalv].code,kbdus[ev[yalv].code],kbdus[ev[yalv].code]);
					}
                    if (outmode == 0){
                        if (kbdus[ev[yalv].code] == '\n'){
							if (length == 0){
								printf("%s\n",key+start);
							}
                            else{
								printf("%.*s\n",length,key+start);
							}
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
				else if (debug > 1){
					fprintf(stderr,"debug (not ev_released: %d): %d | %c | %d\n", ev[yalv].value, ev[yalv].code,kbdus[ev[yalv].code],kbdus[ev[yalv].code]);
				}
            }
			else if (debug > 1){
				fprintf(stderr,"debug (not a key: %d): %d | %c | %d\n", ev[yalv].type, ev[yalv].code, kbdus[ev[yalv].code],kbdus[ev[yalv].code]);
			}
        }
    }
    free(key);
    close(fd);
    return 0;
}
