
#include "frobnicate.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "die.h"

static inline void frobrnd(uint32_t *state) {
    *state *= 1103515245;
    *state += 12345;
}

/* Scramble the given string */
void frob(uint32_t key, uchar const* src, uchar *dest) {
    do {
        key ^= *src << 16;
        *dest++ = key >> 16;
        frobrnd(&key);
    } while (*src++);
}

/* Descramble the given string */
void defrob(uint32_t key, uchar const* src, uchar *dest) {
    do {
        key ^= *src++ << 16;
        *dest++ = key >> 16;
        frobrnd(&key);
    } while (*dest++);
}

/* Zero out the given string */
void frobclr(uchar *str) {
    memset(str, 0, strlen((char *) str));
}

#ifdef FROBNICATE_STANDALONE

union numbuf {
    uint32_t num;
    char str[4];
};

/* Frobnicate uint32_t-key-uint32_t-length-and-prefixed strings from stdin to
 * stdout */
int main(int argc, char *argv[]) {
    uchar *buf = NULL;
    for (;;) {
        /* Read key and length */
        union numbuf rdbuf[2];
        int32_t l, key;
        int rd = read(STDIN_FILENO, rdbuf, 8), wr;
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
        frob(key, buf, buf);
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

#endif
