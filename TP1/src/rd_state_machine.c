#include "../include/rd_state_machine.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../include/macros.h"

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

bool process_char(RD_Machine *this, char c) {
    switch (this->state)
    {
    case RD_START:
        if (c == FLAG)
            this->state = RD_FLAG_RCV;
        break;
    case RD_FLAG_RCV:
        if (c == A_EMISSOR || c == A_EMISSOR_RESPONSE) {
            this->address = c;
            this->state = RD_A_RCV;
        }
        else if (c != FLAG)
            this->state = RD_START;
        break;

    case RD_A_RCV:
        // Improve if
        if (c == C_SET || c == C_DISC || c == C_UA || c == C_INFORMATION(0) || c == C_INFORMATION(1) || c == RR(0) || c == RR(1) || c == REJ(0) || c == REJ(1)) {
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
            // Was a supervision/unnunbered
            this->state = RD_START;
            this->hasData = false;
            this->dataSize = 0;
            return true;
        }
        else {
            // Is information
            this->state = RD_DATA;
            this->hasData = true;
            this->dataIndex = 0;
        }
        break;
    
    case RD_DATA:
        if (this->escaped) {
            if (c != ESCAPE && c != FLAG) {
                // Unexpected char
                // TODO:
                fprintf(stderr, "Unexpected escaped character, don't know what to do\n");
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
                    // bcc was incorrect
                    // TODO:
                    fprintf(stderr, "Incorrect bcc2, don't know what to do\n");
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
