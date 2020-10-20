#pragma once

#include <stdbool.h>

typedef struct rd_machine RD_Machine;

RD_Machine* new_wr_machine();
void free_rd_machine(RD_Machine *machine);

bool rd_machine_is_data(RD_Machine *this);
char rd_machine_get_control(RD_Machine *this);
char rd_machine_get_data_size(RD_Machine *this);
void rd_machine_copy_data(RD_Machine *this, char *dest);

bool process_char(RD_Machine *this, char c);
