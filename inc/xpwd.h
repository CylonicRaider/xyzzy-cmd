
#ifndef _XPWD_H
#define _XPWD_H

#include <sys/types.h>

#define NAME_SIZE 128

struct xpwd {
    uid_t uid;
    char name[NAME_SIZE];
};

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
