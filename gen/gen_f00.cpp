// GENERATED
#include "gen.h"
namespace rs {
double Gen::F_fband(double p0, double p1, double p2, double p3) {
  L_fband_i = p0; L_fband_xx = p1; L_fband_yy = p2; L_fband_xy = p3;
  return ((void)(ASG(L_fband_dd, rt_sqrt((L_fband_xx * L_fband_yy)))), (void)(ASG(M((g_m_cor + L_fband_i)), ((L_fband_dd > g_tiny) ? (double)rt_max((-1.0), rt_min(1.0, (L_fband_xy / L_fband_dd))) : (double)0.0))), (void)(ASG(M((g_m_lev + L_fband_i)), (10.0 * rt_log10(((0.5 * (L_fband_xx + L_fband_yy)) + g_tiny))))), (void)(ASG(M((g_m_val + L_fband_i)), ((((10.0 * rt_log10((L_fband_xx + g_tiny))) > (-100.0)) || ((10.0 * rt_log10((L_fband_yy + g_tiny))) > (-100.0))) ? 1.0 : 0.0))), ASG(M((g_m_mon + L_fband_i)), (10.0 * rt_log10(rt_max(g_tiny12, (((L_fband_xx + L_fband_yy) + (2.0 * L_fband_xy)) / ((2.0 * (L_fband_xx + L_fband_yy)) + g_tiny)))))));
}

double Gen::F_fbands(double p0, double p1, double p2, double p3, double p4, double p5, double p6, double p7) {
  L_fbands_eng = p0; L_fbands_n = p1; L_fbands_sx = p2; L_fbands_sy = p3; L_fbands_sc = p4; L_fbands_cx = p5; L_fbands_cy = p6; L_fbands_cc = p7;
  return ((void)([&]() -> double& { double& r_ = M(L_fbands_cx); r_ = dnf([&]() -> double& { double& r_ = M(L_fbands_cy); r_ = dnf((M(L_fbands_cc) = 0.0)); return r_; }()); return r_; }()), (void)((L_fbands_k = 0.0)), (void)([&]() -> double { int n_ = rt_i32((L_fbands_n / 2.0)); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)(ASG(M((L_fbands_cx + (L_fbands_k + 1.0))), (M((L_fbands_cx + L_fbands_k)) + M((L_fbands_sx + L_fbands_k))))), (void)(ASG(M((L_fbands_cy + (L_fbands_k + 1.0))), (M((L_fbands_cy + L_fbands_k)) + M((L_fbands_sy + L_fbands_k))))), (void)(ASG(M((L_fbands_cc + (L_fbands_k + 1.0))), (M((L_fbands_cc + L_fbands_k)) + M((L_fbands_sc + L_fbands_k))))), (L_fbands_k += 1.0))); } return 0.0; }()), (void)(ASG(L_fbands_nrm, (2.0 / ((L_fbands_n * L_fbands_n) * 0.375)))), (void)((L_fbands_i = 0.0)), [&]() -> double { int n_ = rt_i32(g_nb); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)((EQ(M((g_m_eng + L_fbands_i)), L_fbands_eng) ? (double)[&]() -> double { const double a1_ = ([&]() -> double { const double a_ = F_fsum(L_fbands_cx, M((g_m_u1 + L_fbands_i)), (L_fbands_n / 2.0)); const double b_ = F_fsum(L_fbands_cx, M((g_m_u0 + L_fbands_i)), (L_fbands_n / 2.0)); return (a_ - b_); }() * L_fbands_nrm); const double a2_ = ([&]() -> double { const double a_ = F_fsum(L_fbands_cy, M((g_m_u1 + L_fbands_i)), (L_fbands_n / 2.0)); const double b_ = F_fsum(L_fbands_cy, M((g_m_u0 + L_fbands_i)), (L_fbands_n / 2.0)); return (a_ - b_); }() * L_fbands_nrm); const double a3_ = ([&]() -> double { const double a_ = F_fsum(L_fbands_cc, M((g_m_u1 + L_fbands_i)), (L_fbands_n / 2.0)); const double b_ = F_fsum(L_fbands_cc, M((g_m_u0 + L_fbands_i)), (L_fbands_n / 2.0)); return (a_ - b_); }() * L_fbands_nrm); return F_fband(L_fbands_i, a1_, a2_, a3_); }() : 0.0)), (L_fbands_i += 1.0))); } return 0.0; }());
}

double Gen::F_fframe() {
  
  return ((void)(F_fspec(g_fn, g_fst1, g_m_win, g_m_sxx, g_m_syy, g_m_sxy, g_m_sxi, (1.0 - rt_exp(((-g_fhop) / ((rt_max(1.0, sl[3]) * 0.001) * v_srate)))))), (B(g_ffm) ? (double)F_fbands(0.0, g_fn, g_m_sxx, g_m_syy, g_m_sxy, g_m_cxx, g_m_cyy, g_m_cxy) : 0.0));
}

double Gen::F_fframe2() {
  
  return ((void)(F_fspec(g_fs2, g_fst2, g_m_wi2, g_m_s2x, g_m_s2y, g_m_s2c, 0.0, (1.0 - rt_exp(((-g_fhop2) / ((rt_max(1.0, sl[3]) * 0.001) * v_srate)))))), F_fbands(1.0, g_fs2, g_m_s2x, g_m_s2y, g_m_s2c, g_m_c2x, g_m_c2y, g_m_c2c));
}

double Gen::F_foffset() {
  
  return ((void)(memset(g_m_fb, 0.0, (2.0 * g_fn))), (void)((L_foffset_k = 1.0)), (void)([&]() -> double { int n_ = rt_i32((g_fnb - 1.0)); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)(ASG(L_foffset_re, M((g_m_sxy + L_foffset_k)))), (void)(ASG(L_foffset_im, M((g_m_sxi + L_foffset_k)))), (void)(ASG(L_foffset_mg, rt_sqrt(((L_foffset_re * L_foffset_re) + (L_foffset_im * L_foffset_im))))), (void)(((L_foffset_mg > g_tiny) ? (double)((void)(ASG(M((g_m_fb + (2.0 * L_foffset_k))), (L_foffset_re / L_foffset_mg))), (void)(ASG(M((g_m_fb + ((2.0 * L_foffset_k) + 1.0))), (L_foffset_im / L_foffset_mg))), (void)(ASG(M((g_m_fb + (2.0 * (g_fn - L_foffset_k)))), (L_foffset_re / L_foffset_mg))), ASG(M((g_m_fb + ((2.0 * (g_fn - L_foffset_k)) + 1.0))), ((-L_foffset_im) / L_foffset_mg))) : 0.0)), (L_foffset_k += 1.0))); } return 0.0; }()), (void)(fft_ipermute(g_m_fb, g_fn)), (void)(ifft(g_m_fb, g_fn)), (void)(ASG(L_foffset_lm, std::floor((0.01 * v_srate)))), (void)((L_foffset_bv = 0.0)), (void)((L_foffset_bi = 0.0)), (void)((L_foffset_s2 = 0.0)), (void)(ASG(L_foffset_i, (-L_foffset_lm))), (void)([&]() -> double { int n_ = rt_i32(((2.0 * L_foffset_lm) + 1.0)); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)(ASG(L_foffset_v, M((g_m_fb + (2.0 * rt_mod((L_foffset_i + g_fn), g_fn)))))), (void)((L_foffset_s2 += (L_foffset_v * L_foffset_v))), (void)(((std::fabs(L_foffset_v) > std::fabs(L_foffset_bv)) ? (double)((void)((L_foffset_bv = L_foffset_v)), (L_foffset_bi = L_foffset_i)) : 0.0)), (L_foffset_i += 1.0))); } return 0.0; }()), (void)(ASG(L_foffset_y0, std::fabs(M((g_m_fb + (2.0 * rt_mod(((L_foffset_bi - 1.0) + g_fn), g_fn))))))), (void)(ASG(L_foffset_y1, std::fabs(L_foffset_bv))), (void)(ASG(L_foffset_y2, std::fabs(M((g_m_fb + (2.0 * rt_mod(((L_foffset_bi + 1.0) + g_fn), g_fn))))))), (void)(ASG(L_foffset_den, ((L_foffset_y0 - (2.0 * L_foffset_y1)) + L_foffset_y2))), (void)(ASG(L_foffset_fr, (NE(L_foffset_den, 0.0) ? (double)rt_max((-0.5), rt_min(0.5, (0.5 * ((L_foffset_y0 - L_foffset_y2) / L_foffset_den)))) : (double)0.0))), (void)(ASG(g_ofs_conf, ((L_foffset_s2 > 0.0) ? (double)(std::fabs(L_foffset_bv) / rt_sqrt((L_foffset_s2 / ((2.0 * L_foffset_lm) + 1.0)))) : (double)0.0))), (void)(ASG(g_ofs_ok, (((g_ofs_conf > 8.0) && B(g_bval)) ? 1.0 : 0.0))), (void)(ASG(g_ofs_lag, (L_foffset_bi + L_foffset_fr))), ASG(g_ofs_sgn, ((L_foffset_bv < 0.0) ? (double)(-1.0) : (double)1.0)));
}

double Gen::F_fspec(double p0, double p1, double p2, double p3, double p4, double p5, double p6, double p7) {
  L_fspec_n = p0; L_fspec_st = p1; L_fspec_win = p2; L_fspec_sx = p3; L_fspec_sy = p4; L_fspec_sc = p5; L_fspec_ci = p6; L_fspec_kf = p7;
  return ((void)((L_fspec_i = 0.0)), (void)((L_fspec_j = L_fspec_st)), (void)([&]() -> double { int n_ = rt_i32(L_fspec_n); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)(ASG(M((g_m_fb + (2.0 * L_fspec_i))), (M((g_m_rl + L_fspec_j)) * M((L_fspec_win + L_fspec_i))))), (void)(ASG(M((g_m_fb + ((2.0 * L_fspec_i) + 1.0))), (M((g_m_rr + L_fspec_j)) * M((L_fspec_win + L_fspec_i))))), (void)(ASG(L_fspec_j, rt_and((L_fspec_j + 1.0), 65535.0))), (L_fspec_i += 1.0))); } return 0.0; }()), (void)(fft(g_m_fb, L_fspec_n)), (void)(fft_permute(g_m_fb, L_fspec_n)), (void)((L_fspec_k = 1.0)), [&]() -> double { int n_ = rt_i32(((L_fspec_n / 2.0) - 1.0)); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)(ASG(L_fspec_a, M((g_m_fb + (2.0 * L_fspec_k))))), (void)(ASG(L_fspec_b, M((g_m_fb + ((2.0 * L_fspec_k) + 1.0))))), (void)(ASG(L_fspec_c, M((g_m_fb + (2.0 * (L_fspec_n - L_fspec_k)))))), (void)(ASG(L_fspec_d, M((g_m_fb + ((2.0 * (L_fspec_n - L_fspec_k)) + 1.0))))), (void)(ASG(L_fspec_xr, ((L_fspec_a + L_fspec_c) * 0.5))), (void)(ASG(L_fspec_xi, ((L_fspec_b - L_fspec_d) * 0.5))), (void)(ASG(L_fspec_yr, ((L_fspec_b + L_fspec_d) * 0.5))), (void)(ASG(L_fspec_yi, ((L_fspec_c - L_fspec_a) * 0.5))), (void)((M((L_fspec_sx + L_fspec_k)) += (((L_fspec_xr * L_fspec_xr) + ((L_fspec_xi * L_fspec_xi) - M((L_fspec_sx + L_fspec_k)))) * L_fspec_kf))), (void)((M((L_fspec_sy + L_fspec_k)) += (((L_fspec_yr * L_fspec_yr) + ((L_fspec_yi * L_fspec_yi) - M((L_fspec_sy + L_fspec_k)))) * L_fspec_kf))), (void)((M((L_fspec_sc + L_fspec_k)) += (((L_fspec_xr * L_fspec_yr) + ((L_fspec_xi * L_fspec_yi) - M((L_fspec_sc + L_fspec_k)))) * L_fspec_kf))), (void)((B(L_fspec_ci) ? (double)(M((L_fspec_ci + L_fspec_k)) += ((((L_fspec_xi * L_fspec_yr) - (L_fspec_xr * L_fspec_yi)) - M((L_fspec_ci + L_fspec_k))) * L_fspec_kf)) : 0.0)), (L_fspec_k += 1.0))); } return 0.0; }());
}

double Gen::F_fsum(double p0, double p1, double p2) {
  L_fsum_c = p0; L_fsum_u = p1; L_fsum_nbn = p2;
  return ((void)(ASG(L_fsum_k, rt_min(std::floor(L_fsum_u), (L_fsum_nbn - 1.0)))), (M((L_fsum_c + L_fsum_k)) + ((L_fsum_u - L_fsum_k) * (M((L_fsum_c + (L_fsum_k + 1.0))) - M((L_fsum_c + L_fsum_k))))));
}

double Gen::F_ldec(double p0, double p1, double p2) {
  L_ldec_str = p0; L_ldec_dst = p1; L_ldec_n = p2;
  return ((void)((L_ldec_i = 0.0)), [&]() -> double { int n_ = rt_i32(L_ldec_n); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)(ASG(L_ldec_c, str_getchar({L_ldec_str, L_ldec_i}))), (void)(ASG(M((L_ldec_dst + L_ldec_i)), ((L_ldec_c <= 57.0) ? (double)(L_ldec_c - 48.0) : (double)((L_ldec_c - 97.0) + 10.0)))), (L_ldec_i += 1.0))); } return 0.0; }());
}

double Gen::F_setup() {
  
  return ((void)(ASG(g_nb, M((g_m_bl + rt_max(0.0, rt_min(9.0, std::floor((sl[1] + 0.5)))))))), (void)(ASG(g_ffm, ((g_nb > 64.0) ? 1.0 : 0.0))), (void)(ASG(g_fn, ((v_srate <= 50000.0) ? (double)16384.0 : (double)32768.0))), (void)(ASG(g_fhop, (g_fn / 16.0))), (void)(ASG(g_fnb, (g_fn / 2.0))), (void)(ASG(g_fs2, (g_fn / 4.0))), (void)(ASG(g_fhop2, (g_fs2 / 8.0))), (void)(ASG(g_fnb2, (g_fs2 / 2.0))), (void)((g_flo = 20.0)), (void)(ASG(g_fhi, rt_min(20000.0, (0.45 * v_srate)))), (void)(ASG(L_setup_sp, ((rt_log((g_fhi / g_flo)) / rt_log(2.0)) / g_nb))), (void)(ASG(L_setup_wm, rt_pow(2.0, (1.0 - (2.0 * (sl[2] / 100.0)))))), (void)(ASG(L_setup_bw, ((L_setup_sp * L_setup_wm) * 1.6))), (void)(ASG(L_setup_bo, (L_setup_sp * L_setup_wm))), (void)(memset(g_m_st, 0.0, 2048.0)), (void)(memset(g_m_rl, 0.0, 65536.0)), (void)(memset(g_m_rr, 0.0, 65536.0)), (void)(memset(g_m_sxx, 0.0, 114800.0)), (void)(memset(g_m_s2x, 0.0, 49200.0)), (void)((L_setup_i = 0.0)), (void)([&]() -> double { int n_ = rt_i32(g_fn); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)(ASG(M((g_m_win + L_setup_i)), (0.5 - (0.5 * rt_cos((6.283185307179586 * ((L_setup_i + 0.5) / g_fn))))))), (L_setup_i += 1.0))); } return 0.0; }()), (void)((L_setup_i = 0.0)), (void)([&]() -> double { int n_ = rt_i32(g_fs2); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)(ASG(M((g_m_wi2 + L_setup_i)), (0.5 - (0.5 * rt_cos((6.283185307179586 * ((L_setup_i + 0.5) / g_fs2))))))), (L_setup_i += 1.0))); } return 0.0; }()), (void)((g_fwp = 0.0)), (void)((g_fhc = 0.0)), (void)((g_fhc2 = 0.0)), (void)((g_fpend = 0.0)), (void)((g_f2pend = 0.0)), (void)((g_foffp = 0.0)), (void)((L_setup_i = 0.0)), (void)([&]() -> double { int n_ = rt_i32(g_nb); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)(ASG(L_setup_fc, (g_flo * rt_pow((g_fhi / g_flo), ((L_setup_i + 0.5) / g_nb))))), (void)((B(g_ffm) ? ((void)(ASG(L_setup_e0, (L_setup_fc * rt_pow(2.0, ((-L_setup_bo) / 2.0))))), (void)(ASG(L_setup_e1, (L_setup_fc * rt_pow(2.0, (L_setup_bo / 2.0))))), (void)(ASG(M((g_m_eng + L_setup_i)), ((((L_setup_e1 - L_setup_e0) * (g_fs2 / v_srate)) >= 3.0) ? 1.0 : 0.0))), (B(M((g_m_eng + L_setup_i))) ? ((void)(ASG(M((g_m_u0 + L_setup_i)), rt_max(0.5, rt_min((g_fnb2 - 1.0), ((L_setup_e0 * (g_fs2 / v_srate)) + 0.5))))), ASG(M((g_m_u1 + L_setup_i)), rt_max(0.5, rt_min((g_fnb2 - 1.0), ((L_setup_e1 * (g_fs2 / v_srate)) + 0.5))))) : ((void)(ASG(M((g_m_u0 + L_setup_i)), rt_max(0.5, rt_min((g_fnb - 1.0), ((L_setup_e0 * (g_fn / v_srate)) + 0.5))))), ASG(M((g_m_u1 + L_setup_i)), rt_max(0.5, rt_min((g_fnb - 1.0), ((L_setup_e1 * (g_fn / v_srate)) + 0.5))))))) : ((void)((M((g_m_eng + L_setup_i)) = 0.0)), (void)(ASG(L_setup_w0, (6.283185307179586 * (L_setup_fc / v_srate)))), (void)(ASG(L_setup_xs, (((rt_log(2.0) / 2.0) * L_setup_bw) * (L_setup_w0 / rt_sin(L_setup_w0))))), (void)(ASG(L_setup_al, (rt_sin(L_setup_w0) * ((rt_exp(L_setup_xs) - rt_exp((-L_setup_xs))) / 2.0)))), (void)(ASG(L_setup_p, (g_m_st + (L_setup_i * 32.0)))), (void)(ASG(M(L_setup_p), (L_setup_al / (1.0 + L_setup_al)))), (void)(ASG(M((L_setup_p + 1.0)), ((-2.0) * (rt_cos(L_setup_w0) / (1.0 + L_setup_al))))), ASG(M((L_setup_p + 2.0)), ((1.0 - L_setup_al) / (1.0 + L_setup_al)))))), (void)((M((g_m_fc + L_setup_i)) = L_setup_fc)), (void)((M((g_m_cor + L_setup_i)) = 0.0)), (void)((M((g_m_lev + L_setup_i)) = (-200.0))), (void)((M((g_m_val + L_setup_i)) = 0.0)), (void)((M((g_m_mon + L_setup_i)) = 0.0)), (void)((M((g_m_hld + L_setup_i)) = 1.0)), (void)((M((g_m_ht + L_setup_i)) = 0.0)), (void)((M((g_m_hmo + L_setup_i)) = 0.0)), (void)((M((g_m_hmt + L_setup_i)) = 0.0)), (L_setup_i += 1.0))); } return 0.0; }()), (void)(ASG(g_bxy, ASG(g_bxx, (g_byy = 0.0)))), (void)((g_bcor = 0.0)), (void)((g_bval = 0.0)), (void)((g_bhld = 1.0)), (void)((g_bht = 0.0)), (void)((g_ofs_ok = 0.0)), (void)((g_ofs_lag = 0.0)), (void)((g_ofs_sgn = 1.0)), (void)((g_refon = 0.0)), (void)((g_lnb = sl[1])), (void)((g_lsel = sl[2])), (void)((g_lsr = v_srate)), (g_lin = sl[4]));
}

double Gen::F_uarc(double p0, double p1, double p2, double p3, double p4, double p5) {
  L_uarc_cx = p0; L_uarc_cy = p1; L_uarc_rad = p2; L_uarc_a1 = p3; L_uarc_a2 = p4; L_uarc_th = p5;
  return ((void)((L_uarc_k = 0.0)), [&]() -> double { int n_ = rt_i32(rt_max(1.0, std::ceil((L_uarc_th * 2.0)))); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)(gfx_arc({L_uarc_cx, L_uarc_cy, ((L_uarc_rad - (L_uarc_th / 2.0)) + (L_uarc_k * 0.5)), L_uarc_a1, L_uarc_a2, 1.0})), (L_uarc_k += 1.0))); } return 0.0; }());
}

double Gen::F_ubtn(double p0, double p1, double p2, double p3, double p4, double p5, double p6) {
  L_ubtn_x = p0; L_ubtn_y = p1; L_ubtn_w = p2; L_ubtn_h = p3; L_ubtn_on = p4; L_ubtn_hot = p5; L_ubtn_str = p6;
  return ((void)((B(L_ubtn_on) ? (double)((void)(F_ucol(g_ucact, 1.0)), (void)(F_urr(L_ubtn_x, L_ubtn_y, L_ubtn_w, L_ubtn_h, (3.0 * g_usc), 1.0)), (void)(F_ucol(g_ucacto, 1.0)), (void)(F_urr(L_ubtn_x, L_ubtn_y, L_ubtn_w, L_ubtn_h, (3.0 * g_usc), 0.0)), F_ucol(g_ucactx, 1.0)) : (double)((void)(F_ucol((B(L_ubtn_hot) ? g_uchair : g_ucsurf), 1.0)), (void)(F_urr(L_ubtn_x, L_ubtn_y, L_ubtn_w, L_ubtn_h, (3.0 * g_usc), 1.0)), (void)(F_ucol(g_ucbord, 1.0)), (void)(F_urr(L_ubtn_x, L_ubtn_y, L_ubtn_w, L_ubtn_h, (3.0 * g_usc), 0.0)), F_ucol((B(L_ubtn_hot) ? g_uctext : g_ucmuted), 1.0)))), F_utext(L_ubtn_str, (L_ubtn_x + (L_ubtn_w / 2.0)), (L_ubtn_y + ((L_ubtn_h / 2.0) - (6.0 * g_usc)))));
}

double Gen::F_ucol(double p0, double p1) {
  L_ucol_c = p0; L_ucol_a = p1;
  return ((void)(ASG(v_gfx_r, (rt_and(rt_shr(L_ucol_c, 16.0), 255.0) / 255.0))), (void)(ASG(v_gfx_g, (rt_and(rt_shr(L_ucol_c, 8.0), 255.0) / 255.0))), (void)(ASG(v_gfx_b, (rt_and(L_ucol_c, 255.0) / 255.0))), (v_gfx_a = L_ucol_a));
}

double Gen::F_ucorr(double p0, double p1) {
  L_ucorr_v = p0; L_ucorr_a = p1;
  return ((L_ucorr_v < 0.0) ? (double)F_ucol(g_ucmute, L_ucorr_a) : (double)((void)(ASG(L_ucorr_t, rt_min(1.0, L_ucorr_v))), (void)(ASG(v_gfx_r, (((rt_and(rt_shr(g_ucacto, 16.0), 255.0) * (1.0 - L_ucorr_t)) + (rt_and(rt_shr(g_ucgrn, 16.0), 255.0) * L_ucorr_t)) / 255.0))), (void)(ASG(v_gfx_g, (((rt_and(rt_shr(g_ucacto, 8.0), 255.0) * (1.0 - L_ucorr_t)) + (rt_and(rt_shr(g_ucgrn, 8.0), 255.0) * L_ucorr_t)) / 255.0))), (void)(ASG(v_gfx_b, (((rt_and(g_ucacto, 255.0) * (1.0 - L_ucorr_t)) + (rt_and(g_ucgrn, 255.0) * L_ucorr_t)) / 255.0))), (v_gfx_a = L_ucorr_a)));
}

double Gen::F_udef(double p0) {
  L_udef_id = p0;
  return (EQ(L_udef_id, 1.0) ? (double)((void)((sl[1] = 8.0)), slider_automate_mask({1.0})) : (double)(EQ(L_udef_id, 2.0) ? (double)((void)((sl[2] = 50.0)), slider_automate_mask({2.0})) : (double)((void)((sl[3] = 300.0)), slider_automate_mask({4.0}))));
}

double Gen::F_ufrac(double p0) {
  L_ufrac_id = p0;
  return (EQ(L_ufrac_id, 1.0) ? (double)(sl[1] / 9.0) : (double)(EQ(L_ufrac_id, 2.0) ? (double)(sl[2] / 100.0) : (double)(rt_log((rt_max(10.0, sl[3]) / 10.0)) / rt_log(500.0))));
}

double Gen::F_ufreq(double p0, double p1) {
  L_ufreq_dst = p0; L_ufreq_f = p1;
  return ((L_ufreq_f < 100.0) ? (double)sprintf({L_ufreq_dst, 10000.0, L_ufreq_f}) : (double)((L_ufreq_f < 1000.0) ? (double)sprintf({L_ufreq_dst, 10001.0, std::floor((L_ufreq_f + 0.5))}) : (double)sprintf({L_ufreq_dst, 10002.0, (L_ufreq_f / 1000.0)})));
}

double Gen::F_uin(double p0, double p1, double p2, double p3) {
  L_uin_x = p0; L_uin_y = p1; L_uin_w = p2; L_uin_h = p3;
  return (((((v_mouse_x >= L_uin_x) && (v_mouse_x < (L_uin_x + L_uin_w))) && (v_mouse_y >= L_uin_y)) && (v_mouse_y < (L_uin_y + L_uin_h))) ? 1.0 : 0.0);
}

double Gen::F_uinc(double p0, double p1, double p2) {
  L_uinc_cx = p0; L_uinc_cy = p1; L_uinc_rad = p2;
  return (((rt_sqr((v_mouse_x - L_uinc_cx)) + rt_sqr((v_mouse_y - L_uinc_cy))) <= (L_uinc_rad * L_uinc_rad)) ? 1.0 : 0.0);
}

double Gen::F_uknob(double p0, double p1, double p2, double p3, double p4, double p5, double p6, double p7) {
  L_uknob_id = p0; L_uknob_cx = p1; L_uknob_cy = p2; L_uknob_rad = p3; L_uknob_f = p4; L_uknob_label = p5; L_uknob_val = p6; L_uknob_steps = p7;
  return ((void)(ASG(L_uknob_hot, (((EQ(g_uhov, L_uknob_id) || EQ(g_udrag, L_uknob_id)) || (EQ(L_uknob_id, 1.0) && EQ(g_udrag, 2.0))) ? 1.0 : 0.0))), (void)(gfx_setfont({7.0})), (void)(F_ucol(g_ucmuted, 1.0)), (void)(F_utext(L_uknob_label, L_uknob_cx, ((L_uknob_cy - L_uknob_rad) - (20.0 * g_usc)))), (void)(F_ucol(g_uchair, 1.0)), (void)(F_uarc(L_uknob_cx, L_uknob_cy, L_uknob_rad, (-2.356), 2.356, (3.0 * g_usc))), (void)(((L_uknob_f > 0.002) ? (double)((void)(F_ucol(g_ucctl, (B(L_uknob_hot) ? (double)1.0 : (double)0.85))), F_uarc(L_uknob_cx, L_uknob_cy, L_uknob_rad, (-2.356), ((-2.356) + (4.712 * L_uknob_f)), (3.0 * g_usc))) : 0.0)), (void)(((L_uknob_steps > 0.0) ? (double)((void)((L_uknob_k = 0.0)), [&]() -> double { int n_ = rt_i32((L_uknob_steps + 1.0)); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)(ASG(L_uknob_ka, ((-2.356) + (4.712 * (L_uknob_k / L_uknob_steps))))), (void)(F_ucol(g_ucdim, 1.0)), (void)(gfx_circle({(L_uknob_cx + (rt_sin(L_uknob_ka) * (L_uknob_rad + (5.0 * g_usc)))), (L_uknob_cy - (rt_cos(L_uknob_ka) * (L_uknob_rad + (5.0 * g_usc)))), rt_max(0.8, g_usc), 1.0, 1.0})), (L_uknob_k += 1.0))); } return 0.0; }()) : 0.0)), (void)(ASG(L_uknob_br, (L_uknob_rad - (6.0 * g_usc)))), (void)(F_ucol((B(L_uknob_hot) ? g_uchair : g_ucsurf), 1.0)), (void)(gfx_circle({L_uknob_cx, L_uknob_cy, L_uknob_br, 1.0, 1.0})), (void)(F_ucol(g_ucbord, 1.0)), (void)(gfx_circle({L_uknob_cx, L_uknob_cy, L_uknob_br, 0.0, 1.0})), (void)(ASG(L_uknob_ang, ((-2.356) + (4.712 * L_uknob_f)))), (void)(ASG(L_uknob_sx, rt_sin(L_uknob_ang))), (void)(ASG(L_uknob_sy, (-rt_cos(L_uknob_ang)))), (void)(F_ucol(g_ucctl, 1.0)), (void)((L_uknob_k = (-1.0))), (void)([&]() -> double { int n_ = rt_i32(3.0); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)(gfx_line({(L_uknob_cx + (((L_uknob_sx * L_uknob_br) * 0.25) - (((L_uknob_sy * L_uknob_k) * 0.6) * g_usc))), ((L_uknob_cy + ((L_uknob_sy * L_uknob_br) * 0.25)) + (((L_uknob_sx * L_uknob_k) * 0.6) * g_usc)), (L_uknob_cx + (((L_uknob_sx * L_uknob_br) * 0.85) - (((L_uknob_sy * L_uknob_k) * 0.6) * g_usc))), ((L_uknob_cy + ((L_uknob_sy * L_uknob_br) * 0.85)) + (((L_uknob_sx * L_uknob_k) * 0.6) * g_usc)), 1.0})), (L_uknob_k += 1.0))); } return 0.0; }()), (void)(gfx_setfont({2.0})), (void)(F_ucol(g_uctext, 1.0)), (void)(F_utext(L_uknob_val, L_uknob_cx, ((L_uknob_cy + L_uknob_rad) + (5.0 * g_usc)))), (B(F_uinc(L_uknob_cx, L_uknob_cy, (L_uknob_rad + (8.0 * g_usc)))) ? (double)(g_uhov = L_uknob_id) : 0.0));
}

double Gen::F_ulogo(double p0, double p1) {
  L_ulogo_rx = p0; L_ulogo_cy = p1;
  return ((void)(ASG(L_ulogo_big, ((g_usc >= 1.5) ? 1.0 : 0.0))), (void)(ASG(L_ulogo_w, (B(L_ulogo_big) ? g_lgw2 : g_lgw1))), (void)(ASG(L_ulogo_h, (B(L_ulogo_big) ? g_lgh2 : g_lgh1))), (void)(ASG(L_ulogo_m, (B(L_ulogo_big) ? g_m_lg2 : g_m_lg1))), (void)(ASG(L_ulogo_x0, std::floor((L_ulogo_rx - L_ulogo_w)))), (void)(ASG(L_ulogo_y0, std::floor((L_ulogo_cy - (L_ulogo_h / 2.0))))), (void)((L_ulogo_r = 0.0)), [&]() -> double { int n_ = rt_i32(L_ulogo_h); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)((L_ulogo_c = 0.0)), (void)([&]() -> double { int n_ = rt_i32(L_ulogo_w); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)(ASG(L_ulogo_v, M((L_ulogo_m + ((L_ulogo_r * L_ulogo_w) + L_ulogo_c))))), (void)(((L_ulogo_v > 0.0) ? (double)((void)(ASG(v_gfx_a, (L_ulogo_v / 15.0))), gfx_rect({(L_ulogo_x0 + L_ulogo_c), (L_ulogo_y0 + L_ulogo_r), 1.0, 1.0})) : 0.0)), (L_ulogo_c += 1.0))); } return 0.0; }()), (L_ulogo_r += 1.0))); } return 0.0; }());
}

double Gen::F_umap(double p0) {
  L_umap_v = p0;
  return (B(g_uview) ? (double)std::floor(((g_ugy + g_upt) + (((g_ugh - g_upt) - g_upd) * rt_min(1.0, rt_max(0.0, ((-L_umap_v) / g_urng)))))) : (double)std::floor(((g_ugy + g_upd) + ((g_ugh - (2.0 * g_upd)) * ((1.0 - L_umap_v) / 2.0)))));
}

double Gen::F_uright(double p0, double p1, double p2) {
  L_uright_str = p0; L_uright_rx = p1; L_uright_ty = p2;
  return ((void)(gfx_measurestr(L_uright_str, L_uright_tw, L_uright_th)), (void)(ASG(v_gfx_x, std::floor((L_uright_rx - L_uright_tw)))), (void)(ASG(v_gfx_y, std::floor(L_uright_ty))), gfx_drawstr({L_uright_str}));
}

double Gen::F_urr(double p0, double p1, double p2, double p3, double p4, double p5) {
  L_urr_x = p0; L_urr_y = p1; L_urr_w = p2; L_urr_h = p3; L_urr_rad = p4; L_urr_fill = p5;
  return ((void)(ASG(L_urr_rr, std::floor(rt_min(L_urr_rad, (rt_min(L_urr_w, L_urr_h) / 2.0))))), (B(L_urr_fill) ? (double)((L_urr_rr < 1.0) ? (double)gfx_rect({L_urr_x, L_urr_y, L_urr_w, L_urr_h}) : (double)((void)(gfx_rect({(L_urr_x + L_urr_rr), L_urr_y, (L_urr_w - (2.0 * L_urr_rr)), L_urr_h})), (void)(gfx_rect({L_urr_x, (L_urr_y + L_urr_rr), L_urr_rr, (L_urr_h - (2.0 * L_urr_rr))})), (void)(gfx_rect({(L_urr_x + (L_urr_w - L_urr_rr)), (L_urr_y + L_urr_rr), L_urr_rr, (L_urr_h - (2.0 * L_urr_rr))})), (void)(gfx_circle({(L_urr_x + L_urr_rr), (L_urr_y + L_urr_rr), L_urr_rr, 1.0, 1.0})), (void)(gfx_circle({(L_urr_x + ((L_urr_w - L_urr_rr) - 1.0)), (L_urr_y + L_urr_rr), L_urr_rr, 1.0, 1.0})), (void)(gfx_circle({(L_urr_x + L_urr_rr), (L_urr_y + ((L_urr_h - L_urr_rr) - 1.0)), L_urr_rr, 1.0, 1.0})), gfx_circle({(L_urr_x + ((L_urr_w - L_urr_rr) - 1.0)), (L_urr_y + ((L_urr_h - L_urr_rr) - 1.0)), L_urr_rr, 1.0, 1.0}))) : (double)gfx_roundrect({L_urr_x, L_urr_y, (L_urr_w - 1.0), (L_urr_h - 1.0), L_urr_rr, 1.0})));
}

double Gen::F_uset(double p0, double p1) {
  L_uset_id = p0; L_uset_f = p1;
  return ((void)(ASG(L_uset_f, rt_max(0.0, rt_min(1.0, L_uset_f)))), (EQ(L_uset_id, 1.0) ? (double)((void)(ASG(sl[1], std::floor(((L_uset_f * 9.0) + 0.5)))), slider_automate_mask({1.0})) : (double)(EQ(L_uset_id, 2.0) ? (double)((void)(ASG(sl[2], (std::floor(((L_uset_f * 1000.0) + 0.5)) / 10.0))), slider_automate_mask({2.0})) : (double)((void)(ASG(sl[3], std::floor(((10.0 * rt_pow(500.0, L_uset_f)) + 0.5)))), slider_automate_mask({4.0})))));
}

double Gen::F_usign(double p0, double p1) {
  L_usign_dst = p0; L_usign_v = p1;
  return ((std::fabs(L_usign_v) < 0.005) ? (double)sprintf({L_usign_dst, 10003.0}) : (double)((L_usign_v > 0.0) ? (double)sprintf({L_usign_dst, 10004.0, L_usign_v}) : (double)sprintf({L_usign_dst, 10005.0, (-L_usign_v)})));
}

double Gen::F_utab(double p0, double p1, double p2, double p3, double p4) {
  L_utab_x = p0; L_utab_w = p1; L_utab_on = p2; L_utab_hot = p3; L_utab_str = p4;
  return ((void)(ASG(L_utab_bot, std::floor((50.0 * g_usc)))), (void)(ASG(L_utab_r, std::floor((6.0 * g_usc)))), (void)(ASG(L_utab_b, rt_max(1.0, std::floor((1.4 * g_usc))))), (void)((B(L_utab_on) ? (double)((void)(ASG(L_utab_ty, std::floor((23.0 * g_usc)))), (void)(F_ucol(g_ucacto, 1.0)), (void)(F_urr(L_utab_x, L_utab_ty, L_utab_w, (((L_utab_bot - L_utab_ty) + L_utab_r) + 2.0), L_utab_r, 1.0)), (void)(F_ucol(g_ucbg, 1.0)), (void)(F_urr((L_utab_x + L_utab_b), (L_utab_ty + L_utab_b), (L_utab_w - (2.0 * L_utab_b)), (((L_utab_bot - L_utab_ty) + L_utab_r) + 2.0), (L_utab_r - L_utab_b), 1.0)), F_ucol(g_uctext, 1.0)) : (double)((void)(ASG(L_utab_ty, std::floor((26.0 * g_usc)))), (void)(F_ucol((B(L_utab_hot) ? g_uchair : g_ucsurf), 1.0)), (void)(F_urr(L_utab_x, L_utab_ty, L_utab_w, (((L_utab_bot - L_utab_ty) + L_utab_r) + 2.0), L_utab_r, 1.0)), F_ucol((B(L_utab_hot) ? g_uctext : g_ucmuted), 1.0)))), F_utext(L_utab_str, (L_utab_x + (L_utab_w / 2.0)), ((B(L_utab_on) ? (double)28.0 : (double)30.0) * g_usc)));
}

double Gen::F_utext(double p0, double p1, double p2) {
  L_utext_str = p0; L_utext_cx = p1; L_utext_ty = p2;
  return ((void)(gfx_measurestr(L_utext_str, L_utext_tw, L_utext_th)), (void)(ASG(v_gfx_x, std::floor((L_utext_cx - (L_utext_tw / 2.0))))), (void)(ASG(v_gfx_y, std::floor(L_utext_ty))), gfx_drawstr({L_utext_str}));
}
}
