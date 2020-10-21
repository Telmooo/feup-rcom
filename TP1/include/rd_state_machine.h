#pragma once

#include <stdbool.h>

typedef struct rd_machine RD_Machine;

RD_Machine* new_rd_machine();
void free_rd_machine(RD_Machine *machine);

bool rd_machine_is_data(RD_Machine *this);
char rd_machine_get_control(RD_Machine *this);
char rd_machine_get_data_size(RD_Machine *this);
void rd_machine_copy_data(RD_Machine *this, char *dest);

bool rd_process_char(RD_Machine *this, char c);

typedef enum RD_state {
    RD_START, RD_FLAG_RCV, RD_A_RCV, RD_C_RCV, RD_BCC1_OK, RD_DATA, RD_STOP
} RD_State;

struct rd_machine {
    RD_State state;
    char address, control;
    bool escaped, hasData;
    int dataIndex, dataSize;
    char data[DATA_MAX_SIZE + 1];
};
