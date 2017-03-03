
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "ioutils.h"
#include "xfile.h"

#define BUFSIZE 4096

static inline int fbuf_init(struct fbuf *buf) {
    buf->data = malloc(BUFSIZE);
    if (buf->data == NULL) return -1;
    buf->cap = BUFSIZE;
    buf->begin = 0;
    buf->len = 0;
    return 0;
}
static inline void fbuf_del(struct fbuf *buf) {
    free(buf->data);
}

XFILE *xfdopen(int fd, int flags) {
    if (flags & ~_XFILE_MASK) {
        errno = EINVAL;
        return NULL;
    }
    XFILE *file = malloc(sizeof(XFILE));
    if (file == NULL) return NULL;
    file->fd = fd;
    file->flags = flags;
    if (fbuf_init(&file->rdbuf) == -1) {
        free(file);
        return NULL;
    }
    if (fbuf_init(&file->wrbuf) == -1) {
        fbuf_del(&file->rdbuf);
        free(file);
        return NULL;
    }
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
    fbuf_del(&f->rdbuf);
    fbuf_del(&f->wrbuf);
    if (f->flags & XFILE_CLOSEFD) {
        if (close(f->fd) == -1) ret = -1;
    }
    free(f);
    return ret;
}
