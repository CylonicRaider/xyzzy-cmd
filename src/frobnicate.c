
#include "frobnicate.h"

#include <stdlib.h>
#include <string.h>

static inline void frobrnd(uint32_t *state) {
    *state *= 1103515245;
    *state += 12345;
}

/* Scramble the given string */
void frob(uint32_t key, uchar const* src, uchar *dest) {
    uchar in;
    do {
        in = *src++;
        key ^= in << 16;
        *dest++ = key >> 16;
        frobrnd(&key);
    } while (in);
}

/* Descramble the given string */
void defrob(uint32_t key, uchar const* src, uchar *dest) {
    do {
        *dest = (key >> 16) ^ *src++;
        key ^= *dest << 16;
        frobrnd(&key);
    } while (*dest++);
}

/* Zero out the given string */
void frobclr(uchar *str) {
    memset(str, 0, strlen((char *) str));
}
