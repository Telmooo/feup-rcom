#pragma once

#include <stdint.h>

#define TLV_TYPE_FILE_SIZE  0
#define TLV_TYPE_FILE_NAME  1

#define CTRL_INFO_TLV_COUNT 2

#define CHUNK_SIZE          255

typedef struct _app_ctrl_packet app_ctrl_packet_t;

typedef struct _app_data_packet app_data_packet_t;

typedef struct _app_tlv_packet app_tlv_packet_t;

typedef struct _app_ctrl_info app_ctrl_info_t;

typedef enum _app_ctrl_type app_ctrl_type_t;

struct _app_ctrl_packet {
    uint8_t ctrl_field;
    uint8_t tlv_packet_count;
    app_tlv_packet_t *tlv_packets;
};

struct _app_data_packet {
    uint8_t ctrl_field;
    uint8_t seq_no;
    uint16_t length;
    uint8_t *packet_data;
};

struct _app_tlv_packet {
    uint8_t type;
    uint8_t length;
    uint8_t *value;
};

struct _app_ctrl_info {
    long file_size;
    char *file_name;
    // permissions
};

enum _app_ctrl_type {
    DATA = 1,
    START = 2,
    END = 3
};

int app_read_file(int fd, app_ctrl_info_t *file_info);

int app_send_file(int fd, char *filename);