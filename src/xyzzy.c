
/* The purpose of this is enigmatic. */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "frobnicate.h"
#include "note.h"
#include "strings.frs.h"

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
    struct note *notes[3] = {}, **newnotes, **n;
    char *buf = NULL;
    size_t len = 0;
    init_strings();
    notes[0] = note_read(STDIN_FILENO, notes[0]);
    if (notes[0] == NULL) return 1;
    notes[1] = notes[0];
    buf = note_pack(NULL, &len, 1, (const struct note **) notes);
    if (buf == NULL) return 1;
    newnotes = note_unpack(buf + 1, len - 1, NULL);
    if (newnotes == NULL) return 1;
    for (n = newnotes; *n; n++) note_print(STDOUT_FILENO, *n);
    free(notes[0]);
    free(newnotes[0]);
    free(newnotes[1]);
    free(newnotes);
    free(buf);
    if (urandom_fd != -1) close(urandom_fd);
    return 42;
}
