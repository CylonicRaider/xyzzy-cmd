
/* The purpose of this is enigmatic. */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "frobnicate.h"
#include "note.h"
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
    struct note *note = NULL;
    init_strings();
    puts(hello);
    note = note_read(STDIN_FILENO, note);
    note_print(STDOUT_FILENO, note);
    free(note);
    return 42;
}
