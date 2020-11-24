#pragma once

#define URL_INFO_MAX_SIZE   256
#define URL_DEFAULT_USER    "anonymous_platypus"
#define URL_DEFAULT_PSWD    "really_strong_password_tm"

char* get_ip(char *host);

typedef struct _url_info url_info_t;

typedef enum _url_type url_type_t;

enum _url_type {
    URL_ERROR,
    URL_USER_PASSWORD,
    URL_USER,
    URL_ANONYM
};

struct _url_info {
    char user[URL_INFO_MAX_SIZE];
    char pswd[URL_INFO_MAX_SIZE];
    char host[URL_INFO_MAX_SIZE];
    char path[URL_INFO_MAX_SIZE];
};

url_type_t parse_url(char *url, url_info_t *url_info);