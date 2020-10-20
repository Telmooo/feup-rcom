#include "../include/link_layer.h"
#include "../include/macros.h"

#include <stdlib.h>

struct linkLayer {
    char port[20]; /*Dispositivo /dev/ttySx, x = 0, 1*/
    int baudRate; /*Velocidade de transmissão*/
    unsigned int sequenceNumber; /*Número de sequência da trama: 0, 1*/
    unsigned int timeout; /*Valor do temporizador: 1 s*/
    unsigned int numTransmissions; /*Número de tentativas em caso de
    falha*/
    char frame[BUF_SIZE]; /*Trama*/
};

LinkLayer* new_link_layer(char* port) {
    LinkLayer *this = (LinkLayer*) malloc(sizeof(LinkLayer));

    this->baudRate = BAUDRATE;
    this->sequenceNumber = 0;
    this->timeout = LL_TIMEOUT;
    this->numTransmissions = LL_RETRIES;
    this->frame[0] = '\0';
}
