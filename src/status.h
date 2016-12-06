
#ifndef _STATUS_H
#define _STATUS_H

#define STATUS_ENABLED  1 /* Whether the status is enabled */
#define STATUS_UNLOCKED 2 /* Whether the status is not locked to 0 */

#define STATUSCTL_DISABLE 1 /* Disable the status */
#define STATUSCTL_ENABLE  2 /* Enable the status */
#define STATUSCTL_FORCE   4 /* Force enabling (break lock) */

#define STATUSRES_ERROR -1 /* Generic failure (see errno for details) */
#define STATUSRES_AGAIN -2 /* Tried to enable locked status */

struct status {
    int flags;
};

/* Modify the status as stored in st by action,
 * which is a bitmask of STATUSCTL_* flags.
 * Returns the new status, or a (negative) STATUSRES_* constant on error. */
int statusctl(struct status *st, int action);

#endif
