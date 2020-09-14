
#include "frobnicate.h"

#include <stdlib.h>
#include <string.h>

static inline void frobrnd(uint32_t *state) {
    *state *= 1103515245;
    *state += 12345;
    *state ^= *state >> 24;
}

static inline uint32_t _frobks(uint32_t key) {
    uint32_t ret = 0;
    int i;
    for (i = 0; i < 4; i++) {
        ret ^= (key >> (i * 8 - 8) & 0xFF) << 16;
        frobrnd(&ret);
    }
    for (i = 0; i < sizeof(_frobkey); i++) {
        ret ^= _frobkey[i] << 16;
        frobrnd(&ret);
    }
    return ret;
}

static inline size_t _frobr(uint32_t *key, const char *src, char *dest,
                            ssize_t l) {
    size_t ret;
    uint32_t k = *key;
    for (ret = 0; ret != l; ret++) {
        uint8_t in = *src++;
        k ^= in << 16;
        *dest++ = k >> 16;
        frobrnd(&k);
        if (l < 0 && ! in) break;
    }
    *key = k;
    return ret;
}

static inline size_t _defrobr(uint32_t *key, const char *src, char *dest,
                              ssize_t l) {
    size_t ret;
    uint32_t k = *key;
    for (ret = 0; ret != l; ret++) {
        *dest = (k >> 16) ^ *src++;
        k ^= ((uint8_t) *dest) << 16;
        frobrnd(&k);
        if (! *dest++ && l < 0) break;
    }
    *key = k;
    return ret;
}

uint32_t frobks(uint32_t key) {
    return _frobks(key);
}

size_t frobr(uint32_t *key, const char *src, char *dest, ssize_t l) {
    return _frobr(key, src, dest, l);
}

size_t defrobr(uint32_t *key, const char *src, char *dest, ssize_t l) {
    return _defrobr(key, src, dest, l);
}

void frob(uint32_t key, const char *src, char *dest) {
    key = _frobks(key);
    _frobr(&key, src, dest, -1);
}

void frobl(uint32_t key, const char *src, char *dest, size_t l) {
    key = _frobks(key);
    _frobr(&key, src, dest, l);
}

void defrob(uint32_t key, const char *src, char *dest) {
    key = _frobks(key);
    _defrobr(&key, src, dest, -1);
}

void defrobl(uint32_t key, const char *src, char *dest, size_t l) {
    key = _frobks(key);
    _defrobr(&key, src, dest, l);
}

void frobclr(char *str) {
    memset(str, 0, strlen(str));
}

void frobclrl(char *str, size_t l) {
    memset(str, 0, l);
}
