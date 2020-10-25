#pragma once

#include "../include/link_layer.h"

int open_serial_port(LinkLayer *layer);
int close_serial_port(LinkLayer *layer);

void print_frame(char *frame);

int set_serial_port(LinkLayer *layer);

int ack_serial_port(LinkLayer *layer);
