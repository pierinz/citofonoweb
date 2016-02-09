/* 
 * File:   gpio.h
 * Author: pierino
 *
 * Created on 17 marzo 2015, 14.15
 */

#ifndef GPIO_H
#define	GPIO_H

/*#define _BOARD RASPBERRY or BEAGLEBONE*/
#define RASPBERRY 1
#define BEAGLEBONE 2

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

#ifndef _BOARD
#define _BOARD 0
#endif

#if _BOARD == RASPBERRY
	const static int gpios[] = {5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28};
#elif BOARD == BEAGLEBONE
	const static int gpios[] = {2, 3, 4, 5, 7, 8, 9, 10, 11, 12, 13, 14, 15, 20, 22, 23, 26, 27, 30, 31, 32, 33, 34, 35,
		36, 37, 38, 39, 44, 45, 46, 47, 48, 49, 50, 51, 60, 61, 62, 63, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76,
		77, 78, 79, 80, 81, 86, 87, 88, 89, 110, 111, 112, 113, 114, 115, 116, 117};
#else
#pragma message "board non specified!"
#endif

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
