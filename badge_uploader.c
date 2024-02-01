#define _GNU_SOURCE
#include "common.h"
#include "badge_logger_common.h"

#define VERSION "0.3-r2"

char *datahandler, *params;
char *tmpf, *queue;

int qsize = 1500, msize;
uint16_t *start, *current;
int interval = 30;
short verbose = 0;
short loop = 1;

short drop = 0;

int fd = 0;

void loadConf(char *conffile) {
	FILE* fp;
	char *line = NULL, *def = NULL, *val = NULL;
	size_t n = 1;

	fp = fopen(conffile, "r");
	if (!fp) {
		fprintf(stderr, "File %s:\n", conffile);
		perror("Error opening configuration: ");
		exit(1);
	}

	line = calloc(n, sizeof (char));
	while (getline(&line, &n, fp) > 0) {
		sscanf(line, "%ms %m[^\n]", &def, &val);
		if (def == NULL)
			asprintf(&def, " ");
		if (val == NULL)
			asprintf(&val, " ");

		if (strcmp(def, "verbose") == 0) {
			verbose = atoi(val);
		} else if (strcmp(def, "datahandler") == 0) {
			asprintf(&datahandler, "%s", val);
		} else if (strcmp(def, "params") == 0) {
			asprintf(&params, "%s", val);
		} else if (strcmp(def, "queuefile") == 0) {
			asprintf(&tmpf, "%s", val);
		} else if (strcmp(def, "queuesize") == 0) {
			qsize = atoi(val);
		} else if (strcmp(def, "interval") == 0) {
			interval = atoi(val);
		}

		/* Free current values */
		free(def);
		free(val);
		def = val = NULL;
	}
	free(line);
	fclose(fp);

	if (datahandler == NULL) {
		fprintf(stderr, "Error: datahandler not found in configuration.\n");
		exit(1);
	}
	if (tmpf == NULL) {
		fprintf(stderr, "Error: queue file not found in configuration.\n");
		exit(1);
	}

	if (verbose > 1) {
		fprintf(stderr, "Configuration loaded.\n");
	}
}

int sendData(char param[elsize]) {
	char* command;
	int success = -1;

	asprintf(&command, "%s %s", datahandler, param);
	if (verbose > 0) {
		printf("Running save command: %s\n", command);
		fflush(stdout);
	}
	success = system(command);
	free(command);

	if (success == 0) {
		return 1;
	} else {
		printf("Error: the datahandler exited with status %d. Will retry later.\n", success);
		fflush(stdout);
		return -1;
	}
}

int formatData(char* element, char** f_element) {
	char data[elsize];
	char dt[15];
	if (sscanf(element, "%s %s", dt, data) == 2) {
		/* prepare the string as defined in configuration */
		if (asprintf(f_element, params, dt, data) > 0) {
			return 1;
		} else {
			return -1;
		}
	} else {
		return -1;
	}
}

int emptyQueue() {
	char *element, *f_element;
	int i = 0, total, queued;

	total = abs(*current - *start);
	queued = total;
	if (total > 0) {
		element = malloc(sizeof (char)*elsize);
		while (pickData(&element) > 0) {
			if (verbose) {
				printf("Got element %s from queue\n", element);
				fflush(stdout);
			}
			if (formatData(element, &f_element) < 1) {
				/* Stop on first error and retry in the future */
				printf("Cannot format data.\n");
				break;
			}
			if (verbose) {
				printf("Formatted data: %s \n", f_element);
				fflush(stdout);
			}

			if (drop) {
				popData(NULL);
				loop = 0;
				return 1;
			}

			if (sendData(f_element) < 1) {
				/* Stop on first error and retry in the future */
				printf("%d elements sent before server vanished.\n", i);
				fflush(stdout);
				break;
			} else {
				queued = popData(NULL);
			}
			i++;
		}
		free(element);
		free(f_element);
	}

	printf("Queued elements: %d/%d elements sent.\n", total / elsize, i);
	fflush(stdout);
	return queued;
}

void signal_handler(int signum) {
	if ((signum == SIGTERM) || (signum == SIGINT) || (signum == SIGQUIT)) {
		printf("Caught signal %d, shutting down...\n", signum);
		loop = 0;
	} else if (signum == SIGUSR1) {
		printf("Caught signal %d, sending elements to server...\n", signum);
	}
	fflush(stdout);
}

int main(int argc, char *argv[]) {
	struct sigaction sig_h;
	int c, pages;
	char *conffile = NULL;

	short new = 0;

	/* Load settings from commandline */
	while ((c = getopt(argc, argv, "f:Dh")) != -1) {
		switch (c) {
			case 'f':
				if (asprintf(&conffile, "%s", optarg) < 0) {
					perror("asprintf");
					exit(1);
				}
				break;
			case 'D':
				drop = 1;
			case 'h':
				printf("Usage: badge_uploader [ -f configuration file ] [ -h ]\n"
						"\n"
						"-f FILE\t\tLoad configuration from FILE\n"
						"-D\t\tDrops the first element in queue and exits\n"
						"-h\t\tShow this message\n\n"
						);
				exit(1);
		}
	}
	if (!conffile) {
		fprintf(stderr, "Configuration file missing.\n");
		exit(1);
	}

	loadConf(conffile);
	free(conffile);

	fd = open(tmpf, O_RDWR);
	if (fd < 0 && errno == ENOENT) {
		/* First run: create file and set flag */
		fd = open(tmpf, O_RDWR | O_CREAT, S_IRWXU);
		new = 1;
	}
	if (fd < 0) {
		perror("fopen");
		exit(1);
	}

	/* Mmapped memory must be aligned to page size */
	pages = ((sizeof (char) * (qsize + 1) * elsize) + (sysconf(_SC_PAGE_SIZE) - 1)) / sysconf(_SC_PAGE_SIZE);
	msize = pages * sysconf(_SC_PAGE_SIZE);

	/* Enlarge the file to the defined size */
	if (ftruncate(fd, msize) < 0) {
		perror("ftruncate");
		exit(1);
	}
	/* Allocate the "shared memory" */
	queue = (char*) mmap(NULL, msize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if (queue == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}

	start = (uint16_t*) (queue + ((qsize) * elsize * sizeof (char)));
	current = (uint16_t*) (queue + ((qsize) * elsize * sizeof (char))) + sizeof (uint16_t);

	if (new > 0) {
		*start = 0;
		*current = 0;
		msync(queue, msize, MS_SYNC);
	}

	/* Catch exit signals */
	sig_h.sa_handler = signal_handler;
	sig_h.sa_flags = 0;
	/* Signals blocked during the execution of the handler. */
	sigemptyset(&sig_h.sa_mask);
	sigaddset(&sig_h.sa_mask, SIGINT);
	sigaddset(&sig_h.sa_mask, SIGTERM);
	sigaddset(&sig_h.sa_mask, SIGQUIT);
	sigaddset(&sig_h.sa_mask, SIGUSR1);

	sigaction(SIGQUIT, &sig_h, NULL);
	sigaction(SIGINT, &sig_h, NULL);
	sigaction(SIGTERM, &sig_h, NULL);
	sigaction(SIGUSR1, &sig_h, NULL);

	if (verbose) {
		printf("Queued elements: %d\n", abs(*current - *start) / elsize);
	}

	printf("Ready to accept data.\n");
	fflush(stdout);
	while (loop) {
		emptyQueue();
		if (verbose) {
			fprintf(stderr, "%d %d\n", *start, *current);
		}
		if (!drop) {
			sleep(interval);
		}
	}

	free(tmpf);
	free(datahandler);
	free(params);
	exit(0);
}
