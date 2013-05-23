#ifndef DOOR_H
#define DOOR_H

void pin_init();

void pin_on(short pin);

void pin_off(short pin);

int read_pin(short pin);

void pin_clean();

#endif