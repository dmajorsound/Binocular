# EEL2 AST -> C++ code generator for the roomservice port.
# Semantics follow the REAPER/ysfx x64 EEL2 JIT (see notes in rt.h).
import sys, re, collections
sys.setrecursionlimit(200000)
from eelparse import N, isnum, pure, is_trivial

HOST_SCALARS = ['srate', 'samplesblock', 'play_state', 'play_position', 'beat_position', 'tempo', 'ts_num', 'ts_denom',
                'num_ch', 'ext_noinit', 'ext_nodenorm', 'pdc_delay', 'pdc_bot_ch', 'pdc_top_ch', 'pdc_midi',
                'gfx_r', 'gfx_g', 'gfx_b', 'gfx_a', 'gfx_x', 'gfx_y', 'gfx_w', 'gfx_h', 'gfx_mode', 'gfx_clear',
                'gfx_dest', 'gfx_texth', 'gfx_ext_retina', 'mouse_x', 'mouse_y', 'mouse_cap', 'mouse_wheel', 'mouse_hwheel']

BUILTIN_ARGC = {
    'abs': 1, 'atan2': 2, 'ceil': 1, 'cos': 1, 'exp': 1, 'floor': 1, 'log': 1, 'log10': 1, 'max': 2, 'min': 2,
    'sin': 1, 'sqr': 1, 'sqrt': 1, 'tan': 1, 'pow': 2, 'sign': 1,
    'fft': 2, 'ifft': 2, 'fft_permute': 2, 'fft_ipermute': 2, 'memset': 3, 'memcpy': 3,
    'file_avail': 1, 'file_mem': 3, 'file_var': 2,
    'slider': 1, 'sliderchange': 1, 'time_precise': 0,
    'sprintf': -2, 'strcpy': 2, 'strcat': 2, 'strcmp': 2, 'strlen': 1, 'str_getchar': -2, 'str_setlen': 2,
    'gfx_arc': -5, 'gfx_blit': -3, 'gfx_circle': -3, 'gfx_drawstr': -1, 'gfx_getchar': None, 'gfx_gradrect': -8,
    'gfx_line': -4, 'gfx_lineto': -2, 'gfx_measurestr': 3, 'gfx_rect': -4, 'gfx_roundrect': -5, 'gfx_setcursor': -1,
    'gfx_setfont': -1, 'gfx_setimgdim': 3, 'gfx_triangle': -6,
}
WRAP1 = {'sin': 'rt_sin', 'cos': 'rt_cos', 'tan': 'rt_tan', 'exp': 'rt_exp', 'log': 'rt_log', 'log10': 'rt_log10'}

def tov(c, k):
    return '(%s ? 1.0 : 0.0)' % c if k == 'B' else c

def cnum(v):
    if v != v: return 'RT_NAN'
    if v == float('inf'): return 'RT_INF'
    if v == float('-inf'): return '(-RT_INF)'
    r = repr(v)
    if 'e' not in r and '.' not in r: r += '.0'
    if v < 0: r = '(' + r + ')'
    return r

def cident(name):
    return re.sub(r'[^A-Za-z0-9_]', lambda m: '_%02X' % ord(m.group(0)), name)

class Gen:
    def __init__(self, funcs):
        self.funcs = funcs
        self.globals = set(); self.strs = {}; self.lits = {}; self.ntemp = 0
        self.scope = None; self.fname = None
        self.warn = collections.Counter()
        self.fmembers = []

    # ---------- names ----------
    def var(self, name):
        if self.scope is not None and name in self.scope:
            return 'L_%s_%s' % (cident(self.fname), cident(name))
        m = re.fullmatch(r'slider(\d+)', name)
        if m and 1 <= int(m.group(1)) <= 256: return 'sl[%s]' % m.group(1)
        m = re.fullmatch(r'spl(\d+)', name)
        if m and int(m.group(1)) < 64: return 'spl[%s]' % m.group(1)
        if name in HOST_SCALARS: return 'v_' + name
        self.globals.add(name)
        return 'g_' + cident(name)

    def strid(self, name):
        if name == '#':
            self.ntemp += 1; key = '#tmp%d' % self.ntemp
            self.strs[key] = 190000 + self.ntemp
            return '%d.0' % self.strs[key]
        if name not in self.strs: self.strs[name] = 90000 + sum(1 for k in self.strs if not k.startswith('#tmp'))
        return '%d.0' % self.strs[name]

    def litid(self, s):
        if s not in self.lits: self.lits[s] = 10000 + len(self.lits)
        return '%d.0' % self.lits[s]

    # ---------- expressions: returns (code, kind) kind in L (double&), V (double), B (bool) ----------
    def v(self, n):
        c, k = self.g(n)
        return tov(c, k)
    def b(self, n):
        c, k = self.g(n)
        return c if k == 'B' else 'B(%s)' % c
    def lv(self, n):
        c, k = self.g(n)
        if k != 'L':
            raise SyntaxError('line %d: not an lvalue: %r' % (n.line, n))
        return c

    def g(self, n):
        if n is None: return '0.0', 'V'
        t = n.t
        if t == 'num': return cnum(n.a[0]), 'V'
        if t == 'var': return self.var(n.a[0]), 'L'
        if t == 'sid': return self.strid(n.a[0]), 'V'
        if t == 'lit': return self.litid(n.a[0]), 'V'
        if t == 'mem': return 'M(%s)' % self.v(n.a[0]), 'L'
        if t == 'seq':
            parts = ['(void)(%s)' % self.v(x) if not (x.t in ('asg',)) else '(void)(%s)' % self.g(x)[0] for x in n.a[:-1]]
            c, k = self.g(n.a[-1])
            return '(%s, %s)' % (', '.join(parts), c), k
        if t == 'if':
            c = self.b(n.a[0])
            a, b_ = n.a[1], n.a[2]
            if a is not None and b_ is not None:
                ca, ka = self.g(a); cb, kb = self.g(b_)
                if ka == 'L' and kb == 'L': return '(%s ? %s : %s)' % (c, ca, cb), 'L'
                if ka == 'B' and kb == 'B': return '(%s ? %s : %s)' % (c, ca, cb), 'B'
                return '(%s ? (double)%s : (double)%s)' % (c, tov(ca, ka), tov(cb, kb)), 'V'
            if a is not None: return '(%s ? (double)%s : 0.0)' % (c, self.v(a)), 'V'
            return '(%s ? 0.0 : (double)%s)' % (c, self.v(b_)), 'V'
        if t == 'neg': return '(-%s)' % self.v(n.a[0]), 'V'
        if t == 'not': return '(!%s)' % self.b(n.a[0]), 'B'
        if t == 'notnot': return self.b(n.a[0]), 'B'
        if t == 'or0': return 'rt_or0(%s)' % self.v(n.a[0]), 'V'
        if t == 'asg': return self.asg(n)
        if t == 'call': return self.call(n)
        if t == 'while2':
            return ('[&]() -> double { int c_ = 1048576; while (%s) { (void)(%s); if (--c_ <= 0) break; } return 0.0; }()'
                    % (self.b(n.a[0]), self.v(n.a[1]))), 'V'
        # binary
        l, r = n.a
        if t in ('&&', '||'):
            return '(%s %s %s)' % (self.b(l), t, self.b(r)), 'B'
        both_impure = not pure(l) and not pure(r)
        if both_impure: self.warn['binary both impure'] += 1
        def two(fmt):
            if both_impure:
                return '[&]() -> double { const double a_ = %s; const double b_ = %s; return %s; }()' % (
                    self.v(l), self.v(r), fmt % ('a_', 'b_'))
            return fmt % (self.v(l), self.v(r))
        if t in ('+', '-', '*', '/'): return two('(%%s %s %%s)' % t), 'V'
        if t == '%': return two('rt_mod(%s, %s)'), 'V'
        if t == '^': return two('rt_pow(%s, %s)'), 'V'
        if t == '&': return two('rt_and(%s, %s)'), 'V'
        if t == '|': return two('rt_or(%s, %s)'), 'V'
        if t == '~': return two('rt_xor(%s, %s)'), 'V'
        if t == '<<': return two('rt_shl(%s, %s)'), 'V'
        if t == '>>': return two('rt_shr(%s, %s)'), 'V'
        if t in ('<', '>', '<=', '>=', '===', '!=='):
            op = {'===': '==', '!==': '!='}.get(t, t)
            if both_impure:
                return '[&]() -> bool { const double a_ = %s; const double b_ = %s; return a_ %s b_; }()' % (
                    self.v(l), self.v(r), op), 'B'
            return '(%s %s %s)' % (self.v(l), op, self.v(r)), 'B'
        if t in ('==', '!='):
            f = 'EQ' if t == '==' else 'NE'
            if both_impure:
                return '[&]() -> bool { const double a_ = %s; const double b_ = %s; return %s(a_, b_); }()' % (
                    self.v(l), self.v(r), f), 'B'
            return '%s(%s, %s)' % (f, self.v(l), self.v(r)), 'B'
        raise SyntaxError('unknown node %s' % t)

    def asg(self, n):
        op, lhs, rhs = n.a
        L = self.lv(lhs)
        simple_lhs = lhs.t == 'var'
        if op == '=':
            R = self.v(rhs)
            if is_trivial(rhs) and (simple_lhs or pure(lhs)): return '(%s = %s)' % (L, R), 'L'
            if simple_lhs or pure(rhs) and pure(lhs):
                return 'ASG(%s, %s)' % (L, R), 'L'
            return '[&]() -> double& { double& r_ = %s; r_ = dnf(%s); return r_; }()' % (L, R), 'L'
        R = self.v(rhs)
        fn = {'%=': 'rt_mod', '^=': 'rt_pow', '|=': 'rt_or', '&=': 'rt_and', '~=': 'rt_xor'}.get(op)
        if simple_lhs or (pure(rhs) and pure(lhs)):
            if fn: return 'CASG(%s, %s(%s, %s))' % (L, fn, L, R), 'L'
            return '(%s %s %s)' % (L, op, R), 'L'
        expr = '%s(r_, %s)' % (fn, R) if fn else 'r_ %s %s' % (op[0], R)
        return '[&]() -> double& { double& r_ = %s; r_ = %s; return r_; }()' % (L, expr), 'L'

    def call(self, n):
        name = n.a[0]; args = n.a[1:]
        if name in self.funcs:
            f = self.funcs[name]
            if len(args) != len(f.params):
                raise SyntaxError('line %d: %s() expects %d args' % (n.line, name, len(f.params)))
            cname = 'F_' + cident(name)
            if not args: return cname + '()', 'V'
            if all(pure(a) for a in args):
                return '%s(%s)' % (cname, ', '.join(self.v(a) for a in args)), 'V'
            # EEL: non-trivial params left to right, then trivial ones (vars/consts) are read
            self.warn['eel call impure args'] += 1
            pre = []; names = []
            for i, a in enumerate(args):
                if is_trivial(a): names.append(None)
                else:
                    pre.append('const double a%d_ = %s;' % (i, self.v(a))); names.append('a%d_' % i)
            callargs = [nm if nm else self.v(a) for nm, a in zip(names, args)]
            return '[&]() -> double { %s return %s(%s); }()' % (' '.join(pre), cname, ', '.join(callargs)), 'V'
        if name == 'loop':
            if len(args) != 2: raise SyntaxError('loop args line %d' % n.line)
            return ('[&]() -> double { int n_ = rt_i32(%s); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(%s); } return 0.0; }()'
                    % (self.v(args[0]), self.v(args[1]))), 'V'
        if name == 'while':
            if len(args) != 1: raise SyntaxError('while args line %d' % n.line)
            return ('[&]() -> double { int c_ = 1048576; while (%s) { if (--c_ <= 0) break; } return 0.0; }()'
                    % self.b(args[0])), 'V'
        if name not in BUILTIN_ARGC:
            raise SyntaxError('line %d: unknown function %s' % (n.line, name))
        want = BUILTIN_ARGC[name]
        if want is not None and (want >= 0 and len(args) != want or want < 0 and len(args) < -want):
            raise SyntaxError('line %d: %s() arg count %d' % (n.line, name, len(args)))
        if name == 'slider': return 'SL(%s)' % self.v(args[0]), 'L'
        if name == 'slider_automate':
            pass
        imp = sum(1 for a in args if not pure(a))
        if imp >= 2: self.warn['builtin multi impure args'] += 1
        vals = [self.v(a) for a in args]
        if name in WRAP1: return '%s(%s)' % (WRAP1[name], vals[0]), 'V'
        simple = {'abs': 'std::fabs', 'floor': 'std::floor', 'ceil': 'std::ceil', 'sqrt': 'rt_sqrt', 'sqr': 'rt_sqr',
                  'min': 'rt_min', 'max': 'rt_max', 'atan2': 'rt_atan2', 'pow': 'rt_pow', 'sign': 'rt_sign'}
        if name in simple: return '%s(%s)' % (simple[name], ', '.join(vals)), 'V'
        if name in ('file_var',):
            return 'file_var(%s, %s)' % (vals[0], self.lv(args[1])), 'V'
        if name == 'gfx_measurestr':
            return 'gfx_measurestr(%s, %s, %s)' % (vals[0], self.lv(args[1]), self.lv(args[2])), 'V'
        if name in ('sprintf', 'gfx_drawstr', 'gfx_setfont', 'gfx_arc', 'gfx_blit', 'gfx_circle', 'gfx_getchar',
                    'gfx_gradrect', 'gfx_line', 'gfx_lineto', 'gfx_rect', 'gfx_roundrect', 'gfx_setcursor', 'gfx_triangle', 'str_getchar',
                    'slider_automate_idx', 'slider_automate_mask'):
            return '%s({%s})' % (name, ', '.join(vals)), 'V'
        return '%s(%s)' % (name, ', '.join(vals)), 'V'

def slider_automate_fix(n):
    """slider_automate(sliderN[, touch]) / sliderchange(sliderN): pass the slider index, not its value."""
    if n is None or not isinstance(n, N): return
    if n.t == 'call' and n.a[0] in ('slider_automate', 'sliderchange'):
        a0 = n.a[1]
        m = re.fullmatch(r'slider(\d+)', a0.a[0]) if a0.t == 'var' else None
        if m:
            n.a[0] = n.a[0] + '_idx'; n.a[1] = N('num', float(m.group(1)))
        else:
            n.a[0] = n.a[0] + '_mask'
    for x in n.a:
        if isinstance(x, N): slider_automate_fix(x)

BUILTIN_ARGC.update({'slider_automate_idx': -1, 'slider_automate_mask': -1, 'sliderchange_idx': 1, 'sliderchange_mask': 1})
