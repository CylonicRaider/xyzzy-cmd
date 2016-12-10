
#ifndef _SERVER_H
#define _SERVER_H

#define CLOSEFDS 1024

/* Handle a single client connection
 * Return -1 on failure (setting errno), or something else otherwise. */
typedef int (*srvhandler_t)(int fd, void *data);

/* Set up the server socket
 * Returns the socket FD on success, or -1 on error (setting errno). */
int server_listen(void);

/* Run the server main loop
 * ipipe, if not -1, is close()d after the server socket is set up, allowing
 * e.g. parent processes to gain knowledge of the event.
 * Returns zero on successful termination (if that happens), and -1 on
 * error, setting errno appropriately. */
int server_main(srvhandler_t handler, void *data, int ipipe);

/* Spawn the server in a separate process
 * Expects the argc and argv values from main().
 * Returns -1 if any failure (within this process) has occurred (setting
 * errno), or the PID of the child otherwise. */
int server_spawn(int argc, char *argv[], srvhandler_t handler, void *data);

#endif
