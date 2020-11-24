#include "utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>

char* get_ip(char *host) {
    struct hostent *h;
    if ((h = gethostbyname(host)) == NULL) {
        herror("Failed to get host name\n");
        return NULL;
    }

    return inet_ntoa(*((struct in_addr *)h->h_addr));
}

url_type_t parse_url(char *url, url_info_t *url_info) {
    // ftp://[<user>:<password>@]<host>/<url-path>
    if (sscanf(url, "ftp://%[^:]:%[^@]@%[^/]/%[^\n]", url_info->user, url_info->pswd, url_info->host, url_info->path) == 4) {
        return URL_USER_PASSWORD;
    }
    else if (sscanf(url, "ftp://%[^@]@%[^/]/%[^\n]", url_info->user, url_info->host, url_info->path) == 3) {
        strcpy(url_info->pswd, URL_DEFAULT_PSWD);
        return URL_USER;
    }
    else if (sscanf(url, "ftp://%[^/]/%[^\n]", url_info->host, url_info->path) == 2) {
        strcpy(url_info->user, URL_DEFAULT_USER);
        strcpy(url_info->pswd, URL_DEFAULT_PSWD);
        return URL_ANONYM;
    }
    return URL_ERROR;
}