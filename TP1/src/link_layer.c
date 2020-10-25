#include "link_layer.h"
#include "serial_port.h"
#include "macros.h"

#include <stdlib.h>
#include <stdio.h>
#include <state_machine.h>
#include <unistd.h>

static LinkLayer link_layer;

int llopen(int port, device_type type) {
    sprintf(link_layer.port, "/dev/ttyS%d", port);
    link_layer.baudRate = BAUDRATE;
    link_layer.timeout = LL_TIMEOUT;
    link_layer.numTransmissions = LL_RETRIES;

    if (type != TRANSMITTER && type != RECEIVER) {
        printf("llopen device type can only be either TRANSMITTER or RECEIVER\n");
        return -1;
    }

    link_layer.state_machine = new_state_machine(type);

    open_serial_port(&link_layer);
    if (link_layer.fd == -1) {
        printf("Failed to open serial port\n");
        return -1;
    }

    int ret;
    switch (type) {
        case TRANSMITTER:
            link_layer.sequenceNumber = 0;
            ret = set_serial_port(&link_layer);
            break;
        case RECEIVER:
            link_layer.sequenceNumber = 1;
            ret = ack_serial_port(&link_layer);
            break;
    }

    if (ret != 0) {
        close_serial_port(&link_layer);
        free_state_machine(link_layer.state_machine);
        return -1;
    }

    return link_layer.fd;;
}

int llwrite(int fd, char *buffer, int length) {

    return -1;
}

int llread(int fd, char *buffer) {

    // Read data

    // Upon disc

    return -1;
}

int llclose(int fd) {
    int ret = close_serial_port(&link_layer);
    // TODO: DISC
    free_state_machine(link_layer.state_machine);
    return ret;
}
