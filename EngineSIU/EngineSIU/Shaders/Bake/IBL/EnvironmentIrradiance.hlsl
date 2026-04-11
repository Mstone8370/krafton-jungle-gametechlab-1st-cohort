
#include "Shaders/ImageBasedLightingCommon.hlsl"

#ifndef NUM_SAMPLES
#define NUM_SAMPLES 4096
#endif

TextureCube SourceTexture : register(t0);
RWTexture2DArray<float4> OutputTexture : register(u0);

cbuffer FIrradianceData : register(b0)
{
    uint SourceResolution; // 원본 큐브맵의 해상도
    
    uint SourceNumMipLevels; // 원본 큐브맵의 밉맵 개수
    
    uint2 FIrradianceData_Padding;
}

float4 IntegrateDiffuseCube(float3 N)
{
    float3 AccumulatedBrdf = 0.0;
    
    for (uint i = 0; i < NUM_SAMPLES; ++i)
    {
        float2 Xi = Hammersley(i, NUM_SAMPLES);
        float3 L = ImportanceSampleCosine(Xi, N);
        
        float NoL = saturate(dot(N, L));
        if (NoL > 0)
        {
            // Filtered Importance Sampling
            float Pdf = NoL / PI;
            
            float OmegaP = 4.0 * PI / (6.0 * SourceResolution * SourceResolution);
            float OmegaS = 1.0 / (NUM_SAMPLES * Pdf + 0.0001);
            
            float MipLevel = clamp(0.5 * log2(OmegaS / OmegaP), 0, SourceNumMipLevels);
            
            float3 SampledColor = SourceTexture.SampleLevel(SamplerLinearClamp, L, MipLevel).rgb;
            SampledColor = SoftClampColor(SampledColor, 10.0, 5.0);
            
            AccumulatedBrdf += SampledColor;
        }
    }
    
    return float4(AccumulatedBrdf / NUM_SAMPLES, 1.0f);
}

float4 IntegrateLambertIrradiance(float3 N)
{
    float3 AccumulatedIrradiance = 0.0;

    for (uint i = 0; i < NUM_SAMPLES; ++i)
    {
        float2 Xi = Hammersley(i, NUM_SAMPLES);
        float3 L = ImportanceSampleCosine(Xi, N);

        float NoL = saturate(dot(N, L));
        if (NoL > 0)
        {
            // Lambert diffuse irradiance can be baked directly from the source cubemap.
            // With cosine-weighted sampling, averaging Li is enough to estimate irradiance.
            float3 SampledColor = SourceTexture.SampleLevel(SamplerLinearClamp, L, 6).rgb;
            SampledColor = SoftClampColor(SampledColor, 10.0, 5.0);
            
            AccumulatedIrradiance += SampledColor;
        }
    }

    return float4(AccumulatedIrradiance / NUM_SAMPLES, 1.0f);
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
    
    float3 N = GetDirection(DispatchThreadID, Width, Height);
    
    //float4 IntegratedDiffuse = IntegrateLambertIrradiance(N);
    float4 IntegratedDiffuse = IntegrateDiffuseCube(N);
    
    OutputTexture[DispatchThreadID] = IntegratedDiffuse;
}
