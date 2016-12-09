
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

/* Read exactly len bytes from fd, taking as many attempts as necessary
 * Returns the amount of bytes read (less than len on EOF), or -1 on error
 * (having errno set).
 * If len is zero, this returns instant success. */
ssize_t read_exactly(int fd, void *buf, size_t len);

/* Write exactly len bytes to fd
 * Returns the amount of bytes written (i.e. len), or -1 on error (having
 * errno set); if a single system call writes zero bytes, EBUSY is raised.
 * If len is zero, this returns instant success. */
ssize_t write_exactly(int fd, void *buf, size_t len);

/* Send a message into the given file descriptor
 * Flags is a bitwise OR of COMM_* constants (or zero).
 * Returns the amount of bytes read, or -1 on error (setting errno).
 * NOTE that this can use multiple system calls. */
ssize_t send_message(int fd, const struct message *msg, int flags);

/* Receive a message from the given file descriptor
 * Flags is a bitwise OR of COMM_* constants (or zero).
 * Returns the amount of bytes written, or -1 on error (setting errno).
 * NOTE that this can use multiple system calls, and that their pattern
 *      may not align with that of send_message() (i.e. don't use those
 *      on packet sockets). */
ssize_t recv_message(int fd, struct message *msg, int flags);

#endif
