#pragma once

#include <common/common.h>

using Frame = agz::math::tcoord3<float>;

enum class LightingMode
{
    NoShadow,
    Shadow,
    InterRefl
};
