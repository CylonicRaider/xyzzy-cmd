
#ifndef _IOUTILS_H
#define _IOUTILS_H

#include <stdio.h>
#include <unistd.h>

struct xtime {
    int year;
    unsigned short month, day;
    unsigned short hour, minute, second;
};

/* Write result of formatting fmt with additional arguments to stream
 * Returns the amount of bytes written on success, or -1 on error, setting
 * errno appropriately.
 * Supported format specifiers:
 * %i -- int
 * %s -- char *
 * Who needs more? */
int xprintf(FILE *stream, const char *fmt, ...);

/* Break the given UNIX timestamp into its fields
 * Timestamps too large are silently truncated. */
void xgmtime(struct xtime *tm, time_t ts);

/* Parse the password database and retrieve the given user's name
 * Returns a pointer to a static buffer (the name may be truncated!), or NULL
 * in case of failure, setting errno accordingly. */
char *xgetpwuid(uid_t uid);

#endif
