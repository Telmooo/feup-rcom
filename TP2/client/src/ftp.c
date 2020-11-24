#include "ftp.h"

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>

int ftp_open(char *ip, int port) {
    
    struct sockaddr_in server_addr;
    bzero((char*)&server_addr, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(SERVER_PORT);

    int sock_fd;
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "%s: Failed to open socket\n", __func__);
        return -1;
    }

    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "%s: Failed to connect socket\n", __func__);
        return -1;
    }

    return sock_fd;
}

int wait_for_response(int fd, const char *response) {

    char buffer[RESPONSE_MAX_SIZE];
    char status_code[8];

    int bytes_read;

    while (strcmp(status_code, response) != 0) {

        bytes_read = read(fd, buffer, RESPONSE_MAX_SIZE);

        if (bytes_read == -1) {
            fprintf(stderr, "%s: failed to read from socket\n", __func__);
            return -1;
        }

        if (bytes_read == 0) {
            fprintf(stderr, "%s: didn't get expected response\n", __func__);
            return -1;
        }

        sscanf(buffer, "%5[^ ]", status_code);
    }
    return 0;
}

int ftp_login(int fd, url_info_t *ftp_info) {

    char buffer[RESPONSE_MAX_SIZE];
    int msg_length;

    sprintf(buffer, "user %s\n", ftp_info->user);

    msg_length = strlen(buffer);
    if (write(fd, buffer, msg_length) != msg_length) {
        fprintf(stderr, "%s: write failed to send user properly\n", __func__);
        return -1;
    }

    if (wait_for_response(fd, FTP_NEED_PSWD)) {
        fprintf(stderr, "%s: expected %s but did not receive\n", __func__, FTP_NEED_PSWD);
        return 1;
    }

    memset(buffer, 0, msg_length);

    sprintf(buffer, "pass %s\n", ftp_info->pswd);

    msg_length = strlen(buffer);
    if (write(fd, buffer, msg_length) != msg_length) {
        fprintf(stderr, "%s: write failed to send password properly\n", __func__);
        return -1;
    }

    if (wait_for_response(fd, FTP_LOGIN_SUCC)) {
        fprintf(stderr, "%s: login failed, expected %s but did not receive\n", __func__, FTP_LOGIN_SUCC);
        return 1;
    }

    return 0;
}

int ftp_enter_passive_mode(int fd, ftp_psv_mode_info_t *psv_mode_info) {

    char buffer[RESPONSE_MAX_SIZE];
    int msg_length;

    sprintf(buffer, "pasv\n");
    msg_length = strlen(buffer);

    if (write(fd, buffer, msg_length) != msg_length) {
        fprintf(stderr, "%s: write failed to enter passive mode\n", __func__);
        return -1;
    }

    if (wait_for_response(fd, FTP_PASSIVE)) {
        fprintf(stderr, "%s: failed to enter passive mode, expected %s but did not receive\n", __func__, FTP_PASSIVE);
        return 1;
    }

    //  227 Entering Passive Mode (193,136,28,12,19,91)
    //int ip_1, ip_2, ip_3, ip_4, port_high, port_low;
    return -1;
}