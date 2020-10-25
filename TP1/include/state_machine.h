#pragma once

#include <stdbool.h>
#include "enums.h"

typedef struct _frame frame_t;

frame_t* new_state_machine(device_type dev_type);
void free_state_machine(frame_t *machine);

bool state_machine_is_data(frame_t *this);
char state_machine_get_control(frame_t *this);
char state_machine_get_data_size(frame_t *this);
void state_machine_copy_data(frame_t *this, char *dest);
void state_machine_restart(frame_t *this);

bool state_machine_process_char(frame_t *this, char c);

typedef enum _state_machine_states {
    STATE_START, STATE_FLAG_RCV, STATE_A_RCV, STATE_C_RCV, STATE_BCC1_OK, STATE_DATA, STATE_STOP
} sm_state_t;

struct _frame {
    sm_state_t state;
    device_type dev_type;
    char address, control, device_type;
    bool escaped, hasData;
    int dataIndex, dataSize;
    char* data;
};
