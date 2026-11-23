import sys, eellex, eelparse
sys.setrecursionlimit(100000)
def load(path):
    secs = eellex.sections(path); funcs = {}; trees = {}
    for name in ('init', 'slider', 'block', 'sample', 'serialize', 'gfx'):
        if name in secs:
            st, src = secs[name]
            trees[name] = eelparse.parse_section(src, st - 1, funcs)
    for f in funcs.values(): f.body = eelparse.opt(f.body)
    for k in trees: trees[k] = eelparse.opt(trees[k])
    return secs, funcs, trees
if __name__ == '__main__':
    secs, funcs, trees = load(sys.argv[1])
    print('funcs', len(funcs), {k: (v.t if v else None) for k, v in trees.items()})
