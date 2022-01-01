#ifndef NOISE_HPP
#define NOISE_HPP

#include "OpenSimplexNoise/OpenSimplexNoise/OpenSimplexNoise.h"
#include "common.hpp"

using Seed = i64;

class OpenSimplexNoiseWParam {
  unique_ptr<OpenSimplexNoise::Noise> osn = nullptr;
  Seed seed = 0;
  float frequency = 0.0f;
  float amplitude = 0.0f;

 public:
  OpenSimplexNoiseWParam() {}

  OpenSimplexNoiseWParam(float _frequency, float _amplitude, float a, float b,
                         Seed _seed)
      : osn(make_unique<OpenSimplexNoise::Noise>(
            OpenSimplexNoise::Noise(_seed))),
        seed(_seed),
        frequency(_frequency),
        amplitude(_amplitude) {}

  inline float noise(u32 octaves, int x, int y) {
    return this->noise(octaves, (float)x, (float)y);
  }

  float noise(u32 octaves, float x, float y) {
    float amp = this->amplitude;
    float res = 0.0f;
    float amp_sum = 0.0f;
    for (int i = 0; i < octaves; ++i) {
      int kk = 2 << i;
      amp_sum += amp;
      res += amp * this->osn->eval(kk * this->frequency * x,
                                   kk * this->frequency * y);
      amp /= 2.0f;
    }
    res /= amp_sum;
    return res;
  }
};

#endif
