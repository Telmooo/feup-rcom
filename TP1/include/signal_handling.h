#pragma once

#include <signal.h>

extern int alarm_was_called;

int set_signal_handlers();

int set_alarm(unsigned int seconds);
int cancel_alarm();
