
#define _GNU_SOURCE
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "comm.h"
#include "frobnicate.h"
#include "ioutils.h"
#include "strings.frs.h"

#define ANCBUF_SIZE 256

void prepare_address(struct sockaddr_un *addr, socklen_t *addrlen) {
    addr->sun_family = AF_UNIX;
    addr->sun_path[0] = 0;
    memcpy(addr->sun_path + 1, socket_addr, FROB_SIZE(sizeof(socket_addr)));
    *addrlen = sizeof(sa_family_t) + FROB_SIZE(sizeof(socket_addr)) + 1;
}

ssize_t recv_message(int fd, struct message *msg, int flags) {
    char recvbuf[8], *recvptr;
    ssize_t ret, rd;
    if (flags & ~_COMM_MASK) {
        errno = EINVAL;
        return -1;
    }
    if (flags & COMM_PEERAUTH) {
        char ancbuf[ANCBUF_SIZE];
        struct iovec bufdesc = { recvbuf, sizeof(recvbuf) };
        struct msghdr hdr = { NULL, 0, &bufdesc, 1,
                              ancbuf, sizeof(ancbuf), 0 };
        struct cmsghdr *cm;
        ret = recvmsg(fd, &hdr, 0);
        if (ret == -1) return -1;
        msg->sender = -1;
        for (cm = CMSG_FIRSTHDR(&hdr); cm; cm = CMSG_NXTHDR(&hdr, cm)) {
            if (cm->cmsg_level != SOL_SOCKET) continue;
            if (cm->cmsg_type == SCM_RIGHTS) {
                int *fds = (int *) CMSG_DATA(cm);
                int len = (cm->cmsg_len - CMSG_LEN(0)) / sizeof(int);
                while (len--) close(*fds++);
            } else if (cm->cmsg_type == SCM_CREDENTIALS) {
                if (msg->sender != -1) continue;
                struct ucred *creds = (struct ucred *) CMSG_DATA(cm);
                msg->sender = creds->uid;
            }
        }
    } else {
        msg->sender = -1;
        ret = 0;
    }
    rd = read_exactly(fd, recvbuf + ret, sizeof(recvbuf) - ret);
    if (rd == -1) return -1;
    ret += rd;
    if (ret == 0) {
        return 0;
    } else if (ret < 8) {
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
    char ancbuf[ANCBUF_SIZE];
    struct iovec bufdesc;
    struct msghdr hdr = { NULL, 0, &bufdesc, 1, ancbuf, 0, 0 };
    ssize_t ret = -1, wr;
    if (flags & ~_COMM_MASK) {
        errno = EINVAL;
        return -1;
    }
    bufdesc.iov_len = msg->length + 8;
    bufdesc.iov_base = malloc(bufdesc.iov_len);
    if (bufdesc.iov_base == NULL) return -1;
    ((union intcast *) bufdesc.iov_base)[0].num = htonl(msg->key);
    ((union intcast *) bufdesc.iov_base)[1].num = htonl(msg->length);
    memcpy(bufdesc.iov_base + 8, msg->data, msg->length);
    if (! (flags & COMM_NOSCRAMBLE))
        frobl(msg->key, bufdesc.iov_base + 8, bufdesc.iov_base + 8,
              msg->length);
    if (flags & COMM_PEERAUTH) {
        // Assuming the system calls are infallible
        struct ucred creds = { getpid(), getuid(), getgid() };
        struct cmsghdr *cm;
        hdr.msg_controllen = CMSG_SPACE(sizeof(struct ucred));
        memset(ancbuf, 0, hdr.msg_controllen);
        cm = CMSG_FIRSTHDR(&hdr);
        cm->cmsg_len = CMSG_LEN(sizeof(struct ucred));
        cm->cmsg_level = SOL_SOCKET;
        cm->cmsg_type = SCM_CREDENTIALS;
        memcpy(CMSG_DATA(cm), &creds, sizeof(creds));
        ret = sendmsg(fd, &hdr, 0);
        if (ret == -1) goto end;
    } else {
        ret = 0;
    }
    wr = write_exactly(fd, bufdesc.iov_base + ret, bufdesc.iov_len - ret);
    ret = (wr == -1) ? -1 : ret + wr;
    end:
        free(bufdesc.iov_base);
        return ret;
}
