
Texture2D SceneTexture : register(t100);
Texture2D TranslucentTexture : register(t101);
Texture2D PP_PostProcessTexture : register(t102);
Texture2D EditorTexture : register(t103);
Texture2D EditorOverlayTexture : register(t104);
Texture2D DebugTexture : register(t106);
Texture2D CameraEffectTexture : register(t107);
Texture2D CameraW13Texture : register(t108);

SamplerState CompositingSampler : register(s1); // Linear Clamp

#define VMI_Lit_Gouraud      0
#define VMI_Lit_Lambert      1
#define VMI_Lit_BlinnPhong   2
#define VMI_Lit_SG           3
#define VMI_Unlit            4
#define VMI_Wireframe        5
#define VMI_SceneDepth       6
#define VMI_WorldNormal      7
#define VMI_WorldTangent     8
#define VMI_LightHeatMap     9

cbuffer ViewMode : register(b0)
{
    uint ViewMode; 
    float3 Padding;
}

cbuffer Gamma : register(b1)
{
    float GammaValue;
    float3 GammaPadding;
}

struct PS_Input
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD;
};

float3 ApplyToneMapping(float3 hdrColor)
{
    // ACES Filmic
    float A = 2.51f;
    float B = 0.03f;
    float C = 2.43f;
    float D = 0.59f;
    float E = 0.14f;
    return saturate((hdrColor * (A * hdrColor + B)) / (hdrColor * (C * hdrColor + D) + E));
}

// Khronos PBR Neutral Tone Mapper
// https://github.com/KhronosGroup/ToneMapping/blob/main/PBR_Neutral/pbrNeutral.glsl
float3 PBRNeutralToneMapping(float3 Color)
{
    const float StartCompression = 0.8f - 0.04f;
    const float Desaturation = 0.15f;

    float X = min(Color.r, min(Color.g, Color.b));
    float Offset = X < 0.08f ? X - 6.25f * X * X : 0.04f;
    Color -= Offset;

    float Peak = max(Color.r, max(Color.g, Color.b));
    if (Peak < StartCompression)
    {
        return Color;
    }

    const float D = 1.0f - StartCompression;
    float NewPeak = 1.0f - D * D / (Peak + D - StartCompression);
    Color *= NewPeak / Peak;

    float G = 1.0f - 1.0f / (Desaturation * (Peak - NewPeak) + 1.0f);
    return lerp(Color, float3(NewPeak, NewPeak, NewPeak), G);
}

float4 main(PS_Input Input) : SV_TARGET
{
    float4 Scene = SceneTexture.Sample(CompositingSampler, Input.UV);
    float4 Translucent = TranslucentTexture.Sample(CompositingSampler, Input.UV);
    float4 PostProcess = PP_PostProcessTexture.Sample(CompositingSampler, Input.UV);
    float4 Editor = EditorTexture.Sample(CompositingSampler, Input.UV);
    float4 EditorOverlay = EditorOverlayTexture.Sample(CompositingSampler, Input.UV);
    float4 Debug = DebugTexture.Sample(CompositingSampler, Input.UV);
    float4 CameraEffect = CameraEffectTexture.Sample(CompositingSampler, Input.UV);
    float4 CameraW13 = CameraW13Texture.Sample(CompositingSampler, Input.UV);
    
    float4 FinalColor = Scene;
    if (ViewMode == VMI_LightHeatMap)
    {
        FinalColor = lerp(FinalColor, Debug, 0.5);
        FinalColor = lerp(FinalColor, Editor, Editor.a);
        FinalColor = lerp(FinalColor, Translucent, Translucent.a);
    }
    else
    {
        FinalColor = lerp(FinalColor, PostProcess, PostProcess.a);
        // TODO: 반투명 물체는 포스트 프로세싱을 어떻게 처리해야하는지 고민해야 함.
        FinalColor = lerp(FinalColor, Translucent, Translucent.a);
        FinalColor = lerp(FinalColor, CameraEffect, CameraEffect.a);
        FinalColor = lerp(FinalColor, CameraW13, CameraW13.a);
        
        // Exposure
        float Exposure = 1.0f;
        FinalColor.rgb *= Exposure;
        
        // Tone mapping
        //FinalColor.rgb = ApplyToneMapping(FinalColor.rgb);
        //FinalColor.rgb = PBRNeutralToneMapping(FinalColor.rgb);
        
        // Gamma Correction
        FinalColor = pow(FinalColor, 1 / GammaValue);
        
        // Editor
        FinalColor = lerp(FinalColor, Editor, Editor.a);
        FinalColor = lerp(FinalColor, EditorOverlay, EditorOverlay.a);
    }

    return FinalColor;
}
