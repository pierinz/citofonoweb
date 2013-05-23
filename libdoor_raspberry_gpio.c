#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>

void pin_init(){
    return;
}

void pin_on(short pin){
    char *command,*command2;
    if (!asprintf(&command,"/usr/local/bin/gpio -g %s %d out","mode",pin)){
        perror("asprintf: ");
        return;
    }
    if (!asprintf(&command2,"/usr/local/bin/gpio -g %s %d 1","write",pin)){
        perror("asprintf: ");
        return;
    }
    if (!system(command))
        perror("system: ");
    if (system(command2))
        perror("system: ");
    free(command);
    free(command2);
}

void pin_off(short pin){
    char *command,*command2;
    if (!asprintf(&command,"/usr/local/bin/gpio -g %s %d out","mode",pin)){
        perror("asprintf: ");
        return;
    }
    if (!asprintf(&command2,"/usr/local/bin/gpio -g %s %d 0","write",pin)){
        perror("asprintf: ");
        return;
    }
    if (!system(command))
        perror("system: ");
    if (system(command2))
        perror("system: ");
    free(command);
    free(command2);
}

int read_pin(short pin){
    /* Not implemented */
    return 0;
}

void pin_clean(){
    return;
}