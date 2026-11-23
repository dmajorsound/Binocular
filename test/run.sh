#!/bin/bash
J="$(dirname "$0")/../jsfx/Binocular.jsfx"; ok=0; bad=0
for spec in "0 0 48000 256" "1 0 48000 256" "2 0 44100 64" "3 3 48000 512" "0 1 48000 256" "0 2 96000 1024" "4 4 44100 33" "1 1 96000 128" "3 2 48000 2048" "0 0 192000 256"; do
  set -- $spec
  ./refcheck $J $1 $2 $3 $4 > r.txt; ./natcheck $1 $2 $3 $4 > n.txt
  res=$(python3 - <<'PY'
r=dict(l.split() for l in open('r.txt')); n=dict(l.split() for l in open('n.txt'))
keys=sorted(set(r)|set(n)); worst=0; wk=''
for k in keys:
    a=float(r.get(k,'nan')); b=float(n.get(k,'nan'))
    d=abs(a-b) if a==a and b==b else (0 if (a!=a and b!=b) else 1e9)
    if d>worst: worst=d; wk=k
print(f"{len(keys)} values, max diff {worst:.3g} ({wk})  bcor {float(r['bcor']):+.3f} ofs {float(r['ofs_lag']):+.2f} ok {r['ofs_ok']} sgn {r['ofs_sgn']} nb {r['nb']}")
PY
)
  echo "kind $1 case $2 sr $3 bs $4: $res"
done
