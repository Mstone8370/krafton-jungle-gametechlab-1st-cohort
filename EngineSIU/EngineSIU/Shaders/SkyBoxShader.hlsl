
#include "ShaderRegisters.hlsl"

TextureCube SkyBoxTexture : register(t0);

static const float3 CubeVertices[8] = { // from .obj file
    float3(-1.000000, -1.000000, 1.000000),
    float3(-1.000000, 1.000000, 1.000000),
    float3(-1.000000, -1.000000, -1.000000),
    float3(-1.000000, 1.000000, -1.000000),
    float3(1.000000, -1.000000, 1.000000),
    float3(1.000000, 1.000000, 1.000000),
    float3(1.000000, -1.000000, -1.000000),
    float3(1.000000, 1.000000, -1.000000),
};

static const int CubeIndices[36] = { // from .obj file
    2, 3, 1,
    4, 7, 3,
    8, 5, 7,
    6, 1, 5,
    7, 1, 3,
    4, 6, 8,
    2, 4, 3,
    4, 8, 7,
    8, 6, 5,
    6, 2, 1,
    7, 5, 1,
    4, 2, 6,
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float3 UV : TEXCOORD0;
};

VS_OUTPUT mainVS(uint VertexID : SV_VertexID)
{
    VS_OUTPUT Output = (VS_OUTPUT)0;
    
    float3 LocalPosition = CubeVertices[CubeIndices[VertexID] - 1]; // .obj 파일의 인덱스는 1에서 시작하므로 1 감소시킴
    
    Output.Position = float4(LocalPosition, 0.0f);
    Output.Position = mul(Output.Position, ViewMatrix);
    Output.Position = mul(Output.Position, ProjectionMatrix);
    
    Output.Position.z = Output.Position.w; // 핵심: Perspective Divide(Z/W) 후 깊이값이 항상 1.0이 되도록 함
    
    Output.UV = LocalPosition;
    
    return Output;
}

float4 mainPS(VS_OUTPUT Input) : SV_Target
{
    return SkyBoxTexture.Sample(SamplerPointClamp, normalize(Input.UV));
}
