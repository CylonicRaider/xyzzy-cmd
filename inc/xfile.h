
#ifndef _XFILE_H
#define _XFILE_H

#include <sys/types.h>

#define XFILE_CLOSEFD 1 /* Close the file descriptor on xfclose() */
#define XFILE_NOEXACT 2 /* At most one syscall per file operation */

#define _XFILE_MASK 3

#define XFLUSH_READ  1 /* Flush reading buffer */
#define XFLUSH_WRITE 2 /* Flush writing buffer */

#define _XFLUSH_MASK 3

struct fbuf {
    char *data;
    size_t cap, begin, len;
};

typedef struct {
    int fd;
    int flags;
    struct fbuf rdbuf;
    struct fbuf wrbuf;
} XFILE;

/* Open the given file descriptor
 * fd is an OS file descriptor (as obtained from open()); flags is the
 * bitwise OR of XFILE_* constants (or zero).
 * Returns a pointer to an XFILE object, or NULL on failure. */
XFILE *xfdopen(int fd, int flags);

/* Read len bytes from f into buf
 * If the XFILE_NOEXACT flag is set, data are returned from the reading
 * buffer (if there are any), and at most one additional read() system call;
 * otherwise, semantics from read_exactly() apply. In either case, less data
 * may be read than requested (although in the second one, this indicates
 * EOF).
 * Returns the amount of bytes actually read (and retrieved from buffer). */
ssize_t xfread(XFILE *f, char *buf, size_t len);

/* Write len bytes to f from buf
 * As many data as possible are appended to the writing buffer, then an
 * attempt made to write as many as possible into the underlying file
 * descriptor. If the XFILE_NOEXACT flag is not set, the process is repeated
 * until the entire buffer is consumed or an error appears.
 * Returns the amount of bytes consumed from buf. */
ssize_t xfwrite(XFILE *f, const char *buf, size_t len);

/* Flush the writing buffer of f
 * Returns 0 on success or -1 on error. */
int xfflush(XFILE *f);

/* Close the file
 * The file is flushed, and, if the XFILE_CLOSEFD flag is set, the underlying
 * file descriptor is closed.
 * Errors while flushing or closing the file are reported with a return value
 * of -1; in any case, the underlying file descriptor is closed (if
 * requested) to avoid race conditions. */
int xfclose(XFILE *f);

/* Read a whole line from the given stream
 * The given buffer *buf (with size buflen) is dynamically reallocated as
 * necessary; it is in particular left up to the caller to free() it.
 * A line ends at the newline character '\n' or EOF; the newline character
 * (if any) is not included in the result.
 * Returns the actual length of the line (may include NUL-s and be less than
 * the new buffer length), -2 if there is nothing left to read, or -1 in case
 * of failure (leaving buf and buflen in a consistent state, i.e. buf can
 * either be a buffer of size buflen, or NULL; and leaving the state of the
 * stream undefined). */
ssize_t xgetline(XFILE *f, char **buf, size_t *buflen);

/* Write the given character to the given file descriptor
 * Returns the character written, or -1 on error (having errno set). */
int xputc(XFILE *f, int ch);

/* Write the given NUL-terminated string to f
 * Shortcut for xfwrite(f, s, strlen(s)); */
ssize_t xputs(XFILE *f, const char *s);

/* Write result of formatting fmt with additional arguments to stream
 * Returns the amount of bytes written on success, or -1 on error, setting
 * errno appropriately.
 * Supported format specifiers:
 * %d -- int
 * %s -- char *
 * %% -- Percent sign
 * The space and minus flags and a field width are supported.
 * Who needs more? */
ssize_t xprintf(XFILE *f, const char *fmt, ...);

#endif
