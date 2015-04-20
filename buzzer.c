#define _GNU_SOURCE
#include <string.h>
#include <time.h>
#include <sys/resource.h>
#include "gpio.h"

int main(int argc, char *argv[]){
	int c, g=0, verbose=0;
	int freq=0;
	float duration=2;
	time_t starttime;
	float p=1000000/400;

	/* Load settings from commandline */
    while ((c = getopt (argc, argv, "g:f:d:v")) != -1){
        switch (c){
            case 'g':
				g=atoi(optarg);
			break;

			case 'f':
				freq=atoi(optarg);
				p=1000000/freq;
			break;

			case 'd':
				duration=atof(optarg);
			break;

			case 'v':
				verbose++;
			break;

            case 'h':
				printf("Usage: buzzer -g pin [-f freq] -d duration [ -h ]\n"
					"Beep on single pin\n");
				if (geteuid() != 0){
					printf("This program should be run as root to raise self-priority.\n"
						"You can run it as user, but sometimes you could encounter race conditions with udev.\n"
					);
				}
				printf("\n"
					"-g pin\t\tSelect output pin"
					"-f freq\t\tThe sound frequency. If omitted, the pin is turned on for 'duration' seconds."
					"-d duration\t\tBeep for 'duration' seconds"
                    "-h\t\tShow this message\n\n"
                );
                exit(1);
        }
    }

	if (! GPIOValid(g)){
		fprintf(stderr,"Invalid gpio number\n");
		goto clean;
	}

	//Test if initialization is succesful
	c=0;

	if (verbose){
		printf("exporting pin...\n");
	}
	c+=GPIOExport(g);
	if (verbose){
		printf("setting direction...\n");
	}
	c+=GPIODirection(g, OUT);
	if (verbose){
		printf("setting pin to LOW...\n");
	}
	c+=GPIOWrite(g, LOW);

	if ( c < 0 ){
		exit(1);
	}
	if (verbose){
		printf("Ready\n");
	}

	if (setpriority(PRIO_PROCESS, 0, -20) < 0){
		if (verbose){
			perror("setpriority: ");
			fprintf(stderr,"Failed to raise self priority. This is not a fatal error.\n");
		}
	}

	starttime=time(NULL);

	if (freq > 0){
		while (time(NULL) < starttime + duration){
			GPIOWrite(g, HIGH);
			usleep(p);
			GPIOWrite(g, LOW);
			if (verbose){
				printf("%li\n\n", time(NULL));
			}
		}
	}
	else{
		GPIOWrite(g, HIGH);
		usleep(duration*1000000);
		GPIOWrite(g, LOW);
	}

	clean:

	if (verbose){
		printf("unexporting pin...\n");
	}
	GPIOUnexport(g);
	exit(0);
}
