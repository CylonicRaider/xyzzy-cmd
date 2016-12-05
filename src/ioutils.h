
#ifndef _IOUTILS_H
#define _IOUTILS_H

#include <stdio.h>
#include <unistd.h>

#define DECSPACE(type) (sizeof(type) * CHAR_BIT / 3 + 4)
#define INT_SPACE DECSPACE(int)

struct xtime {
    int year;
    unsigned short month, day;
    unsigned short hour, minute, second;
};

/* Convert the given integer to a string in the caller-passed buffer
 * The buffer must be at least INT_SPACE bytes large.
 * This function is infallible. */
void xitoa(char *buf, int i);

/* Write result of formatting fmt with additional arguments to stream
 * Returns the amount of bytes written on success, or -1 on error, setting
 * errno appropriately.
 * Supported format specifiers:
 * %d -- int
 * %s -- char *
 * The space and minus flags and a field width are supported.
 * Who needs more? */
ssize_t xprintf(FILE *stream, const char *fmt, ...);

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
ssize_t xgetline(FILE *stream, char **buf, size_t *buflen);

/* Break the given UNIX timestamp into its fields
 * Timestamps too large are silently truncated. */
void xgmtime(struct xtime *tm, time_t ts);

/* Parse the password database and retrieve the given user's name
 * Returns a pointer to a static buffer (the name may be truncated!), or NULL
 * in case of failure, setting errno accordingly. */
char *xgetpwuid(uid_t uid);

#endif
