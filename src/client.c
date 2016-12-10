
#define _GNU_SOURCE
#include <errno.h>
#include <unistd.h>

#include "client.h"
#include "comm.h"

int client_connect() {
    struct sockaddr_un addr;
    socklen_t addrlen;
    int one = 1;
    int ret = socket(AF_UNIX, SOCK_STREAM, 0);
    if (ret == -1) return -1;
    prepare_address(&addr, &addrlen);
    if (connect(ret, (void *) &addr, addrlen) == -1)
        goto error;
    if (setsockopt(ret, SOL_SOCKET, SO_PASSCRED, &one, sizeof(one)) == -1)
        goto error;
    return ret;
    error:
        close(ret);
        return -1;
}
