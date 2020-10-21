#pragma once

#include "../include/link_layer.h"

typedef struct _frame frame_t;

int open_serial_port(LinkLayer *layer);

int set_serial_port(LinkLayer *layer);

int ack_serial_port();



struct _frame {
    int sequence_number;

};