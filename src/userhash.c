
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "userhash.h"

/* Initial capacity indeed */
#define BASE_CAPACITY 17
/* Multiplied by that */
#define CAPACITY_INCR 3
/* (1 - 1/LOAD_THRESHOLD) is the actual value */
#define LOAD_THRESHOLD 3

static inline size_t hash(uid_t uid, size_t reduce) {
    return ((size_t) uid) % reduce;
}

static inline void insert(struct uhnode **buffer, size_t capacity,
                          struct uhnode *node) {
    struct uhnode **bucket = buffer + hash(node->uid, capacity);
    node->next = *bucket;
    *bucket = node;
}

int userhash_init(struct userhash *ht) {
    ht->data = calloc(BASE_CAPACITY, sizeof(struct uhnode *));
    if (ht->data == NULL) return -1;
    ht->datacap = BASE_CAPACITY;
    ht->datacount = 0;
    return 0;
}

void userhash_del(struct userhash *ht) {
    size_t i;
    for (i = 0; i < ht->datacap; i++) {
        struct uhnode *node = ht->data[i], *next;
        while (node != NULL) {
            size_t j;
            next = node->next;
            for (j = 0; j < node->notesize; j++) {
                free(node->notes[j]);
            }
            free(node);
            node = next;
        }
    }
}

struct uhnode *userhash_get(const struct userhash *ht, uid_t uid) {
    size_t h = hash(uid, ht->datacap);
    struct uhnode *node;
    for (node = ht->data[h]; node != NULL; node = node->next) {
        if (node->uid == uid) return node;
    }
    return NULL;
}

struct uhnode *userhash_make(struct userhash *ht, uid_t uid) {
    size_t h = hash(uid, ht->datacap);
    struct uhnode *node;
    for (node = ht->data[h]; node != NULL; node = node->next) {
        if (node->uid == uid) return node;
    }
    ht->datacount++;
    node = calloc(1, sizeof(struct uhnode));
    if (node == NULL) return NULL;
    node->uid = uid;
    if (status_init(&node->status)) {
        free(node);
        return NULL;
    }
    if ((ht->datacap - ht->datacount) * LOAD_THRESHOLD > ht->datacap) {
        size_t newcap = ht->datacap * CAPACITY_INCR, i;
        struct uhnode **newdata = calloc(newcap, sizeof(struct uhnode *));
        if (newdata == NULL) {
            free(node);
            return NULL;
        }
        for (i = 0; i < ht->datacap; i++) {
            struct uhnode *n = ht->data[i];
            while (n != NULL) {
                struct uhnode *nn = n->next;
                insert(newdata, newcap, n);
                n = nn;
            }
        }
        free(ht->data);
        ht->data = newdata;
        ht->datacap = newcap;
    }
    insert(ht->data, ht->datacap, node);
    return node;
}

int uhnode_addnote(struct uhnode *node, struct note *note) {
    size_t size = NOTE_SIZE(note);
    if (node->notesize + size > NOTES_MAX) {
        errno = EDQUOT;
        return -1;
    }
    if (node->notelen + 2 > node->notecap) {
        size_t newcap = (node->notecap == 0) ? 2 : node->notecap * 2;
        struct note **newnotes = malloc(newcap);
        if (newnotes == NULL) return -1;
        memcpy(newnotes, node->notes, node->notelen * sizeof(struct note *));
        free(node->notes);
        node->notecap = newcap;
        node->notes = newnotes;
    }
    node->notesize += size;
    node->notes[node->notelen++] = note;
    node->notes[node->notelen] = NULL;
    return 0;
}

int uhnode_countnotes(const struct uhnode *node) {
    struct note **n;
    int ret = 0;
    if (node->notes == NULL) return 0;
    for (n = node->notes; *n; n++) ret++;
    return ret;
}

struct note **uhnode_popnotes(struct uhnode *node) {
    struct note **ret = node->notes;
    node->notecap = 0;
    node->notelen = 0;
    node->notesize = 0;
    node->notes = NULL;
    return ret;
}
