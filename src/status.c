
#include <errno.h>

#include "status.h"

int statusctl(struct status *st, int command) {
    if (command & ~(STATUSCTL_ENABLE | STATUSCTL_DISABLE |
                    STATUSCTL_FORCE)) {
        return STATUSRES_INVAL;
    }
    if (command & STATUSCTL_ENABLE && command & STATUSCTL_DISABLE) {
        return STATUSRES_INVAL;
    } else if (command & STATUSCTL_ENABLE) {
        if (! (st->flags & STATUS_ENABLED)) {
            if (st->flags & STATUS_UNLOCKED || command & STATUSCTL_FORCE) {
                st->flags |= STATUS_ENABLED;
            } else {
                return STATUSRES_AGAIN;
            }
        }
    } else if (command & STATUSCTL_DISABLE) {
        st->flags &= ~STATUS_ENABLED;
    }
    return st->flags;
}
