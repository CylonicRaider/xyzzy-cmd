
#include <errno.h>

#include "ioutils.h"

int xprintf(FILE *stream, const char *fmt, ...) {
    errno = ENOSYS;
    return -1;
}

void xgmtime(struct xtime *tm, time_t ts) {
    time_t z, era;
    unsigned int doe, yoe, doy, mp, m;
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
    m = mp + (mp < 10 ? 3 : -9);
    tm->day = doy - (153 * mp + 2) / 5 + 1;
    tm->month = m;
    tm->year = yoe + era * 400 + (m <= 2);
}

char *xgetpwuid(uid_t uid) {
    errno = ENOSYS;
    return NULL;
}
