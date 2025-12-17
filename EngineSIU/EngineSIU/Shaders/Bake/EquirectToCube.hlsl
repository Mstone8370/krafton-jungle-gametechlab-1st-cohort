
SamplerState SamplerLinearWrap : register(s0);

Texture2D SourceTexture : register(t0);
RWTexture2DArray<float4> OutputCubeMap : register(u0);

static const float PI = 3.14159265359;

float2 SampleSphericalMap(float3 Direction)
{
    float Yaw_Rad = atan2(Direction.y, Direction.x); // [-PI rad, PI rad]
    float Pitch_Rad = -asin(Direction.z); // [ -PI/2 rad, PI/2 rad]
    float2 UV = float2(Yaw_Rad, Pitch_Rad);
    
    UV *= float2(0.1591, 0.3183); // (1/2pi, 1/pi). 라디안 값을 [-0.5, 0.5] 사이로 매핑
    UV += 0.5;
    
    return UV;
}

float3 GetDirection(uint3 DispatchThreadID, float CubeMapWidth, float CubeMapHeight)
{
    const uint FaceIndex = DispatchThreadID.z;
    
    float2 UV = float2(DispatchThreadID.xy) / float2(CubeMapWidth, CubeMapHeight);
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
    //OutputCubeMap[DispatchThreadID.xyz] = float4(Direction, 1.f);
    //OutputCubeMap[DispatchThreadID.xyz] = float4(UV, 0.f, 1.f);
}
