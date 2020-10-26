#include "link_layer.h"
#include "signal_handling.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
    if (argc < 3) {
        printf("Usage: sp t/r 0-11\n");
        return 1;
    }
    
    device_type t = TRANSMITTER;
    if (argv[1][0] == 'r' || argv[1][0] == 'R') {
        t = RECEIVER;
    } else if (argv[1][0] != 't' && argv[1][0] != 'T') {
        printf("Must be either [T]ransmitter or [R]eceiver (was %c)\nUsage: sp t/r 0-11\n", argv[2][0]);
        return 1;
    }

    if (set_signal_handlers()) {
        printf("Failed to set signal handlers\n");
        return 1;
    }

    int fd = llopen(atoi(argv[2]), t);
    if (fd == -1) {
        printf("Failed to open link layer\n");
        return 1;
    }

    for (int i = 0; i < 8; ++i) {
        if (t == TRANSMITTER) {
            int ret = llwrite(fd, "NieR: Automata ~ Original Soundtrack", 36);
            printf("Written %d chars\n", ret);
        }
        else {
            char *msg = NULL;
            int size = llread(fd, &msg);
            printf("Read %d chars:\n_", size);
            
            for (int i = 0; i < size; ++i) {
                printf("%c", msg[i]);
            }
            printf("_\n");
        }
    }

    if (llclose(fd)) {
        printf("Failed to close link layer\n");
        return 1;
    }

    printf("Exited pretty damn fine :ok_hand:\n");
    return 0;
}
