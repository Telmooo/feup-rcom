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

    switch (type) {
        case TRANSMITTER:
            set_serial_port(&link_layer);
            break;
        case RECEIVER:
            ack_serial_port(&link_layer);
            break;
    }
}
