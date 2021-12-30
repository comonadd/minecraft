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

  float noise(u32 octaves, float x, float y) {
    float k = 1.0f;
    float res = 0.0f;
    for (int i = 0; i < octaves; ++i) {
      int kk = 2 << i;
      res += k * this->osn->eval(kk * this->frequency * x,
                                 kk * this->frequency * y);
      k /= 2.0f;
    }
    return res;
  }
};

#endif