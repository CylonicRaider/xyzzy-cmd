
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

#include "ioutils.h"
#include "xfile.h"

#define BUFSIZE 4096

#define _XPRINTF_ZPAD 1
#define _XPRINTF_LEFT 2

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

int xputc(XFILE *f, int ch) {
    char buf[1] = { ch };
    if (xfwrite(f, buf, 1) == -1) return -1;
    return ch;
}

ssize_t xputs(XFILE *f, const char *s) {
    return xfwrite(f, s, strlen(s));
}

ssize_t xprintf(XFILE *f, const char *fmt, ...) {
    size_t written = 0;
    char numbuf[INT_SPACE];
    va_list ap;
    va_start(ap, fmt);
    while (*fmt) {
        int length, flags;
        char conv, *output;
        size_t outputlen;
        const char *oldfmt = fmt;
        while (*fmt && *fmt != '%') fmt++;
        if (fmt != oldfmt) {
            outputlen = fmt - oldfmt;
            if (xfwrite(f, oldfmt, outputlen) != outputlen)
                return -1;
            written++;
        }
        if (! *fmt) break;
        length = 0, flags = 0, conv = 0;
        while (*++fmt) {
            if (('1' <= *fmt && *fmt <= '9') || (length && *fmt == '0')) {
                length = (length * 10) + (*fmt - '0');
            } else if (*fmt == '0') {
                flags |= _XPRINTF_ZPAD;
            } else if (*fmt == '-' && ! length) {
                flags |= _XPRINTF_LEFT;
            } else if (*fmt == 's' || *fmt == 'd' || *fmt == '%') {
                conv = *fmt++;
                break;
            } else {
                errno = EINVAL;
                return -1;
            }
        }
        if (conv == 0) {
            errno = EINVAL;
            return -1;
        }
        if (conv == 's') {
            output = va_arg(ap, char *);
        } else if (conv == 'd') {
            int val = va_arg(ap, int);
            xitoa(numbuf, val);
            output = numbuf;
        } else {
            output = "%";
        }
        outputlen = strlen(output);
        if (outputlen >= length) {
            written += outputlen;
            if (xputs(f, output) == -1) return -1;
        } else if (flags & _XPRINTF_LEFT) {
            written += length;
            if (xputs(f, output) == -1) return -1;
            for (; outputlen < length; length--)
                if (xputc(f, ' ') == -1) return -1;
        } else if (flags & _XPRINTF_ZPAD) {
            written += length;
            for (; outputlen < length; length--)
                if (xputc(f, '0') == -1) return -1;
            if (xputs(f, output) == -1) return -1;
        } else {
            written += length;
            for (; outputlen < length; length--)
                if (xputc(f, ' ') == -1) return -1;
            if (xputs(f, output) == -1) return -1;
        }
    }
    va_end(ap);
    return written;
}
