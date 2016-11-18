
#ifndef _FROBNICATE_H
#define _FROBNICATE_H

#include <inttypes.h>

typedef unsigned char uchar;

/* Scramble the given string */
void frob(uint32_t key, const uchar *src, uchar *dest);

/* Descramble the given string */
void defrob(uint32_t key, const uchar *src, uchar *dest);

/* Zero out the given string */
void frobclr(uchar *str);

#endif
