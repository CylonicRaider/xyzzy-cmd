#!/usr/bin/env python3
# -*- coding: ascii -*-

import sys, re, subprocess

ESCAPES = {"'": "'", '"': '"', '?': '?', '\\': '\\', 'a': '\a', 'b': '\b',
           'f': '\f', 'n': '\n', 'r': '\r', 't': '\t', 'v': '\v'}
# Octals are up to three characters long and end at the first non-octal.
# Hexadecimals can be arbitrarily long.

TMPL_TOKEN = re.compile(r'(?P<ident>[a-zA-Z_][a-zA-Z0-9_]*)|(?P<op>=)|'
    r'(?P<str>"(?:[^"\\\\]|\\\\.)*")|(?P<comment>//.*$)')

def parse_tmpl(inpt):
    for rawline in inpt:
        # End when no lines left
        if not rawline: break
        line = rawline.strip()
        # Ignore empty lines
        if not line: continue
        # Respect preprocessor instructions
        if line.startswith('#'):
            yield ('prep', line)
            continue
        # Parse!
        raise NotImplementedError

def main():
    raise NotImplementedError

if __name__ == '__main__': main()
