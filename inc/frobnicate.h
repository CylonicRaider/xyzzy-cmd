
#ifndef _FROBNICATE_H
#define _FROBNICATE_H

#include <inttypes.h>
#include <stddef.h>

#define FROBKEYLEN 8

extern char _frobkey[FROBKEYLEN];

/* Perform the key schedule on the given IV */
uint32_t frobks(uint32_t key);

/* Scramble the given memory block without performing the key schedule
 * If l is negative, frobnication is done until a NUL byte is encountered in
 * the plaintext. The amount of bytes actually scrambled is returned. */
size_t frobr(uint32_t *key, const char *src, char *dest, size_t l);

/* Descramble the given memory block without performing the key schedule */
size_t defrobr(uint32_t *key, const char *src, char *dest, size_t l);

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
