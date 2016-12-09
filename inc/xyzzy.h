
#ifndef _XYZZY_H
#define _XYZZY_H

#define MSG_RESERVED  0

#define CMD_PING      1
#define CMD_STATUS    2
#define CMD_READ      3
#define CMD_WRITE     4

#define RSP_PING     -1
#define RSP_STATUS   -2
#define RSP_READ     -3
#define RSP_WRITE    -4

enum main_action { PING, STATUS, ON, OFF, READ, WRITE, HELP, ERROR };

#endif
