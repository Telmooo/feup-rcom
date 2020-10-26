#include "signal_handling.h"

#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

int alarm_was_called = 0;

void alarm_handler (int signum) {
    alarm_was_called = 1;
}

int set_alarm(unsigned int seconds) {
    alarm_was_called = 0;
    return alarm(seconds);
}

int cancel_alarm() {
    return alarm(0);
}

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
