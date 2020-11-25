#include "utils.h"
#include "ftp.h"

#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {

    /**
     * download ftp://[<user>:<password>@]<host>/<url-path>
     */
    if (argc != 2) {
        fprintf(stdout, "%s: invalid usage\n"
                        "   Program usage:\n"
                        "       %s ftp://[<user>:<password>@]<host>/<url-path>\n"
                        "       -> user: username used for login (optional)\n"
                        "       -> password: password used for login (optional)\n"
                        "       -> host: server host where file is hosted\n"
                        "       -> url-path: path to file in host server\n"
                , argv[0], argv[0]);
        return 0;
    }
    url_info_t url_info;

    url_type_t url_type = parse_url(argv[1], &url_info);

    if (url_type == URL_ERROR) {
        fprintf(stdout, "%s: invalid usage\n"
                        "   Program usage:\n"
                        "       %s ftp://[<user>:<password>@]<host>/<url-path>\n"
                        "       -> user: username used for login (optional)\n"
                        "       -> password: password used for login (optional)\n"
                        "       -> host: server host where file is hosted\n"
                        "       -> url-path: path to file in host server\n"
                , argv[0], argv[0]);
        return 0;
    }

    char *ip = get_ip(url_info.host);
    if (ip == NULL) {
        fprintf(stderr, "%s: failed to get server IP\n", argv[0]);
        return 1;
    }

    int socket_fd = ftp_open_client(ip, SERVER_PORT);
    if (socket_fd == -1) {
        fprintf(stderr, "%s: failed to open FTP connection\n", argv[0]);
        return 1;
    }

    if (ftp_wait_for_response(socket_fd, FTP_READY)) {
        fprintf(stderr, "%s: expected %s but did not receive\n", argv[0], FTP_READY);
        close(socket_fd);
        return 1;
    }

    if (ftp_login(socket_fd, &url_info)) {
        fprintf(stderr, "%s: failed to login\n", argv[0]);
        close(socket_fd);
        return 1;
    }
    
    ftp_psv_mode_info_t data_sock_info;

    if (ftp_enter_passive_mode(socket_fd, &data_sock_info) == -1) {
        fprintf(stderr, "%s: failed to enter passive mode\n", argv[0]);
        close(socket_fd);
        return -1;
    }

    int data_sock_fd = ftp_open_client(data_sock_info.ip, data_sock_info.port);

    if (data_sock_fd == -1) {
        fprintf(stderr, "%s: failed to open data socket connection\n", argv[0]);
        return -1;
    }

    if (ftp_retrieve_file(socket_fd, url_info.path) == -1) {
        fprintf(stderr, "%s: failed to request file from server\n", argv[0]);
        close(data_sock_fd);
        close(socket_fd);
        return -1;
    }

    if (ftp_read_file(data_sock_fd, url_info.path) == -1) {
        fprintf(stderr, "%s: failed to download file\n", argv[0]);
        close(data_sock_fd);
        close(socket_fd);
        return -1;
    }

    if (ftp_close_client(socket_fd) == -1) {
        fprintf(stderr, "%s: failed to close connection\n", argv[0]);
        close(data_sock_fd);
        close(socket_fd);
        return -1;
    }

    if (close(data_sock_fd) == -1) {
        fprintf(stderr, "%s: failed to close data socket\n", argv[0]);
        close(socket_fd);
        return -1;
    }

    if (close(socket_fd) == -1) {
        fprintf(stderr, "%s: failed to close data socket\n", argv[0]);
        return -1;
    }

    return 0;
}

