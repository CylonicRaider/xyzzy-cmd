
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
#include "xfile.h"
#include "xpwd.h"
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

int do_request(int fd, const char *sbuf, size_t sbuflen,
               char **rbuf, size_t *rbuflen) {
    if (send_packet(fd, sbuf, sbuflen) == -1)
        return -1;
    return recv_packet(fd, rbuf, rbuflen, NULL);
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
        count = node->notelen;
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
                struct note **p;
                buf = note_pack(buf, &buflen, 1, notes);
                for (p = notes; *p; p++) free(*p);
                free(notes);
                if (buf == NULL) goto abort;
            }
        }
        buf[0] = RSP_READ;
        if (send_packet(fd, buf, buflen) == -1) goto abort;
    } else if (*buf == CMD_WRITE) {
        struct note **notes, **p;
        notes = note_unpack(buf + 1, buflen - 1, NULL);
        if (notes == NULL) goto abort;
        for (p = notes; *p; p++) {
            struct uhnode *node = userhash_make(data, (*p)->sender);
            if (node == NULL) goto cancel;
            (*p)->sender = sender;
            if (uhnode_addnote(node, *p) == -1) goto cancel;
        }
        free(notes);
        buf[0] = RSP_WRITE;
        if (send_packet(fd, buf, 1) == -1) goto abort;
        goto end;
        cancel:
            for (; *p; p++) free(*p);
            free(notes);
            goto abort;
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
    int subact = 0, sockfd = -1, ret = 63;
    char *user = NULL, *sbuf = NULL, *rbuf = NULL;
    size_t sbuflen, rbuflen;
    struct note *tosend = NULL;
    XFILE *stdout = xfdopen(1, 0);
    XFILE *stderr = xfdopen(2, 0);
    init_strings();
    if (argc <= 1) {
        act = STATUS;
    } else if (strcmp(argv[1], cmd_help) == 0 ||
               strcmp(argv[1], cmd_help + 2) == 0) {
        if (argc == 3) {
            act = USAGE;
            argv[1] = argv[2];
        } else {
            xprintf(stderr, usage_tmpl, PROGNAME, usage_list);
            xfflush(stderr);
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
    } else if (strcmp(argv[1], cmd_pong) == 0) {
        act = (argc == 2) ? PONG : USAGE;
    } else if (strcmp(argv[1], PROGNAME) == 0) {
        act = (argc == 2) ? XYZZY : USAGE;
    } else if (strcmp(argv[1], "-t") == 0) {
        struct note *n = note_read(0, NULL);
        note_print(stdout, n);
        free(n);
        xfflush(stdout);
        return 0;
    } else {
        xprintf(stderr, usage_tmpl, PROGNAME, usage_list);
        xfflush(stderr);
        return 1;
    }
    if (act == USAGE || act == USAGE_OK) {
        if (strcmp(argv[1], cmd_status) == 0 ||
                strcmp(argv[1], cmd_read) == 0 ||
                strcmp(argv[1], cmd_ping) == 0 ||
                strcmp(argv[1], cmd_pong) == 0 ||
                strcmp(argv[1], PROGNAME) == 0) {
            xprintf(stderr, usage_tmpl, PROGNAME, argv[1]);
        } else if (strcmp(argv[1], cmd_on) == 0 ||
                strcmp(argv[1], cmd_off) == 0) {
            xprintf(stderr, usage_onoff, PROGNAME, argv[1]);
        } else if (strcmp(argv[1], cmd_write) == 0) {
            xprintf(stderr, usage_tmpl, PROGNAME, usage_write);
        } else {
            xprintf(stderr, usage_tmpl, PROGNAME, usage_list);
            act = USAGE;
        }
        xfflush(stderr);
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
            if (errno == 0) xprintf(stdout, error_nouser, user);
            goto exit_errno;
        }
        tosend = note_read(STDIN_FILENO, NULL);
        if (tosend == NULL) goto exit_errno;
        tosend->sender = pw.uid;
    }
    sockfd = client_connect();
    if (sockfd == -1) {
        struct userhash uh;
        if (errno != ECONNREFUSED)
            goto exit_errno;
        userhash_init(&uh);
        if (server_spawn(argc, argv, &server_handler, &uh) == -1)
            goto exit_errno;
        userhash_del(&uh);
        sockfd = client_connect();
        if (sockfd == -1)
            goto exit_errno;
    }
    if (act == PING || act == PONG) {
        sbuf = malloc(1);
        if (sbuf == NULL)
            goto exit_errno;
        sbuf[0] = CMD_PING;
        if (do_request(sockfd, sbuf, 1, &rbuf, &rbuflen) == -1)
            goto exit_errno;
        if (rbuflen == 0 || *rbuf != RSP_PING) goto oops;
        if (act == PING) {
            xprintf(stdout, msg_pingpong, cmd_pong);
        } else {
            xprintf(stdout, msg_pingpong, cmd_ping);
        }
    } else if (act == STATUS) {
        int rsp, count;
        sbuf = malloc(1 + sizeof(int));
        if (sbuf == NULL)
            goto exit_errno;
        sbuf[0] = CMD_STATUS;
        memcpy(sbuf + 1, &subact, sizeof(int));
        if (do_request(sockfd, sbuf, 1 + sizeof(int), &rbuf, &rbuflen) == -1)
            goto exit_errno;
        if (rbuflen != 1 + sizeof(int) * 2 || *rbuf != RSP_STATUS) goto oops;
        memcpy(&rsp, rbuf + 1, sizeof(int));
        memcpy(&count, rbuf + 1 + sizeof(int), sizeof(int));
        if (rsp < 0) {
            if (rsp != STATUSRES_AGAIN) goto oops;
            xprintf(stderr, msg_sure);
            ret = 2;
            goto end;
        } else if (subact != 0) {
            /* NOP */
        } else if (count) {
            xprintf(stdout, msg_status_cnt, (rsp & STATUS_ENABLED) ?
                    cmd_on : cmd_off, count);
        } else {
            xprintf(stdout, msg_status, (rsp & STATUS_ENABLED) ?
                    cmd_on : cmd_off);
        }
    } else if (act == READ) {
        struct note **notes, **p;
        sbuf = malloc(1);
        if (sbuf == NULL) goto exit_errno;
        sbuf[0] = CMD_READ;
        if (do_request(sockfd, sbuf, 1, &rbuf, &rbuflen) == -1)
            goto exit_errno;
        if (rbuflen == 0 || *rbuf != RSP_READ) goto oops;
        notes = note_unpack(rbuf + 1, rbuflen - 1, NULL);
        if (notes == NULL) {
            if (errno == EBADMSG) goto oops;
            goto exit_errno;
        }
        for (p = notes; *p; p++) {
            if (p != notes && xputc(stdout, '\n') == -1)
                goto notes_abort;
            if (note_print(stdout, *p) == -1)
                goto notes_abort;
            free(*p);
        }
        free(notes);
        ret = 0;
        goto end;
        notes_abort:
            while (*p) free(*p++);
            free(notes);
            goto exit_errno;
    } else if (act == WRITE) {
        sbuflen = 1 + NOTE_SIZE(tosend);
        sbuf = malloc(sbuflen);
        if (sbuf == NULL) goto exit_errno;
        sbuf[0] = CMD_WRITE;
        memcpy(sbuf + 1, tosend, NOTE_SIZE(tosend));
        if (do_request(sockfd, sbuf, sbuflen, &rbuf, &rbuflen) == -1)
            goto exit_errno;
        if (rbuflen != 1 || *rbuf != RSP_WRITE) goto oops;
        /* NOP */
    } else {
        goto oops;
    }
    ret = 0;
    goto end;
    oops:
        xprintf(stderr, msg_oops);
        ret = 62;
        goto end;
    exit_errno:
        ret = EXIT_ERRNO;
    end:
        xfflush(stdout);
        xfflush(stderr);
        free(sbuf);
        free(rbuf);
        free(tosend);
        if (urandom_fd != -1) close(urandom_fd);
        if (sockfd != -1) close(sockfd);
        return ret;
}
