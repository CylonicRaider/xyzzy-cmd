
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "ioutils.h"
#include "xfile.h"

#define BUFSIZE 4096

XFILE *xfdopen(int fd, int flags) {
    if (flags & ~_XFILE_MASK) {
        errno = EINVAL;
        return NULL;
    }
    XFILE *file = malloc(sizeof(XFILE));
    if (file == NULL) return NULL;
    file->fd = fd;
    file->flags = flags;
    file->rdbuf.data = malloc(BUFSIZE);
    if (file->rdbuf.data == NULL) {
        free(file);
        return NULL;
    }
    file->rdbuf.cap = BUFSIZE;
    file->rdbuf.begin = 0;
    file->rdbuf.len = 0;
    file->wrbuf.data = malloc(BUFSIZE);
    if (file->wrbuf.data == NULL) {
        free(file->rdbuf.data);
        free(file);
        return NULL;
    }
    file->wrbuf.cap = BUFSIZE;
    file->wrbuf.begin = 0;
    file->wrbuf.len = 0;
    return file;
}

ssize_t xfread(XFILE *f, char *buf, size_t len) {
    if (f->flags & XFILE_NOEXACT) {
        return read(f->fd, buf, len);
    } else {
        return read_exactly(f->fd, buf, len);
    }
}

ssize_t xfwrite(XFILE *f, const char *buf, size_t len) {
    if (f->flags & XFILE_NOEXACT) {
        return write(f->fd, buf, len);
    } else {
        return write_exactly(f->fd, buf, len);
    }
}

int xfflush(XFILE *f) {
    return 0;
}

int xfclose(XFILE *f) {
    int ret = 0;
    if (xfflush(f) == -1) ret = -1;
    free(f->rdbuf.data);
    free(f->wrbuf.data);
    if (f->flags & XFILE_CLOSEFD) {
        if (close(f->fd) == -1) ret = -1;
    }
    free(f);
    return ret;
}
