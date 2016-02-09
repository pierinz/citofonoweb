#include "gpio.h"

int GPIOValid(int pin) {
#ifdef BOARD
	int i;

	for (i = 0; i < 23; i++) {
		if (gpios[i] == pin) {
			return 1;
		}
	}
	return ;
#else
	return 1;
#endif
}

int GPIOExport(int pin) {
	char buffer[DIRECTION_MAX];
	ssize_t bytes_written;
	int fd;

	snprintf(buffer, DIRECTION_MAX, "/sys/class/gpio/gpio%d/", pin);
	if (access(buffer, X_OK) != -1) {
		/* gpio already exported */
		return (0);
	}

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open export for writing!\n");
		return (-1);
	}

	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	if (-1 == write(fd, buffer, bytes_written)) {
		fprintf(stderr, "Failed to export gpio!\n");
		return (-1);
	}
	close(fd);

	// Wait udev-related things
	usleep(200000);
	return (0);
}

int GPIOUnexport(int pin) {
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;

	fd = open("/sys/class/gpio/unexport", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open unexport for writing!\n");
		return (-1);
	}

	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return (0);
}

int GPIODirection(int pin, int dir) {
	static const char s_directions_str[] = "in\0out";

	char path[DIRECTION_MAX];
	int fd;

	snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio direction %s for writing!\n", path);
		return (-1);
	}

	if (-1 == write(fd, &s_directions_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3)) {
		fprintf(stderr, "Failed to set direction %s on %s!\n", &s_directions_str[IN == dir ? 0 : 3], path);
		return (-1);
	}

	close(fd);
	return (0);
}

int GPIORead(int pin) {
	char path[VALUE_MAX];
	char value_str[3];
	int fd;

	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_RDONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio value for reading!\n");
		return (-1);
	}

	if (-1 == read(fd, value_str, 3)) {
		fprintf(stderr, "Failed to read value!\n");
		return (-1);
	}

	close(fd);

	return (atoi(value_str));
}

int GPIOWrite(int pin, int value) {
	static const char s_values_str[] = "01";

	char path[VALUE_MAX];
	int fd;

	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio value %s for writing!\n", path);
		perror("open");
		return (-1);
	}

	if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1)) {
		fprintf(stderr, "Failed to write value on %s!\n", path);
		perror("write");
		return (-1);
	}

	close(fd);
	return (0);
}