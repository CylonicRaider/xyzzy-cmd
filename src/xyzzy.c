
/* The purpose of this is enigmatic. */

#include <stdio.h>

#include "frobnicate.h"
#include "strings.frs.h"

void init_strings() {
    struct frobstring *p;
    for (p = strings; p->str != NULL; p++) {
        defrob(p->key, p->str, p->str);
    }
}

int main(int argc, char *argv[]) {
    init_strings();
    puts((char *) hello);
    return 42;
}
