
#ifndef _NOTE_H
#define _NOTE_H

#include <stddef.h>
#include <sys/time.h>
#include <sys/types.h>

#define NOTE_SIZE(n) (offsetof(struct note, content) + (n)->length)

struct note {
    struct timeval time;
    size_t length;
    uid_t sender;
    char content[];
};

/* Initialize the given note with the current user and time */
int note_init(struct note *note);

/* Initialize a note with the current parameters and read its content from
 * the given file descriptor
 * note is assumed to be dynamically allocated, and will be realloc()-ed
 * as necessary (in particular if it is NULL); the new value for it is
 * returned. */
struct note *note_read(int fd, struct note *note);

/* Write a human-readable representation of the note to a file descriptor */
int note_print(int fd, const struct note *note);

/* Pack a list of notes into the given structure
 * buf is realloc()-ed as necessary, and may be NULL, the new value is
 * returned (or NULL in case of error). length (if not NULL) is  written to
 * the new length of buf. header bytes of free (and uninitialized) space are
 * kept in the beginning of the buffer. The array of note pointers is
 * terminated by a NULL. */
char *note_pack(char *buf, size_t *length, int header,
                const struct note *notes[]);

/* Unpack a list of notes into a dynamically allocated array of pointers
 * A header argument is not provided, the caller has to increment buf
 * accordingly before passing it if necessary. length is the length of the
 * buffer (exluding any headers). ptrbuf is realloc()-ed as necessary (it
 * may in particular be NULL); the return value is the new value for it,
 * and points to a NULL-terminated array of note pointers; the note objects
 * themselves are dynamically allocated as well. */
struct note **note_unpack(const char *buf, size_t length,
                          struct note *ptrbuf[]);

#endif
