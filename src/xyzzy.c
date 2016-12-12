
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
static int alarmed = 0;

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

int send_packet(int fd, char *buf, size_t buflen) {
    struct message msg = { 0, buflen, buf, -1 };
    if (mkrand(&msg.key, sizeof(msg.key)) == -1)
        return -1;
    return send_message(fd, &msg, COMM_PEERAUTH);
}
int recv_packet(int fd, char **buf, size_t *buflen, uid_t *uid) {
    struct message msg = { 0, 0, NULL, -1 };
    if (recv_message(fd, &msg, COMM_PEERAUTH) == -1)
        return -1;
    *buflen = msg.length;
    *buf = msg.data;
    if (uid != NULL) *uid = msg.sender;
    return 0;
}

void alarm_handler(int signo) {
    alarmed = 1;
}
int install_handler(int enable) {
    if (enable) {
        struct sigaction act;
        memset(&act, 0, sizeof(act));
        sigemptyset(&act.sa_mask);
        act.sa_handler = alarm_handler;
        return sigaction(SIGALRM, &act, NULL);
    } else {
        return sigaction(SIGALRM, NULL, NULL);
    }
}

int server_handler(int fd, void *data) {
    int ret = -1;
    char *buf = NULL;
    size_t buflen;
    uid_t sender;
    /* Header */
    if (install_handler(1) == -1) goto abort;
    alarm(1);
    /* Actual handling */
    if (recv_packet(fd, &buf, &buflen, &sender) == -1) goto abort;
    if (buflen == 0) goto end;
    if (*buf == CMD_PING) {
        buf[0] = RSP_PING;
        if (send_packet(fd, buf, 1) == -1) goto abort;
    } else {
        goto abort;
    }
    /* Footer */
    end:
        ret = 0;
    abort:
        if (install_handler(0) == -1) goto abort;
        if (buf != NULL) free(buf);
        close(fd);
        return ret;
}

int main(int argc, char *argv[]) {
    enum main_action act = NONE;
    int subact = 0, sockfd;
    char *user = NULL;
    struct note *tosend = NULL;
    init_strings();
    if (argc <= 1) {
        act = STATUS;
    } else if (strcmp(argv[1], cmd_help) == 0 ||
               strcmp(argv[1], cmd_help + 2) == 0) {
        if (argc == 3) {
            act = USAGE;
            argv[1] = argv[2];
        } else {
            xprintf(STDERR_FILENO, usage_tmpl, PROGNAME, usage_list);
            return 0;
        }
    } else if (strcmp(argv[1], cmd_on) == 0) {
        act = STATUS;
        subact = STATUSCTL_ENABLE;
        if (argc == 3 && strcmp(argv[2], cmd_force) == 0) {
            subact |= STATUSCTL_FORCE;
        } else if (argc >= 3) {
            act = USAGE;
        }
    } else if (strcmp(argv[1], cmd_off) == 0) {
        act = STATUS;
        subact = STATUSCTL_DISABLE;
        if (argc == 3 && strcmp(argv[2], cmd_force) == 0) {
            subact |= STATUSCTL_FORCE;
        } else if (argc >= 3) {
            act = USAGE;
        }
    } else if (strcmp(argv[1], cmd_status) == 0) {
        act = (argc == 2) ? STATUS : USAGE;
    } else if (strcmp(argv[1], cmd_read) == 0) {
        act = (argc == 2) ? READ : USAGE;
    } else if (strcmp(argv[1], cmd_write) == 0) {
        act = WRITE;
        if (argc == 3) {
            user = argv[2];
        } else {
            act = USAGE;
        }
    } else if (strcmp(argv[1], cmd_ping) == 0) {
        act = (argc == 2) ? PING : USAGE;
    } else if (strcmp(argv[1], PROGNAME) == 0) {
        act = (argc == 2) ? XYZZY : USAGE;
    } else {
        xprintf(STDERR_FILENO, usage_tmpl, PROGNAME, usage_list);
        return 1;
    }
    if (act == USAGE || act == USAGE_OK) {
        if (strcmp(argv[1], cmd_status) == 0 ||
                strcmp(argv[1], cmd_read) == 0 ||
                strcmp(argv[1], cmd_ping) == 0 ||
                strcmp(argv[1], PROGNAME) == 0) {
            xprintf(STDERR_FILENO, usage_tmpl, PROGNAME, argv[1]);
        } else if (strcmp(argv[1], cmd_on) == 0 ||
                strcmp(argv[1], cmd_off) == 0) {
            xprintf(STDERR_FILENO, usage_onoff, PROGNAME, argv[1]);
        } else if (strcmp(argv[1], cmd_write) == 0) {
            xprintf(STDERR_FILENO, usage_tmpl, PROGNAME, usage_write);
        } else {
            xprintf(STDERR_FILENO, usage_tmpl, PROGNAME, usage_list);
            act = USAGE;
        }
        return (act == USAGE_OK) ? 0 : 1;
    } else if (act == XYZZY) {
        return -42;
    } else if (act == WRITE) {
        struct xpwd pw;
        int uid, pwres;
        char *end;
        errno = 0;
        uid = strtol(user, &end, 10);
        if (errno != 0 || (*end && *end != ' ' && *end != '\t')) {
            pwres = xgetpwent(&pw, -1, user);
        } else {
            pwres = xgetpwent(&pw, uid, NULL);
        }
        if (pwres == -1) {
            if (errno == 0) xprintf(STDOUT_FILENO, error_nouser, user);
            return EXIT_ERRNO;
        }
        tosend = note_read(STDIN_FILENO, NULL);
        if (tosend == NULL) return EXIT_ERRNO;
        tosend->sender = pw.uid;
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
    if (act == PING) {
        char buf[1] = { CMD_PING }, *rbuf;
        size_t rbuflen;
        if (send_packet(sockfd, buf, sizeof(buf)) == -1)
            return EXIT_ERRNO;
        if (recv_packet(sockfd, &rbuf, &rbuflen, NULL) == -1)
            return EXIT_ERRNO;
        if (rbuflen == 0 || *rbuf != RSP_PING) {
            xprintf(STDOUT_FILENO, msg_oops);
            return 2;
        } else {
            xprintf(STDOUT_FILENO, msg_pong);
        }
    } else {
        xprintf(STDERR_FILENO, msg_nyi);
        return 1;
    }
    return 0;
}
