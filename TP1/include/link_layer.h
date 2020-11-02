#pragma once

#include "macros.h"
#include "enums.h"
#include "state_machine.h"

typedef struct linkLayer LinkLayer;

struct linkLayer {
    char port[20]; /*Dispositivo /dev/ttySx, x = 0, 1*/
    int sequenceNumber; /*Número de sequência da trama: 0, 1*/
    device_type type;
    frame_t* state_machine;
};

int llopen(int port, device_type type);

int llwrite(int fd, char *buffer, int length);

int llread(int fd, char *buffer);

int llclose(int fd);
