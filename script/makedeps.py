#!/usr/bin/env python3
# -*- coding: ascii -*-

import sys, os, re

ALLOWED_RE = re.compile(r'^.*(?<!\.frs)\.c$')
PROGRAM_RE = re.compile(r'^.*:.*$')
INCLUDE_RE = re.compile(r'\s*#\s*include\s+"([^"]*)"')

HEADER_RE = re.compile(r'^inc/(.*)\.h$')
SOURCE_RE = re.compile(r'^src/(.*)\.c$')
OBJECT_RE = re.compile(r'^build/(.*)\.o$')

HEADER_SUB = r'inc/\1.h'
SOURCE_SUB = r'src/\1.c'
OBJECT_SUB = r'build/\1.o'

def read_deps(f, basedir=None):
    if basedir is None:
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

def get_deps(f, basedir=None, cache=None, _stack=None):
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
        for rd in read_deps(f, basedir):
            ret.append(rd)
            nd = get_deps(rd, basedir, cache, _stack)
            if nd is None: continue
            ret.extend(nd)
        cache[f] = frozenset(ret)
    return cache[f]

def get_link_deps(f, basedir=None, cache=None, lcache=None, _stack=None):
    if cache is None:
        cache = {}
    if lcache is None:
        lcache = {}
    if _stack is None:
        _stack = ()
    elif f in _stack:
        sys.stderr.write('WARNING: Circular dependency ' +
            ' <- '.join(_stack) + ' <- ' + f + ' found\n')
        sys.stderr.flush()
        return None
    if f not in lcache:
        ret = []
        _stack += (f,)
        src = OBJECT_RE.sub(SOURCE_SUB, f)
        deps = get_deps(src, basedir, cache)
        for d in deps:
            s = HEADER_RE.sub(SOURCE_SUB, d)
            o = SOURCE_RE.sub(OBJECT_SUB, s)
            ret.append(o)
            ret.extend(HEADER_RE.sub(OBJECT_SUB, i)
                       for i in get_deps(s, basedir, cache))
        lcache[f] = frozenset(ret)
    return lcache[f]

def main():
    basedir = os.environ.get('INCLUDE')
    cache, lcache, progs, proglist = {}, {}, {}, []
    for f in sys.argv[1:]:
        if PROGRAM_RE.match(f):
            prog, sep, dep = f.partition(':')
            if prog not in progs: proglist.append(prog)
            progs.setdefault(prog, []).append(dep)
            continue
        if not ALLOWED_RE.match(f): continue
        l = get_deps(f, basedir, cache)
        if not l: continue
        print (SOURCE_RE.sub(OBJECT_SUB, f) + ': ' + ' '.join(l))
    print ('')
    for p in proglist:
        l = set()
        for f in progs[p]:
            l.add(f)
            l.update(get_link_deps(f, basedir, cache, lcache))
        if not l: continue
        print (p + ': ' + ' '.join(l))

if __name__ == '__main__': main()
