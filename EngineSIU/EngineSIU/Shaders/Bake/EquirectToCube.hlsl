
#include "Shaders/ImageBasedLightingCommon.hlsl"

Texture2D SourceTexture : register(t0);
RWTexture2DArray<float4> OutputCubeMap : register(u0);

float2 SampleSphericalMap(float3 Direction)
{
    float Phi_Rad = atan2(Direction.y, Direction.x); // [-PI rad, PI rad]
    float Theta_Rad = -asin(Direction.z); // [ -PI/2 rad, PI/2 rad]
    float2 UV = float2(Phi_Rad, Theta_Rad);
    
    UV *= float2(0.1591549, 0.3183099); // (1/2pi, 1/pi). 라디안 값을 [-0.5, 0.5] 사이로 매핑
    UV += 0.5;
    
    return UV;
}

[numthreads(THREADS_X, THREADS_Y, 1)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    uint Width;
    uint Height;
    uint Elements;
    OutputCubeMap.GetDimensions(Width, Height, Elements);
    
    if (DispatchThreadID.x >= Width || DispatchThreadID.y >= Height)
    {
        return;
    }
    
    float3 Direction = GetDirection(DispatchThreadID, Width, Height);
    
    float2 UV = SampleSphericalMap(Direction);
    float3 Color = SourceTexture.SampleLevel(SamplerLinearWrap, UV, 0).rgb;
    
    OutputCubeMap[DispatchThreadID.xyz] = float4(Color, 1.0f);
}
