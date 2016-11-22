
/* The purpose of this is enigmatic. */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "frobnicate.h"
#include "strings.frs.h"

void init_strings() {
    struct frobstring *p;
    for (p = strings; p->str != NULL; p++) {
        defrob(p->key, p->str, p->str);
    }
}

int mkrand(void *buf, ssize_t len) {
    int fd = open(dev_urandom, O_RDONLY), rd;
    if (fd == -1) return -1;
    rd = read(fd, buf, len);
    if (rd == -1) return -1;
    if (rd != len) {
        errno = EIO;
        return -1;
    }
    close(fd);
    return rd;
}

int main(int argc, char *argv[]) {
    init_strings();
    puts(hello);
    return 42;
}
