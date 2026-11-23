#include "ysfx.h"
#include "common.h"
#include <string>
int main(int argc, char** argv) {
    const char* path = argv[1]; int kind = atoi(argv[2]), cs = atoi(argv[3]); double sr = atof(argv[4]); int bs = atoi(argv[5]); int frames = (int)(sr * 2.5);
    ysfx_config_t* cfg = ysfx_config_new(); ysfx_t* fx = ysfx_new(cfg);
    if (!ysfx_load_file(fx, path, 0)) { fprintf(stderr, "load failed\n"); return 1; }
    if (!ysfx_compile(fx, 0)) { fprintf(stderr, "compile failed\n"); return 1; }
    ysfx_set_sample_rate(fx, sr); ysfx_set_block_size(fx, bs);
    for (auto s : caseSliders(cs)) ysfx_slider_set_value(fx, s.n - 1, s.v);
    auto in = makeSignal(kind, sr, frames);
    std::vector<double> c[4], o[2]; for (auto& x : c) x.resize(bs); for (auto& x : o) x.resize(bs);
    ysfx_time_info_t ti{}; ti.tempo = 120; ti.playback_state = ysfx_playback_playing; ti.time_signature[0] = 4; ti.time_signature[1] = 4;
    double maxdiff = 0;
    for (int pos = 0; pos < frames; pos += bs) {
        int nf = std::min(bs, frames - pos);
        for (int i = 0; i < nf; i++) for (int k = 0; k < 4; k++) c[k][i] = in[4 * (size_t)(pos + i) + k];
        ti.time_position = pos / sr; ysfx_set_time_info(fx, &ti);
        const double* ins[4] = {c[0].data(), c[1].data(), c[2].data(), c[3].data()}; double* outs[2] = {o[0].data(), o[1].data()};
        ysfx_process_double(fx, ins, outs, 4, 2, nf);
        for (int i = 0; i < nf; i++) for (int k = 0; k < 2; k++) maxdiff = std::max(maxdiff, std::fabs(o[k][i] - c[k][i]));
    }
    printf("passthrough_maxdiff %.17g\n", maxdiff);
    for (int i = 0; kVars[i]; i++) { ysfx_real* p = ysfx_find_var(fx, kVars[i]); printf("%s %.17g\n", kVars[i], p ? *p : -12345.0); }
    double nb = *ysfx_find_var(fx, "nb");
    const uint32_t bases[] = {4096, 5120, 6144, 7168, 402592, 403616};
    const char* names[] = {"cor", "lev", "val", "hld", "mon", "hmo"};
    std::vector<double> buf((size_t)nb);
    for (int b = 0; b < 6; b++) { ysfx_read_vmem(fx, bases[b], buf.data(), (uint32_t)nb); for (int i = 0; i < (int)nb; i++) printf("%s%d %.17g\n", names[b], i, buf[(size_t)i]); }
    std::vector<double> g(8192); ysfx_read_vmem(fx, 409600, g.data(), 8192); double sum = 0; for (double x : g) sum += x; printf("gonio_sum %.17g\n", sum);
    ysfx_free(fx); ysfx_config_free(cfg);
}
