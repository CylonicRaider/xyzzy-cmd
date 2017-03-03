
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
static inline size_t fbuf_extract(struct fbuf *buf, char *data, size_t len) {
    if (len >= buf->len) {
        len = buf->len;
        memcpy(data, buf->data + buf->begin, len);
        buf->begin = 0;
        buf->len = 0;
        return len;
    } else {
        memcpy(data, buf->data + buf->begin, len);
        buf->begin += len;
        buf->len -= len;
        return len;
    }
}
static inline size_t fbuf_insert(struct fbuf *buf, const char *data,
                                 size_t len) {
    char *end = buf->data + buf->begin + buf->len;
    size_t rem = buf->cap - buf->begin - buf->len;
    memcpy(end, data, rem);
    return rem;
}
static inline ssize_t fbuf_read(struct fbuf *buf, int fd) {
    char *end = buf->data + buf->begin + buf->len;
    size_t rem = buf->cap - buf->begin - buf->len;
    ssize_t ret = read(fd, end, rem);
    if (ret > 0) buf->len += ret;
    return ret;
}
static inline ssize_t fbuf_write(struct fbuf *buf, int fd) {
    ssize_t ret = write(fd, buf->data + buf->begin, buf->len);
    if (ret > 0) {
        buf->len -= ret;
        if (buf->len == 0) {
            buf->begin = 0;
        } else {
            buf->begin += ret;
        }
    }
    return ret;
}
static inline ssize_t fbuf_writeall(struct fbuf *buf, int fd) {
    ssize_t ret = write_exactly(fd, buf->data + buf->begin, buf->len);
    if (ret > 0) {
        buf->len -= ret;
        if (buf->len == 0) {
            buf->begin = 0;
        } else {
            buf->begin += ret;
        }
    }
    return ret;
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
    ssize_t ret = fbuf_extract(&f->rdbuf, buf, len);
    if (ret == len) return ret;
    buf += ret;
    len -= ret;
    do {
        ssize_t rd = fbuf_read(&f->rdbuf, f->fd);
        if (rd == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (rd == 0) break;
        rd = fbuf_extract(&f->rdbuf, buf, len);
        ret += rd;
        buf += rd;
        len -= rd;
    } while (len && ! (f->flags & XFILE_NOEXACT));
    return ret;
}

ssize_t xfwrite(XFILE *f, const char *buf, size_t len) {
    ssize_t ret = fbuf_insert(&f->wrbuf, buf, len);
    if (ret == len) return ret;
    buf += ret;
    len -= ret;
    if (f->flags & XFILE_NOEXACT) {
        ssize_t wr = fbuf_write(&f->wrbuf, f->fd);
        if (wr == -1) return -1;
        wr = fbuf_insert(&f->wrbuf, buf, len);
        return ret + wr;
    }
    do {
        ssize_t wr = fbuf_writeall(&f->wrbuf, f->fd);
        if (wr == -1) return -1;
        wr = fbuf_insert(&f->wrbuf, buf, len);
        ret += wr;
        buf += wr;
        len -= wr;
    } while (len);
    return ret;
}

int xfflush(XFILE *f) {
    ssize_t wr = fbuf_writeall(&f->wrbuf, f->fd);
    return (wr == -1) ? -1 : 0;
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
