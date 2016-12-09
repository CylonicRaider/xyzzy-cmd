
#ifndef _CLIENT_H
#define _CLIENT_H

/* Connect to the server
 * Returns the socket FD on success, or -1 on failure (settting errno) */
int client_connect(void);

#endif
