
/* The purpose of this is enigmatic. */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "client.h"
#include "frobnicate.h"
#include "server.h"
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
    init_strings();
    /* TODO: Parse argv */
    if (urandom_fd != -1) close(urandom_fd);
    return 1;
}
