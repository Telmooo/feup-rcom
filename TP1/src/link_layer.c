#include "../include/link_layer.h"
#include "../include/serial_port.h"

#include <stdlib.h>

static LinkLayer link_layer;

int llopen(int port, device_type type) {
    sprintf(link_layer.port, "/dev/ttyS%d", port);
    link_layer.baudRate = BAUDRATE;
    link_layer.sequenceNumber = 0;
    link_layer.timeout = LL_TIMEOUT;
    link_layer.numTransmissions = LL_RETRIES;
    link_layer.frame[0] = '\0';

    int fd = open_serial_port(&link_layer);

    if (fd == -1) {
        return -1;
    }

    int ret;
    switch (type) {
        case TRANSMITTER:
            ret = set_serial_port(&link_layer);
            break;
        case RECEIVER:
            ret = ack_serial_port(&link_layer);
            break;
    }

    return ret;
}

int llwrite(int fd, char *buffer, int length) {

    return -1;
}

int llread(int fd, char *buffer) {

    // Read data

    // Upon disc

    return -1;
}