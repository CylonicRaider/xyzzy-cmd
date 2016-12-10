
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "comm.h"
#include "frobnicate.h"
#include "strings.frs.h"

void prepare_address(struct sockaddr_un *addr, socklen_t *addrlen) {
    addr->sun_family = AF_UNIX;
    addr->sun_path[0] = 0;
    memcpy(addr->sun_path + 1, socket_addr, sizeof(socket_addr) - 2);
    *addrlen = sizeof(sa_family_t) + sizeof(socket_addr) - 1;
}

ssize_t read_exactly(int fd, void *buf, size_t len) {
    char *ptr = buf;
    ssize_t ret = 0;
    if (len == 0) return 0;
    while (ret != len) {
        ssize_t rd = read(fd, ptr + ret, len - ret);
        if (rd == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (rd == 0) break;
        ret += rd;
    }
    return ret;
}

ssize_t write_exactly(int fd, const void *buf, size_t len) {
    const char *ptr = buf;
    ssize_t ret = 0;
    if (len == 0) return 0;
    while (ret != len) {
        ssize_t wr = write(fd, ptr + ret, len - ret);
        if (wr == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (wr == 0) {
            errno = EBUSY;
            return -1;
        }
        ret += wr;
    }
    return ret;
}

ssize_t recv_message(int fd, struct message *msg, int flags) {
    char recvbuf[8], *recvptr;
    ssize_t ret;
    if (flags & ~_COMM_MASK) {
        errno = EINVAL;
        return -1;
    }
    ret = read_exactly(fd, recvbuf, sizeof(recvbuf));
    if (ret == -1) return ret;
    if (ret < 8) {
        errno = EBADMSG;
        return -1;
    }
    // Workaround for strict aliasing rules.
    recvptr = recvbuf;
    msg->key = ntohl(((union intcast *) recvptr)[0].num);
    msg->length = ntohl(((union intcast *) recvptr)[1].num);
    if (msg->length == 0) {
        free(msg->data);
        msg->data = NULL;
    } else {
        size_t rd;
        msg->data = realloc(msg->data, msg->length);
        if (msg->data == NULL) return -1;
        rd = read_exactly(fd, msg->data, msg->length);
        if (rd == -1) return -1;
        if (rd < msg->length) {
            errno = EBADMSG;
            return -1;
        }
        if (! (flags & COMM_NOSCRAMBLE))
            defrobl(msg->key, msg->data, msg->data, msg->length);
    }
    return ret;
}

ssize_t send_message(int fd, const struct message *msg, int flags) {
    char *buf;
    ssize_t ret = -1;
    if (flags & ~_COMM_MASK) {
        errno = EINVAL;
        return -1;
    }
    buf = malloc(msg->length + 8);
    if (buf == NULL) return -1;
    ((union intcast *) buf)[0].num = htonl(msg->key);
    ((union intcast *) buf)[1].num = htonl(msg->length);
    memcpy(buf + 8, msg->data, msg->length);
    if (! (flags & COMM_NOSCRAMBLE))
        frobl(msg->key, buf + 8, buf + 8, msg->length);
    ret = write_exactly(fd, buf, msg->length + 8);
    free(buf);
    return ret;
}
