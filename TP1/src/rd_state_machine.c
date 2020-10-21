#include "../include/rd_state_machine.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../include/macros.h"

// TODO: Rename state machine (finish unification) && review machine

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

RD_Machine* new_rd_machine() {
    RD_Machine *this = (RD_Machine*) malloc(sizeof(RD_Machine));
    this->state = RD_START;
    this->escaped = false;
    this->hasData = false;
    this->dataIndex = 0;
    this->dataSize = 0;
    return this;
}

void free_RD_machine(RD_Machine *this) {
    free(this);
}

bool rd_machine_is_data(RD_Machine *this) {
    return this->hasData;
}

char rd_machine_get_control(RD_Machine *this) {
    return this->control;
}

char rd_machine_get_data_size(RD_Machine *this) {
    return this->dataSize;
}

void rd_machine_copy_data(RD_Machine *this, char *dest) {
    memcpy(dest, this->data, this->dataSize * sizeof(char));
}

bool rd_process_char(RD_Machine *this, char c) {
    switch (this->state)
    {
    case RD_START:
        if (c == FLAG)
            this->state = RD_FLAG_RCV;
        break;
    case RD_FLAG_RCV:
        if (c == A_EMISSOR || c == A_RECEPTOR) {
            this->address = c;
            this->state = RD_A_RCV;
        }
        else if (c != FLAG)
            this->state = RD_START;
        break;

    case RD_A_RCV:
        // Improve if
        if (is_valid_control_frame(c)) {
            this->control = c;
            this->state = RD_C_RCV;
        }
        else if (c == FLAG)
            this->state = RD_FLAG_RCV;
        else
            this->state = RD_START;
        break;

    case RD_C_RCV:
        if ((this->address ^ this->control) == c)
            this->state = RD_BCC1_OK;
        else if (c == FLAG)
            this->state = RD_FLAG_RCV;
        else
            this->state = RD_START;
        break;

    case RD_BCC1_OK:
        if (c == FLAG) {
            if (is_non_info_frame(this->control)) {
                this->state = RD_START;
                this->hasData = false;
                this->dataSize = 0;
                return true;
            }
            this->state = RD_START;
        } else {
            if (is_info_frame(this->control)) {
                this->state = RD_DATA;
                this->hasData = true;
                this->dataIndex = 0;
            } else {
                this->state = RD_START;
            }
        }
        break;
    
    case RD_DATA:
        if (this->escaped) {
            if (c != ESCAPE && c != FLAG) {
                this->state = RD_START;
                this->escaped = false;
            }
            else {
                this->data[++this->dataIndex] = c;
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
                    this->state = RD_START;
                }
                else {
                    this->state = RD_START;
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
