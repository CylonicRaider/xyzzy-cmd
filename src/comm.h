
#ifndef _COMM_H
#define _COMM_H

#include <inttypes.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#define COMM_NOSCRAMBLE 1 /* Do not (de)scramble the data automatically */

#define _COMM_MASK 1 /* Allowed COMM_* values */

struct message {
    uint32_t key;
    uint32_t length;
    void *data;
};

union intcast {
    uint32_t num;
    char data[4];
};

/* Write server socket address into the given structure
 * An infallible convenience function. */
void prepare_address(struct sockaddr_un *addr, socklen_t *addrlen);

/* Send a message into the given file descriptor
 * Flags is a bitwise OR of COMM_* constants (or zero). */
int send_message(int fd, const struct message *msg, int flags);

/* Receive a message from the given file descriptor
 * Flags is a bitwise OR of COMM_* constants (or zero). */
int recv_message(int fd, struct message *msg, int flags);

#endif
