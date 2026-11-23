#include "engine.h"

namespace rs {

Engine::Engine() : gen(std::make_unique<Gen>()) {
    gen->host = this;
    for (int i = 0; i < 1024; i++) gen->setString(i, "");   // pre-create user strings: no map inserts at run time
    gen->initStrings();
    for (int i = 0; i < kNumSliders; i++) gen->sl[kSliders[i].idx] = kSliders[i].def;
}

void Engine::setSampleRate(double sr) { if (sr != srate) { srate = sr; mustInit = true; } }
void Engine::setBlockSize(int bs) { if (bs != blockSize) { blockSize = bs; mustInit = true; } }

void Engine::setSlider(int n, double v) {
    if (n < 1 || n > 256) return;
    if (gen->sl[n] != v) { gen->sl[n] = v; mustSlider = true; }
}

void Engine::init() {
    Gen& g = *gen;
    g.v_samplesblock = blockSize;
    g.v_srate = srate;
    g.v_pdc_delay = g.v_pdc_bot_ch = g.v_pdc_top_ch = g.v_pdc_midi = 0;
    g.S_init();
    mustInit = false; mustSlider = true; gfxMustInit = true;
}

template <class Real>
void Engine::processT(const Real* const* in, Real* const* out, int nIn, int nOut, int frames, const TimeInfo& ti) {
    Gen& g = *gen;
    // ysfx_set_time_info: re-run @init on transport start unless ext_noinit
    const int prev = (int)g.v_play_state;
    if (!B(g.v_ext_noinit)) {
        auto running = [](int s) { return s == 1 || s == 5; };
        if (!running(prev) && running(ti.playState)) mustInit = true;
    }
    g.v_tempo = ti.tempo; g.v_play_state = ti.playState; g.v_play_position = ti.position; g.v_beat_position = ti.beat;
    g.v_ts_num = ti.tsNum; g.v_ts_denom = ti.tsDenom;

    if (mustInit) init();
    const int codeIns = 4, codeOuts = 2;   // in_pin: Left, Right, Sidechain Left, Sidechain Right
    const int ni = nIn < codeIns ? nIn : codeIns, no = nOut < codeOuts ? nOut : codeOuts;
    g.v_samplesblock = frames;
    g.v_num_ch = ni;
    if (mustSlider) { g.S_slider(); mustSlider = false; }
    g.S_block();
    for (int i = 0; i < frames; i++) {
        for (int c = 0; c < ni; c++) g.spl[c] = (double)in[c][i];
        for (int c = ni; c < codeIns; c++) g.spl[c] = 0;
        g.S_sample();
        for (int c = 0; c < no; c++) out[c][i] = (Real)g.spl[c];
    }
    for (int c = no; c < nOut; c++) for (int i = 0; i < frames; i++) out[c][i] = 0;
}
template void Engine::processT<float>(const float* const*, float* const*, int, int, int, const TimeInfo&);
template void Engine::processT<double>(const double* const*, double* const*, int, int, int, const TimeInfo&);

void Engine::process(const double* const* in, double* const* out, int nIn, int nOut, int frames, const TimeInfo& ti) {
    processT<double>(in, out, nIn, nOut, frames, ti);
}

std::vector<float> Engine::saveState() {
    Gen& g = *gen;
    g.serBuf.clear(); g.serPos = 0; g.serMode = 1;
    g.S_serialize();
    g.serMode = -1;
    return g.serBuf;
}

void Engine::loadState(const std::vector<float>& data) {
    Gen& g = *gen;
    g.serBuf = data; g.serPos = 0; g.serMode = 0;
    g.S_serialize();
    g.serMode = -1; g.serBuf.clear();
    mustSlider = true;
}

void Engine::runGfx(GfxBackend* b, int w, int h) {
    Gen& g = *gen;
    g.gfx = b;
    g.v_gfx_w = w; g.v_gfx_h = h;
    if (gfxMustInit) {
        g.v_gfx_r = g.v_gfx_g = g.v_gfx_b = g.v_gfx_a = 1; g.v_gfx_x = g.v_gfx_y = 0; g.v_gfx_mode = 0; g.v_gfx_dest = -1;
        gfxMustInit = false;
    }
    g.S_gfx();
    g.gfx = nullptr;
}

} // namespace rs
