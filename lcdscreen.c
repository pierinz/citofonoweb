#define _GNU_SOURCE
#include "gpio.h"
#include "f_lock.h"
#include <string.h>

/*
# HD44780 20x4 LCD
# Raspberry Pi
#
# Made from the Matt Hawkins' python script (http://www.raspberrypi-spy.co.uk/)
#
# The wiring for the LCD is as follows:
# 1 : GND
# 2 : 5V
# 3 : Contrast (0-5V)*
# 4 : RS (Register Select)
# 5 : R/W (Read Write)       - GROUND THIS PIN
# 6 : Enable or Strobe
# 7 : Data Bit 0             - NOT USED
# 8 : Data Bit 1             - NOT USED
# 9 : Data Bit 2             - NOT USED
# 10: Data Bit 3             - NOT USED
# 11: Data Bit 4
# 12: Data Bit 5
# 13: Data Bit 6
# 14: Data Bit 7
# 15: LCD Backlight +5V**
# 16: LCD Backlight GND
*/

/* Define lockfile */
#define LOCKFILE "/tmp/lcdscreen.lock"

// Define GPIO to LCD mapping
#define LCD_RS 7
#define LCD_E  8
#define LCD_D4 25
#define LCD_D5 24
#define LCD_D6 23
#define LCD_D7 18
#define LED_ON 15

// Define some device constants
#define LCD_WIDTH 20    // Maximum characters per line
#define LCD_CHR 1
#define LCD_CMD 0

#define LCD_LINE_1 0x80 // LCD RAM address for the 1st line
#define LCD_LINE_2 0xC0 // LCD RAM address for the 2nd line
#define LCD_LINE_3 0x94 // LCD RAM address for the 3rd line
#define LCD_LINE_4 0xD4 // LCD RAM address for the 4th line

// Timing constants (µs)
#define E_PULSE 80
#define E_DELAY 80

void lcd_byte(int bits, int mode){
	// Send byte to data pins
	// bits = data
	// mode = 1  for character
	//        0 for command

	GPIOWrite(LCD_RS, mode); // RS

	// High bits
	GPIOWrite(LCD_D4, 0);
	GPIOWrite(LCD_D5, 0);
	GPIOWrite(LCD_D6, 0);
	GPIOWrite(LCD_D7, 0);

	if ((bits&0x10) == 0x10)
		GPIOWrite(LCD_D4, 1);
	if ((bits&0x20) == 0x20)
		GPIOWrite(LCD_D5, 1);
	if ((bits&0x40) == 0x40)
		GPIOWrite(LCD_D6, 1);
	if ((bits&0x80) == 0x80)
		GPIOWrite(LCD_D7, 1);

	// Toggle 'Enable' pin
	usleep(E_DELAY);
	GPIOWrite(LCD_E, 1);
	usleep(E_PULSE);
	GPIOWrite(LCD_E, 0);
	usleep(E_DELAY);

	// Low bits
	GPIOWrite(LCD_D4, 0);
	GPIOWrite(LCD_D5, 0);
	GPIOWrite(LCD_D6, 0);
	GPIOWrite(LCD_D7, 0);

	if ((bits&0x01) == 0x01)
		GPIOWrite(LCD_D4, 1);
	if ((bits&0x02) == 0x02)
		GPIOWrite(LCD_D5, 1);
	if ((bits&0x04) == 0x04)
		GPIOWrite(LCD_D6, 1);
	if ((bits&0x08) == 0x08)
		GPIOWrite(LCD_D7, 1);

	// Toggle 'Enable' pin
	usleep(E_DELAY);
	GPIOWrite(LCD_E, 1);
	usleep(E_PULSE);
	GPIOWrite(LCD_E, 0);
	usleep(E_DELAY);
}

void lcd_init(){
	// Initialise display
	lcd_byte(0x33,LCD_CMD);
	lcd_byte(0x32,LCD_CMD);
	lcd_byte(0x28,LCD_CMD);
	lcd_byte(0x0C,LCD_CMD);
	lcd_byte(0x06,LCD_CMD);
	lcd_byte(0x01,LCD_CMD);
}

void lcd_string(char *message){
	int len=0, i=0;
	
	len=strlen(message);
	
	for (i=0; i<LCD_WIDTH; i++){
		if (i < len-1){ /* Strip EOL */
			lcd_byte(message[i],LCD_CHR);
		}
		else{
			lcd_byte(' ',LCD_CHR);
		}
	}
}

void lcd_empty(){
	// Blank display
	lcd_byte(LCD_LINE_1, LCD_CMD);
	lcd_string(" ");
	lcd_byte(LCD_LINE_2, LCD_CMD);
	lcd_string(" ");
	lcd_byte(LCD_LINE_3, LCD_CMD);
	lcd_string(" ");
	lcd_byte(LCD_LINE_4, LCD_CMD);
	lcd_string(" ");
}

int main(int argc, char *argv[]){
	char *line;
	char *lines[4];
	int i=0, z=0, n=0;
	size_t bytes=40;
	int fd;

	fd=open(LOCKFILE, O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if (fd < 0){
		fd=creat(LOCKFILE, O_RDWR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
		if (fd < 0){
			perror("creat");
		}
	}
	f_elock(fd);

	line=(char*) malloc(bytes+1);
	while( (n=getline(&line, &bytes, stdin)) != -1 ){
		if (n > LCD_WIDTH+1){
			fprintf(stderr,"Too many characters!\n");
			exit(1);
		}
		asprintf(&lines[i], "%s", line);
		i++;
		if (i>4){
			fprintf(stderr,"Too many lines!\n");
			exit(1);
		}
	}
	free(line);

	GPIOExport(LCD_E);
	GPIOExport(LCD_RS);
	GPIOExport(LCD_D4);
	GPIOExport(LCD_D5);
	GPIOExport(LCD_D6);
	GPIOExport(LCD_D7);

	GPIODirection(LCD_E, OUT);
	GPIODirection(LCD_RS, OUT);
	GPIODirection(LCD_D4, OUT);
	GPIODirection(LCD_D5, OUT);
	GPIODirection(LCD_D6, OUT);
	GPIODirection(LCD_D7, OUT);
	
	// Initialise display
	lcd_init();

	if (i == 0){
		lcd_empty();
	}
	if (i > 0){
		lcd_byte(LCD_LINE_1, LCD_CMD);
		lcd_string(lines[0]);
	}
	if (i > 1){
		lcd_byte(LCD_LINE_2, LCD_CMD);
		lcd_string(lines[1]);
	}
	if (i > 2){
		lcd_byte(LCD_LINE_3, LCD_CMD);
		lcd_string(lines[2]);
	}
	if (i > 3){
		lcd_byte(LCD_LINE_4, LCD_CMD);
		lcd_string(lines[3]);
	}

	// Cleanup
	for (z=0; z<i; z++){
		free(lines[z]);
	}

	f_unlock(fd);
	close(fd);
	unlink(LOCKFILE);
	exit(0);
}
