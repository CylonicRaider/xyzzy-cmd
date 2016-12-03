
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ioutils.h"
#include "strings.frs.h"

#define DECSPACE(type) (sizeof(type) * CHAR_BIT / 3 + 4)

#define LINESIZE 128

#define _XPRINTF_ZPAD 1
#define _XPRINTF_LEFT 2

ssize_t xprintf(FILE *stream, const char *fmt, ...) {
    size_t written = 0;
    char numbuf[DECSPACE(int)];
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
            if (fwrite(oldfmt, 1, outputlen, stream) != outputlen)
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
            unsigned int uval;
            char *dp = numbuf, *ddp, *rdp;
            if (val < 0) {
                *dp++ = '-';
                uval = -val;
            } else {
                uval = val;
            }
            rdp = dp;
            do {
                *rdp++ = uval % 10 + '0';
                uval /= 10;
            } while (uval);
            for (ddp = dp, dp = rdp, rdp--; ddp < rdp; ddp++, rdp--) {
                char temp = *rdp;
                *rdp = *ddp;
                *ddp = temp;
            }
            *dp = 0;
            output = numbuf;
        } else {
            output = "%";
        }
        outputlen = strlen(output);
        if (outputlen >= length) {
            written += outputlen;
            if (fputs(output, stream) == EOF) return -1;
        } else if (flags & _XPRINTF_LEFT) {
            written += length;
            if (fputs(output, stream) == EOF) return -1;
            for (; outputlen < length; length--)
                if (putc(' ', stream) == EOF) return -1;
        } else if (flags & _XPRINTF_ZPAD) {
            written += length;
            for (; outputlen < length; length--)
                if (putc('0', stream) == EOF) return -1;
            if (fputs(output, stream) == EOF) return -1;
        } else {
            written += length;
            for (; outputlen < length; length--)
                if (putc(' ', stream) == EOF) return -1;
            if (fputs(output, stream) == EOF) return -1;
        }
    }
    va_end(ap);
    return written;
}

ssize_t xgetline(FILE *stream, char **buf, size_t *buflen) {
    ssize_t read = 0;
    if (*buf == NULL || *buflen < LINESIZE) {
        *buflen = LINESIZE;
        *buf = realloc(*buf, *buflen);
        if (*buf == NULL) return -1;
    }
    for (;;) {
        int ch;
        if (read == *buflen) {
            *buflen *= 2;
            *buf = realloc(*buf, *buflen);
            if (*buf == NULL) return -1;
        }
        ch = fgetc(stream);
        if (ch == EOF || ch == '\n') {
            if (ferror(stream)) return -1;
            if (ch == EOF && read == 0) return -2;
            break;
        }
        (*buf)[read++] = ch;
    }
    return read;
}

void xgmtime(struct xtime *tm, time_t ts) {
    time_t z, era;
    unsigned int doe, yoe, doy, mp;
    tm->second = ts % 60;
    tm->minute = ts / 60 % 60;
    tm->hour = ts / 3600 % 24;
    /* http://stackoverflow.com/a/32158604 */
    z = ts / 86400 + 719468;
    era = (z >= 0 ? z : z - 146096) / 146097;
    doe = z - era * 146097;
    yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    mp = (5 * doy + 2) / 153;
    tm->day = doy - (153 * mp + 2) / 5 + 1;
    tm->month = mp + (mp < 10 ? 3 : -9);
    tm->year = yoe + era * 400 + (tm->month <= 2);
}

char *xgetpwuid(uid_t uid) {
    errno = ENOSYS;
    return NULL;
}
