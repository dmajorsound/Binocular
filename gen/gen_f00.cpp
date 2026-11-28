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

double Gen::F_lcalc() {
  
  return ((void)((L_lcalc_n = 0.0)), (void)((L_lcalc_e = 0.0)), (void)((L_lcalc_b = 0.0)), (void)([&]() -> double { int n_ = rt_i32(800.0); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)((L_lcalc_n += M((g_m_hmc + L_lcalc_b)))), (void)((L_lcalc_e += M((g_m_hme + L_lcalc_b)))), (L_lcalc_b += 1.0))); } return 0.0; }()), (void)((g_lint = (-200.0))), (void)((((L_lcalc_n > 0.5) && (L_lcalc_e > 0.0)) ? (double)((void)(ASG(L_lcalc_g, rt_max(0.0, std::ceil(((((-0.691) + ((10.0 * rt_log10((L_lcalc_e / L_lcalc_n))) - 10.0)) + 70.0) * 10.0))))), (void)((L_lcalc_n2 = 0.0)), (void)((L_lcalc_e2 = 0.0)), (void)((L_lcalc_b = L_lcalc_g)), (void)([&]() -> double { int n_ = rt_i32(rt_max(0.0, (800.0 - L_lcalc_g))); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)((L_lcalc_n2 += M((g_m_hmc + L_lcalc_b)))), (void)((L_lcalc_e2 += M((g_m_hme + L_lcalc_b)))), (L_lcalc_b += 1.0))); } return 0.0; }()), (((L_lcalc_n2 > 0.5) && (L_lcalc_e2 > 0.0)) ? (double)ASG(g_lint, ((-0.691) + (10.0 * rt_log10((L_lcalc_e2 / L_lcalc_n2))))) : 0.0)) : 0.0)), (void)((L_lcalc_n = 0.0)), (void)((L_lcalc_e = 0.0)), (void)((L_lcalc_b = 0.0)), (void)([&]() -> double { int n_ = rt_i32(800.0); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)((L_lcalc_n += M((g_m_hsc + L_lcalc_b)))), (void)((L_lcalc_e += M((g_m_hse + L_lcalc_b)))), (L_lcalc_b += 1.0))); } return 0.0; }()), (void)((g_llra = (-1.0))), (void)((((L_lcalc_n > 0.5) && (L_lcalc_e > 0.0)) ? (double)((void)(ASG(L_lcalc_g, rt_max(0.0, std::ceil(((((-0.691) + ((10.0 * rt_log10((L_lcalc_e / L_lcalc_n))) - 20.0)) + 70.0) * 10.0))))), (void)((L_lcalc_n2 = 0.0)), (void)((L_lcalc_b = L_lcalc_g)), (void)([&]() -> double { int n_ = rt_i32(rt_max(0.0, (800.0 - L_lcalc_g))); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)((L_lcalc_n2 += M((g_m_hsc + L_lcalc_b)))), (L_lcalc_b += 1.0))); } return 0.0; }()), ((L_lcalc_n2 > 0.5) ? (double)((void)((L_lcalc_cum = 0.0)), (void)((L_lcalc_b = L_lcalc_g)), (void)((L_lcalc_lo = (-1.0))), (void)((L_lcalc_hi = (-1.0))), (void)([&]() -> double { int n_ = rt_i32(rt_max(0.0, (800.0 - L_lcalc_g))); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)((L_lcalc_cum += M((g_m_hsc + L_lcalc_b)))), (void)((((L_lcalc_lo < 0.0) && (L_lcalc_cum >= (0.1 * L_lcalc_n2))) ? (double)(L_lcalc_lo = L_lcalc_b) : 0.0)), (void)((((L_lcalc_hi < 0.0) && (L_lcalc_cum >= (0.95 * L_lcalc_n2))) ? (double)(L_lcalc_hi = L_lcalc_b) : 0.0)), (L_lcalc_b += 1.0))); } return 0.0; }()), ASG(g_llra, rt_max(0.0, ((L_lcalc_hi - L_lcalc_lo) * 0.1)))) : 0.0)) : 0.0)), (g_ldirty = 0.0));
}

double Gen::F_lcoef() {
  
  return ((void)(ASG(L_lcoef_k, rt_tan((3.141592653589793 * (1681.974450955533 / v_srate))))), (void)((L_lcoef_q = 0.7071752369554196)), (void)((L_lcoef_vh = 1.5848647011308556)), (void)(ASG(L_lcoef_vb, rt_pow(L_lcoef_vh, 0.4996667741545416))), (void)(ASG(L_lcoef_a0, ((1.0 + (L_lcoef_k / L_lcoef_q)) + (L_lcoef_k * L_lcoef_k)))), (void)(ASG(g_kb0, (((L_lcoef_vh + (L_lcoef_vb * (L_lcoef_k / L_lcoef_q))) + (L_lcoef_k * L_lcoef_k)) / L_lcoef_a0))), (void)(ASG(g_kb1, (2.0 * (((L_lcoef_k * L_lcoef_k) - L_lcoef_vh) / L_lcoef_a0)))), (void)(ASG(g_kb2, (((L_lcoef_vh - (L_lcoef_vb * (L_lcoef_k / L_lcoef_q))) + (L_lcoef_k * L_lcoef_k)) / L_lcoef_a0))), (void)(ASG(g_ka1, (2.0 * (((L_lcoef_k * L_lcoef_k) - 1.0) / L_lcoef_a0)))), (void)(ASG(g_ka2, (((1.0 - (L_lcoef_k / L_lcoef_q)) + (L_lcoef_k * L_lcoef_k)) / L_lcoef_a0))), (void)(ASG(L_lcoef_k, rt_tan((3.141592653589793 * (38.13547087613982 / v_srate))))), (void)((L_lcoef_q = 0.5003270373253953)), (void)(ASG(L_lcoef_a0, ((1.0 + (L_lcoef_k / L_lcoef_q)) + (L_lcoef_k * L_lcoef_k)))), (void)((g_hb0 = 1.0)), (void)((g_hb1 = (-2.0))), (void)((g_hb2 = 1.0)), (void)(ASG(g_ha1, (2.0 * (((L_lcoef_k * L_lcoef_k) - 1.0) / L_lcoef_a0)))), (void)(ASG(g_ha2, (((1.0 - (L_lcoef_k / L_lcoef_q)) + (L_lcoef_k * L_lcoef_k)) / L_lcoef_a0))), (void)(ASG(g_tpo, ((v_srate < 100000.0) ? (double)4.0 : (double)2.0))), (void)(ASG(L_lcoef_l, (12.0 * g_tpo))), (void)(ASG(L_lcoef_c, ((L_lcoef_l - 1.0) / 2.0))), (void)((L_lcoef_t = 0.0)), (void)([&]() -> double { int n_ = rt_i32(L_lcoef_l); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)(ASG(L_lcoef_x, ((L_lcoef_t - L_lcoef_c) / g_tpo))), (void)(ASG(L_lcoef_w, (0.5 - (0.5 * rt_cos((6.283185307179586 * ((L_lcoef_t + 1.0) / (L_lcoef_l + 1.0)))))))), (void)(ASG(M((g_m_tpc + L_lcoef_t)), (((std::fabs(L_lcoef_x) < 1e-06) ? (double)1.0 : (double)(rt_sin((3.141592653589793 * L_lcoef_x)) / (3.141592653589793 * L_lcoef_x))) * L_lcoef_w))), (L_lcoef_t += 1.0))); } return 0.0; }()), (void)((L_lcoef_p = 0.0)), (void)([&]() -> double { int n_ = rt_i32(g_tpo); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)((L_lcoef_s = 0.0)), (void)((L_lcoef_k = 0.0)), (void)([&]() -> double { int n_ = rt_i32(12.0); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)((L_lcoef_s += M((g_m_tpc + (L_lcoef_p + (g_tpo * L_lcoef_k)))))), (L_lcoef_k += 1.0))); } return 0.0; }()), (void)((L_lcoef_k = 0.0)), (void)([&]() -> double { int n_ = rt_i32(12.0); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)(ASG(M((g_m_tpc + (L_lcoef_p + (g_tpo * L_lcoef_k)))), (M((g_m_tpc + (L_lcoef_p + (g_tpo * L_lcoef_k)))) / L_lcoef_s))), (L_lcoef_k += 1.0))); } return 0.0; }()), (L_lcoef_p += 1.0))); } return 0.0; }()), (g_lsr2 = v_srate));
}

double Gen::F_lcolr(double p0, double p1, double p2) {
  L_lcolr_l = p0; L_lcolr_t = p1; L_lcolr_tol = p2;
  return ((L_lcolr_l <= (-70.0)) ? g_ucdim : (((L_lcolr_l - L_lcolr_t) > (L_lcolr_tol + 2.0)) ? g_ucred : (((L_lcolr_l - L_lcolr_t) > L_lcolr_tol) ? g_ucwarn : (((L_lcolr_l - L_lcolr_t) < (-(L_lcolr_tol + 6.0))) ? g_ucblue : g_ucmint))));
}

double Gen::F_ldec(double p0, double p1, double p2) {
  L_ldec_str = p0; L_ldec_dst = p1; L_ldec_n = p2;
  return ((void)((L_ldec_i = 0.0)), [&]() -> double { int n_ = rt_i32(L_ldec_n); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)(ASG(L_ldec_c, str_getchar({L_ldec_str, L_ldec_i}))), (void)(ASG(M((L_ldec_dst + L_ldec_i)), ((L_ldec_c <= 57.0) ? (double)(L_ldec_c - 48.0) : (double)((L_ldec_c - 97.0) + 10.0)))), (L_ldec_i += 1.0))); } return 0.0; }());
}

double Gen::F_lhist(double p0, double p1, double p2, double p3) {
  L_lhist_e = p0; L_lhist_cn = p1; L_lhist_en = p2; L_lhist_sg = p3;
  return ((L_lhist_e > 0.0) ? (double)((void)(ASG(L_lhist_l, ((-0.691) + (10.0 * rt_log10(L_lhist_e))))), ((L_lhist_l >= (-70.0)) ? (double)((void)(ASG(L_lhist_b, rt_min(799.0, std::floor(((L_lhist_l + 70.0) * 10.0))))), (void)((M((L_lhist_cn + L_lhist_b)) += L_lhist_sg)), (M((L_lhist_en + L_lhist_b)) += (L_lhist_sg * L_lhist_e))) : 0.0)) : 0.0);
}

double Gen::F_lradd(double p0) {
  L_lradd_e = p0;
  return ((void)(ASG(L_lradd_s, F_lrspan())), (void)((M((g_m_lhi + rt_mod(g_lht, g_ln))) = L_lradd_e)), (void)(ASG(L_lradd_k, std::floor((rt_mod(g_lht, L_lradd_s) * (360.0 / L_lradd_s))))), (void)((NE(L_lradd_k, g_lrk) ? (double)((void)((M((g_m_wde + L_lradd_k)) = 0.0)), (void)((M((g_m_wdn + L_lradd_k)) = 0.0)), (g_lrk = L_lradd_k)) : 0.0)), (void)((M((g_m_wde + L_lradd_k)) += L_lradd_e)), (void)((M((g_m_wdn + L_lradd_k)) += 1.0)), (g_lht += 1.0));
}

double Gen::F_lrbuild() {
  
  return ((void)(ASG(L_lrbuild_s, F_lrspan())), (void)(memset(g_m_wde, 0.0, 360.0)), (void)(memset(g_m_wdn, 0.0, 360.0)), (void)(ASG(L_lrbuild_n, rt_min(g_lht, L_lrbuild_s))), (void)(ASG(L_lrbuild_j, (g_lht - L_lrbuild_n))), (void)((g_lrk = (-1.0))), (void)([&]() -> double { int n_ = rt_i32(L_lrbuild_n); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)(ASG(L_lrbuild_k, std::floor((rt_mod(L_lrbuild_j, L_lrbuild_s) * (360.0 / L_lrbuild_s))))), (void)((M((g_m_wde + L_lrbuild_k)) += M((g_m_lhi + rt_mod(L_lrbuild_j, g_ln))))), (void)((M((g_m_wdn + L_lrbuild_k)) += 1.0)), (void)((g_lrk = L_lrbuild_k)), (L_lrbuild_j += 1.0))); } return 0.0; }()), (g_lrsp = sl[12]));
}

double Gen::F_lreset() {
  
  return ((void)(memset(g_m_lme, 0.0, g_ln)), (void)(memset(g_m_lse, 0.0, g_ln)), (void)(memset(g_m_lhs, 0.0, g_ln)), (void)(memset(g_m_hmc, 0.0, 4096.0)), (void)(memset(g_m_lr30, 0.0, 32.0)), (void)(memset(g_m_wde, 0.0, 360.0)), (void)(memset(g_m_wdn, 0.0, 360.0)), (void)(memset(g_m_tph, 0.0, 32.0)), (void)(ASG(g_lk1l, ASG(g_lk2l, ASG(g_lk1r, ASG(g_lk2r, ASG(g_lh1l, ASG(g_lh2l, ASG(g_lh1r, (g_lh2r = 0.0))))))))), (void)((g_lcur = (-1000000000.0))), (void)((g_lprev = g_lcur)), (void)((g_lcont = 0.0)), (void)((g_lsum = 0.0)), (void)((g_lcnt = 0.0)), (void)((g_lvn = 0.0)), (void)((g_lri = 0.0)), (void)(ASG(g_lmom, (g_lst = (-200.0)))), (void)((g_lnsl = 0.0)), (void)((g_lht = 0.0)), (void)((g_lrk = (-1.0))), (void)((g_lint = (-200.0))), (void)((g_llra = (-1.0))), (void)((g_ltpk = 0.0)), (void)((g_ltpi = 0.0)), (void)((g_ldirty = 0.0)), (g_lreq = 0.0));
}

double Gen::F_lrspan() {
  
  return ((sl[12] < 0.5) ? (double)600.0 : (double)((sl[12] < 1.5) ? (double)3000.0 : (double)((sl[12] < 2.5) ? (double)9000.0 : (double)((sl[12] < 3.5) ? (double)18000.0 : (double)((sl[12] < 4.5) ? (double)36000.0 : (double)((sl[12] < 5.5) ? (double)72000.0 : (double)180000.0))))));
}

double Gen::F_lslot() {
  
  return ((void)(ASG(L_lslot_e, (g_lsum / g_lcnt))), (void)(ASG(g_lcont, (EQ(g_lcur, (g_lprev + 1.0)) ? (double)(g_lcont + 1.0) : (double)1.0))), (void)((g_lprev = g_lcur)), (void)((M((g_m_lr30 + g_lri)) = L_lslot_e)), (void)(ASG(g_lri, rt_mod((g_lri + 1.0), 30.0))), (void)((L_lslot_em = 0.0)), (void)((L_lslot_i = 0.0)), (void)(ASG(L_lslot_n, rt_min(4.0, g_lcont))), (void)([&]() -> double { int n_ = rt_i32(L_lslot_n); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)((L_lslot_em += M((g_m_lr30 + rt_mod((((g_lri - 1.0) - L_lslot_i) + 60.0), 30.0))))), (L_lslot_i += 1.0))); } return 0.0; }()), (void)(ASG(L_lslot_em, (L_lslot_em / L_lslot_n))), (void)((L_lslot_es = 0.0)), (void)((L_lslot_i = 0.0)), (void)(ASG(L_lslot_n, rt_min(30.0, g_lcont))), (void)([&]() -> double { int n_ = rt_i32(L_lslot_n); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)((L_lslot_es += M((g_m_lr30 + rt_mod((((g_lri - 1.0) - L_lslot_i) + 60.0), 30.0))))), (L_lslot_i += 1.0))); } return 0.0; }()), (void)(ASG(L_lslot_es, (L_lslot_es / L_lslot_n))), (void)(ASG(g_lmom, ((L_lslot_em > 0.0) ? (double)((-0.691) + (10.0 * rt_log10(L_lslot_em))) : (double)(-200.0)))), (void)(ASG(g_lst, ((L_lslot_es > 0.0) ? (double)((-0.691) + (10.0 * rt_log10(L_lslot_es))) : (double)(-200.0)))), (void)(F_lstore(g_lcur, ((g_lcont >= 4.0) ? (double)L_lslot_em : (double)0.0), ((g_lcont >= 30.0) ? (double)L_lslot_es : (double)0.0))), (void)(F_lradd(L_lslot_em)), (g_ldirty = 1.0));
}

double Gen::F_lstore(double p0, double p1, double p2) {
  L_lstore_sid = p0; L_lstore_em = p1; L_lstore_es = p2;
  return ((void)(ASG(L_lstore_i, (L_lstore_sid - (std::floor((L_lstore_sid / g_ln)) * g_ln)))), (void)((B(M((g_m_lhs + L_lstore_i))) ? (double)((void)(F_lhist(M((g_m_lme + L_lstore_i)), g_m_hmc, g_m_hme, (-1.0))), (void)(F_lhist(M((g_m_lse + L_lstore_i)), g_m_hsc, g_m_hse, (-1.0))), (g_lnsl -= 1.0)) : 0.0)), (void)((M((g_m_lme + L_lstore_i)) = L_lstore_em)), (void)((M((g_m_lse + L_lstore_i)) = L_lstore_es)), (void)((M((g_m_lhs + L_lstore_i)) = 1.0)), (void)(F_lhist(L_lstore_em, g_m_hmc, g_m_hme, 1.0)), (void)(F_lhist(L_lstore_es, g_m_hsc, g_m_hse, 1.0)), (g_lnsl += 1.0));
}

double Gen::F_ltgt(double p0) {
  L_ltgt_p = p0;
  return ((L_ltgt_p < 0.5) ? (double)(-23.0) : (double)((L_ltgt_p < 1.5) ? (double)(-27.0) : (double)(((L_ltgt_p > 3.5) && (L_ltgt_p < 4.5)) ? (double)(-16.0) : (double)(-14.0))));
}

double Gen::F_ltol(double p0) {
  L_ltol_p = p0;
  return ((L_ltol_p < 0.5) ? (double)0.5 : (double)((L_ltol_p < 1.5) ? (double)2.0 : (double)1.0));
}

double Gen::F_ltpl(double p0) {
  L_ltpl_p = p0;
  return (((L_ltpl_p > 0.5) && (L_ltpl_p < 1.5)) ? (double)(-2.0) : (double)(-1.0));
}

double Gen::F_lvcol(double p0, double p1, double p2, double p3) {
  L_lvcol_l = p0; L_lvcol_t = p1; L_lvcol_tol = p2; L_lvcol_p = p3;
  return ((L_lvcol_l <= (-70.0)) ? g_ucdim : ((std::fabs((L_lvcol_l - L_lvcol_t)) <= L_lvcol_tol) ? g_ucmint : ((L_lvcol_p < 1.5) ? g_ucred : ((L_lvcol_l > L_lvcol_t) ? g_ucwarn : g_ucblue))));
}

double Gen::F_setup() {
  
  return ((void)(ASG(g_nb, M((g_m_bl + rt_max(0.0, rt_min(9.0, std::floor((sl[1] + 0.5)))))))), (void)((g_ffm = 1.0)), (void)(ASG(g_fn, ((v_srate <= 50000.0) ? (double)16384.0 : (double)32768.0))), (void)(ASG(g_fhop, (g_fn / 16.0))), (void)(ASG(g_fnb, (g_fn / 2.0))), (void)(ASG(g_fs2, (g_fn / 4.0))), (void)(ASG(g_fhop2, (g_fs2 / 8.0))), (void)(ASG(g_fnb2, (g_fs2 / 2.0))), (void)((g_flo = 20.0)), (void)(ASG(g_fhi, rt_min(20000.0, (0.45 * v_srate)))), (void)(ASG(L_setup_sp, ((rt_log((g_fhi / g_flo)) / rt_log(2.0)) / g_nb))), (void)(ASG(L_setup_wm, rt_pow(2.0, (1.0 - (2.0 * (sl[2] / 100.0)))))), (void)(ASG(L_setup_bw, ((L_setup_sp * L_setup_wm) * 1.6))), (void)(ASG(L_setup_bo, (L_setup_sp * L_setup_wm))), (void)(memset(g_m_st, 0.0, 2048.0)), (void)(memset(g_m_rl, 0.0, 65536.0)), (void)(memset(g_m_rr, 0.0, 65536.0)), (void)(memset(g_m_sxx, 0.0, 114800.0)), (void)(memset(g_m_s2x, 0.0, 49200.0)), (void)((L_setup_i = 0.0)), (void)([&]() -> double { int n_ = rt_i32(g_fn); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)(ASG(M((g_m_win + L_setup_i)), (0.5 - (0.5 * rt_cos((6.283185307179586 * ((L_setup_i + 0.5) / g_fn))))))), (L_setup_i += 1.0))); } return 0.0; }()), (void)((L_setup_i = 0.0)), (void)([&]() -> double { int n_ = rt_i32(g_fs2); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)(ASG(M((g_m_wi2 + L_setup_i)), (0.5 - (0.5 * rt_cos((6.283185307179586 * ((L_setup_i + 0.5) / g_fs2))))))), (L_setup_i += 1.0))); } return 0.0; }()), (void)((g_fwp = 0.0)), (void)((g_fhc = 0.0)), (void)((g_fhc2 = 0.0)), (void)((g_fpend = 0.0)), (void)((g_f2pend = 0.0)), (void)((g_foffp = 0.0)), (void)((L_setup_i = 0.0)), (void)([&]() -> double { int n_ = rt_i32(g_nb); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)(ASG(L_setup_fc, (g_flo * rt_pow((g_fhi / g_flo), ((L_setup_i + 0.5) / g_nb))))), (void)((B(g_ffm) ? ((void)(ASG(L_setup_e0, (L_setup_fc * rt_pow(2.0, ((-L_setup_bo) / 2.0))))), (void)(ASG(L_setup_e1, (L_setup_fc * rt_pow(2.0, (L_setup_bo / 2.0))))), (void)(ASG(M((g_m_eng + L_setup_i)), ((((L_setup_e1 - L_setup_e0) * (g_fs2 / v_srate)) >= 3.0) ? 1.0 : 0.0))), (B(M((g_m_eng + L_setup_i))) ? ((void)(ASG(M((g_m_u0 + L_setup_i)), rt_max(0.5, rt_min((g_fnb2 - 1.0), ((L_setup_e0 * (g_fs2 / v_srate)) + 0.5))))), ASG(M((g_m_u1 + L_setup_i)), rt_max(0.5, rt_min((g_fnb2 - 1.0), ((L_setup_e1 * (g_fs2 / v_srate)) + 0.5))))) : ((void)(ASG(M((g_m_u0 + L_setup_i)), rt_max(0.5, rt_min((g_fnb - 1.0), ((L_setup_e0 * (g_fn / v_srate)) + 0.5))))), ASG(M((g_m_u1 + L_setup_i)), rt_max(0.5, rt_min((g_fnb - 1.0), ((L_setup_e1 * (g_fn / v_srate)) + 0.5))))))) : ((void)((M((g_m_eng + L_setup_i)) = 0.0)), (void)(ASG(L_setup_w0, (6.283185307179586 * (L_setup_fc / v_srate)))), (void)(ASG(L_setup_xs, (((rt_log(2.0) / 2.0) * L_setup_bw) * (L_setup_w0 / rt_sin(L_setup_w0))))), (void)(ASG(L_setup_al, (rt_sin(L_setup_w0) * ((rt_exp(L_setup_xs) - rt_exp((-L_setup_xs))) / 2.0)))), (void)(ASG(L_setup_p, (g_m_st + (L_setup_i * 32.0)))), (void)(ASG(M(L_setup_p), (L_setup_al / (1.0 + L_setup_al)))), (void)(ASG(M((L_setup_p + 1.0)), ((-2.0) * (rt_cos(L_setup_w0) / (1.0 + L_setup_al))))), ASG(M((L_setup_p + 2.0)), ((1.0 - L_setup_al) / (1.0 + L_setup_al)))))), (void)((M((g_m_fc + L_setup_i)) = L_setup_fc)), (void)((M((g_m_cor + L_setup_i)) = 0.0)), (void)((M((g_m_lev + L_setup_i)) = (-200.0))), (void)((M((g_m_val + L_setup_i)) = 0.0)), (void)((M((g_m_mon + L_setup_i)) = 0.0)), (void)((M((g_m_hld + L_setup_i)) = 1.0)), (void)((M((g_m_ht + L_setup_i)) = 0.0)), (void)((M((g_m_hmo + L_setup_i)) = 0.0)), (void)((M((g_m_hmt + L_setup_i)) = 0.0)), (L_setup_i += 1.0))); } return 0.0; }()), (void)(ASG(g_bxy, ASG(g_bxx, (g_byy = 0.0)))), (void)((g_bcor = 0.0)), (void)((g_bval = 0.0)), (void)((g_bhld = 1.0)), (void)((g_bht = 0.0)), (void)((g_ofs_ok = 0.0)), (void)((g_ofs_lag = 0.0)), (void)((g_ofs_sgn = 1.0)), (void)((g_refon = 0.0)), (void)((g_lnb = sl[1])), (void)((g_lsel = sl[2])), (void)((g_lsr = v_srate)), (g_lin = sl[4]));
}

double Gen::F_uarc(double p0, double p1, double p2, double p3, double p4, double p5) {
  L_uarc_cx = p0; L_uarc_cy = p1; L_uarc_rad = p2; L_uarc_a1 = p3; L_uarc_a2 = p4; L_uarc_th = p5;
  return ((void)((L_uarc_k = 0.0)), [&]() -> double { int n_ = rt_i32(rt_max(1.0, std::ceil((L_uarc_th * 2.0)))); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)(gfx_arc({L_uarc_cx, L_uarc_cy, ((L_uarc_rad - (L_uarc_th / 2.0)) + (L_uarc_k * 0.5)), L_uarc_a1, L_uarc_a2, 1.0})), (L_uarc_k += 1.0))); } return 0.0; }());
}

double Gen::F_ubtn(double p0, double p1, double p2, double p3, double p4, double p5, double p6) {
  L_ubtn_x = p0; L_ubtn_y = p1; L_ubtn_w = p2; L_ubtn_h = p3; L_ubtn_on = p4; L_ubtn_hot = p5; L_ubtn_str = p6;
  return ((void)((B(L_ubtn_on) ? (double)((void)(F_ucol(g_ucacc, 1.0)), (void)(F_urr(L_ubtn_x, L_ubtn_y, L_ubtn_w, L_ubtn_h, (8.0 * g_usc), 1.0)), (void)(gfx_setfont({6.0})), F_ucol(g_ucacct, 1.0)) : (double)((void)(F_ucol((B(L_ubtn_hot) ? g_ucline : g_ucp2), 1.0)), (void)(F_urr(L_ubtn_x, L_ubtn_y, L_ubtn_w, L_ubtn_h, (8.0 * g_usc), 1.0)), (void)(F_ucol(g_ucline, 1.0)), (void)(F_urr(L_ubtn_x, L_ubtn_y, L_ubtn_w, L_ubtn_h, (8.0 * g_usc), 0.0)), (void)(gfx_setfont({9.0})), F_ucol(g_uctext, 1.0)))), F_utc(L_ubtn_str, (L_ubtn_x + (L_ubtn_w / 2.0)), (L_ubtn_y + (L_ubtn_h / 2.0))));
}

double Gen::F_ucol(double p0, double p1) {
  L_ucol_c = p0; L_ucol_a = p1;
  return ((void)(ASG(v_gfx_r, (rt_and(rt_shr(L_ucol_c, 16.0), 255.0) / 255.0))), (void)(ASG(v_gfx_g, (rt_and(rt_shr(L_ucol_c, 8.0), 255.0) / 255.0))), (void)(ASG(v_gfx_b, (rt_and(L_ucol_c, 255.0) / 255.0))), (v_gfx_a = L_ucol_a));
}

double Gen::F_ucorr(double p0, double p1) {
  L_ucorr_v = p0; L_ucorr_a = p1;
  return [&]() -> double { const double a0_ = F_ucorrc(L_ucorr_v); return F_ucol(a0_, L_ucorr_a); }();
}

double Gen::F_ucorrc(double p0) {
  L_ucorrc_v = p0;
  return ((L_ucorrc_v >= 0.0) ? (double)F_umix(g_ucblue, g_ucmint, (L_ucorrc_v / 0.6)) : (double)F_umix(g_uccoral, g_ucred, ((-L_ucorrc_v) / 0.6)));
}

double Gen::F_ucorrm(double p0, double p1) {
  L_ucorrm_v = p0; L_ucorrm_a = p1;
  return [&]() -> double { const double a0_ = [&]() -> double { const double a1_ = F_ucorrc(L_ucorrm_v); return F_umix(g_ucp, a1_, L_ucorrm_a); }(); return F_ucol(a0_, 1.0); }();
}

double Gen::F_udef(double p0) {
  L_udef_id = p0;
  return (EQ(L_udef_id, 1.0) ? (double)((void)((sl[1] = 7.0)), slider_automate_mask({1.0})) : (double)(EQ(L_udef_id, 2.0) ? (double)((void)((sl[2] = 50.0)), slider_automate_mask({2.0})) : (double)((void)((sl[3] = 300.0)), slider_automate_mask({4.0}))));
}

double Gen::F_udrop(double p0, double p1, double p2, double p3, double p4, double p5, double p6) {
  L_udrop_x = p0; L_udrop_y = p1; L_udrop_w = p2; L_udrop_h = p3; L_udrop_hot = p4; L_udrop_str = p5; L_udrop_fnt = p6;
  return ((void)(F_ucol((B(L_udrop_hot) ? g_ucline : g_ucp2), 1.0)), (void)(F_urr(L_udrop_x, L_udrop_y, L_udrop_w, L_udrop_h, (8.0 * g_usc), 1.0)), (void)(F_ucol(g_ucline, 1.0)), (void)(F_urr(L_udrop_x, L_udrop_y, L_udrop_w, L_udrop_h, (8.0 * g_usc), 0.0)), (void)(gfx_setfont({L_udrop_fnt})), (void)(F_ucol(g_uctext, 1.0)), (void)(F_utc(L_udrop_str, (L_udrop_x + ((L_udrop_w - (12.0 * g_usc)) / 2.0)), (L_udrop_y + (L_udrop_h / 2.0)))), (void)(F_ucol(g_ucmuted, 1.0)), gfx_triangle({(L_udrop_x + (L_udrop_w - (15.0 * g_usc))), (L_udrop_y + ((L_udrop_h / 2.0) - (2.0 * g_usc))), (L_udrop_x + (L_udrop_w - (9.0 * g_usc))), (L_udrop_y + ((L_udrop_h / 2.0) - (2.0 * g_usc))), (L_udrop_x + (L_udrop_w - (12.0 * g_usc))), ((L_udrop_y + (L_udrop_h / 2.0)) + (2.0 * g_usc))}));
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

double Gen::F_uknob(double p0, double p1, double p2, double p3, double p4, double p5, double p6) {
  L_uknob_id = p0; L_uknob_cx = p1; L_uknob_cy = p2; L_uknob_rad = p3; L_uknob_f = p4; L_uknob_label = p5; L_uknob_val = p6;
  return ((void)(ASG(L_uknob_hot, (((EQ(g_uhov, L_uknob_id) || EQ(g_udrag, L_uknob_id)) || (EQ(L_uknob_id, 1.0) && EQ(g_udrag, 2.0))) ? 1.0 : 0.0))), (void)(gfx_setfont({7.0})), (void)(F_ucol(g_ucmuted, 1.0)), (void)(F_utc(L_uknob_label, L_uknob_cx, g_ulby)), (void)(F_ucol((B(L_uknob_hot) ? g_ucline : g_ucp2), 1.0)), (void)(F_uarc(L_uknob_cx, L_uknob_cy, L_uknob_rad, (-2.356), 2.356, (3.0 * g_usc))), (void)(((L_uknob_f > 0.002) ? (double)((void)(F_ucol(g_ucacc, 1.0)), F_uarc(L_uknob_cx, L_uknob_cy, L_uknob_rad, (-2.356), ((-2.356) + (4.712 * L_uknob_f)), (3.0 * g_usc))) : 0.0)), (void)(ASG(L_uknob_ang, ((-2.356) + (4.712 * L_uknob_f)))), (void)(F_ucol(g_uctext, 1.0)), (void)(F_uquad(L_uknob_cx, L_uknob_cy, (L_uknob_cx + (rt_sin(L_uknob_ang) * (L_uknob_rad - (6.0 * g_usc)))), (L_uknob_cy - (rt_cos(L_uknob_ang) * (L_uknob_rad - (6.0 * g_usc)))), (1.6 * g_usc))), (void)(gfx_setfont({13.0})), (void)(F_ucol(g_uctext, 1.0)), (void)(F_utc(L_uknob_val, L_uknob_cx, g_unty)), (B(F_uinc(L_uknob_cx, L_uknob_cy, (L_uknob_rad + (8.0 * g_usc)))) ? (double)(g_uhov = L_uknob_id) : 0.0));
}

double Gen::F_ulc(double p0, double p1, double p2) {
  L_ulc_str = p0; L_ulc_x = p1; L_ulc_cy = p2;
  return ((void)(gfx_measurestr(L_ulc_str, L_ulc_tw, L_ulc_th)), (void)(ASG(v_gfx_x, std::floor((L_ulc_x + 0.5)))), (void)(ASG(v_gfx_y, std::floor(((L_ulc_cy - (L_ulc_th / 2.0)) + 0.5)))), gfx_drawstr({L_ulc_str}));
}

double Gen::F_ulogo(double p0, double p1) {
  L_ulogo_rx = p0; L_ulogo_cy = p1;
  return ((void)(ASG(L_ulogo_big, ((g_usc >= 1.5) ? 1.0 : 0.0))), (void)(ASG(L_ulogo_w, (B(L_ulogo_big) ? g_lgw2 : g_lgw1))), (void)(ASG(L_ulogo_h, (B(L_ulogo_big) ? g_lgh2 : g_lgh1))), (void)(ASG(L_ulogo_m, (B(L_ulogo_big) ? g_m_lg2 : g_m_lg1))), (void)(ASG(L_ulogo_x0, std::floor((L_ulogo_rx - L_ulogo_w)))), (void)(ASG(L_ulogo_y0, std::floor((L_ulogo_cy - (L_ulogo_h / 2.0))))), (void)((L_ulogo_r = 0.0)), [&]() -> double { int n_ = rt_i32(L_ulogo_h); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)((L_ulogo_c = 0.0)), (void)([&]() -> double { int n_ = rt_i32(L_ulogo_w); if (n_ > 1048576) n_ = 1048576; for (; n_ > 0; --n_) { (void)(((void)(ASG(L_ulogo_v, M((L_ulogo_m + ((L_ulogo_r * L_ulogo_w) + L_ulogo_c))))), (void)(((L_ulogo_v > 0.0) ? (double)((void)(ASG(v_gfx_a, (L_ulogo_v / 15.0))), gfx_rect({(L_ulogo_x0 + L_ulogo_c), (L_ulogo_y0 + L_ulogo_r), 1.0, 1.0})) : 0.0)), (L_ulogo_c += 1.0))); } return 0.0; }()), (L_ulogo_r += 1.0))); } return 0.0; }());
}

double Gen::F_umap(double p0) {
  L_umap_v = p0;
  return (B(g_uview) ? (double)std::floor(((g_ugy + g_upt) + ((g_ugh - g_upt) * rt_min(1.0, rt_max(0.0, ((-L_umap_v) / g_urng)))))) : (double)std::floor((g_ugy + (g_ugh * ((1.0 - L_umap_v) / 2.0)))));
}

double Gen::F_umix(double p0, double p1, double p2) {
  L_umix_c1 = p0; L_umix_c2 = p1; L_umix_t = p2;
  return ((void)(ASG(L_umix_t, rt_max(0.0, rt_min(1.0, L_umix_t)))), (((std::floor((((rt_and(rt_shr(L_umix_c1, 16.0), 255.0) * (1.0 - L_umix_t)) + (rt_and(rt_shr(L_umix_c2, 16.0), 255.0) * L_umix_t)) + 0.5)) * 65536.0) + (std::floor((((rt_and(rt_shr(L_umix_c1, 8.0), 255.0) * (1.0 - L_umix_t)) + (rt_and(rt_shr(L_umix_c2, 8.0), 255.0) * L_umix_t)) + 0.5)) * 256.0)) + std::floor((((rt_and(L_umix_c1, 255.0) * (1.0 - L_umix_t)) + (rt_and(L_umix_c2, 255.0) * L_umix_t)) + 0.5))));
}

double Gen::F_uquad(double p0, double p1, double p2, double p3, double p4) {
  L_uquad_x1 = p0; L_uquad_y1 = p1; L_uquad_x2 = p2; L_uquad_y2 = p3; L_uquad_w = p4;
  return ((void)(ASG(L_uquad_dx, (L_uquad_x2 - L_uquad_x1))), (void)(ASG(L_uquad_dy, (L_uquad_y2 - L_uquad_y1))), (void)(ASG(L_uquad_l, rt_max(0.0001, rt_sqrt(((L_uquad_dx * L_uquad_dx) + (L_uquad_dy * L_uquad_dy)))))), (void)(ASG(L_uquad_dx, ((L_uquad_dx / L_uquad_l) * (L_uquad_w / 2.0)))), (void)(ASG(L_uquad_dy, ((L_uquad_dy / L_uquad_l) * (L_uquad_w / 2.0)))), gfx_triangle({(L_uquad_x1 - L_uquad_dy), (L_uquad_y1 + L_uquad_dx), (L_uquad_x2 - L_uquad_dy), (L_uquad_y2 + L_uquad_dx), (L_uquad_x2 + L_uquad_dy), (L_uquad_y2 - L_uquad_dx), (L_uquad_x1 + L_uquad_dy), (L_uquad_y1 - L_uquad_dx)}));
}

double Gen::F_urc(double p0, double p1, double p2) {
  L_urc_str = p0; L_urc_rx = p1; L_urc_cy = p2;
  return ((void)(gfx_measurestr(L_urc_str, L_urc_tw, L_urc_th)), (void)(ASG(v_gfx_x, std::floor(((L_urc_rx - L_urc_tw) + 0.5)))), (void)(ASG(v_gfx_y, std::floor(((L_urc_cy - (L_urc_th / 2.0)) + 0.5)))), gfx_drawstr({L_urc_str}));
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

double Gen::F_utc(double p0, double p1, double p2) {
  L_utc_str = p0; L_utc_cx = p1; L_utc_cy = p2;
  return ((void)(gfx_measurestr(L_utc_str, L_utc_tw, L_utc_th)), (void)(ASG(v_gfx_x, std::floor(((L_utc_cx - (L_utc_tw / 2.0)) + 0.5)))), (void)(ASG(v_gfx_y, std::floor(((L_utc_cy - (L_utc_th / 2.0)) + 0.5)))), gfx_drawstr({L_utc_str}));
}

double Gen::F_ux(double p0) {
  L_ux_v = p0;
  return (g_uox + (L_ux_v * g_usc));
}

double Gen::F_uy(double p0) {
  L_uy_v = p0;
  return (L_uy_v * g_usc);
}
}
