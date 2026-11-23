import re
TOK = re.compile(r'''
 (?P<ws>\s+) | (?P<lc>//[^\n]*) | (?P<bc>/\*.*?\*/)
|(?P<num>0x[0-9a-fA-F]+|(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?|\$pi|\$e|\$phi|\$'(?:\\.|[^'])+'|'(?:\\.|[^'])+')
|(?P<str>"(?:\\.|[^"\\])*")
|(?P<sid>\#[A-Za-z_0-9.]*)
|(?P<id>[A-Za-z_][A-Za-z_0-9.]*)
|(?P<op>===|!==|<<=|>>=|==|!=|<=|>=|&&|\|\||<<|>>|\+=|-=|\*=|/=|%=|\^=|\|=|&=|~=|[-+*/%^|&~!<>=?:;,()\[\]])
''', re.S | re.X)
def lex(src):
    out = []; pos = 0; line = 1
    while pos < len(src):
        m = TOK.match(src, pos)
        if not m: raise SyntaxError("lex error line %d: %r" % (line, src[pos:pos+40]))
        k = m.lastgroup; t = m.group(k)
        if k not in ('ws','lc','bc'): out.append((k, t, line))
        line += t.count('\n'); pos = m.end()
    out.append(('eof','',line))
    return out
def sections(path):
    lines = open(path, encoding='utf-8').read().split('\n')
    secs = {}; cur = None; buf = []; start = 0
    for i, l in enumerate(lines):
        if l.startswith('@'):
            if cur: secs[cur] = (start, '\n'.join(buf))
            cur = l.split()[0][1:]; buf = []; start = i + 2
        elif cur: buf.append(l)
    if cur: secs[cur] = (start, '\n'.join(buf))
    return secs
