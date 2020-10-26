#include "../include/serial_port.h"

#include "macros.h"
#include "state_machine.h"

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

volatile int STOP=FALSE;

struct termios oldtio;

int open_serial_port(LinkLayer *layer) {
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

    layer->fd = fd;

    return fd;
}

int close_serial_port(LinkLayer *layer) {
    sleep(1);
    if (tcsetattr(layer->fd, TCSANOW, &oldtio) < 0) {
        return -1;
    }

    if (close(layer->fd) < 0) {
        return -1;
    }

    return 0;
}

inline int reverse_sequence_number(int seq) {
    return seq ^ 0x01;
}

void print_frame(char *frame) {
    printf("%x ", frame[0]);
    printf("%x ", frame[1]);
    printf("%x ", frame[2]);
    printf("%x ", frame[3]);
    printf("%x ", frame[4]);
    printf("\n");
}

void create_control_frame(char* frame, char A, char C) {
    frame[0] = FLAG;
    frame[1] = A;
    frame[2] = C;
    frame[3] = A ^ C;
    frame[4] = FLAG;
}

static int read_frame_base(LinkLayer *layer, char address, char control, ssize_t (*read_func)(int, void *, size_t, LinkLayer*)) {
    char c;
    bool done = 1;
    int ret;
    while (done) {
        ret = read_func(layer->fd, &c, 1, layer);
        if (ret == -1) {
            if (errno != EINTR) {
                perror("read_control_frame read");
            }
            return -1;
        } else if (ret == 1) {
            done = state_machine_process_char(layer->state_machine, c);
        }
    }

    if (address != READ_FRAME_IGNORE_CHECK && layer->state_machine->address != address) {
        printf("Unexpected address field: Expected 0x%x but received 0x%x\n", address, layer->state_machine->address);
        return -1;
    }

    if (control != READ_FRAME_IGNORE_CHECK && layer->state_machine->control != control) {
        printf("Unexpected control field: Expected 0x%x but received 0x%x\n", control, layer->state_machine->control);
        return -1;
    }

    return ret;
}

static inline ssize_t read_wrapper(int fd, void* buf, size_t length, LinkLayer *layer) {
    return read(fd, buf, length);
}

int read_frame(LinkLayer *layer, char address, char control) {
    return read_frame_base(layer, address, control, read_wrapper);
}

static inline ssize_t timeout_read(int fd, void* buf, size_t length, LinkLayer *layer) {
    alarm(layer->timeout);
    int ret = read(fd, buf, length);
    alarm(0);
    return ret;
}

int read_frame_timeout(LinkLayer *layer, char address, char control) {
    return read_frame_base(layer, address, control, timeout_read);
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

int stuff_frame(char **stuffed, char *buffer, int length, int sequence_number) {
    *stuffed = (char*) malloc((5 + length * 2 + 2) * sizeof(char));
    if (*stuffed == NULL) {
        printf("Failed to allocate stuffed frame\n");
        return 1;
    }

    (*stuffed)[0] = FLAG;
    (*stuffed)[1] = A_EMISSOR;
    (*stuffed)[2] = C_INFORMATION(sequence_number);
    (*stuffed)[3] = A_EMISSOR ^ C_INFORMATION(sequence_number);
    int stuffed_i = 4;
    
    char bcc2 = 0;
    for (int i = 0; i < length; ++i) {
        bcc2 ^= buffer[i];
        stuff_char(*stuffed, &stuffed_i, buffer[i]);
    }
    
    stuff_char(*stuffed, &stuffed_i, bcc2);
    (*stuffed)[stuffed_i++] = FLAG;

    return stuffed_i;
}

int set_serial_port(LinkLayer *layer) {
    char frame[CONTROL_FRAME_SIZE];
    create_control_frame(frame, A_EMISSOR, C_SET);

    int ntries = layer->numTransmissions;
    while (ntries) {
        write(layer->fd, frame, CONTROL_FRAME_SIZE);
        // print_frame(frame);
        if (read_frame_timeout(layer, A_RECEPTOR_RESPONSE, C_UA) != -1)
            return 0;
        ntries--;
    }

    return 1;
}

int ack_serial_port(LinkLayer *layer) {
    
    if (read_frame(layer, C_SET, A_EMISSOR) == -1) {
        printf("ack_serial_port: Failed to read control frame\n");
    }
    else {
        char ua_frame[CONTROL_FRAME_SIZE];
        create_control_frame(ua_frame, A_RECEPTOR_RESPONSE, C_UA);

        if (write(layer->fd, ua_frame, CONTROL_FRAME_SIZE) != CONTROL_FRAME_SIZE)
            return -1;

        return 0;
    }

    return -1;
}

int send_info_serial_port(LinkLayer *layer, char *buffer, int length) {
    int written, ret;
    bool send = true;
    int ntries = layer->numTransmissions;
    while (ntries) {
        if (send)
            written = write(layer->fd, buffer, length);
        send = true;
        if ((ret = read_frame_timeout(layer, A_EMISSOR, READ_FRAME_IGNORE_CHECK)) != -1) {
            if (state_machine_get_control(layer->state_machine) == C_RR(reverse_sequence_number(layer->sequenceNumber))) {
                return written;
            }
            if (state_machine_get_control(layer->state_machine) != C_REJ(reverse_sequence_number(layer->sequenceNumber))) {
                send = false;
            }
            // We don't want to decrease the num of tries, we only decrease in timeouts
            ntries++;
        }
        ntries--;
    }

    return -1;
}

int read_info_frame(LinkLayer *layer) {
    int ret;
    char ack[CONTROL_FRAME_SIZE];
    while (true) {
        if ((ret = read_frame(layer, A_EMISSOR, C_INFORMATION(reverse_sequence_number(layer->sequenceNumber)))) != -1) {
            if (state_machine_is_data_valid(layer->state_machine)) {
                // Send RR
                create_control_frame(ack, A_RECEPTOR_RESPONSE, C_RR(layer->sequenceNumber));
                write(layer->fd, ack, CONTROL_FRAME_SIZE);
                return 0;
            }

            // Send REJ
            create_control_frame(ack, A_RECEPTOR_RESPONSE, C_REJ(layer->sequenceNumber));
            write(layer->fd, ack, CONTROL_FRAME_SIZE);
        }
    }
}
