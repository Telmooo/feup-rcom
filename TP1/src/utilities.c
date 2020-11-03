#include "utilities.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int parse_argv(int argc, char **argv, program_info_t *out_info) {

    if (argc < 3) {
        printf("%s: invalid number of arguments\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "R") == 0 || strcmp(argv[1], "r") == 0) {
        out_info->dev_type = RECEIVER;

        out_info->file_name = (argc >= 4 ? argv[3] : NULL);
    } else if (strcmp(argv[1], "T") == 0 || strcmp(argv[1], "t") == 0) {
        out_info->dev_type = TRANSMITTER;

        if (argc < 4) {
            printf("%s: invalid number of arguments - missing <file_to_transfer> parameter\n", argv[0]);
            return 1;
        }

        out_info->file_name = argv[3];
    } else {
        printf("%s: invalid argument - unkown device type\n", argv[0]);
        return 1;
    }

    out_info->port = atoi(argv[2]);
    if (out_info->port < 0 || out_info->port > 11) {
        printf("%s: invalid argument - invalid port number\n", argv[0]);
        return 1;
    }

    return 0;
}