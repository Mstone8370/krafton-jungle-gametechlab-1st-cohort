
#ifndef IMAGE_BASED_LIGHTING_COMMON
#define IMAGE_BASED_LIGHTING_COMMON

#include "Shaders/ShaderRegisters.hlsl"
#include "Shaders/BRDF.hlsl"

float3 GetDirection(uint3 DispatchThreadID, float CubeMapWidth, float CubeMapHeight)
{
    const uint FaceIndex = DispatchThreadID.z;
    
    float2 UV = float2(DispatchThreadID.xy + 0.5f) / float2(CubeMapWidth, CubeMapHeight);
    float2 Scan = UV * 2.0f - 1.0f;
    
    float3 Direction;
    switch (FaceIndex)
    {
    case 0: // Right  (DirectX Coord: +X)
        Direction = float3(1.0f, -Scan.y, -Scan.x);
        break;
    case 1: // Left   (DirectX Coord: -X)
        Direction = float3(-1.0f, -Scan.y, Scan.x);
        break;
    case 2: // Top    (DirectX Coord: +Y)
        Direction = float3(Scan.x, 1.0f, Scan.y);
        break;
    case 3: // Bottom (DirectX Coord: -Y)
        Direction = float3(Scan.x, -1.0f, -Scan.y);
        break;
    case 4: // Front  (DirectX Coord: +Z)
        Direction = float3(Scan.x, -Scan.y, 1.0f);
        break;
    case 5: // Back   (DirectX Coord: -Z)
        Direction = float3(-Scan.x, -Scan.y, -1.0f);
        break;
    }
    return normalize(Direction);
}

// 1. 비트 뒤집기 (Van der Corput sequence)
float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

// 2. Hammersley 점 생성 (i: 현재 샘플 인덱스, N: 총 샘플 개수)
float2 Hammersley_Fast(uint i, uint N)
{
    return float2(float(i) / float(N), RadicalInverse_VdC(i)); // float2(Linear, Random)
}

float2 Hammersley(uint k, uint N)
{
    float u = 0;
    float p = 0.5;
    for (int kk = k; kk; kk >>= 1)
    {
        if (kk & 1) // kk mod 2 == 1
        {
            u += p;
        }
        p *= 0.5;
    }
    
    // float v = (k + 0.5) / N; 값 0.5는 논문에서 픽셀의 중앙을 맞추기 위한 오프셋. 
    float v = (k + 0.5) / N;
    return float2(v, u); // float2(Linear, Random)
}

float3 ImportanceSampleGGX(float2 Xi, float Roughness, float3 N)
{
    float a = Roughness * Roughness;
    
    float Phi = 2 * PI * Xi.x;
    float CosTheta = sqrt((1 - Xi.y) / (1 + (a * a - 1) * Xi.y));
    float SinTheta = sqrt(1 - CosTheta * CosTheta);
    
    float3 H;
    H.x = SinTheta * cos(Phi);
    H.y = SinTheta * sin(Phi);
    H.z = CosTheta;
    
    float3 UpVector = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 TangentX = normalize(cross(UpVector, N));
    float3 TangentY = cross(N, TangentX);
    
    // Tangent to world space
    return TangentX * H.x + TangentY * H.y + N * H.z;
}

float3 ImportanceSampleCosine(float2 Xi, float3 N)
{
    float Phi = 2 * PI * Xi.x;
    float CosTheta = sqrt(1.0 - Xi.y);
    float SinTheta = sqrt(Xi.y);
    
    float3 H;
    H.x = SinTheta * cos(Phi);
    H.y = SinTheta * sin(Phi);
    H.z = CosTheta;
    
    float3 UpVector = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 TangentX = normalize(cross(UpVector, N));
    float3 TangentY = cross(N, TangentX);
    
    return TangentX * H.x + TangentY * H.y + N * H.z;
}



float3 SpecularIBL_Reference(float3 SpecularColor, float Roughness, float3 N, float3 V)
{
    float3 SpecularLighting = 0;
    
    const uint NumSamples = 128;
    for (uint i = 0; i < NumSamples; ++i)
    {
        //float2 Xi = Hammersley(i, NumSamples);
        float2 Xi = Hammersley_Fast(i, NumSamples);
        float3 H = ImportanceSampleGGX(Xi, Roughness, N);
        float3 L = 2 * dot(V, H) * H - V;
        
        float NoL = saturate(dot(N, L));
        if (NoL > 0)
        {
            float3 SampleColor = EnvironmentMap.SampleLevel(SamplerLinearClamp, L, 0).rgb;
            SampleColor = min(SampleColor, 5.0);
            
            float alpha = Roughness * Roughness;
            
            float NoV = saturate(dot(N, V));
            float VoH = saturate(dot(V, H));
            float NoH = saturate(dot(N, H));
            
            float G = G_Smith(NoV, NoL, alpha);
            float3 F = F_Schlick(SpecularColor, VoH);
            
            // Incident light = SampleColor * NoL
            // Microfacet specular = D*G*F / (4*NoL*NoV)
            // pdf = D * NoH / (4 * VoH)
            SpecularLighting += SampleColor * F * G * VoH * rcp(NoH * NoV + 0.0001);
        }
    }
    
    return SpecularLighting / NumSamples;
}

float3 SpecularIBL_SplitSumApprox(float3 SpecularColor, float Roughness, float3 N, float3 V)
{
    float NoV = saturate(dot(N, V));
    float3 R = 2 * dot(V, N) * N - V;
    
    float PrefilterLod = Roughness * (9 - 1);
    
    float3 PrefilteredColor = EnvironmentPrefilter.SampleLevel(SamplerLinearClamp, R, PrefilterLod).rgb;
    float2 EnvBRDF = EnvironmentBRDF.SampleLevel(SamplerLinearClamp, float2(NoV, Roughness), 0).rg;
    
    return PrefilteredColor * (SpecularColor * EnvBRDF.x + EnvBRDF.y);
}

#endif
