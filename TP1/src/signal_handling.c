#include "../include/signal_handling.h"

#include <signal.h>

void alarm_handler (int signum) {}

int set_signal_handlers() {

    struct sigaction new_action;

    /* Set up the structure to specify the new action. */
    new_action.sa_handler = alarm_handler;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;

    return sigaction(SIGALRM, &new_action, NULL);
}