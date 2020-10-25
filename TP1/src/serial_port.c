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
    sleep(1);  // TODO: Why this?
    if (tcsetattr(layer->fd, TCSANOW, &oldtio) < 0) {
        return -1;
    }

    if (close(layer->fd) < 0) {
        return -1;
    }

    return 0;
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

int read_control_frame(LinkLayer *layer, char address, char control) {
    char c;
    bool done = false;
    while (!done) {
        int ret = read(layer->fd, &c, 1);

        if (ret == -1) {
            perror("read_control_frame read");
            return 1;
        } else if (ret == 1) {
            done = state_machine_process_char(layer->state_machine, c);
        }
    }

    if (layer->state_machine->control != control) {
        printf("Unexpected control field: Expected 0x%x but received 0x%x\n", control, layer->state_machine->control);
        return 1;
    }

    return 0;
}

int read_control_frame_timeout(LinkLayer *layer, char A, char C) {
    char c;
    bool done = false;
    while (!done) {
        alarm(layer->timeout);
        int ret = read(layer->fd, &c, 1);
        alarm(0);

        if (ret == -1) {
            if (errno != EINTR)
                perror("read_control_frame_timeout read");
            return 1;
        } else if (ret == 1) {
            done = state_machine_process_char(layer->state_machine, c);
        }
    }

    if (layer->state_machine->control != C) {
        printf("Unexpected control field: Expected '%c' but received '%c'\n", C, layer->state_machine->control);
        return 1;
    }

    return 0;
}

int read_data(LinkLayer *layer) {

    frame_t *state_machine = new_state_machine(A_RECEPTOR);

    int current = 0;

    for (; current < BUF_SIZE; current++) {
        char ch;
        if (read(layer->fd, &ch, 1) != 1) {
            return -1; // TODO
        }

        if (state_machine_process_char(state_machine, ch)) {
            if (state_machine->control != C_INFORMATION(layer->sequenceNumber)) {
                free_state_machine(state_machine);
                return -1;
            }
            
        }
    }
    return -1;
}

int set_serial_port(LinkLayer *layer) {
    char frame[CONTROL_FRAME_SIZE];
    create_control_frame(frame, A_EMISSOR, C_SET);

    int ntries = layer->numTransmissions;
    while (ntries) {
        write(layer->fd, frame, CONTROL_FRAME_SIZE);
        // print_frame(frame);
        if (read_control_frame_timeout(layer, A_EMISSOR, C_UA) == 0)
            return 0;
        ntries--;
    }

    return 1;
}

int ack_serial_port(LinkLayer *layer) {
    
    if (read_control_frame(layer, C_SET, A_EMISSOR) != 0) {
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
