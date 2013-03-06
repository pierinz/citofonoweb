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

void handler (int sig)
{
	fprintf (stderr,"exiting...(%d)\n", sig);
	exit (0);
}
 
void perror_exit (char *error)
{
	perror (error);
	handler (9);
}
 
int main (int argc, char *argv[])
{
	setbuf(stdout, NULL);
	//struct input_event event;
	int fd = sizeof (struct input_event);
	//int rd, value, size = sizeof (struct input_event);
	char name[256] = "Unknown";
	char *device = NULL;

	//Setup check
	if (argv[1] == NULL){
		fprintf(stderr,"Please specify (on the command line) the path to the dev event interface device\n");
		exit (1);
    }

	if (argc > 1)
		device = argv[1];

	//Open Device
	if ((fd = open (device, O_RDONLY)) == -1){
		fprintf(stderr,"%s is not a vaild device.\n", device);
                perror("error: ");
                exit(1);
        }

	//Print Device Name
	ioctl (fd, EVIOCGNAME (sizeof (name)), name);
	fprintf (stderr,"Reading From : %s (%s)\n", device, name);

	//Uso esclusivo
	ioctl (fd, EVIOCGRAB, 1);

	while (1){
		/* how many bytes were read */
		size_t rb;
		/* the events (up to 64 at once) */
		struct input_event ev[64];

		rb=read(fd,ev,sizeof(struct input_event)*64);

                if (rb == -1){
                    	perror("read error: ");
			exit (1);
                }
		else if (rb < (int) sizeof(struct input_event)) {
			perror("evtest: short read");
			exit (1);
		}

		int yalv;
		for (yalv = 0; yalv < (int) (rb / sizeof(struct input_event)); yalv++)
		{
			if (EV_KEY == ev[yalv].type){
				//printf("%ld.%06ld ", ev[yalv].time.tv_sec, ev[yalv].time.tv_usec);
				//printf("type %d code %d value %d\n", ev[yalv].type, ev[yalv].code, ev[yalv].value);
				
				//Stampa solo al keydown
				if (ev[yalv].value==0)
					printf("%d\n", ev[yalv].code);
			}
		}
	}
	close(fd);
	return 0;
} 