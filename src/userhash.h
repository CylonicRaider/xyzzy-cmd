
#ifndef _USERHASH_H
#define _USERHASH_H

#include <unistd.h>

#include "status.h"

#define NOTES_MAX 1048576

struct uhnode {
    struct uhnode *next;
    uid_t uid;
    struct status status;
    struct note **notes;
    size_t notecap, notesize;
};

struct userhash {
    struct uhnode **data;
    size_t datacap, datacount;
};

/* Initialize the userhash structure to be empty
 * Returns zero on success and -1 on failure. */
int userhash_init(struct userhash *ht);
/* Dispose of all the contents of hash */
void userhash_del(struct userhash *ht);

/* Return the specified node from the hash, or NULL if none */
struct uhnode *userhash_get(struct userhash *ht, uid_t uid);
/* Return the specified node from the hash (allocating if necesssary)
 * Returns NULL on error. */
struct uhnode *userhash_make(struct userhash *ht, uid_t uid);

#endif
