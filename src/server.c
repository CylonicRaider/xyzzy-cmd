
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

#include "comm.h"
#include "server.h"
#include "strings.frs.h"

int server_listen() {
    struct sockaddr_un addr;
    socklen_t addrlen;
    int one = 1;
    int ret = socket(AF_UNIX, SOCK_STREAM, 0);
    if (ret == -1) return -1;
    prepare_address(&addr, &addrlen);
    if (bind(ret, (void *) &addr, addrlen) == -1)
        goto error;
    if (listen(ret, SOMAXCONN) == -1)
        goto error;
    if (setsockopt(ret, SOL_SOCKET, SO_PASSCRED, &one, sizeof(one)) == -1)
        goto error;
    return ret;
    error:
        close(ret);
        return -1;
}

int server_main(srvhandler_t handler, void *data, int ipipe) {
    int sockfd, fd = -1;
    sockfd = server_listen();
    if (sockfd == -1) return -1;
    if (ipipe != -1) close(ipipe);
    for (;;) {
        fd = accept(sockfd, NULL, NULL);
        if (fd == -1 && errno != EINTR) goto error;
        if (fd != -1) {
            (*handler)(fd, data);
            fd = -1;
        }
        /* Do not summon zombies
         * Just in case. */
        for (;;) {
            if (waitpid(-1, NULL, WNOHANG) == -1) {
                if (errno == ECHILD) break;
                goto error;
            }
        }
    }
    error:
        if (fd != -1) close(fd);
        close(sockfd);
        return -1;
}

int server_spawn(int argc, char *argv[], srvhandler_t handler, void *data) {
    char *p, buf[1];
    int pid, fd, ret, pipefds[2];
    /* Create communication pipe */
    if (pipe(pipefds) == -1) return -1;
    /* Fork server */
    pid = fork();
    if (pid == -1) return -1;
    if (pid != 0) {
        /* Wait for child to finish preparing */
        close(pipefds[1]);
        if (read(pipefds[0], buf, 1) == -1) return -1;
        close(pipefds[0]);
        /* Done */
        return pid;
    }
    /* Close read end */
    close(pipefds[0]);
    /* Rewrite argv
     * NOTE: Assuming the program name is always present. */
    for (p = argv[0]; *p; p++) {
        if ('a' <= *p && *p <= 'z') *p += 'A' - 'a';
    }
    /* Redirect stdio descriptors */
    fd = open(dev_null, O_RDWR);
    if (fd == -1) _exit(1);
    if (dup2(fd, STDIN_FILENO) == -1) _exit(1);
    if (dup2(fd, STDOUT_FILENO) == -1) _exit(1);
    if (dup2(fd, STDERR_FILENO) == -1) _exit(1);
    close(fd);
    /* Change to the root directory */
    if (chdir("/") == -1) _exit(1);
    /* Spawn new session */
    if (setsid() == -1) _exit(1);
    /* Fork another time */
    pid = fork();
    if (pid < 0) _exit(1);
    if (pid != 0) _exit(0);
    /* Run main loop */
    ret = server_main(handler, data, pipefds[1]);
    /* Done */
    _exit((ret == 0) ? 0 : 1);
    return -1;
}
