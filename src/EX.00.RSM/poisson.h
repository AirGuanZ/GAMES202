#pragma once

#include "./common.h"

ComPtr<ID3D11ShaderResourceView> createPoissionDiskSamplesSRV(
    int sampleCount, float radius);
