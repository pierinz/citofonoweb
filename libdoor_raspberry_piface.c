#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "piface/c/src/piface/pfio.h"

void pin_on(short pin){
    pfio_init();
    pfio_digital_write(pin, 1);
    pfio_deinit();
}

void pin_off(short pin){
    pfio_init();
    pfio_digital_write(pin, 0);
    pfio_deinit();
}

int read_pin(short pin){
    //will be implemented when needed
    return 0;
}
