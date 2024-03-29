#pragma once

#include <stdbool.h>
#include "enums.h"
#include "macros.h"

typedef struct _frame frame_t;

frame_t* new_state_machine();
void free_state_machine(frame_t *machine);

bool state_machine_is_data(frame_t *this);
bool state_machine_is_data_valid(frame_t *this);
char state_machine_get_control(frame_t *this);
char state_machine_get_address(frame_t *this);
int state_machine_get_data_size(frame_t *this);
int state_machine_copy_data(frame_t *this, char *dest);
int state_machine_write_data_to_file(frame_t *this, int file_fd);
void state_machine_restart(frame_t *this);

int state_machine_process_char(frame_t *this, char c);

typedef enum _state_machine_states {
    STATE_START, STATE_FLAG_RCV, STATE_A_RCV, STATE_C_RCV, STATE_BCC1_OK, STATE_DATA, STATE_STOP
} sm_state_t;

struct _frame {
    sm_state_t state;
    char address, control, bcc2;
    bool escaped, hasData, successful;
    int dataIndex, dataSize;
    char data[UNSTUFFED_MAX_SIZE];
};
