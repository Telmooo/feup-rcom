#include "../include/app_layer.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stats.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

int file_size(int fd) {
    struct stat st;
    fstat(fd, &st);
    return st.st_size;
}

int app_create_ctrl_packet(app_ctrl_type_t ctrl_type, app_ctrl_info_t *ctrl_info, app_ctrl_packet_t *out_packet) {
    int file_name_size = strlen(ctrl_info->file_name);

    out_packet = (app_ctrl_packet_t*)malloc(sizeof(app_ctrl_packet_t));

    if (out_packet == NULL) {
        perror("malloc failed at creating app_ctrl_packet_t");
        return -1;
    }

    out_packet->ctrl_field = ctrl_type;
    out_packet->tlv_packet_count = CTRL_INFO_TLV_COUNT;

    out_packet->tlv_packets = (app_tlv_packet_t*)malloc(sizeof(app_tlv_packet_t) * out_packet->tlv_packet_count);

    if (out_packet->tlv_packets == NULL) {
        perror("malloc failed at creating app_tlv_packet_t");
        return -1;
    }

    // -- START TLV PACKETS --
    
    // File Size
    out_packet->tlv_packets[0].type = TLV_TYPE_FILE_SIZE;
    out_packet->tlv_packets[0].length = sizeof(long);
    out_packet->tlv_packets[0].value = (uint8_t*)malloc(sizeof(uint8_t) * out_packet->tlv_packets[0].length);
    if (out_packet->tlv_packets[0].value == NULL) {
        perror("malloc failed at creating file size value array for app_tlv_packet_t");
        return -1;
    }

    *out_packet->tlv_packets[0].value = ctrl_info->file_size;

    // File Name
    out_packet->tlv_packets[1].type = TLV_TYPE_FILE_NAME;
    out_packet->tlv_packets[1].length = strlen(ctrl_info->file_name);
    out_packet->tlv_packets[1].value = (uint8_t*)malloc(sizeof(uint8_t) * out_packet->tlv_packets[1].length);
    if (out_packet->tlv_packets[1].value == NULL) {
        perror("malloc failed at creating file name value array for app_tlv_packet_t");
        return -1;
    }

    memcpy((void*)out_packet->tlv_packets[1].value, (void*)ctrl_info->file_name, out_packet->tlv_packets[1].length);

    // Permissions

    // -- END TLV PACKETS --

    return 0;
}

int app_create_data_packet(uint8_t seq_number, char *buffer, uint16_t length, app_data_packet_t *out_packet) {

    out_packet = (app_data_packet_t*)malloc(sizeof(app_data_packet_t));

    if (out_packet == NULL) {
        perror("malloc failed to create app_data_packet_t");
        return -1;
    }

    out_packet->ctrl_field = app_ctrl_type_t.DATA;
    out_packet->seq_no = seq_number;
    out_packet->length = length;
    out_packet->packet_data = (uint8_t*)malloc(sizeof(uint8_t) * out_packet->length);

    if (out_packet->packet_data == NULL) {
        perror("malloc failed to allocate data array for app_data_packet_t");
        return -1;
    }

    memcpy((void*)out_packet->packet_data, (void*)buffer, out_packet->length);

    return 0;
}

int app_send_ctrl_packet(int fd, app_ctrl_packet_t *packet) {

    int packet_size = 1; // Control Field
    for (int i = 0; i < packet->tlv_packet_count; i++) {
        packet_size += 2; // Type & Length
        packet_size += packet->tlv_packets[i].length; // Value
    }

    char *buffer = (char*)malloc(sizeof(char) * packet_size);

    int index = 0;
    buffer[index++] = packet->ctrl_field; // Control Field
    for (int i = 0; i < packet->tlv_packet_count; i++) {
        app_tlv_packet_t *tlv_aux = packet->tlv_packets[i];

        buffer[index++] = tlv_aux->type;
        buffer[index++] = tlv_aux->length;

        for (int j = 0; j < tlv_aux->length; j++) {
            buffer[index++] = tlv_aux->value[j];
        }
    }

    if (llwrite(fd, buffer, packet_size) != packet_size) {
        fprintf(stderr, "llwrite failed to write full length of the control packet\n");
        free(buffer);
        return -1;
    }

    free(buffer);

    return 0;
}

int app_send_data_packet(int fd, app_data_packet_t *packet) {

    int packet_size = 4 + packet->length; // Control Field & SeqNo & Length (2 bytes) & Data Buffer

    char *buffer = (char*)malloc(sizeof(char) * packet_size);

    int index = 0;

    buffer[index++] = packet->ctrl_field; // Control Field
    buffer[index++] = packet->seq_no; // Sequence Number
    buffer[index] = packet->length; index += 2; // Length (2 bytes)
    
    memcpy((void*)&buffer[index], packet->packet_data, packet->length);

    if (llwrite(fd, buffer, packet_size) != packet_size) {
        fprintf(stderr, "llwrite failed to write full length of the data packet\n");
        free(buffer);
        return -1;
    }

    free(buffer);

    return 0;
}

int app_parse_ctrl_packet(char *buffer, int size, app_ctrl_type_t ctrl_type, app_ctrl_info_t *out_info) {

    if (size < 1) {
        fprintf(stderr, "invalid control packet - empty packet\n");
        return -1;
    }

    out_info = (app_ctrl_info_t*)malloc(sizeof(app_ctrl_info_t));

    if (out_info == NULL) {
        fprintf(stderr, "malloc failed to allocate space for file information\n");
        return -1;
    }

    app_ctrl_type_t ctrl_type_read = (uint8_t)buffer[0];

    if (ctrl_type_read != ctrl_type) {
        fprintf(stderr, "invalid control packet - expected control field with value %d got %d\n", ctrl_type, ctrl_type_read);
        return -1;
    }

    int tlv_packets_parsed = 0;

    for (int i = 1; ;) {
        
        if (i >= size) break;
        uint8_t type = (uint8_t)buffer[i++];

        if (i >= size) {
            fprintf(stderr, "invalid control packet - invalid TLV packet, missing length\n");
            return -1;
        }
        uint8_t length = (uint8_t)buffer[i++];

        if (i + length > size) {
            fprintf(stderr, "invalid control packet - invalid TLV packet, missing value\n");
            return -1;
        }

        switch(type) {
            case TLV_TYPE_FILE_SIZE:
                memcpy(&out_info->file_size, (void*)&buffer[i], length);
                break;
            case TLV_TYPE_FILE_NAME:
            {
                out_info->file_name = (char*)malloc(sizeof(char) * length);
                memcpy(out_info->file_name, (void*)&buffer[i], length);
                break;
            }
            default:
                fprintf(stderr, "invalid control packet - invalid TLV packet, invalid TLV type\n");
                return -1;
        }

        i += length;

        tlv_packets_parsed++;
    }

    if (tlv_packets_parsed != CTRL_INFO_TLV_COUNT) {
        fprintf(stderr, "invalid control packet - missing information\n");
        return -1;
    }

    return 0;
}

int app_parse_data_packet(char *buffer, int size, int sequence_number, char *write_data, int *write_length) {
    
    if (size < 4) {
        return -1;
    }

    app_ctrl_type_t ctrl_type_read = (uint8_t)buffer[0];

    if (ctrl_type_read != app_ctrl_type_t.DATA) {
        fprintf(stderr, "invalid data packet - expected control field with value %d got %d\n", app_ctrl_type_t.DATA, ctrl_type_read);
        return -1;
    }

    uint8_t seq_no = (uint8_t)buffer[1];

    if (seq_no != sequence_number) {
        return -1;
    }

    uint16_t length;
    memcpy(&length, &buffer[2], sizeof(uint16_t));

    if (length + 4 != size) {
        return -1;
    }

    write_data = (char*)malloc(sizeof(char) * length);

    *write_length = (int)length;

    for (int i = 0; i < length; i++) {
        write_data[i] = buffer[4 + i];
    }

    return 0;

}

int app_match_ctrl_info(app_ctrl_info_t *info1, app_ctrl_info_t *info2) {
    return (info1->file_size == info2->file_size) && (strcmp(info1->file_name, info2->file_name) == 0);
}

app_ctrl_info_t* app_cpy_info_packet(app_ctrl_info_t *dest, app_ctrl_info_t *src) {
    dest = (app_ctrl_info_t*)malloc(sizeof(app_ctrl_info_t));
    dest->file_size = src->file_size;
    int length = strlen(src->file_name);
    dest->file_name = (char*)malloc(sizeof(char) * length);
    memcpy(dest->file_name, src->file_name, length);
    return dest;
}

int app_read_file(int fd, app_ctrl_info_t *file_info) {

    char *buffer;

    app_ctrl_info_t *start_info;
    app_ctrl_info_t *end_info;

    int bytes_read;

    // Start Packets

    if ((bytes_read = llread(fd, &buffer)) <= 0) {
        fprintf(stderr, "llread failed at reading start packet\n");
        return -1;
    }

    if (app_parse_ctrl_packet(buffer, bytes_read, app_ctrl_type_t.START, start_info)) {
        return -1;
    }

    if (start_info == NULL || start_info->file_name == NULL) {
        fprintf(stderr, "invalid control info\n");
        return -1;
    }

    // Open destination file
    int dest_fd;

    if ((dest_fd = open(start_info->file_name, O_WRONLY | O_CREAT, 0660)) == -1) {
        fprintf(stderr, "%s: failed to open destination file %s\n", __func__, start_info->file_name);
        return -1;
    }

    // Data Packets
    int sequence_number = 0;
    int hasData = 1;

    while (hasData) {

        if ((bytes_read = llread(fd, &buffer)) <= 0) {
            fprintf(stderr, "%s: llread failed to read or reached the end without receveing end packet\n", __func__, start_info->file_name);
            return -1;
        }

        if (buffer[0] == app_ctrl_type_t.END) {
            hasData = 0;
            continue;
        }

        char *write_data;
        int size;

        if (app_parse_data_packet(buffer, bytes_read, sequence_number, write_data, &size)) {
            return -1;    
        }

        if (write(dest_fd, write_data, size) != size) {
            return -1;
        }

        sequence_number = (sequence_number + 1) % 255;
    }

    // End Packet

    if (app_parse_ctrl_packet(buffer, bytes_read, app_ctrl_type_t.END, end_info)) {
        return -1;
    }

    if (end_info == NULL || end_info->file_name == NULL) {
        fprintf(stderr, "invalid control info\n");
        return -1;
    }

    if (!(app_match_ctrl_info(start_info, end_info))) {
        return -1;
    }

    app_cpy_info_packet(file_info, start_info);

    free(start_info->file_name);
    free(start_info);

    free(end_info->file_name);
    free(end_info);
    return 0;

}

int app_send_file(int fd, char *filename) {
    
    int file_fd = open(filename, O_RDONLY);

    if (file_fd == -1) {
        char error_msg[256];
        sprintf(error_msg, "couldn't open file %s", filename);
        perror(error_msg);
        return -1;
    }

    int file_size = file_size(fd);

    // Start Packet
    app_ctrl_info_t ctrl_info = {
        .file_name = filename,
        .file_size = file_size
    };

    app_ctrl_packet_t *start_packet;

    if (app_create_ctrl_packet(app_ctrl_type_t.START, &ctrl_info, start_packet)) {
        return -1;
    }

    if (app_send_ctrl_packet(fd, start_packet)) {
        return -1;
    }

    // Data Packets
    uint8_t seq_no = 0;
    
    uint8_t data_buff[CHUNK_SIZE];
    int bytes_read;
    while ((bytes_read = read(fd, data_buff, CHUNK_SIZE)) != 0) {
        
        if (bytes_read == -1) {
            fprintf(stderr, "read failed while reading from file to be transfered\n");
            return -1;
        }

        app_data_packet_t *data_packet;

        if (app_create_data_packet(seq_no, data_buff, bytes_read, data_packet)) {
            return -1;
        }

        if (app_send_data_packet(fd, data_packet)) {
            return -1;
        }

        seq_no = (seq_no + 1) % 255;
    }

    // End Packet
    app_ctrl_packet_t *end_packet;

    if (app_create_ctrl_packet(app_ctrl_type_t.END, &ctrl_info, end_packet)) {
        return -1;
    }

    if (app_send_ctrl_packet(fd, end_packet)) {
        return -1;
    }

    // Clean up
    free(start_packet);
    free(end_packet);

    // Close file
    if (close(file_fd) == -1) {
        fprintf(stderr, "close error");
        return -1;
    }

    return 0;
}