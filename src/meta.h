#pragma once
namespace rs {
struct SliderInfo { int idx; double def, lo, hi, step; const char* label; bool hidden; const char* enums; };
extern const char* kDesc;
extern const SliderInfo kSliders[];
extern const int kNumSliders;
}
