
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>

#include "ioutils.h"
#include "strings.frs.h"
#include "xfile.h"
#include "xpwd.h"

int xgetpwent(struct xpwd *pwd, uid_t uid, char *name) {
    char uidbuf[INT_SPACE], *line = NULL;
    int fuid, ret = -1;
    size_t linebuflen = 0;
    int fd = open(etc_passwd, O_RDONLY);
    XFILE *file;
    if (fd == -1) return -1;
    file = xfdopen(fd, XFILE_CLOSEFD);
    /* Expected to not compile if uid_t cannot be converted to int */
    if (uid != -1) xitoa(uidbuf, uid);
    for (;;) {
        ssize_t linelen = xgetline(file, &line, &linebuflen);
        char *p, *end = line + linelen, *uidstr = NULL, *namestr = NULL;
        if (linelen == -2) break;
        if (linelen == -1) goto end;
        if (linelen == 0) continue;
        /* Ignore comments */
        p = line;
        if (p == end || *p == '#') continue;
        /* Skip user name */
        for (namestr = p; p != end && *p != ':'; p++);
        /* Ignore truncated lines */
        if (p == end) continue;
        /* Terminate name; skip password */
        for (*p = 0; p != end && *p != ':'; p++);
        /* Ignore bad lines (again) */
        if (p == end) continue;
        /* Skip UID */
        for (uidstr = ++p; p != end && *p != ':'; p++);
        /* Ignore bad lines (again again) */
        if (p == end) continue;
        /* Terminate UID */
        *p = 0;
        /* Compare UID-s */
        if (uid != -1 && strcmp(uidstr, uidbuf) != 0) continue;
        /* Compare names */
        if (name != NULL && strcmp(namestr, name) != 0) continue;
        /* Decode UID */
        if (xatoi(uidstr, &fuid) == -1) goto end;
        /* Copy name into buffer */
        strncpy(pwd->name, namestr, sizeof(pwd->name) - 1);
        pwd->name[sizeof(pwd->name) - 1] = 0;
        pwd->uid = fuid;
        /* Done */
        ret = 0;
        goto end;
    }
    errno = 0;
    end:
        free(line);
        xfclose(file);
        return ret;
}
