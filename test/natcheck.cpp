#include "engine.h"
#include "common.h"
#include <algorithm>
int main(int argc, char** argv) {
    int kind = atoi(argv[1]), cs = atoi(argv[2]); double sr = atof(argv[3]); int bs = atoi(argv[4]); int frames = (int)(sr * 2.5);
    rs::Engine e; e.setSampleRate(sr); e.setBlockSize(bs);
    for (auto s : caseSliders(cs)) e.setSlider(s.n, s.v);
    auto in = makeSignal(kind, sr, frames);
    std::vector<double> c[4], o[2]; for (auto& x : c) x.resize(bs); for (auto& x : o) x.resize(bs);
    rs::TimeInfo ti; double maxdiff = 0;
    for (int pos = 0; pos < frames; pos += bs) {
        int nf = std::min(bs, frames - pos);
        for (int i = 0; i < nf; i++) for (int k = 0; k < 4; k++) c[k][i] = in[4 * (size_t)(pos + i) + k];
        ti.position = pos / sr;
        const double* ins[4] = {c[0].data(), c[1].data(), c[2].data(), c[3].data()}; double* outs[2] = {o[0].data(), o[1].data()};
        e.process(ins, outs, 4, 2, nf, ti);
        for (int i = 0; i < nf; i++) for (int k = 0; k < 2; k++) maxdiff = std::max(maxdiff, std::fabs(o[k][i] - c[k][i]));
    }
    auto& g = *e.gen;
    printf("passthrough_maxdiff %.17g\n", maxdiff);
    double vals[] = {g.g_bcor, g.g_bxx, g.g_byy, g.g_bxy, g.g_bbal, g.g_bwid, g.g_ofs_lag, g.g_ofs_sgn, g.g_ofs_ok, g.g_ofs_conf, g.g_bhld, g.g_nb, g.g_ffm, g.g_fn, g.g_gwp};
    for (int i = 0; kVars[i]; i++) printf("%s %.17g\n", kVars[i], vals[i]);
    int nb = (int)g.g_nb;
    const int bases[] = {4096, 5120, 6144, 7168, 402592, 403616};
    const char* names[] = {"cor", "lev", "val", "hld", "mon", "hmo"};
    for (int b = 0; b < 6; b++) for (int i = 0; i < nb; i++) printf("%s%d %.17g\n", names[b], i, g.M(bases[b] + i));
    double sum = 0; for (int i = 0; i < 8192; i++) sum += g.M(409600 + i); printf("gonio_sum %.17g\n", sum);
}
