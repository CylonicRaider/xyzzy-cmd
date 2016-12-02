
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
    struct note *notes[3] = {}, **newnotes, **n;
    char *buf = NULL;
    size_t len = 0;
    init_strings();
    puts(hello);
    notes[0] = note_read(STDIN_FILENO, notes[0]);
    if (notes[0] == NULL) return 1;
    notes[1] = notes[0];
    buf = note_pack(NULL, &len, 1, (const struct note **) notes);
    if (buf == NULL) return 1;
    newnotes = note_unpack(buf + 1, len - 1, NULL);
    if (newnotes == NULL) return 1;
    for (n = newnotes; *n; n++) note_print(STDOUT_FILENO, *n);
    return 42;
}
