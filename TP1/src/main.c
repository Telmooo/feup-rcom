#include "app_layer.h"
#include "link_layer.h"
#include "signal_handling.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_usage() {
    printf("Usage:\t<program_name> <t> <0-11> <file_to_transfer>\n\t<program_name> <r> <0-11> [file_saved_name]\n");
}

int main(int argc, char** argv) {
    if (argc < 3) {
        print_usage();
        return 1;
    }
    
    device_type t = TRANSMITTER;
    if (argv[1][0] == 'r' || argv[1][0] == 'R') {
        t = RECEIVER;
    } else if (argv[1][0] == 't' || argv[1][0] == 'T') {
        if (argc < 4) {
            printf("Missing <file_to_transfer> parameter");
            print_usage();
            return 1;
        }
    }
    else {
        print_usage();
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

    if (t == TRANSMITTER) {
        if (app_send_file(fd, argv[3])) {
            printf("Failed send file\n");
        }
    }
    else {
        app_ctrl_info_t file_info = {
            .file_name = NULL,
            .file_size = -1
        };

        if (argc == 4) {
            file_info.file_name = (char*) malloc((strlen(argv[3]) + 1) * sizeof(char));
            strcpy(file_info.file_name, argv[3]);
        }

        if (app_read_file(fd, &file_info)) {
            printf("Failed read file\n");
        } else {
            printf("File name: %s\nFile Size: %ld\n", file_info.file_name, file_info.file_size);
        }

        if (file_info.file_name != NULL) free(file_info.file_name);
    }

    if (llclose(fd)) {
        printf("Failed to close link layer\n");
        return 1;
    }

    return 0;
}
