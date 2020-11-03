#pragma once

#include "enums.h"

typedef struct _program_info program_info_t;

struct _program_info {
    int port;
    device_type dev_type;
    char *file_name;
};

int parse_argv(int argc, char **argv, program_info_t *out_info);