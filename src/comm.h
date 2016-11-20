
/* Client-server communication */

#ifndef _COMM_H
#define _COMM_H

#include <inttypes.h>

struct message {
    uint32_t key;
    uint32_t length;
    uint8_t *data;
};

union intcast {
    uint32_t num;
    char data[4];
};

/* Send a message into the given file descriptor
 * Since none are defined, flags must be zero. */
int send_message(int fd, const struct message *msg, int flags);

/* Receive a message from the given file descriptor
 * Since none are defined, flags must be zero. */
int recv_message(int fd, struct message *msg, int flags);

#endif
