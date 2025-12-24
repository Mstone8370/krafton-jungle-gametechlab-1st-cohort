
#include "Shaders/ImageBasedLightingCommon.hlsl"

#ifndef NUM_SAMPLES
#define NUM_SAMPLES 2048
#endif

TextureCube SourceTexture : register(t0);
RWTexture2DArray<float4> OutputTexture : register(u0);

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
            
            float OmegaP = 4.0 * PI / (6.0 * 1024 * 1024);
            float OmegaS = 1.0 / (NUM_SAMPLES * Pdf + 0.0001);
            
            // 노이즈 때문에 bias를 0.5에서 0.55로 높임
            float MipLevel = clamp(0.55 * log2(OmegaS / OmegaP), 0, 11);
            
            float3 SampledColor = SourceTexture.SampleLevel(SamplerLinearClamp, L, MipLevel).rgb;
            SampledColor = min(SampledColor, 50.0);
            
            AccumulatedBrdf += SampledColor;
        }
    }
    
    return float4(AccumulatedBrdf / NUM_SAMPLES, 1.0f);
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
    
    float4 IntegratedDiffuse = IntegrateDiffuseCube(N);
    
    OutputTexture[DispatchThreadID] = IntegratedDiffuse;
}
