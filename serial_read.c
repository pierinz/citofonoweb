/*
 * Usage: serial_read [options] device
 * read data from serial devices.
    -s X            Serial speed (1200, 2400, 4800, 9600, 19200, 38400, 57600 or 115200)
    -p X            Parity (8N1, 7E1, 701)
    -o X            Output (char|line)
    -v              Be verbose
    -h              Show this message
*/

#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

int verbose=0;
char* devname;

int set_interface_attribs (int fd, int speed, int parity){
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0){
        printf ("error %d from tcgetattr\n", errno);
        return -1;
    }

    cfsetospeed (&tty, speed);
    cfsetispeed (&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    tty.c_iflag &= ~IGNBRK;         // ignore break signal
    tty.c_lflag = 0;                // no signaling chars, no echo,
                                    // no canonical processing
    tty.c_oflag = 0;                // no remapping, no delays
    tty.c_cc[VMIN]  = 0;            // read doesn't block
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                    // enable reading
    tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr (fd, TCSANOW, &tty) != 0){
        printf ("error %d from tcsetattr\n", errno);
        return -1;
    }
    return 0;
}

void set_blocking (int fd, int should_block){
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0){
        printf ("error %d from tggetattr\n", errno);
        return;
    }

    tty.c_cc[VMIN]  = should_block ? 1 : 0;
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    if (tcsetattr (fd, TCSANOW, &tty) != 0)
        printf ("error %d setting term attributes\n", errno);
}

int main(int argc, char* argv[]){
    FILE* port;
    int c;
    int speed, parity=0;

    /* Load settings from commandline */        
    while ((c = getopt (argc, argv, "s:p:vh")) != -1){
        switch (c){
            case 's':
                switch (atoi(optarg)){
                    case 1200:
                        speed=B1200;
                    break;
                    case 2400:
                        speed=B2400;
                    break;
                    case 4800:
                        speed=B4800;
                    break;
                    case 9600:
                        speed=B9600;
                    break;
                    case 19200:
                        speed=B19200;
                    break;
                    case 38400:
                        speed=B38400;
                    break;
                    case 57600:
                        speed=B57600;
                    break;
                    case 115200:
                        speed=B115200;
                    break;
                    default:
                        fprintf(stderr,"Speed not supported\n");
                        exit(1);
                    break;
                }
            break;
            case 'p':
                if (strcmp(optarg,"8N1")==0)
                    parity=0;
                if (strcmp(optarg,"7E1")==0)
                    parity|=PARENB;
                if (strcmp(optarg,"701")==0){
                    parity |= PARENB;
                    parity |= PARODD;
                }
            break;
            case 'v':
                verbose++;
                break;
            case 'h':
                printf("Usage: serial_read [options] device\n"
                    "read data from serial devices.\n\n"
                    "-s X\t\tSerial speed (1200, 2400, 4800, 9600, 19200, 38400, 57600 or 115200)\n"
                    "-p X\t\tParity (8N1, 7E1, 701)\n"
                    "-o X\t\tOutput (char|line)\n"
                    "-v\t\tBe verbose\n"
                    "-h\t\tShow this message\n\n"
                );
                exit (1);
        }
    }
    /* The next argument is the device name */
    devname=argv[optind];
    
    /* Setup check */
    if (devname == NULL){
        fprintf(stderr,"Please specify the path to the serial device\n");
        exit(1);
    }
    
    int fd = open (devname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0){
        fprintf (stderr, "error %d opening %s: %s\n", errno, devname, strerror (errno));
        exit(1);
    }

    if (verbose){
        fprintf(stderr,"Device %s opened.\n",devname);
    }
    set_interface_attribs (fd, speed, parity);
    set_blocking (fd, 0);   // set no blocking

    if (verbose){
        fprintf(stderr,"Set attribs: s: %d p: %d\n",speed,parity);
    }
    port=fdopen(fd,"r");
    char buf [100];
    if (verbose){
        fprintf(stderr,"Waiting data...\n");
    }
    while(1){
        fgets(buf, 100, port);  // read up to 100 characters if ready to read
        if (strlen(buf)>1){
            printf("%s", buf);
            memset(buf,'\0',100);
        }
    }
    return 0;
}