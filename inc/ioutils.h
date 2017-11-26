
#ifndef _IOUTILS_H
#define _IOUTILS_H

#include <limits.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>

#define DECSPACE(type) (sizeof(type) * CHAR_BIT / 3 + 4)
#define INT_SPACE DECSPACE(int)

struct xtime {
    int year;
    unsigned short month, day;
    unsigned short hour, minute, second;
};

/* Convert the given string to an integer
 * On success, returns zero; on error, returns -1, setting errno:
 * ERANGE when s is definitely too large;
 * EINVAL when s contains invalid characters (where +, -, the digits, and
 *        whitespace are valid).
 * i is only written to on success. */
int xatoi(const char *s, int *i);

/* Convert the given integer to a string in the caller-passed buffer
 * The buffer must be at least INT_SPACE bytes large.
 * This function is infallible. */
void xitoa(char *buf, int i);

/* Read exactly len bytes from fd, taking as many attempts as necessary
 * Returns the amount of bytes read (less than len on EOF), or -1 on error
 * (having errno set).
 * If len is zero, this returns instant success. */
ssize_t read_exactly(int fd, void *buf, size_t len);

/* Write exactly len bytes to fd
 * Returns the amount of bytes written (i.e. len), or -1 on error (having
 * errno set); if a single system call writes zero bytes, EBUSY is raised.
 * If len is zero, this returns instant success. */
ssize_t write_exactly(int fd, const void *buf, size_t len);

/* Break the given UNIX timestamp into its fields
 * Timestamps too large are silently truncated. */
void xgmtime(struct xtime *tm, time_t ts);

#endif
