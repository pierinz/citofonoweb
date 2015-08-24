/* 
 * File:   gpio.h
 * Author: pierino
 *
 * Created on 17 marzo 2015, 14.15
 */

#ifndef GPIO_H
#define	GPIO_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define IN  0
#define OUT 1

#define LOW  0
#define HIGH 1

#define BUFFER_MAX 3
#define DIRECTION_MAX 35
#define VALUE_MAX 30

	const static int gpios[] = {5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28};

	int GPIOValid(int pin);

	int GPIOExport(int pin);

	int GPIOUnexport(int pin);

	int GPIODirection(int pin, int dir);

	int GPIORead(int pin);

	int GPIOWrite(int pin, int value);

#ifdef	__cplusplus
}
#endif

#endif	/* GPIO_H */
