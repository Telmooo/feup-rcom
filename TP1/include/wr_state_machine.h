#pragma once

#include <stdbool.h>

typedef struct wr_machine WR_Machine;

typedef enum wr_state {
    WR_START, WR_FLAG_RCV, WR_A_RCV, WR_C_RCV, WR_BCC_OK, WR_STOP
} WR_State;

struct wr_machine {
    WR_State state;
    char address, control;
};

WR_Machine* new_wr_machine();
void free_wr_machine(WR_Machine *machine);

bool wr_process_char(WR_Machine *machine, char c);
