
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "frobnicate.h"

#define die(msg) do { fprintf(stderr, "%s\n", msg); _exit(1); } while (0)
#define die_err(msg) do { perror(msg); _exit(1); } while (0)

union numbuf {
    uint32_t num;
    char str[4];
};

char _frobkey[FROBKEYLEN];

/* Frobnicate uint32_t-key-and-uint32_t-length-prefixed strings from stdin to
 * stdout */
int main(int argc, char *argv[]) {
    char *buf = NULL;
    int rd = read(STDIN_FILENO, _frobkey, FROBKEYLEN);
    if (rd < 0) die_err("read");
    if (rd != FROBKEYLEN) die("Short read");
    for (;;) {
        /* Read key and length */
        union numbuf rdbuf[2];
        int32_t l, key;
        int wr;
        rd = read(STDIN_FILENO, rdbuf, 8);
        if (rd < 0) die_err("read");
        if (rd == 0) break;
        if (rd != 8) die("Short read");
        key = ntohl(rdbuf[0].num);
        l = ntohl(rdbuf[1].num);
        /* Read input */
        buf = realloc(buf, l);
        rd = read(STDIN_FILENO, buf, l);
        if (rd == -1) die_err("read");
        if (rd != l) die("Short read");
        /* Frobnicate */
        frobl(key, buf, buf, l);
        /* Write */
        wr = write(STDOUT_FILENO, rdbuf, 8);
        if (wr == -1) die_err("write");
        if (wr != 8) die("Short write");
        wr = write(STDOUT_FILENO, buf, l);
        if (wr == -1) die_err("write");
        if (wr != l) die("Short write");
    }
    return 0;
}
