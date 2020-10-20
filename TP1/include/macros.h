#pragma once

#define BAUDRATE B38400
// #define MODEMDEVICE "/dev/ttyS1"
// #define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define LL_TIMEOUT 1
#define LL_RETRIES 3

#define DATA_MAX_SIZE 256

#define FLAG 0x7E

// Sent by emissor
#define A_EMISSOR 0x03
// Responses sent by the receptor
#define A_RECEPTOR_RESPONSE 0x03
// Sent by receptor
#define A_RECEPTOR 0x01
// Response sent by the emissor
#define A_EMISSOR_RESPONSE 0x01

#define C_SET 0x03
#define C_DISC 0x11
#define C_UA 0x07

#define BIT(N) (0x01 << N)

// 0 S 0 0 0 0 0 0
// Is either 1 or 0
#define C_INFORMATION(N) (N << 7) 

// R 0 0 0 0 1 0 1
// Is either 1 or 0
#define RR(N) (0x05 | (N << 8))

// R 0 0 0 0 0 0 1
// Is either 1 or 0
#define REJ(N) (0x01 | (N << 8))

#define ESCAPE 0x7d

#define BUF_SIZE 256
