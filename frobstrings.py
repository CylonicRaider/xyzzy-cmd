#!/usr/bin/env python3
# -*- coding: ascii -*-

import sys, subprocess

ESCAPES = {"'": "'", '"': '"', '?': '?', '\\': '\\', 'a': '\a', 'b': '\b',
           'f': '\f', 'n': '\n', 'r': '\r', 't': '\t', 'v': '\v'}
# Octals are up to three characters long and end at the first non-octal.
# Hexadecimals can be arbitrarily long.

def parse_tmpl(inpt):
    raise NotImplementedError

def main():
    raise NotImplementedError

if __name__ == '__main__': main()
