#!/usr/bin/env python3
# -*- coding: ascii -*-

import sys, os, re, struct, ast, subprocess

ESCAPE_RE = re.compile(r'\\([0-7]{1,3}|x[0-9a-fA-F]{2,}|[^0-7x])')
ESCAPES = {"'": "'", '"': '"', '?': '?', '\\': '\\', 'a': '\a', 'b': '\b',
           'f': '\f', 'n': '\n', 'r': '\r', 't': '\t', 'v': '\v', '0': '\0'}
# Octals are up to three characters long and end at the first non-octal.
# Hexadecimals can be arbitrarily long. Unicode ones are left out.
REVESCAPES = {v: k for k, v in ESCAPES.items()}

TMPL_TOKEN = re.compile(r'(?P<ident>[a-zA-Z_][a-zA-Z0-9_]*)|(?P<op>=)|'
    r'(?P<str>"(?:[^"\\]|\\.)*")|(?P<end>;)|(?P<ignore>//.*$|\s+)|'
    r'(?P<comment>/\*.*?\*/)')
SUCCESSORS = {'ident': ('op',), 'op': ('str',), 'str': ('str', 'end'),
              'end': ('ident', 'comment'), 'comment': ('comment', 'ident'),
              None: ('ident', 'comment')}
NAMES = {'ident': 'identifier', 'op': 'operator', 'str': 'string',
         'end': 'statement terminator', 'comment': 'documentation comment'}

PRINTABLE = re.compile(rb'[ !#-.0-\[\]~]+')
HEXDIGITS = re.compile(rb'[0-9a-fA-F]')
NONPRINTABLE = re.compile(rb'[^ !#-.0-\[\]~]+')

if sys.version_info[0] <= 2:
    tobytes = str
    bytechr = chr
    tostr = str
else:
    tobytes = lambda x: x.encode('utf-8')
    bytechr = lambda x: bytes([x])
    tostr = lambda x: x.decode('utf-8')

REVBINESCAPES = {tobytes(k): v for k, v in REVESCAPES.items()}

def parse_tmpl(inpt):
    pstate, name, value = None, None, None
    for linenum, rawline in enumerate(inpt):
        # End when no lines left
        if not rawline: break
        line = rawline.strip()
        # Ignore empty lines
        if not line:
            yield ('empty',)
            continue
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
            for ttype in ('ident', 'op', 'str', 'end', 'ignore', 'comment'):
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
            if ttype == 'ident':
                name = token
                value = []
            elif ttype == 'str':
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
                            raise ValueError('Bad octal escape '
                                '(see line %s; escape \\%s)' %
                                (linenum + 1, p))
                    elif p.startswith('x'):
                        try:
                            value.append(bytechr(int(p[1:], 16)))
                        except ValueError:
                            raise ValueError('Bad hexadecimal escape (see '
                                'line %s; escape \\%s)' % (linenum + 1, p))
                    else:
                        raise ValueError('Bad escape (see line %s; '
                            'escape %r)' % (linenum + 1, p))
            elif ttype == 'end':
                yield ('string', name, b''.join(value))
                name, value = None, None
            elif ttype == 'comment':
                yield ('comment', token)
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
            if isinstance(ch, int): ch = bytechr(ch)
            if ch in REVBINESCAPES:
                ret.append('\\' + REVBINESCAPES[ch])
                can_hex = True
            else:
                ret.append('\\x%02x' % ord(ch))
                can_hex = False
    ret.append('"')
    return ''.join(ret)
def encode_bytes(s):
    if not s:
        return '{}'
    elif isinstance(s[0], int):
        ret = [i for i in s]
    else:
        ret = map(ord, s)
    return '{ ' + ', '.join(map(str, ret)) + ' }'

def write_pkt(f, k, s):
    f.write(struct.pack('!II', k, len(s)) + s)
    f.flush()
def read_pkt(f):
    k, l = struct.unpack('!II', f.read(8))
    s = f.read(l)
    if len(s) != l: raise EOFError('Short read')
    return (k, s)

def open_file(name, mode, default):
    if name is None or name == '-':
        return default
    else:
        return open(name, mode)

def main():
    def writehdr(line):
        if not hdrstream: return
        if el['hdr']: hdrstream.write('\n')
        hdrstream.write(line + '\n')
        el['hdr'] = False
    def writeout(line):
        if el['out']: outstream.write('\n')
        outstream.write(line + '\n')
        el['out'] = False
    # Parse arguments
    infile, hdrfile, outfile, keylen = None, None, None, 8
    try:
        it = iter(sys.argv[1:])
        for arg in it:
            if arg == '--help':
                sys.stderr.write('USAGE: %s [--help] [-o outfile] '
                                 '[-h hdrfile] [-l keylen] [infile]\n' %
                                 sys.argv[0])
                sys.stderr.write('If outfile or infile are "-" or missing, '
                                 'standard streams are used. If hdrfile '
                                 'is "-", standard output is written, if '
                                 'omitted, no header file is written at '
                                 'all. If outfile and hdrfile are "-", '
                                 'a huge mess is the result.\n')
                raise SystemExit
            elif arg == '-o':
                outfile = next(it)
            elif arg == '-h':
                hdrfile = next(it)
            elif arg == '-l':
                keylen = int(next(it))
            elif arg.startswith('-'):
                raise SystemExit('Bad option %s!' % arg)
            elif infile is not None:
                raise SystemExit('Too many positional arguments!')
            else:
                infile = arg
    except StopIteration:
        raise SystemExit('Missing required argument for option %s!' % arg)
    except ValueError:
        raise SystemExit('Bad argument for option %s!' % arg)
    instream = open_file(infile, 'r', sys.stdin)
    outstream = open_file(outfile, 'w', sys.stdout)
    if hdrfile is None:
        hdrstream = None
    else:
        hdrstream = open_file(hdrfile, 'w', sys.stdout)
    # State
    listtype, names, mkey, mkey_sent = None, [], None, False
    keytype, lentype, chartype = 'int', 'int', 'char'
    listkeys = False
    el = {'out': False, 'hdr': False}
    # Spawn frobnication server
    proc = subprocess.Popen(['./frobnicate'], stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE)
    for token in parse_tmpl(instream):
        if token[0] == 'empty':
            el['out'] = True
            el['hdr'] = True
        elif token[0] == 'comment':
            if hdrstream:
                writehdr(token[1])
            else:
                writeout(token[1])
        elif token[0] == 'prep':
            parts = token[1].split()
            if parts and parts[0] == '#listkeys':
                if not 1 <= len(parts) <= 2:
                    raise SystemExit('Bad listkeys pragma')
                elif len(parts) == 1:
                    listkeys = True
                else:
                    v = ast.literal_eval(parts[1])
                    if not isinstance(v, int):
                        raise SystemExit('Bad listkeys pragma')
                    listkeys = bool(v)
            elif parts and parts[0] == '#listdecl':
                if len(parts) != 2:
                    raise SystemExit('Bad listdecl pragma')
                elif listtype is not None:
                    raise SystemExit('Repeated listdecl pragma')
                listtype = parts[1]
                line = ('struct %s { %s %skey; %s len; %s *str; };' %
                        (listtype, keytype, ('' if listkeys else '*'),
                         lentype, chartype))
                if hdrstream:
                    writehdr(line)
                else:
                    writeout(line)
            elif parts and parts[0] == '#listdef':
                if len(parts) != 2:
                    raise SystemExit('Bad listdef pragma')
                elif listtype is None:
                    raise SystemExit('listdef pragma without listdecl!')
                writehdr('struct %s %s[%s];' % (listtype, parts[1],
                                                len(names) + 1))
                writeout('struct %s %s[%s] = {' % (listtype, parts[1],
                                                   len(names) + 1))
                if listkeys:
                    for n in names:
                        writeout('    { 0x%08x, %s, %s },' %
                                 (n[1], n[2], n[0]))
                    writeout('    { 0, 0, NULL }\n};')
                else:
                    for n in names:
                        writeout('    { &%s_key, %s, %s },' %
                                 (n[0], n[2], n[0]))
                    writeout('    { NULL, 0, NULL }\n};')
                listtype, name = None, []
            elif parts and parts[0] == '#includehdr':
                if not 1 <= len(parts) <= 2:
                    raise SystemExit('Bad includehdr pragma')
                elif hdrfile in (None, '', '-'):
                    raise SystemExit('Cannot infer header location')
                if hdrstream:
                    if len(parts) > 1:
                        writeout('#include "%s"' % (os.path.join(parts[1],
                                                                 hdrfile)))
                    else:
                        writeout('#include "%s"' % hdrfile)
            elif parts and parts[0] == '#keytype':
                if len(parts) != 2:
                    raise SystemExit('Bad keytype pragma')
                keytype = parts[1]
            elif parts and parts[0] == '#lentype':
                if len(parts) != 2:
                    raise SystemExit('Bad lentype pragma')
                lentype = parts[1]
            elif parts and parts[0] == '#chartype':
                if len(parts) != 2:
                    raise SystemExit('Bad chartype pragma')
                chartype = parts[1]
            elif parts and parts[0] == '#defsize':
                if len(parts) != 2:
                    raise SystemExit('Bad defsize pragma')
                if hdrstream:
                    writehdr('#define %s(b) (sizeof(b) - 1)' % parts[1])
                else:
                    writeout('#define %s(b) (sizeof(b) - 1)' % parts[1])
            elif parts and parts[0] == '#makekey':
                if len(parts) != 1:
                    raise SystemExit('Bad makekey pragma')
                mkey = os.urandom(keylen)
                writehdr('extern %s _frobkey[%s];' % (chartype, keylen))
                writeout('%s _frobkey[%s] = %s;' % (chartype, keylen,
                    encode_bytes(mkey)))
            else:
                if hdrstream:
                    writehdr(token[1])
                else:
                    writeout(token[1])
        elif token[0] == 'string':
            n, s = token[1], token[2]
            k = struct.unpack('!I', os.urandom(4))[0]
            s += b'\0'
            if not mkey_sent:
                if mkey is None:
                    raise SystemExit('Declaring strings without a key!')
                proc.stdin.write(mkey)
                mkey_sent = True
            write_pkt(proc.stdin, k, s)
            nk, ns = read_pkt(proc.stdout)
            if hdrstream:
                if not listkeys or not listtype:
                    writehdr('%s %s_key;'  % (keytype, n))
                writehdr('%s %s[%s];' % (chartype, n, len(s)))
            if not listkeys or not listtype:
                writeout('%s %s_key = 0x%x;' % (keytype, n, nk))
            writeout('%s %s[%s] = %s;' % (chartype, n, len(s),
                                          encode_bytes(ns)))
            if listtype is not None:
                names.append((n, nk, len(s) - 1))
        else:
            raise SystemExit('Bad token type: %r' % token[0])
    if hdrstream:
        hdrstream.flush()
    outstream.flush()

if __name__ == '__main__': main()
