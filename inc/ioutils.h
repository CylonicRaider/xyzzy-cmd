
#ifndef _IOUTILS_H
#define _IOUTILS_H

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>

#define DECSPACE(type) (sizeof(type) * CHAR_BIT / 3 + 4)
#define INT_SPACE DECSPACE(int)

#define NAME_SIZE 128

struct xtime {
    int year;
    unsigned short month, day;
    unsigned short hour, minute, second;
};

struct xpwd {
    uid_t uid;
    char name[NAME_SIZE];
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

/* Parse the password database and retrieve some data for the given user
 * If uid is not -1, the UID of the user must match, if name is not NULL,
 * the name of the user must match, in particular, of both are given,
 * both must match. Setting both uid and name to the default values returns
 * an arbitrary valid user entry.
 * Fills in pwd (the user name can be truncated!). Returns 0 in case of
 * success, or -1 on error (setting errno to appropriately; if no user
 * matching the criteria is found, -1 is returned with errno set to 0). */
int xgetpwent(struct xpwd *pwd, uid_t uid, char *name);

#endif
