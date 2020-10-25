#pragma once

#include "macros.h"
#include "state_machine.h"

typedef struct linkLayer LinkLayer;

struct linkLayer {
    char port[20]; /*Dispositivo /dev/ttySx, x = 0, 1*/
    int fd; /** Port File Descriptor */
    int baudRate; /*Velocidade de transmissão*/
    int sequenceNumber; /*Número de sequência da trama: 0, 1*/
    int timeout; /*Valor do temporizador: 1 s*/
    int numTransmissions; /*Número de tentativas em caso de falha*/
    frame_t* state_machine;
};

int llopen(int port, device_type type);

int llwrite(int fd, char *buffer, int length);

int llread(int fd, char *buffer);

int llclose(int fd);
