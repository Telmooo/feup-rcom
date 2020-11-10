#include "app_layer.h"
#include "link_layer.h"
#include "signal_handling.h"
#include "utilities.h"

#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_usage(char *program_name) {
    printf( "Program usage:\n"
            "\t%s <device_type> <port_number> <file_name>\n"
            "\t\tdevice_type - [T | t] for the transmitter device, [R | r] for the receiver device\n"
            "\t\tport_number - number of the port, between 0 and 11\n"
            "\t\tfile_name - file to be transfered in case of the transmitter device or name of the file where content will be saved in case of the receiver device\n"
            "\t\t(file_name parameter is optional for the receiver, if not specifed file will be saved with the file name of the original)\n",
            program_name);
}

int main(int argc, char** argv) {

    program_info_t program_info;

    if (parse_argv(argc, argv, &program_info)) {
        print_usage(argv[0]);
        return 1;
    }

    #if defined(RECEIVER_INFO_ERROR_RATE) && defined(RECEIVER_INFO_ERROR_RATE)
        #ifdef SET_SEED
        srand(SET_SEED);
        #endif
        #ifndef SET_SEED
        srand((unsigned) time(NULL));
        #endif
    #endif

    if (set_signal_handlers()) {
        fprintf(stderr, "%s: failed to set signal handlers\n", argv[0]);
        return 1;
    }

    int fd = llopen(program_info.port, program_info.dev_type);
    if (fd == -1) {
        fprintf(stderr, "%s: failed to open link layer\n", argv[0]);
        return 1;
    }

    struct timeval tvalBefore, tvalAfter;
    gettimeofday (&tvalBefore, NULL);

    int ret = 0;
    
    if (program_info.dev_type == TRANSMITTER) {
        if (app_send_file(fd, program_info.file_name)) {
            fprintf(stderr, "%s: file transfer failed\n", argv[0]);
            ret = 1;
        }
    }
    else {
        app_ctrl_info_t file_info = {
            .file_name = program_info.file_name,
            .file_size = -1
        };

        if (app_read_file(fd, &file_info)) {
            fprintf(stderr, "%s: failed to read transfered file\n", argv[0]);
            ret = 1;
        } else {
            char unit[4] = " B";
            double size;
            if (file_info.file_size > (int) 1e9) {
                unit[0] = 'G';
                size = file_info.file_size / 1e9;
            }
            else if (file_info.file_size > (int) 1e6) {
                unit[0] = 'M';
                size = file_info.file_size / 1e6;
            }
            else if (file_info.file_size > (int) 1e3) {
                unit[0] = 'K';
                size = file_info.file_size / 1e3;
            }
            else {
                unit[0] = 'B';
                unit[2] = '\0';
                size = file_info.file_size;
            }
            printf( 
                "File transfered: %s\n"
                "File saved as: %s\n"
                "File size: %.2lf %s\n",
                file_info.file_name,
                program_info.file_name == NULL ? file_info.file_name : program_info.file_name,
                size, unit
            );
        }

        if (file_info.file_name != NULL) free(file_info.file_name);
    }

    gettimeofday (&tvalAfter, NULL);
    double transferDuration = (tvalAfter.tv_sec - tvalBefore.tv_sec)
        + (tvalAfter.tv_usec - tvalBefore.tv_usec) / 1e6;
    
    if (ret)
        printf("Failed to transfer file after %.6lf seconds\n", transferDuration);
    else
        printf("Took %.6lf seconds to transfer the file\n", transferDuration);


    if (llclose(fd)) {
        fprintf(stderr, "%s: failed to close link layer\n", argv[0]);
        return 1;
    }

    return ret;
}
