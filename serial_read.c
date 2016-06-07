/*
 * Usage: serial_read [options] device
 * read data from serial devices.
	-s X            Serial speed (1200, 2400, 4800, 9600, 19200, 38400, 57600 or 115200)
	-p X            Parity (8N1, 7E1, 701)
	-o X            Output (char|line)
	-v              Be verbose
	-h              Show this message
 */
// TODO: use getline instead of fgets

#define _GNU_SOURCE
#include "common.h"
#include <termios.h>
#include <fcntl.h>
#include <stdint.h>

char* devname;
uint8_t loop = 1, verbose = 0;

void signal_handler(int signum) {
	if ((signum == SIGTERM) || (signum == SIGINT) || (signum == SIGQUIT)) {
		loop = 0;
	}
}

int set_interface_attribs(int fd, int speed, int parity) {
	struct termios tty;
	memset(&tty, 0, sizeof tty);
	if (tcgetattr(fd, &tty) != 0) {
		printf("error %d from tcgetattr\n", errno);
		return -1;
	}

	cfsetospeed(&tty, speed);
	cfsetispeed(&tty, speed);

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; /* 8-bit chars */
	/* disable IGNBRK for mismatched speed tests; otherwise receive break
	 * as \000 chars */
	tty.c_iflag &= ~IGNBRK; /* ignore break signal */
	tty.c_lflag = 0; /* no signaling chars, no echo, */
	/* no canonical processing */
	tty.c_oflag = 0; /* no remapping, no delays */
	tty.c_cc[VMIN] = 0; /* read doesn't block */
	tty.c_cc[VTIME] = 5; /* 0.5 seconds read timeout */

	tty.c_iflag &= ~(IXON | IXOFF | IXANY); /* shut off xon/xoff ctrl */

	tty.c_cflag |= (CLOCAL | CREAD); /* ignore modem controls, */
	/* enable reading  */
	tty.c_cflag &= ~(PARENB | PARODD); /* shut off parity */
	tty.c_cflag |= parity;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;

	if (tcsetattr(fd, TCSANOW, &tty) != 0) {
		printf("error %d from tcsetattr\n", errno);
		return -1;
	}
	return 0;
}

void set_blocking(int fd, int should_block) {
	struct termios tty;
	memset(&tty, 0, sizeof tty);
	if (tcgetattr(fd, &tty) != 0) {
		printf("error %d from tggetattr\n", errno);
		return;
	}

	tty.c_cc[VMIN] = should_block ? 1 : 0;
	tty.c_cc[VTIME] = 5; /* 0.5 seconds read timeout */

	if (tcsetattr(fd, TCSANOW, &tty) != 0)
		printf("error %d setting term attributes\n", errno);
}

int main(int argc, char* argv[]) {
	FILE* port;
	int c, fd, i;
	int speed, parity = 0;
	char buf [100];
	uint8_t outmode = 0;
	struct sigaction sig_h;

	/* Load settings from commandline */
	while ((c = getopt(argc, argv, "s:p:o:vh")) != -1) {
		switch (c) {
			case 's':
				switch (atoi(optarg)) {
					case 1200:
						speed = B1200;
						break;
					case 2400:
						speed = B2400;
						break;
					case 4800:
						speed = B4800;
						break;
					case 9600:
						speed = B9600;
						break;
					case 19200:
						speed = B19200;
						break;
					case 38400:
						speed = B38400;
						break;
					case 57600:
						speed = B57600;
						break;
					case 115200:
						speed = B115200;
						break;
					default:
						fprintf(stderr, "Speed not supported\n");
						exit(1);
						break;
				}
				break;
			case 'p':
				if (strcmp(optarg, "8N1") == 0)
					parity = 0;
				if (strcmp(optarg, "7E1") == 0)
					parity |= PARENB;
				if (strcmp(optarg, "701") == 0) {
					parity |= PARENB;
					parity |= PARODD;
				}
				break;
			case 'o':
				if (strcmp(optarg, "line") == 0)
					outmode = 0;
				else
					outmode = 1;
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
				exit(1);
		}
	}
	/* The next argument is the device name */
	devname = argv[optind];

	/* Setup check */
	if (devname == NULL) {
		fprintf(stderr, "Please specify the path to the serial device\n");
		exit(1);
	}

	fd = open(devname, O_RDWR | O_NOCTTY | O_SYNC);
	if (fd < 0) {
		fprintf(stderr, "error %d opening %s: %s\n", errno, devname, strerror(errno));
		exit(1);
	}

	if (verbose) {
		fprintf(stderr, "Device %s opened.\n", devname);
	}

	/* Cattura segnali di uscita */
	sig_h.sa_handler = signal_handler;
	sig_h.sa_flags = 0;
	/* Signals blocked during the execution of the handler. */
	sigemptyset(&sig_h.sa_mask);
	sigaddset(&sig_h.sa_mask, SIGINT);
	sigaddset(&sig_h.sa_mask, SIGTERM);
	sigaddset(&sig_h.sa_mask, SIGQUIT);

	sigaction(SIGQUIT, &sig_h, NULL);
	sigaction(SIGINT, &sig_h, NULL);
	sigaction(SIGTERM, &sig_h, NULL);

	set_interface_attribs(fd, speed, parity);
	set_blocking(fd, 0); /* set no blocking */

	if (verbose) {
		fprintf(stderr, "Set attribs: s: %d p: %d\n", speed, parity);
	}
	port = fdopen(fd, "r");
	if (verbose) {
		fprintf(stderr, "Waiting data...\n");
	}
	while (loop) {
		if (fgets(buf, 100, port) != NULL) { /* read up to 100 characters if ready to read */
			strtok(buf, "\r");
			strtok(buf, "\n");
			if (strlen(buf) > 1) {
				if (outmode == 0) {
					printf("%s\n", buf);
				} else {
					printf("-- BEGIN --\n");
					for (i = 0; i < strlen(buf); i++) {
						printf("-> %i %c\n", buf[i], buf[i]);
					}
					printf("-- END --\n");
				}
				memset(buf, '\0', 100);
				fflush(stdout);
			}
		} else {
			if (ferror(port) && errno != EINTR) {
				fprintf(stderr, "Read error: ");
				perror("fgets: ");
			}
		}
	}
	fclose(port);
	return 0;
}
