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
    link_layer.type = type;

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
        printf("SET/UA failed\n");
        close_serial_port(&link_layer);
        free_state_machine(link_layer.state_machine);
        return -1;
    }

    return link_layer.fd;
}

int llwrite(int fd, char *buffer, int length) {
    char *stuffed = NULL;
    int stuffed_length = stuff_frame(&stuffed, buffer, length, link_layer.sequenceNumber);
    if (stuffed_length == -1) {
        return -1;
    }

    int ret = send_info_serial_port(&link_layer, stuffed, stuffed_length);
    free(stuffed);
    
    if (ret != -1) {
        link_layer.sequenceNumber = reverse_sequence_number(link_layer.sequenceNumber);
    }
    return ret;
}

int llread(int fd, char **buffer) {
    // Read data
    if (read_info_frame(&link_layer)) {
        return -1;
    }

    link_layer.sequenceNumber = reverse_sequence_number(link_layer.sequenceNumber);

    // Allocate & Copy data to buffer
    if (state_machine_copy_data(link_layer.state_machine, buffer))
        return -1;
    return state_machine_get_data_size(link_layer.state_machine);
}

int llread_to_file(int fd, int file_dest_fd) {
    // Read data
    if (read_info_frame(&link_layer)) {
        return -1;
    }

    link_layer.sequenceNumber = reverse_sequence_number(link_layer.sequenceNumber);

    // Allocate & Copy data to buffer
    return state_machine_write_data_to_file(link_layer.state_machine, file_dest_fd);
}

int llclose(int fd) {
    int ret;
    if (link_layer.type == TRANSMITTER) {
        ret = serial_port_transmitter_disc(&link_layer);
    } else {
        ret = serial_port_receiver_disc(&link_layer);
    }

    if (ret != -1) {
        ret = close_serial_port(&link_layer);
        free_state_machine(link_layer.state_machine);
    }
    return ret;
}
