#pragma once

#include <common/common.h>

class LUTGenerator
{
public:

    struct LUT
    {
        // param: dot(wo, n), roughness
        ComPtr<ID3D11ShaderResourceView> Emiu;

        // param: roughness
        ComPtr<ID3D11ShaderResourceView> Eavg;
    };
    
    LUT generate(const Int2 &res, int spp) const;
};
