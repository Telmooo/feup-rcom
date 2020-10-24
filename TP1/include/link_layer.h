#pragma once

#include "../include/macros.h"

typedef struct linkLayer LinkLayer;

typedef enum {
    TRANSMITTER = 0,
    RECEIVER = 1
} device_type;

struct linkLayer {
    char port[20]; /*Dispositivo /dev/ttySx, x = 0, 1*/
    int fd; /** Port File Descriptor */
    int baudRate; /*Velocidade de transmissão*/
    unsigned int sequenceNumber; /*Número de sequência da trama: 0, 1*/
    unsigned int timeout; /*Valor do temporizador: 1 s*/
    unsigned int numTransmissions; /*Número de tentativas em caso de falha*/
    char frame[BUF_SIZE]; /*Trama*/
};