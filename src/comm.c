
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "comm.h"

int send_message(int fd, const struct message *msg, int flags) {
    char *buf;
    int ret = -1;
    if (flags) {
        errno = EINVAL;
        return -1;
    }
    buf = malloc(msg->length + 8);
    if (buf == NULL) return -1;
    ((union intcast *) buf)[0].num = htonl(msg->key);
    ((union intcast *) buf)[1].num = htonl(msg->length);
    memcpy(buf + 8, msg->data, msg->length);
    ret = write(fd, buf, msg->length + 8);
    free(buf);
    return ret;
}

int recv_message(int fd, struct message *msg, int flags) {
    char recvbuf[65536], *recvptr;
    int ret;
    if (flags) {
        errno = EINVAL;
        return -1;
    }
    ret = recv(fd, recvbuf, sizeof(recvbuf), MSG_PEEK);
    if (ret == -1) return ret;
    if (ret < 8) {
        errno = EBADMSG;
        return -1;
    }
    // Workaround for strict aliasing rules.
    recvptr = recvbuf;
    msg->key = ntohl(((union intcast *) recvptr)[0].num);
    msg->length = ntohl(((union intcast *) recvptr)[1].num);
    if (ret != msg->length + 8) {
        errno = EBADMSG;
        return -1;
    }
    if (msg->length == 0) {
        free(msg->data);
        msg->data = NULL;
    } else {
        realloc(msg->data, msg->length);
        if (msg->data == NULL) return -1;
        memcpy(msg->data, recvbuf + 8, msg->length);
    }
    return ret;
}
