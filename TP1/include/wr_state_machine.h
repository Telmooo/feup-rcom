#pragma once

#include <stdbool.h>

typedef struct wr_machine WR_Machine;

WR_Machine* new_wr_machine();
void free_wr_machine(WR_Machine *machine);

bool process_char(WR_Machine *machine, char c);
