cbuffer VSTransform
{
    float4x4 WVP;
}

cbuffer VSEnvSH
{
    int SHCount;
    float3 VSEnvSHPad0;
    float4 EnvSH[25];
}

Buffer<float> VertexSHCoefs;

struct VSInput
{
    float3 position : POSITION;
    uint   vertexID : SV_VertexID;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 color    : COLOR;
};

VSOutput VSMain(VSInput input)
{
    float3 color = float3(0, 0, 0);
    int coefBase = SHCount * input.vertexID;
    for(int i = 0; i < SHCount; ++i)
    {
        float vtxCoef  = VertexSHCoefs[coefBase + i];
        float3 envCoef = EnvSH[i].rgb;
        color += vtxCoef * envCoef;
    }

    VSOutput output;
    output.position = mul(float4(input.position, 1), WVP);
    output.color    = color;

    return output;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    return float4(pow(input.color, 1 / 2.2), 1);
}
