
#ifndef _NOTE_H
#define _NOTE_H

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>

struct note {
    uid_t sender;
    struct timeval time;
    size_t length;
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
int note_print(int fd, struct note *note);

#endif
