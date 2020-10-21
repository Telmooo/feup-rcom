#include "../include/wr_state_machine.h"

#include <stdlib.h>
#include "../include/macros.h"

WR_Machine* new_wr_machine() {
    WR_Machine *machine = (WR_Machine*) malloc(sizeof(WR_Machine));
    machine->state = WR_START;
    return machine;
}

void free_wr_machine(WR_Machine *machine) {
    free(machine);
}

char wr_machine_get_control(WR_Machine *machine) {
    return machine->control;
}

bool wr_process_char(WR_Machine *machine, char c) {
    switch (machine->state)
    {
    case WR_START:
        if (c == FLAG)
            machine->state = WR_FLAG_RCV;
        break;

    case WR_FLAG_RCV:
        if (c == A_RECEPTOR || c == A_EMISSOR_RESPONSE) {
            machine->address = c;
            machine->state = WR_A_RCV;
        }
        else if (c == FLAG)
            machine->state = WR_FLAG_RCV;
        else
            machine->state = WR_START;
        break;

    case WR_A_RCV:
        if (c == C_DISC || c == C_UA) {
            machine->control = c;
            machine->state = WR_C_RCV;
        }
        else if (c == FLAG)
            machine->state = WR_FLAG_RCV;
        else
            machine->state = WR_START;
            machine->state = WR_START;
        break;

    case WR_C_RCV:
        if ((machine->address ^ machine->control) == c)
            machine->state = WR_BCC_OK;
        else
            machine->state = WR_START;
        break;

    case WR_BCC_OK:
        if (c == FLAG) {
            machine->state = WR_STOP;
            return true;
        }
        else
            machine->state = WR_START;
        break;

    case WR_STOP:
        return true;
        break;

    default:
        break;
    }

    return false;
}
