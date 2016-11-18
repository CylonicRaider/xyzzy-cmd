
#ifndef _DIE_H
#define _DIE_H

#include <stdio.h>

#define die(msg) do { fprintf(stderr, "%s\n", msg); _exit(1); } while (0)
#define die_err(msg) do { perror(msg); _exit(1); } while (0)

#endif
