#include "../include/serial_port.h"

#include "macros.h"
#include "wr_state_machine.h"
#include "rd_state_machine.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>

struct termios oldtio;
void *state_machines[2];

void init_state_machines() {
    WR_Machine* transmitter = new_wr_machine();
    RD_Machine* receiver = new_rd_machine();

    state_machines[TRANSMITTER] = transmitter;
    state_machines[RECEIVER] = receiver;
}

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
    newtio.c_cc[VMIN]     = 5;   /* blocking read until 5 chars received */
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

char* create_supervision_frame(char A, char C) {
    char *frame = malloc(5 * sizeof(char));
    frame[0] = FLAG;
    frame[1] = A;
    frame[2] = C;
    frame[3] = A ^ C;
    frame[4] = FLAG;
    return frame;
}

void send_set_frame(LinkLayer *layer) {
    char *set_frame = create_supervision_frame(A_EMISSOR, C_SET);

    write(layer->fd, set_frame, 5);

    // TODO: update retries 
}

char* read_control_frame(LinkLayer *layer, char control_field) {
    // F A C BCC1 F

    char *received_frame = (char*)malloc(sizeof(char) * 5);

    RD_Machine *state_machine = new_rd_machine();

    int current = 0;

    for (; current < BUF_SIZE && state_machine->state != RD_STOP; current++) {
        if (read(layer->fd, &received_frame[current], 1) == -1) {
            return NULL;
        } 

        if (rd_process_char(state_machine, received_frame[current])) {
            if (rd_machine_get_control(state_machine) != control_field) {
                free(received_frame);
                return NULL;
            }
            break;
        }
    }

    return received_frame;
}

char* receiver_read_control_frame(LinkLayer *layer, char control_field) {
    // F A C BCC1 F

    char *received_frame = (char*)malloc(sizeof(char) * 5);

    RD_Machine *state_machine = new_rd_machine();

    int current = 0;

    for (; current < BUF_SIZE && state_machine->state != RD_STOP; current++) {
        read(layer->fd, &received_frame[current], 1);

        if (rd_process_char(state_machine, received_frame[current])) {
            if (rd_machine_get_control(state_machine) != control_field) {
                free(received_frame);
                return NULL;
            }
        }
    }

    return received_frame;
}

int set_serial_port(LinkLayer *layer) {
    // F A C BCC1 F
    char *frame = create_supervision_frame(A_EMISSOR, C_SET);

    // set timeout
    write(layer->fd, frame, 5);

    char *received = transmitter_read_control_frame(layer, C_UA);

    if (received != NULL) {
        free(received);
        return 0;
    }
    return -1;
}

int ack_serial_port(LinkLayer *layer) {

    char *received = receiver_read_control_frame(layer, C_SET);

    if (received != NULL) {
        char *ua_frame = create_supervision_frame(A_RECEPTOR, C_UA);

        write(layer->fd, ua_frame, 5);

        return 0;
    }

    return -1;

}