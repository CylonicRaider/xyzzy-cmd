
#ifndef _STATUS_H
#define _STATUS_H

#define STATUS_ENABLED  1 /* Whether the status is enabled */
#define STATUS_UNLOCKED 2 /* Whether the status is not locked to 0 */

#define _STATUS_MASK 3

#define STATUSCTL_DISABLE 1 /* Disable the status */
#define STATUSCTL_ENABLE  2 /* Enable the status */
#define STATUSCTL_FORCE   4 /* Force enabling (break lock) */

#define _STATUSCTL_MASK 7

#define STATUSRES_ERROR -1 /* Generic failure (see errno for details) */
#define STATUSRES_INVAL -2 /* Invalid argument for statusctl() */
#define STATUSRES_AGAIN -3 /* Tried to enable locked status */

struct status {
    int flags;
};

/* Initialize the given status structure
 * Returns zero on success or -1 on error with errno set (does not
 * happen). */
int status_init(struct status *st);

/* Modify the status as stored in st by action.
 * action is a bitmask of STATUSCTL_* flags
 * Returns the new status, or a (negative) STATUSRES_* constant on error. */
int statusctl(struct status *st, int action);

#endif
