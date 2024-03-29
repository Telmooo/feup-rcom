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

// Print progress of the file transfer:
// -> Undefined: None
// -> 0: Print at least every 10%
// -> 1: Dynamic percentage only
// -> 2: Actual progress bar
#define PROGRESS_BAR 2

// #ifndef OVERRIDE_REC_FILE_NAME
// #define OVERRIDE_REC_FILE_NAME "received.mp4"
// #endif

// Chance for I frame errors [0, 1] PER FRAME
// #define RECEIVER_HEADER_ERROR_RATE 0.1
// Chance for I frame errors [0, 1] PER FRAME
// #define RECEIVER_INFO_ERROR_RATE 0.1

// Set the same fixed seed for srand
// #define SET_SEED 12345678

// Set propagation time (microseconds) (simulated)
// #define T_PROP   200000

#define BAUDRATE B38400


// #define MODEMDEVICE "/dev/ttyS1"
// #define _POSIX_SOURCE 1 /* POSIX compliant source */


#define LL_TIMEOUT 2
#define LL_RETRIES 3

#define FALSE 0
#define TRUE 1

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

// Defining as "Max size without stuffing"
#define FRAME_MAX_SIZE 256
#define LL_HEADERS 5
#define LL_HEADERS_BCC2 6
#define APP_HEADERS 4
#define STUFFED_MAX_SIZE ((FRAME_MAX_SIZE - LL_HEADERS) * 2 + LL_HEADERS)
#define UNSTUFFED_MAX_SIZE (FRAME_MAX_SIZE - LL_HEADERS)
#define CHUNK_SIZE (FRAME_MAX_SIZE - LL_HEADERS_BCC2 - APP_HEADERS)

#define READ_FRAME_OK 0
#define READ_FRAME_ERROR -1
#define READ_FRAME_WAS_INTERRUPTED -2

#define ESCAPE 0x7d
#define ESCAPED_FLAG 0x5e
#define ESCAPED_ESCAPE 0x5d

