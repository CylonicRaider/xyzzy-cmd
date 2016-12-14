
/* The purpose of this is enigmatic. */

#define _POSIX_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
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

int send_packet(int fd, const char *buf, size_t buflen) {
    struct message msg = { 0, buflen, (char *) buf, -1 };
    if (mkrand(&msg.key, sizeof(msg.key)) == -1)
        return -1;
    return send_message(fd, &msg, COMM_PEERAUTH);
}
int recv_packet(int fd, char **buf, size_t *buflen, uid_t *uid) {
    struct message msg = { 0, 0, NULL, -1 };
    int rd = recv_message(fd, &msg, COMM_PEERAUTH);
    if (rd == -1) return -1;
    if (rd == 0) {
        *buflen = 0;
        *buf = NULL;
        if (uid != NULL) *uid = -1;
    } else {
        *buflen = msg.length;
        *buf = msg.data;
        if (uid != NULL) *uid = msg.sender;
    }
    return rd;
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

int do_request(int fd, const char *sbuf, size_t sbuflen,
               char **rbuf, size_t *rbuflen) {
    if (send_packet(fd, sbuf, sbuflen) == -1)
        return -1;
    return recv_packet(fd, rbuf, rbuflen, NULL);
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
    if (recv_packet(fd, &buf, &buflen, &sender) <= 0) goto abort;
    if (buflen == 0) goto end;
    if (*buf == CMD_PING) {
        buf[0] = RSP_PING;
        if (send_packet(fd, buf, 1) == -1) goto abort;
    } else if (*buf == CMD_STATUS) {
        struct uhnode *node;
        int cmd, res, count;
        if (buflen != 1 + sizeof(int)) goto abort;
        node = userhash_make(data, sender);
        if (node == NULL) goto abort;
        memcpy(&cmd, buf + 1, sizeof(int));
        count = uhnode_countnotes(node);
        res = statusctl(&node->status, cmd);
        buf = realloc(buf, 1 + 2 * sizeof(int));
        if (buf == NULL) goto abort;
        buf[0] = RSP_STATUS;
        memcpy(buf + 1, &res, sizeof(int));
        memcpy(buf + 1 + sizeof(int), &count, sizeof(int));
        if (send_packet(fd, buf, 1 + 2 * sizeof(int)) == -1) goto abort;
    } else if (*buf == CMD_READ) {
        struct uhnode *node;
        if (buflen != 1) goto abort;
        node = userhash_get(data, sender);
        if (node != NULL) {
            struct note **notes = uhnode_popnotes(node);
            if (notes != NULL) {
                buf = note_pack(buf, &buflen, 1, notes);
                struct note **p;
                for (p = notes; *p; p++) free(*p);
                free(notes);
                if (buf == NULL) goto abort;
            }
        }
        buf[0] = RSP_READ;
        if (send_packet(fd, buf, buflen) == -1) goto abort;
    } else if (*buf == CMD_WRITE) {
        struct uhnode *node;
        struct note **notes, **p;
        node = userhash_make(data, sender);
        if (node == NULL) goto abort;
        notes = note_unpack(buf + 1, buflen - 1, NULL);
        if (notes == NULL) goto abort;
        for (p = notes; *p; p++) {
            if (uhnode_addnote(node, *p) == -1) {
                for (; *p; p++) free(*p);
                free(notes);
                goto abort;
            }
        }
        free(notes);
        buf[0] = RSP_WRITE;
        if (send_packet(fd, buf, 1) == -1) goto abort;
    } else {
        goto abort;
    }
    /* Footer */
    end:
        ret = 0;
    abort:
        install_handler(0);
        free(buf);
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
    } else if (argc >= 3 && strcmp(argv[2], cmd_help) == 0) {
        act = USAGE_OK;
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
        if (xatoi(user, &uid) == -1) {
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
        if (do_request(sockfd, buf, sizeof(buf), &rbuf, &rbuflen) == -1)
            return EXIT_ERRNO;
        if (rbuflen == 0 || *rbuf != RSP_PING) goto oops;
        xprintf(STDOUT_FILENO, msg_pong);
    } else if (act == STATUS) {
        char buf[1 + sizeof(int)] = { CMD_STATUS }, *rbuf;
        size_t rbuflen;
        int rsp, count;
        memcpy(buf + 1, &subact, sizeof(int));
        if (do_request(sockfd, buf, sizeof(buf), &rbuf, &rbuflen) == -1)
            return EXIT_ERRNO;
        if (rbuflen != 1 + sizeof(int) * 2 || *rbuf != RSP_STATUS) goto oops;
        memcpy(&rsp, rbuf + 1, sizeof(int));
        memcpy(&count, rbuf + 1 + sizeof(int), sizeof(int));
        if (rsp < 0) {
            if (rsp != STATUSRES_AGAIN) goto oops;
            xprintf(STDERR_FILENO, msg_sure);
            return 2;
        } else if (subact != 0) {
            /* NOP */
        } else if (count) {
            xprintf(STDOUT_FILENO, msg_status_cnt, (rsp & STATUS_ENABLED) ?
                    cmd_on : cmd_off, count);
        } else {
            xprintf(STDOUT_FILENO, msg_status, (rsp & STATUS_ENABLED) ?
                    cmd_on : cmd_off);
        }
    } else if (act == READ) {
        char buf[1] = { CMD_READ }, *rbuf;
        size_t rbuflen;
        struct note **notes, **p;
        if (do_request(sockfd, buf, sizeof(buf), &rbuf, &rbuflen) == -1)
            return EXIT_ERRNO;
        if (rbuflen == 0 || *rbuf != RSP_READ) goto oops;
        notes = note_unpack(rbuf + 1, rbuflen - 1, NULL);
        if (notes == NULL) {
            if (errno == EBADMSG) goto oops;
            return EXIT_ERRNO;
        }
        for (p = notes; *p; p++)
            note_print(STDOUT_FILENO, *p);
    } else if (act == WRITE) {
        size_t buflen = 1 + NOTE_SIZE(tosend);
        char *buf = malloc(buflen);
        if (buf == NULL) return EXIT_ERRNO;
        buf[0] = CMD_WRITE;
        memcpy(buf + 1, tosend, NOTE_SIZE(tosend));
        if (do_request(sockfd, buf, buflen, &buf, &buflen) == -1)
            return EXIT_ERRNO;
        if (buflen != 1 || *buf != RSP_WRITE) goto oops;
        /* NOP */
    } else {
        goto oops;
    }
    return 0;
    oops:
        xprintf(STDERR_FILENO, msg_oops);
        return 63;
}
