#include <unistd.h>
#include "piface/c/src/piface/pfio.h"

void pin_init(){
    pfio_init();
}

void pin_on(short pin){
    pfio_digital_write(pin, 1);
}

void pin_off(short pin){
    pfio_digital_write(pin, 0);
}

int read_pin(short pin){
    //will be implemented when needed
    return 0;
}

void pin_clean(){
    pfio_deinit();
}