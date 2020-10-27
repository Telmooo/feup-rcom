#include "app_layer.h"
#include "link_layer.h"
#include "signal_handling.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
    if (argc < 3) {
        printf("Usage: <program_name> <t|r> <0-11> <file_to_transfer>\n");
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

    //for (int i = 0; i < 3; ++i) {
        if (t == TRANSMITTER) {
            /*int ret = llwrite(fd, "NieR: Automata ~ Original Soundtrack", 36);
            // int ret = llwrite(fd, "Look at them, they come to this place when they know they are not pure. Tenno use the keys, but they are mere trespassers. Only I, Vor, know the true power of the Void. I was cut in half, destroyed, but through its Janus Key, the Void called to me. It brought me here and here I was reborn. We cannot blame these creatures, they are being led by a false prophet, an impostor who knows not the secrets of the Void. Behold the Tenno, come to scavenge and desecrate this sacred realm. My brothers, did I not tell of this day? Did I not prophesize this moment? Now, I will stop them. Now I am changed, reborn through the energy of the Janus Key. Forever bound to the Void. Let it be known, if the Tenno want true salvation, they will lay down their arms, and wait for the baptism of my Janus key. It is time. I will teach these trespassers the redemptive power of my Janus key. They will learn its simple truth. The Tenno are lost, and they will resist. But I, Vor, will cleanse this place of their impurity.", 1004);
            printf("Written %d chars\n", ret);*/
            if (app_send_file(fd, argv[3])) {
                printf("Failed send file\n");
            }
        }
        else {
            app_ctrl_info_t *file_info = NULL;

            if (app_read_file(fd, file_info)) {
                printf("Failed read file\n");
            } else {
                printf("File name: %s\nFile Size: %ld\n", file_info->file_name, file_info->file_size);
            }




            /*char *msg = NULL;
            int size = llread(fd, &msg);
            printf("Read %d chars:\n_", size);
            
            for (int i = 0; i < size; ++i) {
                printf("%c", msg[i]);
            }
            printf("_\n");

            free(msg);*/
            free(file_info);
        }
    //}

    if (llclose(fd)) {
        printf("Failed to close link layer\n");
        return 1;
    }

    printf("Exited pretty damn fine :ok_hand:\n");
    return 0;
}
