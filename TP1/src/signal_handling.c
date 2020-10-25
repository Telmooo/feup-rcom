#include "signal_handling.h"

#include <signal.h>
#include <stddef.h>
#include <stdio.h>

void alarm_handler (int signum) {}

int set_signal_handlers() {

    struct sigaction new_action;

    /* Set up the structure to specify the new action. */
    sigemptyset(&new_action.sa_mask);
    new_action.sa_handler = alarm_handler;
    new_action.sa_flags = 0;

    if (sigaction(SIGALRM, &new_action, NULL) < 0) {
        perror("Failed to set SIGALRM handler");
        return 1;
    }

    return 0;
}
