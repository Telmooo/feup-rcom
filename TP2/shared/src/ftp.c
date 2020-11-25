#include "ftp.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>

int ftp_open_client(char *ip, int port) {
    
    struct sockaddr_in server_addr;
    bzero((char*)&server_addr, sizeof(server_addr));
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

int ftp_open_server(char *ip, int port) {
    
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

    if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        fprintf(stderr, "%s: failed to bind address to socket\n", __func__);
        return -1;
    }

    if (listen(sock_fd, 1) == -1) {
        fprintf(stderr, "%s: failed to set maximum length for queue of connections for socket\n", __func__);
        return -1;
    }

    return sock_fd;
}

int ftp_send_response(int fd, const char *status_code, char *response) {

    char buffer[RESPONSE_MAX_SIZE];
    sprintf(buffer, "%5s %s", status_code, response);
    int len = strlen(buffer);
    if (send(fd, buffer, len, 0) != len) {
        fprintf(stderr, "%s: failed to send the full response\n", __func__);
        return -1;
    }
    return 0;
}

int ftp_match_response(const char *match, const char *response) {
    char status_code[8];
    sscanf(response, "%5[^ ]", status_code);
    return strcmp(match, status_code) == 0;
}

int ftp_wait_for_response(int fd, const char *response) {

    char buffer[RESPONSE_MAX_SIZE];

    int bytes_read = 0;

    while (ftp_match_response(response, buffer) == 0) {
        memset(buffer, 0, bytes_read);
        bytes_read = recv(fd, buffer, RESPONSE_MAX_SIZE, 0);

        if (bytes_read == -1) {
            fprintf(stderr, "%s: failed to read from socket\n", __func__);
            return -1;
        }

        if (bytes_read == 0) {
            fprintf(stderr, "%s: didn't get expected response\n", __func__);
            return -1;
        }
    }
    return 0;
}

int ftp_get_response(int fd, char *response) {
    memset(response, 0, RESPONSE_MAX_SIZE * sizeof(char));

    int bytes_read = recv(fd, response, RESPONSE_MAX_SIZE, 0);

    if (bytes_read == -1) {
        fprintf(stderr, "%s: failed to read from socket\n", __func__);
        return -1;
    }

    if (bytes_read == 0) {
        fprintf(stderr, "%s: didn't get any response\n", __func__);
        return -1;
    }
    return bytes_read;
}

int ftp_login(int fd, url_info_t *ftp_info) {

    char buffer[RESPONSE_MAX_SIZE];
    int msg_length;

    sprintf(buffer, "user %s\n", ftp_info->user);

    msg_length = strlen(buffer);
    printf("> %s", buffer);
    if (send(fd, buffer, msg_length, 0) != msg_length) {
        fprintf(stderr, "%s: write failed to send user properly\n", __func__);
        return -1;
    }

    if (ftp_wait_for_response(fd, FTP_NEED_PSWD)) {
        fprintf(stderr, "%s: expected %s but did not receive\n", __func__, FTP_NEED_PSWD);
        return -1;
    }

    memset(buffer, 0, msg_length);

    sprintf(buffer, "pass %s\n", ftp_info->pswd);

    msg_length = strlen(buffer);
    if (send(fd, buffer, msg_length, 0) != msg_length) {
        fprintf(stderr, "%s: write failed to send password properly\n", __func__);
        return -1;
    }

    if (ftp_wait_for_response(fd, FTP_LOGIN_SUCC)) {
        fprintf(stderr, "%s: login failed, expected %s but did not receive\n", __func__, FTP_LOGIN_SUCC);
        return -1;
    }

    return 0;
}

int ftp_enter_passive_mode(int fd, ftp_psv_mode_info_t *psv_mode_info) {

    char buffer[RESPONSE_MAX_SIZE];
    int msg_length;

    sprintf(buffer, "pasv\n");
    msg_length = strlen(buffer);
    printf("> %s", buffer);
    if (send(fd, buffer, msg_length, 0) != msg_length) {
        fprintf(stderr, "%s: write failed to enter passive mode\n", __func__);
        return -1;
    }

    if (ftp_wait_for_response(fd, FTP_PASSIVE)) {
        fprintf(stderr, "%s: failed to enter passive mode, expected %s but did not receive\n", __func__, FTP_PASSIVE);
        return -1;
    }

    int ip_1, ip_2, ip_3, ip_4, port_high, port_low;

    char response[RESPONSE_MAX_SIZE];

    int bytes_read = ftp_get_response(fd, response);

    if (bytes_read == -1) {
        fprintf(stderr, "%s: failed to retrieve response from socket\n", __func__);
        return -1;
    }

    if (sscanf(response, "%*[^(](%d,%d,%d,%d,%d,%d)", &ip_1, &ip_2, &ip_3, &ip_4, &port_high, &port_low) != 6) {
        fprintf(stderr, "%s: failed to get IP and Port to open data socket\n", __func__);
        return -1;
    }

    sprintf(psv_mode_info->ip, "%d.%d.%d.%d", ip_1, ip_2, ip_3, ip_4);
    psv_mode_info->port = (port_high << 8) + port_low;

    return 0;
}

int ftp_retrieve_file(int fd, char *path) {
    char buffer[RESPONSE_MAX_SIZE];
    int msg_length;

    sprintf(buffer, "retr %s\n", path);
    msg_length = strlen(buffer);
    printf("> %s", buffer);

    if (send(fd, buffer, msg_length, 0) != msg_length) {
        fprintf(stderr, "%s: write failed to send file request\n", __func__);
        return -1;
    }

    char response[RESPONSE_MAX_SIZE];

    int bytes_read = ftp_get_response(fd, response);

    if (bytes_read == -1) {
        fprintf(stderr, "%s: failed to retrieve response from socket\n", __func__);
        return -1;
    }

    return (ftp_match_response(FTP_TRANSF_START, response) || ftp_match_response(FTP_OPEN_DATA_CONNECT, response)) ? 0 : -1;
}

int ftp_read_file(int fd, char *path) {

    char *last_slash = strrchr(path, '/');
    char *filename;
    if (last_slash == NULL) filename = path;
    else                    filename = &(last_slash[1]);

    int write_file = open(filename, O_WRONLY | O_CREAT, 0660);

    if (write_file == -1) {
        fprintf(stderr, "%s: failed to create destination file\n", __func__);
        return -1;
    }

    char buffer[FILE_BUFFER_MAX_SIZE];

    int bytes_read = recv(fd, buffer, FILE_BUFFER_MAX_SIZE, 0);
    while (bytes_read != 0) {

        if (bytes_read == -1) {
            fprintf(stderr, "%s: occurred an error while downloading file\n", __func__);
            close(write_file);
            return -1;
        }

        if (write(write_file, buffer, bytes_read) != bytes_read) {
            fprintf(stderr, "%s: failed to write received data to file\n", __func__);
            close(write_file);
            return -1;
        }

        bytes_read = recv(fd, buffer, FILE_BUFFER_MAX_SIZE, 0);
    }

    if (close(write_file) == -1) {
        fprintf(stderr, "%s: failed to close destination file\n", __func__);
        return -1;
    }

    return 0;
}

int ftp_close_client(int fd) {
    char buffer[RESPONSE_MAX_SIZE] = "quit\n";
    int msg_length = strlen(buffer);
    printf("> %s", buffer);
    if (send(fd, buffer, msg_length, 0) != msg_length) {
        fprintf(stderr, "%s: write failed to close socket connection\n", __func__);
        return -1;
    }

    if (ftp_wait_for_response(fd, FTP_CLOSE_CONNECT)) {
        fprintf(stderr, "%s: expected %s but did not receive\n", __func__, FTP_CLOSE_CONNECT);
        return -1;
    }

    return 0;
}