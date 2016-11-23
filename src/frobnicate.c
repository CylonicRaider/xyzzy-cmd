
#include "frobnicate.h"

#include <stdlib.h>
#include <string.h>

static inline void frobrnd(uint32_t *state) {
    *state *= 1103515245;
    *state += 12345;
}

void frob(uint32_t key, const char *src, char *dest) {
    uint8_t in;
    do {
        in = *src++;
        key ^= in << 16;
        *dest++ = key >> 16;
        frobrnd(&key);
    } while (in);
}

void frobl(uint32_t key, const char *src, char *dest, size_t l) {
    size_t i;
    for (i = l; i > 0; i--) {
        key ^= ((uint8_t) *src++) << 16;
        *dest++ = key >> 16;
        frobrnd(&key);
    }
}

void defrob(uint32_t key, const char *src, char *dest) {
    do {
        *dest = (key >> 16) ^ *src++;
        key ^= ((uint8_t) *dest) << 16;
        frobrnd(&key);
    } while (*dest++);
}

void defrobl(uint32_t key, const char *src, char *dest, size_t l) {
    size_t i;
    for (i = l; i > 0; i--) {
        *dest = (key >> 16) ^ *src++;
        key ^= ((uint8_t) *dest++) << 16;
        frobrnd(&key);
    }
}

void frobclr(char *str) {
    memset(str, 0, strlen(str));
}

void frobclrl(char *str, size_t l) {
    memset(str, 0, l);
}
