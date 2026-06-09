
#include "Shaders/BRDF.hlsl"
#include "Shaders/ImageBasedLightingCommon.hlsl"

#ifndef NUM_SAMPLES
#define NUM_SAMPLES 1024
#endif

RWTexture2D<float2> OutputTexture : register(u0);

float2 IntegrateBRDF(float Roughness, float NoV)
{
    float3 V;
    V.x = sqrt(1.0f - NoV * NoV); // sin
    V.y = 0.0f;
    V.z = NoV; // cos
    
    float A = 0;
    float B = 0;
    
    float3 N = float3(0, 0, 1);
    
    for (uint i = 0; i < NUM_SAMPLES; ++i)
    {
        float2 Xi = Hammersley_Fast(i, NUM_SAMPLES);
        float3 H = ImportanceSampleGGX(Xi, Roughness, N);
        float3 L = 2 * dot(V, H) * H - V;
        
        float NoL = saturate(L.z);
        if (NoL > 0)
        {
            float NoH = saturate(H.z);
            float VoH = saturate(dot(V, H));
            
            float alpha = Roughness * Roughness;
            float a2 = alpha * alpha;
            
            float Vis = Vis_SmithJoint(a2, NoV, NoL);
            
            // 직접광에 사용하는 SmithJoint에 맞게 수정
            // G_Vis = G * VoH / (NoH * NoV)
            float V_Vis = Vis * 4.0 * VoH * NoL / (NoH + 1e-4);
            
            float Fc = Pow5(1 - VoH);
            A += (1 - Fc) * V_Vis;
            B += Fc * V_Vis;
        }
    }
    
    return float2(A, B) / NUM_SAMPLES;
}

[numthreads(THREADS_X, THREADS_Y, 1)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    uint Width;
    uint Height;
    OutputTexture.GetDimensions(Width, Height);
    
    if (DispatchThreadID.x >= Width || DispatchThreadID.y >= Height)
    {
        return;
    }
    
    float NoV = (float(DispatchThreadID.x) + 0.5) / float(Width);
    float Roughness = (float(DispatchThreadID.y) + 0.5) / float(Height);
    
    float2 IntegratedBRDF = IntegrateBRDF(Roughness, NoV);
    
    OutputTexture[DispatchThreadID.xy] = IntegratedBRDF;
}
