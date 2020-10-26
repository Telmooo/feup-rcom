#pragma once

#include "../include/link_layer.h"

int open_serial_port(LinkLayer *layer);
int close_serial_port(LinkLayer *layer);

void print_frame(char *frame);
int reverse_sequence_number(int seq);

int set_serial_port(LinkLayer *layer);
int ack_serial_port(LinkLayer *layer);

int stuff_frame(char **stuffed, char *buffer, int length, int sequence_number);

int send_info_serial_port(LinkLayer *layer, char *buffer, int length);
int read_info_frame(LinkLayer *layer);

int serial_port_transmitter_disc(LinkLayer *layer);
int serial_port_receiver_disc(LinkLayer *layer);
