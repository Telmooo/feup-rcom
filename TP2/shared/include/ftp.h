#pragma once

#include "utils.h"

#define SERVER_PORT 21

#define RESPONSE_MAX_SIZE   512

#define FTP_TRANSF_START        "125"
#define FTP_OPEN_DATA_CONNECT   "150"

#define FTP_READY               "220"
#define FTP_CLOSE_CONNECT       "221"
#define FTP_PASSIVE             "227"
#define FTP_LOGIN_SUCC          "230"

#define FTP_NEED_PSWD           "331"

#define FTP_FAILED_DATA_CONNECT "425"
#define FTP_TRANSF_ABORT        "426"

#define FTP_FILE_INVALID        "550"

typedef struct _ftp_pasv_mode_info ftp_psv_mode_info_t;

struct _ftp_pasv_mode_info {
    char ip[RESPONSE_MAX_SIZE];    
    int port;
};

int ftp_open_client(char *ip, int port);

int ftp_open_server(char *ip, int port);

int ftp_send_response(int fd, const char *status_code, char *response);

int ftp_match_response(const char *match, const char *response);

int ftp_wait_for_response(int fd, const char *match, char *response);

int ftp_get_response(int fd, char *response);

int ftp_login(int fd, url_info_t *ftp_info);

int ftp_enter_passive_mode(int fd, ftp_psv_mode_info_t *psv_mode_info);

int ftp_retrieve_file(int fd, char *path);

int ftp_read_file(int fd, char *path);

int ftp_close_client(int fd);