#include "link_layer.h"

#include "macros.h"
#include "state_machine.h"
#include "signal_handling.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

// DEPRECATED
#include <strings.h>

// GLOBAL VARS
static LinkLayer link_layer;

volatile int STOP=FALSE;

struct termios oldtio;

static int open_serial_port(LinkLayer *layer) {
    int fd = open(layer->port, O_RDWR | O_NOCTTY );
    if (fd <0) {perror(layer->port); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    struct termios newtio;

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 1 char received */
  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) pr�ximo(s) caracter(es)
  */
    tcflush(fd, TCIOFLUSH);
    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    return fd;
}

static int close_serial_port(int fd) {
    sleep(1);
    if (tcsetattr(fd, TCSANOW, &oldtio) < 0) {
        return -1;
    }

    if (close(fd) < 0) {
        return -1;
    }

    return 0;
}

static inline int reverse_sequence_number(int seq) {
    return seq ^ 0x01;
}

// static void print_frame(char *frame) {
//     printf("%x ", frame[0]);
//     printf("%x ", frame[1]);
//     printf("%x ", frame[2]);
//     printf("%x ", frame[3]);
//     printf("%x ", frame[4]);
//     printf("\n");
// }

static void create_control_frame(char* frame, char A, char C) {
    frame[0] = FLAG;
    frame[1] = A;
    frame[2] = C;
    frame[3] = A ^ C;
    frame[4] = FLAG;
}

static int read_frame(int fd, char address, char control) {
    char c;
    bool done = 1;
    int ret;
    while (done) {
        if (alarm_was_called) {
            return READ_FRAME_WAS_INTERRUPTED;
        }
        ret = read(fd, &c, 1);
        if (ret == -1) {
            if (errno == EINTR) {
                return READ_FRAME_WAS_INTERRUPTED;
            } else {
                perror("read_control_frame_base read");
                return READ_FRAME_ERROR;
            }
        } else if (ret == 1) {
            if (state_machine_process_char(link_layer.state_machine, c) == 0) {
                if (address != READ_FRAME_IGNORE_CHECK && link_layer.state_machine->address != address) {
                    #ifdef DEBUG_MESSAGES
                    fprintf(stderr, "%s Unexpected address field: Expected 0x%x but received 0x%x\n", __func__, address, link_layer.state_machine->address);
                    #endif
                    continue;
                }

                if (control != READ_FRAME_IGNORE_CHECK && link_layer.state_machine->control != control) {
                    #ifdef DEBUG_MESSAGES
                    fprintf(stderr, "%s: Unexpected control field: Expected 0x%x but received 0x%x\n", __func__, control, link_layer.state_machine->control);
                    #endif
                    continue;
                }

                return READ_FRAME_OK;
            }
        }
    }

    return READ_FRAME_ERROR;
}

static void stuff_char(char *stuffed, int *stuffed_i, char to_stuff) {
    if (to_stuff == FLAG) {
        stuffed[(*stuffed_i)++] = ESCAPE;
        stuffed[(*stuffed_i)++] = ESCAPED_FLAG;
    } else if (to_stuff == ESCAPE) {
        stuffed[(*stuffed_i)++] = ESCAPE;
        stuffed[(*stuffed_i)++] = ESCAPED_ESCAPE;
    } else {
        stuffed[(*stuffed_i)++] = to_stuff;
    }
}

static int stuff_frame(char *stuffed, char *buffer, int length, int sequence_number) {
    if (stuffed == NULL) {
        fprintf(stderr, "%s: Stuffed frame was a NULL pointer\n", __func__);
        return -1;
    }

    stuffed[0] = FLAG;
    stuffed[1] = A_EMISSOR;
    stuffed[2] = C_INFORMATION(sequence_number);
    stuffed[3] = A_EMISSOR ^ C_INFORMATION(sequence_number);
    int stuffed_i = 4;
    
    char bcc2 = 0;
    for (int i = 0; i < length; ++i) {
        bcc2 ^= buffer[i];
        stuff_char(stuffed, &stuffed_i, buffer[i]);
    }
    
    stuff_char(stuffed, &stuffed_i, bcc2);
    stuffed[stuffed_i++] = FLAG;

    return stuffed_i;
}

static int set_serial_port(int fd) {
    char frame[CONTROL_FRAME_SIZE];
    create_control_frame(frame, A_EMISSOR, C_SET);

    int ret, ntries = LL_RETRIES;
    while (ntries) {
        write(fd, frame, CONTROL_FRAME_SIZE);
        // print_frame(frame);
        set_alarm(LL_TIMEOUT);
        ret = read_frame(fd, A_RECEPTOR_RESPONSE, C_UA);
        cancel_alarm();
        if (ret == READ_FRAME_WAS_INTERRUPTED) {
            ntries--;
        } else if (ret == READ_FRAME_ERROR) {
            return -1;
        } else {
            return 0;
        }
    }

    return -1;
}

static int ack_serial_port(int fd) {
    
    if (read_frame(fd, C_SET, A_EMISSOR) != READ_FRAME_OK) {
        fprintf(stderr, "%s: Failed to read control frame\n", __func__);
    }
    else {
        char ua_frame[CONTROL_FRAME_SIZE];
        create_control_frame(ua_frame, A_RECEPTOR_RESPONSE, C_UA);

        if (write(fd, ua_frame, CONTROL_FRAME_SIZE) != CONTROL_FRAME_SIZE)
            return -1;

        return 0;
    }

    return -1;
}

static int send_info_serial_port(int fd, char *buffer, int length) {
    int written, ret;
    bool send = true;
    int ntries = LL_RETRIES;
    while (ntries) {
        if (send) {
            written = write(fd, buffer, length);
            set_alarm(LL_TIMEOUT);
        }
        send = true;

        ret = read_frame(fd, A_EMISSOR, READ_FRAME_IGNORE_CHECK);
        // TODO: Refactor to get the cancel_alarm() calls better organized
        if (ret == READ_FRAME_WAS_INTERRUPTED) {
            ntries--;
        }
        else if (ret == READ_FRAME_ERROR) {
            cancel_alarm();
            fprintf(stderr, "%s: failed to read frame\n", __func__);
            return -1;
        }
        else {
            if (state_machine_get_control(link_layer.state_machine) == C_RR(reverse_sequence_number(link_layer.sequenceNumber))) {
                cancel_alarm();
                return written;
            }
            else if (state_machine_get_control(link_layer.state_machine) == C_REJ(reverse_sequence_number(link_layer.sequenceNumber))) {
                send = true;
            }
            else {
                send = false;
            }
        }
    }

    #ifdef DEBUG_MESSAGES
    fprintf(stderr, "%s: exceed number of retransmissions\n", __func__);
    #endif
    
    return -1;
}

static int read_info_frame(int fd) {
    int ret;
    char ack[CONTROL_FRAME_SIZE];
    while (true) {
        ret = read_frame(fd, A_EMISSOR, C_INFORMATION(reverse_sequence_number(link_layer.sequenceNumber)));
        if (ret == READ_FRAME_OK) {
            if (state_machine_is_data_valid(link_layer.state_machine)) {
                // Send RR
                create_control_frame(ack, A_RECEPTOR_RESPONSE, C_RR(link_layer.sequenceNumber));
                write(fd, ack, CONTROL_FRAME_SIZE);
                return 0;
            }

            // Send REJ
            create_control_frame(ack, A_RECEPTOR_RESPONSE, C_REJ(link_layer.sequenceNumber));
            write(fd, ack, CONTROL_FRAME_SIZE);

            // TODO: (?) We don't send an RR when we receive a "duplicate" I frame
        }
    }
}

static int serial_port_transmitter_disc(int fd) {
    char frame[CONTROL_FRAME_SIZE];
    create_control_frame(frame, A_EMISSOR, C_DISC);

    int ret, ntries = LL_RETRIES;
    while (ntries) {
        write(fd, frame, CONTROL_FRAME_SIZE);
        // print_frame(frame);
        set_alarm(LL_TIMEOUT);
        ret = read_frame(fd, A_RECEPTOR, C_DISC);
        cancel_alarm();
        if (ret == READ_FRAME_WAS_INTERRUPTED) {
            ntries--;
        } else if (ret == READ_FRAME_ERROR) {
            return -1;
        } else {
            create_control_frame(frame, A_EMISSOR_RESPONSE, C_UA);
            if (write(fd, frame, CONTROL_FRAME_SIZE) == -1) {
                return -1;
            }
            return 0;
        }
    }

    return -1;
}


static int serial_port_receiver_disc(int fd) {
    char frame[CONTROL_FRAME_SIZE];
    create_control_frame(frame, A_RECEPTOR, C_DISC);

    int ret = read_frame(fd, A_EMISSOR, C_DISC);
    if (ret != READ_FRAME_OK) {
        return -1;
    }

    int ntries = LL_RETRIES;
    while (ntries) {
        write(fd, frame, CONTROL_FRAME_SIZE);
        // print_frame(frame);
        set_alarm(LL_TIMEOUT);
        ret = read_frame(fd, A_EMISSOR_RESPONSE, C_UA);
        cancel_alarm();
        if (ret == READ_FRAME_WAS_INTERRUPTED) {
            ntries--;
        } else if (ret == READ_FRAME_ERROR) {
            return -1;
        } else {
            return 0;
        }
    }

    return -1;
}




// ----------------------------------
// |        LINK LAYER STUFF        |    
// ----------------------------------

int llopen(int port, device_type type) {
    sprintf(link_layer.port, "/dev/ttyS%d", port);
    link_layer.type = type;

    if (type != TRANSMITTER && type != RECEIVER) {
        fprintf(stderr, "%s: llopen device type can only be either TRANSMITTER or RECEIVER\n", __func__);
        return -1;
    }

    link_layer.state_machine = new_state_machine();

    int fd = open_serial_port(&link_layer);
    if (fd == -1) {
        fprintf(stderr, "%s: Failed to open serial port\n", __func__);
        return -1;
    }

    int ret;
    switch (type) {
        case TRANSMITTER:
            link_layer.sequenceNumber = 0;
            ret = set_serial_port(fd);
            break;
        case RECEIVER:
            link_layer.sequenceNumber = 1;
            ret = ack_serial_port(fd);
            break;
    }

    if (ret != 0) {
        fprintf(stderr, "%s: SET/UA failed\n", __func__);
        close_serial_port(fd);
        free_state_machine(link_layer.state_machine);
        return -1;
    }

    return fd;
}

int llwrite(int fd, char *buffer, int length) {
    char stuffed[STUFFED_MAX_SIZE];
    int stuffed_length = stuff_frame(stuffed, buffer, length, link_layer.sequenceNumber);
    if (stuffed_length == -1) {
        return -1;
    }

    int ret = send_info_serial_port(fd, stuffed, stuffed_length);
    
    if (ret != -1) {
        link_layer.sequenceNumber = reverse_sequence_number(link_layer.sequenceNumber);
    }

    if (ret == stuffed_length) {
        return length;
    }
    else if (ret == -1) {
        return -1;
    }
    else {
        // Calculate how many bytes of the original message were sent
        
        // Account for the BCC2 being stuffed
        int stop_at;
        if (stuffed[stuffed_length - 3] == ESCAPE)
            stop_at = stuffed_length - 4 - 3;
        else
            stop_at = stuffed_length - 4 - 2;

        int original_written = 0;
        // it = 4 to skip the headers
        for (int it = 4; it < stop_at && ret > 0; ++it) {
            if (stuffed[it] != ESCAPE)
                original_written++;
            ret--;
        }

        return original_written;
    }
}

int llread(int fd, char *buffer) {
    // Read data
    if (read_info_frame(fd)) {
        return -1;
    }

    link_layer.sequenceNumber = reverse_sequence_number(link_layer.sequenceNumber);

    // Allocate & Copy data to buffer
    if (state_machine_copy_data(link_layer.state_machine, buffer))
        return -1;
    return state_machine_get_data_size(link_layer.state_machine);
}

int llread_to_file(int fd, int file_dest_fd) {
    // Read data
    if (read_info_frame(fd)) {
        return -1;
    }

    link_layer.sequenceNumber = reverse_sequence_number(link_layer.sequenceNumber);

    // Allocate & Copy data to buffer
    return state_machine_write_data_to_file(link_layer.state_machine, file_dest_fd);
}

int llclose(int fd) {
    int ret;
    if (link_layer.type == TRANSMITTER) {
        ret = serial_port_transmitter_disc(fd);
    } else {
        ret = serial_port_receiver_disc(fd);
    }

    if (ret != -1) {
        ret = close_serial_port(fd);
        free_state_machine(link_layer.state_machine);
    }
    return ret;
}
