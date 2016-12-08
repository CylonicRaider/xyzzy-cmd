
#define _BSD_SOURCE
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

#include "comm.h"
#include "server.h"

int server_listen() {
    struct sockaddr_un addr;
    socklen_t addrlen;
    int ret = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (ret == -1) return -1;
    prepare_address(&addr, &addrlen);
    if (bind(ret, (void *) &addr, addrlen) == -1) goto error;
    if (listen(ret, SOMAXCONN) == -1) goto error;
    return ret;
    error:
        close(ret);
        return -1;
}

int server_main(srvhandler_t handler) {
    int sockfd, fd = -1;
    sockfd = server_listen();
    if (sockfd == -1) return -1;
    for (;;) {
        int pid;
        fd = accept(sockfd, NULL, NULL);
        if (fd == -1 && errno != EINTR) goto error;
        if (fd != -1) {
            pid = fork();
            if (pid == -1) goto error;
            if (pid == 0) {
                int ret = (*handler)(fd);
                _exit((ret == 0) ? 0 : 1);
                return -1;
            }
            fd = -1;
        }
        /* Do not summon zombies */
        for (;;) {
            if (waitpid(-1, NULL, WNOHANG) == -1) {
                if (errno == ECHILD) break;
                goto error;
            }
        }
    }
    error:
        close(sockfd);
        if (fd != -1) close(fd);
        return -1;
}

int server_spawn(int argc, char *argv[], srvhandler_t handler) {
    char *p;
    int pid, ret;
    /* Fork server */
    pid = fork();
    if (pid == -1) return -1;
    if (pid != 0) return pid;
    /* Rewrite argv
     * NOTE: Assuming the program name is always present. */
    for (p = argv[0]; *p; p++) {
        if ('a' <= *p && *p <= 'z') *p += 'A' - 'a';
    }
    /* Detach from session */
    daemon(0, 0);
    /* Run main loop */
    ret = server_main(handler);
    /* Done */
    _exit((ret == 0) ? 0 : 1);
    return -1;
}
