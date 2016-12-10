
#ifndef _COMM_H
#define _COMM_H

#include <inttypes.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#define COMM_NOSCRAMBLE 1 /* Do not (de)scramble the data automatically */
#define COMM_PEERAUTH   2 /* Send/receive authentication of peer */

#define _COMM_MASK 3 /* Allowed COMM_* values */

struct message {
    uint32_t key;
    uint32_t length;
    void *data;
    uid_t sender;
};

union intcast {
    uint32_t num;
    char data[4];
};

/* Write server socket address into the given structure
 * An infallible convenience function. */
void prepare_address(struct sockaddr_un *addr, socklen_t *addrlen);

/* Receive a message from the given file descriptor
 * Flags is a bitwise OR of COMM_* constants (or zero).
 * Returns the amount of bytes written, or -1 on error (setting errno).
 * NOTE that this can use multiple system calls, and that their pattern
 *      may not align with that of send_message() (i.e. don't use those
 *      on packet sockets). */
ssize_t recv_message(int fd, struct message *msg, int flags);

/* Send a message into the given file descriptor
 * Flags is a bitwise OR of COMM_* constants (or zero).
 * Returns the amount of bytes read, or -1 on error (setting errno).
 * NOTE that this can use multiple system calls. */
ssize_t send_message(int fd, const struct message *msg, int flags);

#endif
