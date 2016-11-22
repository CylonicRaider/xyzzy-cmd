
#ifndef _FROBNICATE_H
#define _FROBNICATE_H

#include <inttypes.h>

typedef unsigned char uchar;

/* Scramble the given string */
void frob(uint32_t key, const uchar *src, uchar *dest);

/* Scramble the given memory region */
void frobl(uint32_t key, const uchar *src, uchar *dest, size_t l);

/* Descramble the given string */
void defrob(uint32_t key, const uchar *src, uchar *dest);

/* Descramble the given memory area */
void defrobl(uint32_t key, const uchar *src, uchar *dest, size_t l);

/* Zero out the given string */
void frobclr(uchar *str);

#endif
