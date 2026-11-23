# EEL2 parser following WDL eel2.y exactly (precedence, associativity, assignment rules)
# plus the constant folding / rewrites of nseel-compiler.c optimizeOpcodes that change numerics.
import math, struct
from eellex import lex

class N:
    __slots__ = ('t', 'a', 'line')
    def __init__(self, t, *a, line=0):
        self.t = t; self.a = list(a); self.line = line
    def __repr__(self): return '%s%r' % (self.t, self.a)

def num(v): return N('num', float(v))
def isnum(n): return n is not None and n.t == 'num'

ASSIGN_OPS = {'=', '+=', '-=', '*=', '/=', '%=', '^=', '|=', '&=', '~='}
CMP_OPS = {'<', '>', '<=', '>=', '==', '===', '!=', '!=='}

class FuncDef:
    def __init__(self, name, params, locals_, body, line):
        self.name = name; self.params = params; self.locals = locals_; self.body = body; self.line = line

def numval(t):
    if t.startswith('0x'): return float(int(t, 16))
    if t == '$pi': return math.pi
    if t == '$e': return math.e
    if t == '$phi': return 1.61803398874989484820
    if t.startswith("$'") or t.startswith("'"):
        s = t[2:-1] if t.startswith('$') else t[1:-1]
        s = bytes(s, 'utf-8').decode('unicode_escape')
        v = 0
        for ch in s: v = v * 256 + ord(ch)
        return float(v)
    return float(t)

class Parser:
    def __init__(self, toks, funcs):
        self.t = toks; self.p = 0; self.funcs = funcs
    def peek(self, o=0): return self.t[self.p + o]
    def nx(self):
        tok = self.t[self.p]; self.p += 1; return tok
    def expect(self, v):
        tok = self.nx()
        if tok[1] != v: raise SyntaxError('line %d: expected %r got %r' % (tok[2], v, tok[1]))
        return tok
    def isop(self, v, o=0):
        k = self.peek(o); return k[0] == 'op' and k[1] == v

    # program / expression: if_else (';' if_else?)*   - also strips function definitions
    def expression(self, stop=(')',)):
        items = []
        while True:
            k = self.peek()
            if k[0] == 'eof' or (k[0] == 'op' and k[1] in stop): break
            if k[0] == 'op' and k[1] == ';': self.nx(); continue
            if k[0] == 'id' and k[1].lower() == 'function':
                self.funcdef(); continue
            items.append(self.if_else())
            k = self.peek()
            if k[0] == 'op' and k[1] == ';': self.nx(); continue
            break
        if not items: return None
        if len(items) == 1: return items[0]
        return N('seq', *items)

    def funcdef(self):
        line = self.nx()[2]
        name = self.nx()[1].lower()
        params = []
        self.expect('(')
        while not self.isop(')'):
            tok = self.nx()
            if tok[1] == ',': continue
            params.append(tok[1].lower())
        self.expect(')')
        locals_ = []
        while self.peek()[0] == 'id' and self.peek()[1].lower() in ('local', 'static', 'instance', 'globals', 'global'):
            kw = self.nx()[1].lower()
            if kw != 'local': raise SyntaxError('unsupported %s() line %d' % (kw, line))
            self.expect('(')
            while not self.isop(')'):
                tok = self.nx()
                if tok[1] == ',': continue
                locals_.append(tok[1].lower())
            self.expect(')')
        self.expect('(')
        body = self.expression()
        self.expect(')')
        if name in self.funcs: raise SyntaxError('function %s redefined line %d' % (name, line))
        self.funcs[name] = FuncDef(name, params, locals_, body, line)

    def if_else(self):
        c = self.lor()
        if self.isop('?'):
            self.nx()
            if self.isop(':'):
                self.nx(); b = self.if_else()
                return N('if', c, None, b)
            a = self.if_else()
            if self.isop(':'):
                self.nx(); b = self.if_else()
                return N('if', c, a, b)
            return N('if', c, a, None)
        return c

    def lor(self):
        l = self.cmp()
        while self.isop('&&') or self.isop('||'):
            op = self.nx()[1]; r = self.cmp(); l = N(op, l, r)
        return l

    def cmp(self):
        l = self.andor()
        while self.peek()[0] == 'op' and self.peek()[1] in CMP_OPS:
            op = self.nx()[1]; r = self.andor(); l = N(op, l, r)
        return l

    def andor(self):
        l = self.add()
        while self.peek()[0] == 'op' and self.peek()[1] in ('&', '|', '~'):
            op = self.nx()[1]; r = self.add(); l = N(op, l, r)
        return l

    def add(self):
        l = self.sub()
        while self.isop('+'):
            self.nx(); r = self.sub(); l = N('+', l, r)
        return l

    def sub(self):
        l = self.mul()
        while self.isop('-'):
            self.nx(); r = self.mul(); l = N('-', l, r)
        return l

    def mul(self):
        l = self.div()
        while self.isop('*'):
            self.nx(); r = self.div(); l = N('*', l, r)
        return l

    def div(self):
        l = self.mod()
        while self.isop('/'):
            self.nx(); r = self.mod(); l = N('/', l, r)
        return l

    def mod(self):
        l = self.pow()
        while self.peek()[0] == 'op' and self.peek()[1] in ('%', '<<', '>>'):
            op = self.nx()[1]; r = self.pow(); l = N(op, l, r)
        return l

    def pow(self):
        l = self.unary()
        while self.isop('^'):
            self.nx(); r = self.unary(); l = N('^', l, r)
        return l

    def unary(self):
        if self.isop('+'): self.nx(); return self.unary()
        if self.isop('-'): self.nx(); return N('neg', self.unary())
        if self.isop('!'): self.nx(); return N('not', self.unary())
        return self.assignment()

    def assignment(self):
        k = self.peek()
        if k[0] == 'sid' and self.peek(1)[0] == 'op' and self.peek(1)[1] in ('=', '+='):
            self.nx(); op = self.nx()[1]; rhs = self.if_else()
            return N('call', 'strcpy' if op == '=' else 'strcat', N('sid', k[1].lower()), rhs, line=k[2])
        lv, assignable = self.rvalue()
        k = self.peek()
        if k[0] == 'op' and k[1] in ASSIGN_OPS:
            if not assignable: raise SyntaxError('line %d: not assignable' % k[2])
            op = self.nx()[1]; rhs = self.if_else()
            return N('asg', op, lv, rhs, line=k[2])
        return lv

    def rvalue(self):
        k = self.nx()
        if k[0] == 'num': node = num(numval(k[1])); assignable = False
        elif k[0] == 'sid': node = N('sid', k[1].lower()); assignable = False
        elif k[0] == 'str':
            s = self.unq(k[1])
            while self.peek()[0] == 'str': s += self.unq(self.nx()[1])
            node = N('lit', s); assignable = False
        elif k[0] == 'op' and k[1] == '(':
            e = self.expression(); self.expect(')')
            node = e if e is not None else num(0); assignable = True
        elif k[0] == 'id':
            name = k[1].lower()
            if self.isop('('):
                self.nx()
                args = []
                if not self.isop(')'):
                    while True:
                        args.append(self.expression(stop=(')', ',')) or num(0))
                        if self.isop(','): self.nx(); continue
                        break
                self.expect(')')
                if name == 'while' and self.isop('('):
                    self.nx(); body = self.expression(); self.expect(')')
                    node = N('while2', args[0], body or num(0), line=k[2])
                else:
                    node = N('call', name, *args, line=k[2])
            else:
                node = N('var', name, line=k[2])
            assignable = True
        else:
            raise SyntaxError('line %d: unexpected %r' % (k[2], k[1]))
        while self.isop('['):
            self.nx()
            idx = None if self.isop(']') else self.expression(stop=(']',))
            self.expect(']')
            node = N('mem', node, idx, line=k[2]); assignable = True
        return node, assignable

    @staticmethod
    def unq(s):
        body = s[1:-1]
        out = []; i = 0
        while i < len(body):
            c = body[i]
            if c == '\\' and i + 1 < len(body):
                d = body[i + 1]
                m = {'n': '\n', 't': '\t', 'r': '\r', '\\': '\\', '"': '"', "'": "'", '0': '\0'}
                if d in m: out.append(m[d]); i += 2; continue
                out.append(c); i += 1; continue
            out.append(c); i += 1
        return ''.join(out)

def parse_section(src, line0, funcs):
    toks = [(k, v, l + line0) for (k, v, l) in lex(src)]
    p = Parser(toks, funcs)
    e = p.expression(stop=())
    if p.peek()[0] != 'eof': raise SyntaxError('trailing tokens at line %d: %r' % (p.peek()[2], p.peek()[1]))
    return e

# ---------------- optimizer (numeric-relevant rewrites of nseel optimizeOpcodes) ----------------
CLOSE = 0.00001
def f64(x): return struct.unpack('<d', struct.pack('<d', x))[0]
def cdiv(a, b):
    if b == 0.0:
        if a == 0.0 or a != a: return float('nan')
        return math.copysign(float('inf'), a) * math.copysign(1.0, b)
    return a / b
def trunc64(x):
    if x != x or abs(x) >= 9.2e18: return -(1 << 63)
    return int(x)
def ctrunc32(x):
    if x != x or abs(x) >= 2147483648.0: return -(1 << 31)
    return int(x)

PURE_FUNCS = {'abs', 'atan2', 'ceil', 'cos', 'exp', 'floor', 'log', 'log10', 'max', 'min', 'sin', 'sqr', 'sqrt', 'tan',
              'sign', 'pow', 'atan', 'asin', 'acos', 'invsqrt', 'strlen', 'str_getchar', 'strcmp', 'time_precise'}

def is_trivial(n):  # EEL OPCODE_IS_TRIVIAL: direct value or variable
    return n.t in ('num', 'var', 'sid', 'lit')

def opt(n):
    if n is None: return None
    t = n.t
    if t in ('num', 'var', 'sid', 'lit'): return n
    if t == 'seq':
        items = [opt(x) for x in n.a]
        flat = []
        for x in items:
            if x.t == 'seq': flat.extend(x.a)
            else: flat.append(x)
        # drop trivial non-last statements
        out = [x for x in flat[:-1] if not is_trivial(x)] + [flat[-1]]
        return out[0] if len(out) == 1 else N('seq', *out, line=n.line)
    if t == 'neg':
        x = opt(n.a[0])
        if isnum(x): return num(-x.a[0])
        return N('neg', x, line=n.line)
    if t == 'not':
        x = opt(n.a[0])
        if isnum(x): return num(0.0 if abs(x.a[0]) >= CLOSE else 1.0)
        if x.t == 'neg': return opt(N('not', x.a[0]))
        if x.t == 'not': return N('notnot', x.a[0])
        if x.t == 'notnot': return N('not', x.a[0])
        inv = {'==': '!=', '!=': '==', '===': '!==', '!==': '===', '<': '>=', '<=': '>', '>': '<=', '>=': '<'}
        if x.t in inv: return N(inv[x.t], x.a[0], x.a[1])
        return N('not', x, line=n.line)
    if t == 'if':
        c = opt(n.a[0]); a = opt(n.a[1]); b = opt(n.a[2])
        if isnum(c):
            r = a if abs(c.a[0]) >= CLOSE else b
            return r if r is not None else num(0)
        if c.t == 'notnot': c = c.a[0]
        return N('if', c, a, b, line=n.line)
    if t == 'mem':
        base = opt(n.a[0]); idx = opt(n.a[1])
        if idx is not None:
            addr = opt(N('+', base, idx))
        else:
            addr = base
        return N('mem', addr, None, line=n.line)
    if t == 'asg':
        return N('asg', n.a[0], opt(n.a[1]), opt(n.a[2]), line=n.line)
    if t == 'call':
        args = [opt(x) for x in n.a[1:]]
        return N('call', n.a[0], *args, line=n.line)
    if t == 'while2':
        return N('while2', opt(n.a[0]), opt(n.a[1]), line=n.line)
    if t == 'notnot':
        x = opt(n.a[0])
        if isnum(x): return num(1.0 if abs(x.a[0]) >= CLOSE else 0.0)
        return N('notnot', x)
    # binary
    l = opt(n.a[0]); r = opt(n.a[1])
    if isnum(l) and isnum(r):
        a, b = l.a[0], r.a[0]
        if t == '%':
            ib = ctrunc32(b)
            if ib:
                ia = ctrunc32(a); v = abs(int(math.fmod(ia, ib)))
                return num(v)
            return num(0)
        if t == '<<': return num(ctrunc32(ctrunc32(a) << (ctrunc32(b) & 31)))
        if t == '>>': return num(ctrunc32(a) >> ctrunc32(b))
        if t == '^':
            try: return num(math.pow(a, b))
            except (OverflowError, ValueError): pass
        if t == '/': return num(cdiv(a, b))
        if t == '*': return num(a * b)
        if t == '+': return num(a + b)
        if t == '-': return num(a - b)
        if t == '&': return num(float(trunc64(a) & trunc64(b)))
        if t == '|': return num(float(trunc64(a) | trunc64(b)))
        if t == '~': return num(float(trunc64(a) ^ trunc64(b)))
        if t == '==': return num(1.0 if abs(a - b) < CLOSE else 0.0)
        if t == '!=': return num(1.0 if abs(a - b) >= CLOSE else 0.0)
        if t == '===': return num(1.0 if a == b else 0.0)
        if t == '!==': return num(1.0 if a != b else 0.0)
        if t == '<': return num(1.0 if a < b else 0.0)
        if t == '<=': return num(1.0 if a <= b else 0.0)
        if t == '>': return num(1.0 if a > b else 0.0)
        if t == '>=': return num(1.0 if a >= b else 0.0)
        if t == '&&': return num(1.0 if abs(a) >= CLOSE and abs(b) >= CLOSE else 0.0)
        if t == '||': return num(1.0 if abs(a) >= CLOSE or abs(b) >= CLOSE else 0.0)
    elif isnum(l) or isnum(r):
        dv0 = isnum(l); d = (l if dv0 else r).a[0]; other = r if dv0 else l
        if t in ('|', '~') and trunc64(d) == 0: return N('or0', other)
        if t == '-' and dv0:
            if d == 0.0: return opt(N('neg', r))
        elif t in ('-', '+'):
            if d == 0.0: return other
        if t == '&' and trunc64(d) == 0: d = 0.0; t2 = '*'
        else: t2 = t
        if t2 == '*':
            if d == 0.0: return N('seq', other, num(0)) if not pure(other) else num(0)
            if d == 1.0: return other
        if t == '^':
            if not dv0:
                if abs(d) < 1e-30: return num(1)
                if abs(d - 1.0) < 1e-30: return l
            elif d > 0.0:
                if abs(d - 1.0) < 1e-30: return num(1)
                ld = math.log(d)
                if abs(ld - 1.0) < 1e-9: return N('call', 'exp', r)
        if t == '%' and not dv0 and ctrunc32(d) == 0: return num(0)
        if t == '/':
            if not dv0:
                if d == 1.0: return l
                if d == 0.0: return opt(N('*', l, r))
            elif d == 0.0:
                return N('seq', r, num(0)) if not pure(r) else num(0)
        if t == '&&':
            if dv0:
                return num(0) if abs(d) < CLOSE else N('notnot', r)
            else:
                if abs(d) < CLOSE: return N('seq', l, num(0)) if not pure(l) else num(0)
                return N('notnot', l)
        if t == '||':
            if dv0:
                return num(1) if abs(d) >= CLOSE else N('notnot', r)
            else:
                if abs(d) >= CLOSE: return N('seq', l, num(1)) if not pure(l) else num(1)
                return N('notnot', l)
    if t == '^' and l.t == '^':
        # (a^b)^c -> a^(c*b)
        return opt(N('^', l.a[0], N('*', r, l.a[1])))
    if t in ('&&', '||'):
        if l.t == 'notnot': l = l.a[0]
        if r.t == 'notnot': r = r.a[0]
    return N(t, l, r, line=n.line)

_pure_cache = {}
def pure(n):
    if n is None: return True
    r = _pure_cache.get(id(n))
    if r is not None and r[0] is n: return r[1]
    v = _pure(n); _pure_cache[id(n)] = (n, v); return v
def _pure(n):
    t = n.t
    if t in ('num', 'var', 'sid', 'lit'): return True
    if t in ('asg', 'while2'): return False
    if t == 'call':
        if n.a[0] not in PURE_FUNCS or n.a[0] == 'time_precise': return False
        return all(pure(x) for x in n.a[1:])
    return all(pure(x) for x in n.a if isinstance(x, N))
