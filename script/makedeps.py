#!/usr/bin/env python3
# -*- coding: ascii -*-

import sys, os, re

ALLOWED_RE = re.compile(r'^.*(?<!\.frs)\.c$')
INCLUDE_RE = re.compile(r'\s*#\s*include\s+"([^"]*)"')

OBJECT_RE = re.compile(r'^src/(.*)\.c$')
OBJECT_SUB = r'build/\1.o'

def read_deps(f):
    basedir = os.path.dirname(f)
    ret = []
    try:
        with open(f) as fp:
            for line in fp:
                m = INCLUDE_RE.match(line)
                if not m: continue
                d = m.group(1)
                ed = os.path.normpath(os.path.join(basedir, d))
                ret.append(ed)
    except IOError:
        pass
    return tuple(ret)

def get_deps(f, cache=None, _stack=None):
    if cache is None:
        cache = {}
    if _stack is None:
        _stack = ()
    elif f in _stack:
        sys.stderr.write('WARNING: Circular dependency ' +
            ' <- '.join(_stack) + ' <- ' + f + ' found\n')
        sys.stderr.flush()
        return None
    if f not in cache:
        ret = []
        _stack += (f,)
        for rd in read_deps(f):
            ret.append(rd)
            nd = get_deps(rd, cache, _stack)
            if nd is None: continue
            ret.extend(nd)
        cache[f] = tuple(ret)
    return cache[f]

def main():
    cache = {}
    for f in sys.argv[1:]:
        if not ALLOWED_RE.match(f): continue
        l = get_deps(f, cache)
        if not l: continue
        print (OBJECT_RE.sub(OBJECT_SUB, f) + ': ' + ' '.join(l))

if __name__ == '__main__': main()
