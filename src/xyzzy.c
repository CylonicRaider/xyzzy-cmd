
/* The purpose of this is enigmatic. */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "frobnicate.h"
#include "ioutils.h"
#include "strings.frs.h"
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

int main(int argc, char *argv[]) {
    enum main_action act = ERROR;
    init_strings();
    if (argc <= 1) {
        act = STATUS;
    } else if (strcmp(argv[1], cmd_help) == 0 ||
               strcmp(argv[1], cmd_help + 2) == 0) {
        xprintf(stderr, usage_tmpl, PROGNAME, usage_list);
        return 0;
    } else if (strcmp(argv[1], cmd_on) == 0) {
        act = ON;
    } else if (strcmp(argv[1], cmd_off) == 0) {
        act = OFF;
    } else if (strcmp(argv[1], cmd_status) == 0) {
        act = STATUS;
    } else if (strcmp(argv[1], cmd_read) == 0) {
        act = READ;
    } else if (strcmp(argv[1], cmd_write) == 0) {
        act = WRITE;
    } else if (strcmp(argv[1], cmd_ping) == 0) {
        act = PING;
    } else {
        xprintf(stderr, usage_tmpl, PROGNAME, usage_list);
        return 1;
    }
    xprintf(stdout, "%d\n", act);
    if (urandom_fd != -1) close(urandom_fd);
    return 1;
}
