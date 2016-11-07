
#include "frobnicate.h"

#include <inttypes.h>
#include <string.h>

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

/* Frobnicate int32_t-length-prefixed strings from stdin to stdout */
int main(int argc, char *argv[]) {
    /* TODO */
    return 0;
}

#endif
