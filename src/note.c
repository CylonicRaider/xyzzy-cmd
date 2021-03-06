
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>

#include "ioutils.h"
#include "note.h"
#include "strings.frs.h"
#include "xfile.h"
#include "xpwd.h"

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
        if (rd == -1) {
            if (errno == EINTR) continue;
            goto error;
        }
        if (rd == 0) break;
        note->length += rd;
    }
    return note;
    error:
        if (dynamic) free(note);
        return NULL;
}

int note_print(XFILE *f, const struct note *note) {
    struct xtime tm;
    struct xpwd pwd;
    size_t len;
    if (xgetpwent(&pwd, note->sender, NULL) == -1) {
        if (errno != 0) return -1;
        strcpy(pwd.name, "???");
    }
    xgmtime(&tm, note->time.tv_sec);
    if (xprintf(f, note_header, pwd.name, (int) note->sender,
                (int) tm.year, (int) tm.month, (int) tm.day,
                (int) tm.hour, (int) tm.minute, (int) tm.second,
                (int) note->time.tv_usec, (int) note->length) == -1)
        return -1;
    len = note->length;
    if (len && note->content[len - 1] == '\n') len--;
    if (xfwrite(f, note->content, len) != len)
        return -1;
    if (xputs(f, note_footer) == -1)
        return -1;
    return 0;
}

char *note_pack(char *buf, size_t *length, int header,
                struct note *notes[]) {
    size_t len = header;
    char *res;
    struct note **p;
    for (p = notes; *p; p++)
        len += NOTE_SIZE(*p);
    if (length != 0) *length = len;
    if (len == 0) return realloc(buf, 1);
    buf = realloc(buf, len);
    if (buf == NULL) return NULL;
    res = buf + header;
    for (p = notes; *p; p++) {
        size_t len = NOTE_SIZE(*p);
        memcpy(res, *p, len);
        res += len;
    }
    return buf;
}

struct note **note_unpack(const char *buf, size_t length,
                          struct note *ptrbuf[]) {
    size_t listlen = 0, listfill = 0, i;
    ptrbuf = realloc(ptrbuf, sizeof(*ptrbuf));
    if (ptrbuf == NULL) return NULL;
    for (;;) {
        struct note *note;
        size_t l;
        if (listfill == listlen) {
            struct note **newptrbuf;
            listlen = (listlen) ? listlen * 2 : 1;
            newptrbuf = malloc(listlen * sizeof(*ptrbuf));
            if (newptrbuf == NULL) goto error;
            memcpy(newptrbuf, ptrbuf, listfill * sizeof(*ptrbuf));
            free(ptrbuf);
            ptrbuf = newptrbuf;
        }
        if (length == 0) break;
        l = NOTE_SIZE((struct note *) buf);
        if (l > length) {
            errno = EBADMSG;
            goto error;
        }
        note = malloc(l);
        if (note == NULL) goto error;
        memcpy(note, buf, l);
        ptrbuf[listfill++] = note;
        buf += l;
        length -= l;
    }
    ptrbuf[listfill] = NULL;
    return ptrbuf;
    error:
        for (i = 0; i < listfill; i++) free(ptrbuf[i]);
        free(ptrbuf);
        return NULL;
}
