
#include "Shaders/ImageBasedLightingCommon.hlsl"

#ifndef NUM_SAMPLES
#define NUM_SAMPLES 2048
#endif

cbuffer FPreFilterData : register(b0)
{
    float Roughness;
    
    uint SourceResolution; // 원본 큐브맵의 해상도
    
    uint SourceNumMipLevels; // 원본 큐브맵의 밉맵 개수
    
    int FPreFilterData_Padding;
}

TextureCube SourceTexture : register(t0);
RWTexture2DArray<float4> OutputTexture : register(u0);

float3 PrefilterEnvMap(float Roughness, float3 R)
{
    float3 N = R;
    float3 V = R;
    
    float3 PrefilteredColor = 0.f;
    float TotalWeight = 0.f;
    
    const bool bFullReflection = Roughness < 0.01;
    
    uint NumSamples = bFullReflection ? 1 : NUM_SAMPLES;
    for (uint i = 0; i < NumSamples; ++i)
    {
        float2 Xi = Hammersley(i, NumSamples);
        float3 H = ImportanceSampleGGX(Xi, Roughness, N);
        float3 L = 2 * dot(V, H) * H - V;
        
        float NoL = saturate(dot(N, L));
        if (NoL > 0)
        {
            float NoH = saturate(dot(N, H));
            float VoH = saturate(dot(V, H)); // == LoH
            
            float a = Roughness * Roughness;
            float a2 = a * a;
            
            float Pdf = D_GGX(NoH, a2) * NoH * rcp(4.0 * VoH);
            
            float OmegaS = 1.0 / (NUM_SAMPLES * Pdf + 0.0001);
            float OmegaP = 4.0 * PI / (6.0 * SourceResolution * SourceResolution);
            
            float MipLevel = bFullReflection ? 0.0 : clamp(0.5 * log2(OmegaS / OmegaP), 0, SourceNumMipLevels);
            
            float3 SampledColor = SourceTexture.SampleLevel(SamplerLinearClamp, L, MipLevel).rgb * NoL;
            SampledColor = min(SampledColor, 100.0); // HDR 값의 과도하게 높은 값 때문에 노이즈가 발생하여 적당히 제한.
            
            PrefilteredColor += SampledColor;
            TotalWeight += NoL;
        }
    }
    
    return PrefilteredColor / TotalWeight;
}

[numthreads(THREADS_X, THREADS_Y, 1)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    uint Width;
    uint Height;
    uint Elements;
    OutputTexture.GetDimensions(Width, Height, Elements);
    
    if (DispatchThreadID.x >= Width || DispatchThreadID.y >= Height)
    {
        return;
    }
    
    float3 R = GetDirection(DispatchThreadID, Width, Height);
    
    float3 PrefilteredEnvMap = PrefilterEnvMap(Roughness, R);
    
    OutputTexture[DispatchThreadID.xyz] = float4(PrefilteredEnvMap, 1.f);
}
