
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>

#include "note.h"
#include "strings.frs.h"

int note_init(struct note *note) {
    if (gettimeofday(&note->time, NULL) == -1) return -1;
    // NOTE: getuid() is considered infallible.
    note->sender = getuid();
    return 0;
}

struct note *note_read(int fd, struct note *note) {
    int dynamic = 0;
    size_t buflen = 16;
    if (note == NULL) {
        note = calloc(1, sizeof(*note) + buflen);
        if (note == NULL) return NULL;
        dynamic = 1;
    }
    if (note_init(note) == -1) goto error;
    note->length = 0;
    for (;;) {
        ssize_t rd;
        if (note->length == buflen) {
            buflen *= 2;
            note = realloc(note, sizeof(*note) + buflen);
            if (note == NULL) goto error;
        }
        rd = read(fd, note->content + note->length, buflen - note->length);
        if (rd == -1) goto error;
        if (rd == 0) break;
        note->length += rd;
    }
    return note;
    error:
        if (dynamic) free(note);
        return NULL;
}

int note_print(int fd, struct note *note) {
    FILE *stream;
    int nfd;
    char *p;
    nfd = dup(fd);
    if (nfd == -1) return -1;
    stream = fdopen(nfd, notes_streammode);
    if (stream == NULL) return -1;
    if (fprintf(stream, notes_format, note->sender,
                (long long) note->time.tv_sec,
                (int) (note->time.tv_usec / 1000)) < 0)
        goto error;
    /* No reliable way to output a block of data with explicit length... */
    errno = 0;
    for (p = note->content; p != note->content + note->length; p++) {
        if (fputc(*p, stream) == EOF) goto error;
    }
    if (fputs("\n", stream) == EOF) goto error;
    if (fclose(stream) == EOF) return -1;
    return 0;
    /* Abort */
    error:
        fclose(stream);
        return -1;
}
