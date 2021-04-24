#pragma once

#include <common/common.h>

std::vector<Float3> computeEnvSHCoefs(
    const agz::texture::texture2d_t<Float3> &env,
    int                                      maxOrder,
    int                                      numSamples);

void rotateEnvSHCoefs(
    const agz::math::mat3f_c &rot, std::vector<Float3> &coefs);
