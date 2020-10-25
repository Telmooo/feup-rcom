#include "state_machine.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "macros.h"

static inline int is_control_frame(char control) {
    return  control == C_SET ||
            control == C_DISC ||
            control == C_UA ||
            control == RR(0) ||
            control == RR(1) ||
            control == REJ(0) ||
            control == REJ(1);
}

static inline int is_info_frame(char control) {
    return control == C_INFORMATION(0) || control == C_INFORMATION(1);
}

static inline int is_valid_frame(char control) {
    return is_control_frame(control) || is_info_frame(control);
}

frame_t* new_state_machine(device_type dev_type) {
    frame_t *this = (frame_t*) malloc(sizeof(frame_t));
    this->data = (char*) malloc(sizeof(char) * DATA_DEFAULT_SIZE);
    this->state = STATE_START;
    this->escaped = false;
    this->hasData = false;
    this->dataIndex = 0;
    this->dataSize = 0;
    this->device_type = dev_type;
    return this;
}

void free_state_machine(frame_t *this) {
    free(this->data);
    free(this);
}

void state_machine_copy_data(frame_t *this, char *dest) {
    memcpy(dest, this->data, this->dataSize * sizeof(char));
}

void state_machine_restart(frame_t *this) {
    this->state = STATE_START;
}

bool state_machine_process_char(frame_t *this, char c) {
    switch (this->state) {
    case STATE_START:
        #ifdef DEBUG_STATE_MACHINE
        printf("START %x\n", c);
        #endif
        if (c == FLAG)
            this->state = STATE_FLAG_RCV;
        break;
    case STATE_FLAG_RCV:
        #ifdef DEBUG_STATE_MACHINE
        printf("FLAG_RCV %x\n", c);
        #endif
        if (c == A_EMISSOR || c == A_RECEPTOR) {
            // if (c == this->device_type) {
            //     this->state = STATE_START;
            //     return false;
            // }
            this->address = c;
            this->state = STATE_A_RCV;
        }
        else if (c != FLAG)
            this->state = STATE_START;
        break;

    case STATE_A_RCV:
        #ifdef DEBUG_STATE_MACHINE
        printf("A_RCV %x\n", c);
        #endif
        if (is_valid_frame(c)) {
            this->control = c;
            this->state = STATE_C_RCV;
        }
        else if (c == FLAG)
            this->state = STATE_FLAG_RCV;
        else
            this->state = STATE_START;
        break;

    case STATE_C_RCV:
        #ifdef DEBUG_STATE_MACHINE
        printf("C_RCV %x\n", c);
        #endif
        if ((this->address ^ this->control) == c)
            this->state = STATE_BCC1_OK;
        else if (c == FLAG)
            this->state = STATE_FLAG_RCV;
        else
            this->state = STATE_START;
        break;

    case STATE_BCC1_OK:
        #ifdef DEBUG_STATE_MACHINE
        printf("BCC1_OK 0x%x\n", c);
        #endif
        if (c == FLAG) {
            if (is_control_frame(this->control)) {
                this->state = STATE_START;
                this->hasData = false;
                this->dataSize = 0;
                return true;
            }
            this->state = STATE_START;
        } else {
            if (is_info_frame(this->control)) {
                this->state = STATE_DATA;
                this->hasData = true;
                this->dataIndex = 0;
            } else {
                this->state = STATE_START;
            }
        }
        break;

    case STATE_DATA:
        #ifdef DEBUG_STATE_MACHINE
        printf("DATA 0x%x", c);
        #endif
        if (this->escaped) {
            if (c == ESCAPED_ESCAPE) {
                this->data[++this->dataIndex] = ESCAPE;
                this->escaped = false;
            } else if (c == ESCAPED_FLAG) {
                this->data[++this->dataIndex] = FLAG;
                this->escaped = false;               
            } else if (c == FLAG) {
                this->state = STATE_FLAG_RCV;
                this->escaped = false;
            } else {
                this->state = STATE_START;
                this->escaped = false;
            }
        }
        else {
            if (c == ESCAPE) {
                this->escaped = true;
            }
            else if (c == FLAG) {
                // Verify BCC2
                char bcc2 = 0;
                for (int i = 0; i < this->dataIndex - 2; ++i)
                    bcc2 ^= this->data[i];

                this->successful = (bcc2 == this->data[this->dataIndex - 1]);

                this->state = STATE_START;
                return true;
            }
            else {
                // Regular data
                this->data[++this->dataIndex] = c;
            }
        }

        break;

    default:
        fprintf(stderr, "Hey you, you're finally awake. You were trying to cross the border? Walked right into that imperial ambush, like us and that thief over there.");
        break;
    }

    return false;
}
