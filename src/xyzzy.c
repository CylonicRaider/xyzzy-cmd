
/* The purpose of this is enigmatic. */

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "client.h"
#include "comm.h"
#include "frobnicate.h"
#include "ioutils.h"
#include "server.h"
#include "status.h"
#include "strings.frs.h"
#include "userhash.h"
#include "xyzzy.h"

static int urandom_fd = -1;

void init_strings() {
    struct frobstring *p;
    for (p = strings; p->str != NULL; p++) {
        defrobl(p->key, p->str, p->str, p->len + 1);
    }
}

int mkrand(void *buf, ssize_t len) {
    int rd;
    if (urandom_fd == -1) {
        urandom_fd = open(dev_urandom, O_RDONLY);
        if (urandom_fd == -1) return -1;
    }
    rd = read(urandom_fd, buf, len);
    if (rd == -1) return -1;
    if (rd != len) {
        errno = EIO;
        return -1;
    }
    return rd;
}

int send_msg_rnd(int fd, char *buf, size_t buflen) {
    struct message msg = { 0, buflen, buf };
    if (mkrand(&msg.key, sizeof(msg.key)) == -1)
        return -1;
    return send_message(fd, &msg, 0);
}
int recv_msg_rnd(int fd, char **buf, size_t *buflen) {
    struct message msg = {};
    if (recv_message(fd, &msg, 0) == -1)
        return -1;
    *buflen = msg.length;
    *buf = msg.data;
    return 0;
}

int server_handler(int fd, void *data) {
    close(fd);
    return 0;
}

int main(int argc, char *argv[]) {
    enum main_action act = ERROR;
    int statusact = 0, sockfd;
    init_strings();
    if (argc <= 1) {
        act = STATUS;
    } else if (strcmp(argv[1], cmd_help) == 0 ||
               strcmp(argv[1], cmd_help + 2) == 0) {
        xprintf(STDERR_FILENO, usage_tmpl, PROGNAME, usage_list);
        return 0;
    } else if (strcmp(argv[1], cmd_on) == 0) {
        act = STATUS;
        statusact = STATUSCTL_ENABLE;
    } else if (strcmp(argv[1], cmd_off) == 0) {
        act = STATUS;
        statusact = STATUSCTL_DISABLE;
    } else if (strcmp(argv[1], cmd_status) == 0) {
        act = STATUS;
    } else if (strcmp(argv[1], cmd_read) == 0) {
        act = READ;
    } else if (strcmp(argv[1], cmd_write) == 0) {
        act = WRITE;
    } else if (strcmp(argv[1], cmd_ping) == 0) {
        act = PING;
    } else {
        xprintf(STDERR_FILENO, usage_tmpl, PROGNAME, usage_list);
        return 1;
    }
    sockfd = client_connect();
    if (sockfd == -1) {
        struct userhash uh;
        if (errno != ECONNREFUSED)
            return EXIT_ERRNO;
        userhash_init(&uh);
        if (server_spawn(argc, argv, &server_handler, &uh) == -1)
            return EXIT_ERRNO;
        sockfd = client_connect();
        if (sockfd == -1)
            return EXIT_ERRNO;
    }
    xprintf(STDOUT_FILENO, "%d %d\n", act, statusact);
    if (urandom_fd != -1) close(urandom_fd);
    return 0;
}
