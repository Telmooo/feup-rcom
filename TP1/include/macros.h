#pragma once

// #ifndef DEBUG_STATE_MACHINE
// #define DEBUG_STATE_MACHINE
// #endif

// #ifndef DEBUG_MESSAGES
// #define DEBUG_MESSAGES
// #endif

// #ifndef DEBUG_APP_CTRL_PACKET
// #define DEBUG_APP_CTRL_PACKET
// #endif

// #ifndef DEBUG_APP_DATA_PACKET
// #define DEBUG_APP_DATA_PACKET
// #endif

// #ifndef OVERRIDE_REC_FILE_NAME
// #define OVERRIDE_REC_FILE_NAME "received.mp4"
// #endif

#define BAUDRATE 0xB38400
// #define MODEMDEVICE "/dev/ttyS1"
// #define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define LL_TIMEOUT 1
#define LL_RETRIES 3

#define DATA_DEFAULT_SIZE 256

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
// N s either 1 or 0
#define C_INFORMATION(N) (N << 6) 

// R 0 0 0 0 1 0 1
// N is either 1 or 0
#define C_RR(N) ((char)(0x05 | (N << 7)))

// R 0 0 0 0 0 0 1
// N is either 1 or 0
#define C_REJ(N) ((char)(0x01 | (N << 7)))

// Used on our custom funtions (doesn't collide with anything else)
#define READ_FRAME_IGNORE_CHECK ((char)0xFF)

#define CONTROL_FRAME_SIZE 5

#define FRAME_MAX_SIZE 256
#define LL_HEADERS 6
#define APP_HEADERS 4
#define STUFFED_MAX_SIZE ((FRAME_MAX_SIZE - LL_HEADERS + 1) * 2 + LL_HEADERS - 1)
#define UNSTUFFED_MAX_SIZE (FRAME_MAX_SIZE - LL_HEADERS + 1)
#define CHUNK_SIZE (FRAME_MAX_SIZE - LL_HEADERS - APP_HEADERS)

#define READ_FRAME_OK 0
#define READ_FRAME_ERROR -1
#define READ_FRAME_WAS_INTERRUPTED -2

#define ESCAPE 0x7d
#define ESCAPED_FLAG 0x5e
#define ESCAPED_ESCAPE 0x5d

#define BUF_SIZE 256
