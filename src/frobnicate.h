
#ifndef _FROBNICATE_H
#define _FROBNICATE_H

#include <inttypes.h>

/* Scramble the given string */
void frob(uint32_t key, const char *src, char *dest);

/* Scramble the given memory region */
void frobl(uint32_t key, const char *src, char *dest, size_t l);

/* Descramble the given string */
void defrob(uint32_t key, const char *src, char *dest);

/* Descramble the given memory area */
void defrobl(uint32_t key, const char *src, char *dest, size_t l);

/* Zero out the given string */
void frobclr(char *str);

/* Zero out the given memory block */
void frobclrl(char *str, size_t l);

#endif
