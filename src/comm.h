
/* Client-server communication */

#ifndef _COMM_H
#define _COMM_H

#define COMM_NOSCRAMBLE 1 /* Do not scramble the data sent */

#define _COMM_MASK 1 /* Allowed COMM_* values */

#include <inttypes.h>

struct message {
    uint32_t key;
    uint32_t length;
    char *data;
};

union intcast {
    uint32_t num;
    char data[4];
};

/* Send a message into the given file descriptor
 * Flags is a bitwise OR of COMM_* constants (or zero). */
int send_message(int fd, const struct message *msg, int flags);

/* Receive a message from the given file descriptor
 * Flags is a bitwise OR of COMM_* constants (or zero). */
int recv_message(int fd, struct message *msg, int flags);

#endif
