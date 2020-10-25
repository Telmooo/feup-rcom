#include "state_machine.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "macros.h"


int is_non_info_frame(char frame) {
    return  frame == C_SET ||
            frame == C_DISC ||
            frame == C_UA ||
            frame == RR(0) ||
            frame == RR(1) ||
            frame == REJ(0) ||
            frame == REJ(1);
}

int is_info_frame(char frame) {
    return frame == C_INFORMATION(0) || frame == C_INFORMATION(1);
}

int is_valid_control_frame(char frame) {
    return is_non_info_frame(frame) || is_info_frame(frame);
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
        // printf("NANI %x\n", c);
        if (c == FLAG)
            this->state = STATE_FLAG_RCV;
        break;
    case STATE_FLAG_RCV:
        // printf("FLAG %x\n", c);
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
        // printf("A %x\n", c);
        if (is_valid_control_frame(c)) {
            this->control = c;
            this->state = STATE_C_RCV;
        }
        else if (c == FLAG)
            this->state = STATE_FLAG_RCV;
        else
            this->state = STATE_START;
        break;

    case STATE_C_RCV:
        // printf("C %x\n", c);
        if ((this->address ^ this->control) == c)
            this->state = STATE_BCC1_OK;
        else if (c == FLAG)
            this->state = STATE_FLAG_RCV;
        else
            this->state = STATE_START;
        break;

    case STATE_BCC1_OK:
        // printf("BCC %x\n", c);
        if (c == FLAG) {
            if (is_non_info_frame(this->control)) {
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
        if (this->escaped) {
            if (c == ESCAPED_ESCAPE) {
                this->data[++this->dataIndex] = ESCAPE;
                this->escaped = false;
            }
            else if (c == ESCAPED_FLAG) {
                this->data[++this->dataIndex] = FLAG;
                this->escaped = false;               
            }
            else {
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
                char bcc = 0;
                for (int i = 0; i < this->dataIndex - 2; ++i)
                    bcc ^= this->data[i];

                if (bcc != this->data[this->dataIndex - 1]) {
                    this->state = STATE_START;
                }
                else {
                    this->state = STATE_START;
                    return true;
                }
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
