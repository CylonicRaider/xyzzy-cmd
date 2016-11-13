#!/usr/bin/env python3
# -*- coding: ascii -*-

import sys, re, subprocess

ESCAPE_RE = re.compile(r'\\([0-7]{1,3}|x[0-9a-fA-F]{2,}|[^0-7x])')
ESCAPES = {"'": "'", '"': '"', '?': '?', '\\': '\\', 'a': '\a', 'b': '\b',
           'f': '\f', 'n': '\n', 'r': '\r', 't': '\t', 'v': '\v'}
# Octals are up to three characters long and end at the first non-octal.
# Hexadecimals can be arbitrarily long. Unicode ones are left out.
REVESCAPES = {v: k for k, v in ESCAPES.items()}

TMPL_TOKEN = re.compile(r'(?P<ident>[a-zA-Z_][a-zA-Z0-9_]*)|(?P<op>=)|'
    r'(?P<str>"(?:[^"\\]|\\.)*")|(?P<end>;)|(?P<ignore>//.*$|\s+)')
SUCCESSORS = {'ident': ('op',), 'op': ('str',), 'str': ('str', 'end'),
              'end': ('ident',), None: ('ident',)}
NAMES = {'ident': 'identifier', 'op': 'operator', 'str': 'string',
         'end': 'statement terminator'}

PRINTABLE = re.compile(r'[ !#-.0-~]+')
HEXDIGITS = re.compile(r'[0-9a-fA-F]')
NONPRINTABLE = re.compile(r'[^ !#-.0-~]+')

if sys.version_info[0] <= 2:
    tobytes = str
    bytechr = chr
    tostr = str
else:
    tobytes = lambda x: x.encode('utf-8')
    bytechr = lambda x: bytes([x])
    tostr = lambda x: x.decode('utf-8')

def parse_tmpl(inpt):
    pstate, name, value = None, None, None
    for linenum, rawline in enumerate(inpt):
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
        idx, ll = 0, len(line)
        while idx < ll:
            # Grab next token; advance index
            m = TMPL_TOKEN.match(line, idx)
            if not m:
                raise ValueError('Bad input file (see line %s)' %
                                 (linenum + 1))
            idx = m.end()
            # Determine token type
            for ttype in ('ident', 'op', 'str', 'end', 'ignore'):
                token = m.group(ttype)
                if token: break
            else:
                # Should not happen
                raise SystemError('Bad token ?! (see line %s)' %
                                  (linenum + 1))
            if ttype == 'ignore':
                continue
            if ttype not in SUCCESSORS[pstate]:
                raise ValueError('Bad input file (see line %s; expected %s, '
                    'got %s)' % (linenum + 1,
                        ' or '.join(NAMES[i] for i in SUCCESSORS[pstate]),
                        NAMES[ttype]))
            pstate = ttype
            # Interpret token
            if pstate == 'ident':
                name = token
                value = []
            elif pstate == 'str':
                # Parse string literal
                parts = ESCAPE_RE.split(token[1:-1])
                for n, p in enumerate(parts):
                    if not n % 2:
                        value.append(tobytes(p))
                    elif p in ESCAPES:
                        value.append(tobytes(ESCAPES[p]))
                    elif p.isdigit():
                        try:
                            value.append(bytechr(int(p, 8)))
                        except ValueError:
                            raise ValueError('Bad octal escape (see line %s; '
                                'escape \\%s)' % (linenum + 1, p))
                    elif p.startswith('x'):
                        try:
                            value.append(bytechr(int(p[1:], 16)))
                        except ValueError:
                            raise ValueError('Bad hexadecimal escape (see '
                                'line %s; escape \\%s)' % (linenum + 1, p))
                    else:
                        raise ValueError('Bad escape (see line %s; '
                            'escape %r)' % (linenum + 1, p))
            elif pstate == 'end':
                yield ('string', name, b''.join(value))
                name, value = None, None
    if pstate not in (None, 'end'):
        raise ValueError('Unfinished statement at EOF')

def encode_string(s):
    idx, sl = 0, len(s)
    can_hex, ret = True, ['"']
    while idx < sl:
        m = PRINTABLE.match(s, idx)
        if m:
            if not can_hex and HEXDIGITS.match(s, idx):
                ret.append('""')
                can_hex = True
            ret.append(tostr(m.group()))
            idx = m.end()
        m = NONPRINTABLE.match(s, idx)
        if not m: continue
        idx = m.end()
        for ch in m.group():
            if ch in REVESCAPES:
                ret.append('\\' + REVESCAPES[ch])
                can_hex = True
            else:
                ret.append('\\x%02x' % ord(ch))
                can_hex = False
    ret.append('"')
    return ''.join(ret)

def main():
    for token in parse_tmpl(sys.stdin):
        if token[0] == 'prep':
            sys.stdout.write(token[0] + '\n')
        elif token[0] == 'string':
            sys.stdout.write('%s = %s;\n' % (token[1],
                encode_string(token[2])))
        else:
            raise SystemExit('Bad token type: %r' % token[0])

if __name__ == '__main__': main()
