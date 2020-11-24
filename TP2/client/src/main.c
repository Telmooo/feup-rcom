#include "utils.h"
#include "ftp.h"

#include <stdio.h>

int main(int argc, char **argv) {

    /**
     * download ftp://[<user>:<password>@]<host>/<url-path>
     */
    if (argc != 2) {
        printf("Wrong usage\n");
        return 1;
    }
    url_info_t url_info;

    url_type_t url_type = parse_url(argv[1], &url_info);

    char *ip = get_ip(url_info.host);
    if (ip == NULL) {
        fprintf(stderr, "%s: failed to get server IP\n", __func__);
        return 1;
    }

    int socket_fd = ftp_open(ip, SERVER_PORT);
    if (socket_fd == -1) {
        fprintf(stderr, "%s: failed to open FTP connection\n", __func__);
        return 1;
    }

    if (wait_for_response(socket_fd, FTP_READY)) {
        fprintf(stderr, "%s: expected %s but did not receive\n", __func__, FTP_READY);
        return 1;
    }

    if (ftp_login(socket_fd, &url_info)) {
        fprintf(stderr, "%s: failed to login\n", __func__);
        return 1;
    }

    return 0;
}

