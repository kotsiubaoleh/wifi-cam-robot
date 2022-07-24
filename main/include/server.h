#ifndef SERVER_H
#define SERVER_H

void server_init(void);

enum message_type {
    STREAM_START = 1,
    SET_FRAMESIZE = 2,
    SET_QUALITY = 3
}

typedef struct {
    uint8_t type;
    int32_t value;
} message;

#endif