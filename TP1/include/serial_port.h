#pragma once

#include "../include/link_layer.h"

int open_serial_port(LinkLayer *layer);

int set_serial_port(LinkLayer *layer);

int ack_serial_port();