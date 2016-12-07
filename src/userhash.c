
#include <stdlib.h>

#include "userhash.h"

#define BASE_CAPACITY 17
#define CAPACITY_INCR 3
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

struct uhnode *userhash_get(struct userhash *ht, uid_t uid) {
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
    if (status_init(&node->status)) {
        free(node);
        return NULL;
    }
    if (ht->datacount * LOAD_THRESHOLD > ht->datacap) {
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
