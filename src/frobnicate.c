
#include "frobnicate.h"

#include <stdlib.h>
#include <string.h>

static inline void frobrnd(uint32_t *state) {
    *state *= 1103515245;
    *state += 12345;
}

void frob(uint32_t key, const uchar *src, uchar *dest) {
    uchar in;
    do {
        in = *src++;
        key ^= in << 16;
        *dest++ = key >> 16;
        frobrnd(&key);
    } while (in);
}

void frobl(uint32_t key, const uchar *src, uchar *dest, size_t l) {
    for (size_t i = l; i > 0; i--) {
        key ^= *src++ << 16;
        *dest++ = key >> 16;
        frobrnd(&key);
    }
}

void defrob(uint32_t key, const uchar *src, uchar *dest) {
    do {
        *dest = (key >> 16) ^ *src++;
        key ^= *dest << 16;
        frobrnd(&key);
    } while (*dest++);
}

void defrobl(uint32_t key, const uchar *src, uchar *dest, size_t l) {
    for (size_t i = l; i > 0; i--) {
        *dest = (key >> 16) ^ *src++;
        key ^= *dest++ << 16;
        frobrnd(&key);
    }
}

void frobclr(uchar *str) {
    memset(str, 0, strlen((char *) str));
}
