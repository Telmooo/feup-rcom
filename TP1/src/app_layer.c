#include "app_layer.h"

#include "link_layer.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define PROGRESS_STEP 10
static int progress = 0;

char complete_data_packet_buffer[UNSTUFFED_MAX_SIZE];


static void free_tlv_packet(app_tlv_packet_t *packet) {
    if (packet == NULL) return;

    if (packet->value == NULL) return;

    free(packet->value);
    packet->value = NULL;
}

static void free_ctrl_packet(app_ctrl_packet_t *packet) {
    if (packet == NULL) return;

    if (packet->tlv_packets == NULL) return;

    for (int i = 0; i < packet->tlv_packet_count; i++) {
        if (&(packet->tlv_packets[i]) != NULL) {
            free_tlv_packet(&(packet->tlv_packets[i]));
        }
    }

    free(packet->tlv_packets);
    packet->tlv_packets = NULL;
}

static void free_data_packet(app_data_packet_t *packet) {
    if (packet == NULL) return;

    if (packet->packet_data == NULL) return;

    free(packet->packet_data);
    packet->packet_data = NULL;
}

static void free_ctrl_info(app_ctrl_info_t *info) {
    if (info == NULL) return;

    if (info->file_name == NULL) return;

    free(info->file_name);
    info->file_name = NULL;
}

static void app_show_progress_bar(device_type type, int cur_progress) {
    #ifdef PROGRESS_BAR
    char *intro_message = NULL;

    switch (type) {
        case TRANSMITTER:
            intro_message = "Sending file...  ";
            break;
        case RECEIVER:
            intro_message = "Receiving file...";
            break;
        default:
            intro_message = "";
    }

    #if PROGRESS_BAR == 0
    // Progress only
    if (cur_progress >= progress + PROGRESS_STEP) {
        progress = cur_progress;
        printf("%s\t%d%%\n", intro_message, progress);
    }

    #elif PROGRESS_BAR == 1

    // Progress w/ carriage return (percentage only)
    printf("\r%s\t%d%%\r", intro_message, cur_progress);
    fflush(stdout);

    #elif PROGRESS_BAR == 2

    // Progress bar w/ carriage return
    printf("\r%s\t%d%% [", intro_message, cur_progress);
    for (int block = 0; block < 100; block += 5)
        if (block < cur_progress)
            printf("#");
        else
            printf("-");
    printf("]\r");
    fflush(stdout);

    #endif
    #endif

    if (cur_progress >= 100) printf("\n\tCompleted...\n\n");
}

static int get_file_size(int fd) {
    struct stat st;
    if (fstat(fd, &st) == -1) {
        fprintf(stderr, "%s: failed to get file status\n", __func__);
        return -1;
    }
    return st.st_size;
}

static int app_create_ctrl_packet(app_ctrl_type_t ctrl_type, app_ctrl_info_t *ctrl_info, app_ctrl_packet_t *out_packet) {
    //out_packet = (app_ctrl_packet_t*)malloc(sizeof(app_ctrl_packet_t));

    if (out_packet == NULL) {
        fprintf(stderr, "%s: null destination for out_packet\n", __func__);
        return -1;
    }

    #ifdef DEBUG_APP_CTRL_PACKET
    printf("%s: ctrl_info received\n\tFile Size : %ld\n\tFile name : %s\n", __func__, ctrl_info->file_size, ctrl_info->file_name);
    #endif

    out_packet->ctrl_field = ctrl_type;
    out_packet->tlv_packet_count = CTRL_INFO_TLV_COUNT;

    out_packet->tlv_packets = (app_tlv_packet_t*)malloc(sizeof(app_tlv_packet_t) * out_packet->tlv_packet_count);

    if (out_packet->tlv_packets == NULL) {
        fprintf(stderr, "%s: failed to allocate memory for value array from tlv_packets in app_ctrl_packet_t\n", __func__);
        return -1;
    }

    // -- START TLV PACKETS --
    
    // File Size
    out_packet->tlv_packets[0].type = TLV_TYPE_FILE_SIZE;
    out_packet->tlv_packets[0].length = sizeof(long);
    out_packet->tlv_packets[0].value = (uint8_t*)malloc(sizeof(uint8_t) * out_packet->tlv_packets[0].length);
    if (out_packet->tlv_packets[0].value == NULL) {
        fprintf(stderr, "%s: failed to allocate memory for value arry from app_ctrl_packet_t\n", __func__);
        free_ctrl_packet(out_packet);
        return -1;
    }

    memcpy((void*)out_packet->tlv_packets[0].value, (void*)&ctrl_info->file_size, sizeof(uint8_t) * out_packet->tlv_packets[0].length);

    #ifdef DEBUG_APP_CTRL_PACKET
    printf("%s: File size TLV packet length : %d\n", __func__, out_packet->tlv_packets[0].length);
    printf("%s: File size TLV packet value : ", __func__);
    for (int i = 0; i < out_packet->tlv_packets[0].length; i++) {
        printf("%c", out_packet->tlv_packets[0].value[i]);
    }
    printf("\n");
    #endif

    // File Name
    out_packet->tlv_packets[1].type = TLV_TYPE_FILE_NAME;
    out_packet->tlv_packets[1].length = strlen(ctrl_info->file_name) + 1;
    out_packet->tlv_packets[1].value = (uint8_t*)malloc(sizeof(uint8_t) * out_packet->tlv_packets[1].length);
    if (out_packet->tlv_packets[1].value == NULL) {
        fprintf(stderr, "%s: failed to allocate memory for value array from tlv_packets in app_ctrl_packet_t\n", __func__);
        free_ctrl_packet(out_packet);
        return -1;
    }

    strncpy((char *)out_packet->tlv_packets[1].value, ctrl_info->file_name, out_packet->tlv_packets[1].length);

    // Permissions

    // -- END TLV PACKETS --

    return 0;
}

static int app_create_data_packet(uint8_t seq_number, char *buffer, uint16_t length, app_data_packet_t *out_packet) {

    //out_packet = (app_data_packet_t*)malloc(sizeof(app_data_packet_t));

    if (out_packet == NULL) {
        fprintf(stderr, "%s: null destination for out_packet\n", __func__);
        return -1;
    }

    int previous_length = out_packet->length;

    out_packet->ctrl_field = DATA;
    out_packet->seq_no = seq_number;
    out_packet->length = length;
    
    if (out_packet->packet_data == NULL) {
        out_packet->packet_data = (uint8_t*)malloc(sizeof(uint8_t) * out_packet->length);
    } else if (previous_length < out_packet->length) {
        out_packet->packet_data = (uint8_t*)realloc(out_packet->packet_data, sizeof(uint8_t) * out_packet->length);
    }

    if (out_packet->packet_data == NULL) {
        fprintf(stderr, "%s: failed to allocate memory for data array from app_data_packet_t\n", __func__);
        return -1;
    }

    memcpy((void*)out_packet->packet_data, (void*)buffer, out_packet->length);
    
    return 0;
}

static int app_send_ctrl_packet(int fd, app_ctrl_packet_t *packet) {

    int packet_size = 1; // Control Field
    for (int i = 0; i < packet->tlv_packet_count; i++) {
        packet_size += 2; // Type & Length
        packet_size += packet->tlv_packets[i].length; // Value
    }

    char *buffer = (char*)malloc(sizeof(char) * packet_size);

    if (buffer == NULL) {
        fprintf(stderr, "%s: failed to allocate memory for buffer to write\n", __func__);
        return -1;
    }

    int index = 0;

    buffer[index++] = packet->ctrl_field; // Control Field
    for (int i = 0; i < packet->tlv_packet_count; i++) {
        app_tlv_packet_t *tlv_aux = &(packet->tlv_packets[i]);

        buffer[index++] = tlv_aux->type;
        buffer[index++] = tlv_aux->length;

        for (int j = 0; j < tlv_aux->length; j++) {
            buffer[index++] = tlv_aux->value[j];
        }
    }

    int ret;
    if ((ret = llwrite(fd, buffer, packet_size)) == -1) {
        fprintf(stderr, "%s: failed to write the control packet\n", __func__);
        free(buffer);
        return -1;
    } else if (ret != packet_size) {
        fprintf(stderr, "%s: wrote control packet. Expected %d bytes but wrote %d\n", __func__, packet_size, ret);
        free(buffer);
        return -1;
    }

    free(buffer);

    return 0;
}

static int app_send_data_packet(int fd, app_data_packet_t *packet) {

    int packet_size = 4 + packet->length; // Control Field & SeqNo & Length (2 bytes) & Data Buffer

    // char *complete_data_packet_buffer = (char*)malloc(sizeof(char) * packet_size);

    // if (complete_data_packet_buffer == NULL) {
    //     fprintf(stderr, "%s: failed to allocate memory for complete_data_packet_buffer to write\n", __func__);
    //     return -1;
    // }

    int index = 0;

    complete_data_packet_buffer[index++] = packet->ctrl_field; // Control Field
    complete_data_packet_buffer[index++] = packet->seq_no; // Sequence Number
    // buffer[index] = packet->length; index += 2; // Length (2 bytes)

    complete_data_packet_buffer[index++] = (uint8_t) (packet->length >> 8);
    complete_data_packet_buffer[index++] = (uint8_t) (packet->length);
    
    memcpy((void*)&complete_data_packet_buffer[index], (void*)packet->packet_data, packet->length);

    int ret;
    if ((ret = llwrite(fd, complete_data_packet_buffer, packet_size)) == -1) {
        fprintf(stderr, "%s: failed to write the control packet\n", __func__);
        // free(complete_data_packet_buffer);
        return -1;
    } else if (ret != packet_size) {
        fprintf(stderr, "%s: wrote control packet. Expected %d bytes but wrote %d\n", __func__, packet_size, ret);
        // free(complete_data_packet_buffer);
        return -1;
    }

    // free(complete_data_packet_buffer);

    return 0;
}

static int app_parse_ctrl_packet(char *buffer, int size, app_ctrl_type_t ctrl_type, app_ctrl_info_t *out_info) {

    if (size < 1) {
        fprintf(stderr, "%s: invalid control packet received - empty control packet\n", __func__);
        return -1;
    }

    //out_info = (app_ctrl_info_t*)malloc(sizeof(app_ctrl_info_t));

    if (out_info == NULL) {
        fprintf(stderr, "%s: null destination for out_info\n", __func__);
        return -1;
    }

    app_ctrl_type_t ctrl_type_read = (uint8_t)buffer[0];

    if (ctrl_type_read != ctrl_type) {
        fprintf(stderr, "%s: invalid control packet received - expected control field with value %d, got %d\n", __func__, ctrl_type, ctrl_type_read);
        return -1;
    }

    int tlv_packets_parsed = 0;

    for (int i = 1; ;) {
        
        if (i >= size) break;
        uint8_t type = (uint8_t)buffer[i++];

        if (i >= size) {
            fprintf(stderr, "%s: invalid control packet received - TLV packet is missing length octet\n", __func__);
            free_ctrl_info(out_info);
            return -1;
        }
        uint8_t length = (uint8_t)buffer[i++];

        if (i + length > size) {
            fprintf(stderr, "%s: invalid control packet received - TLV packet is value(s) octet(s)\n", __func__);
            free_ctrl_info(out_info);
            return -1;
        }

        switch(type) {
            case TLV_TYPE_FILE_SIZE:
                #ifdef DEBUG_MESSAGES
                printf("Buffer: 0x");
                for (int k = 0; k < length; ++k) {
                    printf("%02x", buffer[i + k]);
                }
                printf("\n");
                printf("Before copy File Size: %ld\n", out_info->file_size);
                #endif
                memcpy((void*)&out_info->file_size, (void*)&buffer[i], length);
                #ifdef DEBUG_MESSAGES
                printf("After copy File Size: %ld\n", out_info->file_size);
                #endif
                break;
            case TLV_TYPE_FILE_NAME:
            {
                out_info->file_name = (char*)malloc(sizeof(char) * length);
                if (out_info->file_name == NULL) {
                    fprintf(stderr, "%s: failed to allocate memory for file_name from app_ctrl_info_t\n", __func__);
                    free_ctrl_info(out_info);
                    return -1;
                }

                memcpy((void*)out_info->file_name, (void*)&buffer[i], length);
                break;
            }
            default:
                fprintf(stderr, "%s: invalid control packet - unknown TLV packet type received\n", __func__);
                free_ctrl_info(out_info);
                return -1;
        }

        i += length;

        tlv_packets_parsed++;
    }

    if (tlv_packets_parsed != CTRL_INFO_TLV_COUNT) {
        fprintf(stderr, "%s: invalid control packet - information is missing\n", __func__);
        free_ctrl_info(out_info);
        return -1;
    }

    return 0;
}

static int app_parse_data_packet(char *buffer, int size, int sequence_number, char **parsed_data, int *data_length) {
    
    if (size < 4) {
        fprintf(stderr, "%s: invalid data packet received - missing header information(s)\n", __func__);
        return -1;
    }

    app_ctrl_type_t ctrl_type_read = (uint8_t)buffer[0];

    if (ctrl_type_read != DATA) {
        fprintf(stderr, "%s: invalid data packet received - expected control field with value %d, got %d\n", __func__, DATA, ctrl_type_read);
        return -1;
    }

    uint8_t seq_no_read = (uint8_t)buffer[1];

    if (seq_no_read != sequence_number) {
        fprintf(stderr, "%s: invalid data packet received - mismatch on sequence number, expected %d got %d\n", __func__, sequence_number, seq_no_read);
        return -1;
    }

    uint16_t length = (((uint8_t) buffer[2]) << 8) + (uint8_t) buffer[3];

    if (length + 4 != size) {
        fprintf(stderr, "%s: invalid data packet received - mismatch on data size, expected %d got %d\n", __func__, size - 4, length);
        return -1;
    }

    (*parsed_data) = &buffer[4];

    *data_length = (int)length;

    return 0;

}

static int app_match_ctrl_info(app_ctrl_info_t *info1, app_ctrl_info_t *info2) {
    return (info1->file_size == info2->file_size) && (strcmp(info1->file_name, info2->file_name) == 0);
}

static app_ctrl_info_t* app_cpy_info_packet(app_ctrl_info_t *dest, app_ctrl_info_t *src) {
    #ifdef DEBUG_MESSAGES
    printf("SRC File Name: %s\nSRC File Size: %ld\n", src->file_name, src->file_size);
    #endif
    dest->file_size = src->file_size;
    int length = strlen(src->file_name) + 1;
    dest->file_name = (char*)malloc(sizeof(char) * length);
    strncpy(dest->file_name, src->file_name, length);
    #ifdef DEBUG_MESSAGES
    printf("DEST File Name: %s\nDEST File Size: %ld\n", dest->file_name, dest->file_size);
    #endif
    return dest;
}

int app_read_file(int fd, app_ctrl_info_t *file_info) {

    progress = 0;

    char buffer[FRAME_MAX_SIZE];

    app_ctrl_info_t start_info = {
        .file_name = NULL,
        .file_size = -1
    };
    app_ctrl_info_t end_info = {
        .file_name = NULL,
        .file_size = -1
    };

    int bytes_read;

    // Start Packets

    if ((bytes_read = llread(fd, buffer)) <= 0) {
        fprintf(stderr, "%s: failed to read start packet\n", __func__);
        return -1;
    }

    if (app_parse_ctrl_packet(buffer, bytes_read, START, &start_info)) {
        fprintf(stderr, "%s: failed to parse start control info\n", __func__);
        return -1;
    }

    if (start_info.file_name == NULL) {
        fprintf(stderr, "%s: invalid start control info - file name is undefined\n", __func__);
        return -1;
    }

    // Open destination file
    int dest_fd;

    #ifdef OVERRIDE_REC_FILE_NAME
    if ((dest_fd = open(OVERRIDE_REC_FILE_NAME, O_WRONLY | O_CREAT, 0660)) == -1) {
    #else
    char *file_name = file_info->file_name == NULL ? start_info.file_name : file_info->file_name;
    if ((dest_fd = open(file_name, O_WRONLY | O_CREAT, 0660)) == -1) {
    #endif
        fprintf(stderr, "%s: failed to open destination file %s\n", __func__, start_info.file_name);
        free_ctrl_info(&start_info);
        return -1;
    }

    // Data Packets
    int sequence_number = 0;
    int hasData = 1;

    int total_bytes_read = 0;

    while (hasData) {

        if ((bytes_read = llread(fd, buffer)) <= 0) {
            fprintf(stderr, "%s: failed to read or reached the end without receveing end packet\n", __func__);
        
            if (close(dest_fd) == -1)
                fprintf(stderr, "%s: failed to close destination file\n", __func__);

            free_ctrl_info(&start_info);
            return -1;
        }

        if (buffer[0] == END) {
            hasData = 0;
            continue;
        }

        char *write_data = NULL;
        int size;

        if (app_parse_data_packet(buffer, bytes_read, sequence_number, &write_data, &size)) {
            fprintf(stderr, "%s: failed to parse data packet received\n", __func__);

            if (close(dest_fd) == -1)
                fprintf(stderr, "%s: failed to close destination file\n", __func__);

            free_ctrl_info(&start_info);
            return -1;
        }

        if (write(dest_fd, write_data, size) != size) {
            fprintf(stderr, "%s: failed to write data packet received to destination file\n", __func__);
            
            if (close(dest_fd) == -1)
                fprintf(stderr, "%s: failed to close destination file\n", __func__);

            free_ctrl_info(&start_info);
            return -1;
        }

        total_bytes_read += size;

        app_show_progress_bar(RECEIVER, (int)( (float)total_bytes_read / start_info.file_size * 100));

        sequence_number = (sequence_number + 1) % 255;
    }

    // End Packet

    if (app_parse_ctrl_packet(buffer, bytes_read, END, &end_info)) {
        fprintf(stderr, "%s: failed to parse end control info\n", __func__);

        if (close(dest_fd) == -1)
            fprintf(stderr, "%s: failed to close destination file\n", __func__);

        free_ctrl_info(&start_info);
        return -1;
    }

    if (end_info.file_name == NULL) {
        fprintf(stderr, "%s: invalid end control info - file name is undefined\n", __func__);

        if (close(dest_fd) == -1)
            fprintf(stderr, "%s: failed to close destination file\n", __func__);

        free_ctrl_info(&start_info);
        return -1;
    }

    if (!(app_match_ctrl_info(&start_info, &end_info))) {
        fprintf(stderr, "%s: control info match failed - start and end packets are not equal\n", __func__);

        if (close(dest_fd) == -1)
            fprintf(stderr, "%s: failed to close destination file\n", __func__);

        free_ctrl_info(&start_info);
        free_ctrl_info(&end_info);
        return -1;
    }

    app_cpy_info_packet(file_info, &start_info);

    #ifdef DEBUG_MESSAGES
    printf("Post Copy File Name: %s\nPost Copy File Size: %ld\n", file_info->file_name, file_info->file_size);
    #endif

    free_ctrl_info(&start_info);
    free_ctrl_info(&end_info);

    if (close(dest_fd) == -1) {
        fprintf(stderr, "%s: failed to close destination file\n", __func__);
        return -1;
    } 

    return 0;

}

int app_send_file(int fd, char *filename) {

    progress = 0;

    if (filename == NULL) {
        fprintf(stderr, "%s: invalid file name\n", __func__);
        return -1;
    }
    
    int file_fd = open(filename, O_RDONLY);

    if (file_fd == -1) {
        fprintf(stderr, "%s: failed to open file to be transfered\n", __func__);
        return -1;
    }

    int file_size = get_file_size(file_fd);

    if (file_size == -1) {
        fprintf(stderr, "%s: failed to get file size\n", __func__);

        if (close(file_fd) == -1)
            fprintf(stderr, "%s: failed to close file to be sent\n", __func__);

        return -1;
    }

    // Start Packet
    app_ctrl_info_t ctrl_info = {
        .file_name = filename,
        .file_size = file_size
    };

    #ifdef DEBUG_APP_CTRL_PACKET
    printf("%s: ctrl_info defined\n\tFile Size : %ld\n\tFile name : %s\n", __func__, ctrl_info.file_size, ctrl_info.file_name);
    #endif

    app_ctrl_packet_t start_packet;

    #ifdef DEBUG_APP_CTRL_PACKET
    printf("%s: creating start control packet\n", __func__);
    #endif

    if (app_create_ctrl_packet(START, &ctrl_info, &start_packet)) {
        fprintf(stderr, "%s: failed to create start control packet\n", __func__);
        return -1;
    }

    #ifdef DEBUG_APP_CTRL_PACKET
    printf("%s: created start control packet\n", __func__);
    #endif

    if (app_send_ctrl_packet(fd, &start_packet)) {
        fprintf(stderr, "%s: failed to send start control packet\n", __func__);

        if (close(file_fd) == -1)
            fprintf(stderr, "%s: failed to close file to be sent\n", __func__);

        free_ctrl_packet(&start_packet);

        return -1;
    }

    #ifdef DEBUG_APP_CTRL_PACKET
    printf("%s: sent start control packet\n", __func__);
    #endif

    // Data Packets
    uint8_t seq_no = 0;
    
    uint8_t data_buff[CHUNK_SIZE];

    int total_bytes_written = 0;

    app_data_packet_t data_packet = {
        .length = 0,
        .packet_data = NULL
    };

    int bytes_read;
    while ((bytes_read = read(file_fd, data_buff, CHUNK_SIZE)) != 0) {
        
        if (bytes_read == -1) {
            fprintf(stderr, "%s: failed to read data from file to be transfered\n", __func__);

            if (close(file_fd) == -1)
                fprintf(stderr, "%s: failed to close file to be sent\n", __func__);

            free_ctrl_packet(&start_packet);

            return -1;
        }

        #ifdef DEBUG_APP_DATA_PACKET
        printf("%s: creating data packet %d\n", __func__, seq_no);
        #endif

        if (app_create_data_packet(seq_no, (char*) data_buff, bytes_read, &data_packet)) {
            fprintf(stderr, "%s: create data packet %d\n", __func__, seq_no);

            if (close(file_fd) == -1)
                fprintf(stderr, "%s: failed to close file to be sent\n", __func__);

            free_ctrl_packet(&start_packet);

            return -1;
        }

        #ifdef DEBUG_APP_DATA_PACKET
        printf("%s: created data packet %d\n", __func__, seq_no);
        #endif

        if (app_send_data_packet(fd, &data_packet)) {
            fprintf(stderr, "%s: failed to send data packet %d\n", __func__, seq_no);

            if (close(file_fd) == -1)
                fprintf(stderr, "%s: failed to close file to be sent\n", __func__);

            free_ctrl_packet(&start_packet);
            free_data_packet(&data_packet);

            return -1;
        }

        #ifdef DEBUG_APP_DATA_PACKET
        printf("%s: sent data packet %d\n", __func__, seq_no);
        #endif

        total_bytes_written += bytes_read;

        app_show_progress_bar(TRANSMITTER, (int)( (float)total_bytes_written / ctrl_info.file_size * 100.0 ));

        seq_no = (seq_no + 1) % 255;
    }

    free_data_packet(&data_packet);

    // End Packet
    app_ctrl_packet_t end_packet;

    #ifdef DEBUG_APP_CTRL_PACKET
    printf("%s: creating end control packet\n", __func__);
    #endif

    if (app_create_ctrl_packet(END, &ctrl_info, &end_packet)) {
        fprintf(stderr, "%s: failed to create end control packet\n", __func__);

        if (close(file_fd) == -1)
            fprintf(stderr, "%s: failed to close file to be sent\n", __func__);

        free_ctrl_packet(&start_packet);

        return -1;
    }

    #ifdef DEBUG_APP_CTRL_PACKET
    printf("%s: created end control packet\n", __func__);
    #endif

    if (app_send_ctrl_packet(fd, &end_packet)) {
        fprintf(stderr, "%s: failed to send end control packet\n", __func__);

        if (close(file_fd) == -1)
            fprintf(stderr, "%s: failed to close file to be sent\n", __func__);

        free_ctrl_packet(&start_packet);
        free_ctrl_packet(&end_packet);

        return -1;
    }

    free_ctrl_packet(&start_packet);
    free_ctrl_packet(&end_packet);

    #ifdef DEBUG_APP_CTRL_PACKET
    printf("%s: sent end control packet\n", __func__);
    #endif

    // Close file
    if (close(file_fd) == -1) {
        fprintf(stderr, "%s: failed to close file transfered\n", __func__);
        return -1;
    }

    return 0;
}
