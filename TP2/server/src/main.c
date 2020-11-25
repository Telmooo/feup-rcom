#include "utils.h"
#include "ftp.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>

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

    struct sockaddr addr;
    socklen_t addrlen = sizeof(addr);

    int socket_acc = accept(socket_fd, &addr, &addrlen);

    if (socket_acc == -1) {
        fprintf(stderr, "%s: failed to accept a connection request\n", __func__);
        return 1;
    }

    

    return 0;
}