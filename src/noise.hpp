#ifndef NOISE_HPP
#define NOISE_HPP

#include "OpenSimplexNoise/OpenSimplexNoise/OpenSimplexNoise.h"
#include "common.hpp"

class OpenSimplexNoiseWParam {
  OpenSimplexNoise::Noise osn;
  float frequency;
  float amplitude;

 public:
  OpenSimplexNoiseWParam() {}

  OpenSimplexNoiseWParam& operator=(OpenSimplexNoiseWParam op) { return *this; }

  OpenSimplexNoiseWParam(float _frequency, float _amplitude, float a, float b,
                         int seed)
      : osn(seed), frequency(_frequency), amplitude(_amplitude) {}

  float noise(u32 octaves, int x, int y) {
    float k = 1.0f;
    float res = 0.0f;
    for (int i = 0; i < octaves; ++i) {
      int kk = 2 << i;
      res += k *
             this->osn.eval(kk * this->frequency * x, kk * this->frequency * y);
      k /= 2.0f;
    }
    return res;
  }
};

#endif
