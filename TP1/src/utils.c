#include  "../include/utils.h"

#include <stdlib.h>

#include "../include/macros.h"

char* trama_supervisao(char A, char C) {
    char *trama = malloc(5 * sizeof(char));
    trama[0] = FLAG;
    trama[1] = A;
    trama[2] = C;
    trama[3] = A ^ C;
    trama[4] = FLAG;
    return trama;
}

bool verify_trama_supervisao(char *trama) {
    if (trama == NULL)
        return false;
    
    return trama != NULL ? (trama[1] ^ trama[2]) == trama[3] : false;
}
