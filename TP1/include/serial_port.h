#pragma once

#include "../include/link_layer.h"

int open_serial_port(LinkLayer *layer);
int close_serial_port(LinkLayer *layer);

void print_frame(char *frame);

int set_serial_port(LinkLayer *layer);
int ack_serial_port(LinkLayer *layer);

int stuff_buffer(char *stuffed, char *buffer, int length);

int send_info_serial_port(LinkLayer *layer, char *buffer, int length);
