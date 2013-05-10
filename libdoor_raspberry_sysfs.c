#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>

#define GPIO_EXPORT "/sys/class/gpio/export"
#define GPIO_NUM "/sys/class/gpio/gpio"

void pin_export(short pin){
    FILE* f;
    f=fopen(GPIO_EXPORT,"w");
    if (!f){
        perror("fopen: ");
        return;
    }
    fprintf(f,"%d",pin);
    fclose(f);
}

void pin_mode(short pin, char* mode){
    char *path;
    FILE* f;
    
    if (!asprintf(&path,"%s%d/%s",GPIO_NUM,pin,"direction")){
        perror("asprintf: ");
        return;
    }
    f=fopen(path,"w");
    if (!f){
        perror("fopen: ");
        return;
    }
    fprintf(f,"%s",mode);
    fclose(f);
    free(path);
}

void pin_on(short pin){
    char *path;
    FILE* f;
    
    pin_export(pin);
    pin_mode(pin,"out");
    
    if (!asprintf(&path,"%s%d/%s",GPIO_NUM,pin,"value")){
        perror("asprintf: ");
        return;
    }
    f=fopen(path,"w");
    if (!f){
        perror("fopen: ");
        return;
    }
    fprintf(f,"%d",1);
    fclose(f);
    free(path);
}

void pin_off(short pin){
    char *path;
    FILE* f;
    
    pin_export(pin);
    pin_mode(pin,"out");
    
    if (!asprintf(&path,"%s%d/%s",GPIO_NUM,pin,"value")){
        perror("asprintf: ");
        return;
    }
    f=fopen(path,"w");
    if (!f){
        perror("fopen: ");
        return;
    }
    fprintf(f,"%d",0);
    fclose(f);
    free(path);
}

int read_pin(short pin){
    /* Not implemented */
    return 0;
}
