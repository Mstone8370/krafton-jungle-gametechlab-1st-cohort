
#define MAX_LIGHTS 16 

#define MAX_POINT_LIGHT 16
#define MAX_SPOT_LIGHT 16

#define SPOT_LIGHT          0
#define POINT_LIGHT         1
#define SPOT_LIGHT_SHADOW   2
#define POINT_LIGHT_SHADOW  3

#define MAX_LIGHT_PER_TILE 1024

#define NEAR_PLANE 0.1
#define LIGHT_RADIUS_WORLD 7

#define MAX_CASCADE_NUM 5

// for SoptLight and PointLight
struct FLightData
{
    float4 LightColor;

    float3 Position;
    float Radius;

    float3 Direction;
    float Intensity;

    float3 UpVector;
    float ShadowBias;

    float2 SpotRadians; // x: Outer, y: Inner
    /*
     * 0: SpotLight (no shadow),   1: PointLight (no shadow),
     * 2: SpotLight (cast shadow), 3: PointLight (cast shadow)
     */
    uint Type; 
    uint ShadowMapIndex;

    matrix ProjectionMatrix;
};

struct FAmbientLightInfo
{
    float4 AmbientColor;
};

struct FDirectionalLightInfo
{
    float4 LightColor;

    float3 Direction;
    float Intensity;
    
    row_major matrix LightViewProj;
    row_major matrix LightInvProj; // 섀도우맵 생성 시 사용한 VP 행렬
    
    uint ShadowMapArrayIndex;//캐스캐이드전 임시 배열
    uint  CastShadows;
    float ShadowBias;
    float Padding3; // 필요시

    float OrthoWidth;
    // 직교 투영 볼륨의 월드 단위 높이 (섀도우 영역)
    float OrthoHeight;
    // 섀도우 계산을 위한 라이트 시점의 Near Plane (음수 가능)
    float ShadowNearPlane;
    // 섀도우 계산을 위한 라이트 시점의 Far Plane
    float ShadowFarPlane;
};

struct FPointLightInfo
{
    float4 LightColor;

    float3 Position;
    float Radius;

    int Type;
    float Intensity;
    float Attenuation;
    float Padding;

    // --- Shadow Info ---
    row_major matrix LightViewProj[6]; // 섀도우맵 생성 시 사용한 VP 행렬
    
    uint CastShadows;
    float ShadowBias;
    uint ShadowMapArrayIndex; // 필요시
    float Padding2; // 필요시
};

struct FSpotLightInfo
{
    float4 LightColor;

    float3 Position;
    float Radius;

    float3 Direction;
    float Intensity;

    int Type;
    float InnerRad;
    float OuterRad;
    float Attenuation;
    
    // --- Shadow Info ---
    row_major matrix LightViewProj; // 섀도우맵 생성 시 사용한 VP 행렬
    
    uint CastShadows;
    float ShadowBias;
    uint ShadowMapArrayIndex; // 필요시
    float Padding2; // 필요시
};

cbuffer FLightInfoBuffer : register(b0)
{
    FDirectionalLightInfo DirectionalLightInfo;
    FAmbientLightInfo AmbientLightInfo;
    
    int DirectionalLightCount;
    int AmbientLightCount;
    
    int TotalActiveLightCount;
    int LightInfoBufferPadding;
};

cbuffer ShadowFlagConstants : register(b5)
{
    uint IsShadow;
    float3 shadowFlagPad0;
}

cbuffer CascadeConstantBuffer : register(b9)
{
    row_major matrix World;
    row_major matrix CascadedViewProj[MAX_CASCADE_NUM];
    row_major matrix CascadedInvViewProj[MAX_CASCADE_NUM];
    row_major matrix CascadedInvProj[MAX_CASCADE_NUM];
    float4 CascadeSplits;
    float2 cascadepad;
};

struct LightPerTiles
{
    uint NumLights;
    uint Indices[MAX_LIGHT_PER_TILE];
    uint Padding[3];
};

// 인덱스별 색 지정 함수
float4 DebugCSMColor(uint idx)
{
    if (idx == 0)
    {
        return float4(1, 0, 0, 1); // 빨강
    }
    if (idx == 1)
    {
        return float4(0, 1, 0, 1); // 초록
    }
    if (idx == 2)
    {
        return float4(0, 0, 1, 1); // 파랑
    }
    return float4(1, 1, 1, 1); // 나머지 – 흰색
}

StructuredBuffer<FLightData> LightData : register(t10);

// Begin Shadow
SamplerComparisonState ShadowSamplerCmp : register(s10);
SamplerState ShadowPointSampler : register(s11);

Texture2DArray SpotShadowMapArray : register(t50);
Texture2DArray DirectionShadowMapArray : register(t51);
TextureCubeArray PointShadowMapArray : register(t52);

uint GetCascadeIndex(float ViewDepth)
{
    // viewDepth 는 LightSpace 깊이(z) 또는 NDC 깊이 복원 뷰 깊이

    for (uint i = 0; i < MAX_CASCADE_NUM; ++i)
    {
        // splits 배열에는 [0]=near, [N]=far 까지 로그 스플릿 저장됨 ex)0..2..4..46..1000
        if (ViewDepth <= CascadeSplits[i + 1])
        {
            return i;
        }
    }
    return MAX_CASCADE_NUM - 1;
}

float CalculateDirectionalShadowFactor(float3 WorldPosition, float3 WorldNormal, FDirectionalLightInfo LightInfo, // 라이트 정보 전체 전달
                                Texture2DArray DirectionShadowMapArray,
                                SamplerComparisonState ShadowSampler)
{
    if (LightInfo.CastShadows == 0)
    {
        return 1.0f;
    }
    
    float ShadowFactor = 1.0;
    float NoL = dot(normalize(WorldNormal), LightInfo.Direction);
    float bias = 0.01f;
    
    // 1. Project World Position to Light Screen Space
    float4 LightScreen = mul(float4(WorldPosition, 1.0f), LightInfo.LightViewProj);
    LightScreen.xyz /= LightScreen.w; // Perspective Divide -> [-1, 1] 범위로 변환

    // 2. 광원 입장의 Texture 좌표계로 변환
    float2 ShadowMapTexCoord = { LightScreen.x, -LightScreen.y }; // NDC 좌표계와 UV 좌표계는 Y축 방향이 반대
    ShadowMapTexCoord += 1.0;
    ShadowMapTexCoord *= 0.5;

    float LightDistance = LightScreen.z;
    LightDistance -= bias;


    float4 posCam = mul(float4(WorldPosition, 1), ViewMatrix);
    float depthCam = posCam.z; 
    uint CsmIndex = GetCascadeIndex(depthCam);
    //CsmIndex = 1;
    float4 posLS = mul(float4(WorldPosition, 1), CascadedViewProj[CsmIndex]);
    float2 uv = posLS.xy * 0.5f + 0.5f;
    uv.y = 1 - uv.y;
    float zReceiverNdc = posLS.z -= bias;
    ShadowFactor = DirectionShadowMapArray.SampleCmpLevelZero(ShadowSamplerCmp, float3(uv, CsmIndex), zReceiverNdc);

    //ShadowFactor = PCSS(uv, zReceiverNdc, DirectionShadowMapArray, CascadedInvViewProj[CsmIndex], LIGHT_RADIUS_WORLD, CsmIndex, LightInfo);
    return ShadowFactor;
}

int GetMajorFaceIndex(float3 Dir)
{
    float3 absDir = abs(Dir);
    if (absDir.x > absDir.y && absDir.x > absDir.z)
    {
        return Dir.x > 0.0f ? 0 : 1;
    }
    if (absDir.y > absDir.z)
    {
        return Dir.y > 0.0f ? 2 : 3;
    }
    return Dir.z > 0.0f ? 4 : 5;
}

float CalculatePointShadowFactor(float3 WorldPosition, FPointLightInfo LightInfo, // 라이트 정보 전체 전달
                                TextureCubeArray ShadowMapArray,
                                SamplerComparisonState ShadowSampler)
{
    // 1) 광원→조각 방향 (큐브맵 샘플링 좌표)
    float3 Dir = normalize(WorldPosition - LightInfo.Position);
    // 2) 해당 face의 뷰·프로젝션 적용
    int face = GetMajorFaceIndex(Dir);
    float4 posCS = mul(float4(WorldPosition, 1.0f), LightInfo.LightViewProj[face]);
    // 3) 클립스페이스 깊이
    float refDepth = posCS.z / posCS.w;
    // 5) 하드웨어 비교 샘플
    float3 SampleDir = normalize(WorldPosition - LightInfo.Position);
    float shadow = ShadowMapArray.SampleCmpLevelZero(ShadowSampler, float4(SampleDir, (float)LightInfo.ShadowMapArrayIndex), refDepth - LightInfo.ShadowBias).r;
    return shadow;
}

// 기본적인 그림자 계산 함수 (Directional/Spot 용)
// 하드웨어 PCF (SamplerComparisonState 사용) 예시
float CalculateSpotShadowFactor(float3 WorldPosition, FSpotLightInfo LightInfo, // 라이트 정보 전체 전달
                                Texture2DArray ShadowMapArray,
                                SamplerComparisonState ShadowSampler)
{
    // if (!LightInfo.CastShadows)
    // {
    //     return 1.0f; // 그림자 안 드리움
    // }

    // 1 & 2. 라이트 클립 공간 좌표 계산
    float4 PixelPosLightClip = mul(float4(WorldPosition, 1.0f), LightInfo.LightViewProj);

    // 3. 섀도우 맵 UV 계산 [0, 1]
    float2 ShadowMapUV = PixelPosLightClip.xy / PixelPosLightClip.w;
    ShadowMapUV = ShadowMapUV * float2(0.5, -0.5) + 0.5; // Y 반전 필요시

    // 4. 현재 깊이 계산
    float CurrentDepth = PixelPosLightClip.z / PixelPosLightClip.w;

    // UV 범위 체크 (라이트 범위 밖)
    if (any(ShadowMapUV < 0.0f) || any(ShadowMapUV > 1.0f))
    {
        return 1.0f;
    }

    // 5 & 6. 배열의 특정 슬라이스 샘플링 및 비교
    float ShadowFactor = ShadowMapArray.SampleCmpLevelZero(
        ShadowSampler,
        float3(ShadowMapUV, (float)LightInfo.ShadowMapArrayIndex), // UV와 배열 인덱스 사용
        CurrentDepth - LightInfo.ShadowBias  // 바이어스 적용
    );

    return ShadowFactor;
}
// End Shadow

float GetDistanceAttenuation(float Distance, float Radius)
{
    float  InvRadius = 1.0 / Radius;
    float  DistSqr = Distance * Distance;
    float  RadiusMask = saturate(1.0 - DistSqr * InvRadius * InvRadius);
    RadiusMask *= RadiusMask;
    
    return RadiusMask / (DistSqr + 1.0);
}

float GetSpotLightAttenuation(float Distance, float Radius, float3 LightDir, float3 SpotDir, float InnerRadius, float OuterRadius)
{
    float DistAtten = GetDistanceAttenuation(Distance, Radius);
    
    float  CosTheta = dot(SpotDir, -LightDir);
    float  SpotMask = saturate((CosTheta - cos(OuterRadius)) / (cos(InnerRadius) - cos(OuterRadius)));
    SpotMask *= SpotMask;
    
    return DistAtten * SpotMask;
}

////////
/// Diffuse
////////
#define PI 3.14159265359

float Pow5(float x)
{
    return x * x * x * x * x;
}

float3 Diffuse_Lambert(float3 DiffuseColor)
{
    return DiffuseColor / PI;
}

// [Burley 2012, "Physically-Based Shading at Disney"]
float3 Diffuse_Burley( float3 DiffuseColor, float Roughness, float NoV, float NoL, float VoH )
{
    float FD90 = 0.5 + 2 * VoH * VoH * Roughness;
    float FdV = 1 + (FD90 - 1) * Pow5( 1 - NoV );
    float FdL = 1 + (FD90 - 1) * Pow5( 1 - NoL );
    return DiffuseColor * ( (1 / PI) * FdV * FdL );
}

// [Portsmouth et al. 2025, "EON: A Practical Energy-Preserving Rough Diffuse BRDF"]
float3 Diffuse_EON( float3 DiffuseColor, float Roughness, float NoV, float NoL, float VoL )
{
    // Albedo inversion for EON model to maintain a consistent color with lambert
    float3 Rho = DiffuseColor * (1.0 + (0.189468 - 0.189468 * DiffuseColor) * Roughness);

    // This is the main shaping term from the Oren-Nayar model (with tweaks by Fujii)
    float S = VoL - NoV * NoL;
    float SOverT = max(S * rcp(max(1e-6, max(NoV, NoL))), S);
    const float constant1_FON = 0.5f - 2.0f / (3.0f * PI);
    // AF = rcp(1 + Roughness * constant1_FON) is nearly a straight line, so approximate it as such
    float AF = 1 - Roughness * (1 - 1 / (1 + constant1_FON));
    float f_ss = AF * (1 + Roughness * SOverT);

    // 4th Order approximation from the paper is a bit too heavy, first order seems to work just as well
    const float g1 = 0.262048f;
    float GoverPi_V = g1 - g1 * NoV;
    // Use (1 - Eo) only as a non-reciprocal approach to energy conservation
    float f_ms = 1.0f - AF * (1 + Roughness * GoverPi_V);
    // The Rho_ms term from the paper can be approximated as just Rho^2
    return Rho * (f_ss + Rho * f_ms) * (1.0 / PI);
}


////////
/// Specular
////////
float3 F_Schlick(float3 F0, float VoH)
{
    float Fc = Pow5(1 - VoH);
    return Fc + (1 - Fc) * F0;
}

float D_GGX(float NoH, float a2)
{
    float d = ( NoH * a2 - NoH ) * NoH + 1;
    return a2 / (PI * d * d);
}

float G_Smith(float NoV, float NoL, float alpha)
{
    float k = alpha * 0.5 + 0.0001;
    float gV = NoV / (NoV * (1.0 - k) + k);
    float gL = NoL / (NoL * (1.0 - k) + k);
    return gV * gL;
}

// [Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"]
float Vis_SmithJoint(float a2, float NoV, float NoL) 
{
    float Vis_SmithV = NoL * sqrt(NoV * (NoV - NoV * a2) + a2);
    float Vis_SmithL = NoV * sqrt(NoL * (NoL - NoL * a2) + a2);
    return 0.5 * rcp(Vis_SmithV + Vis_SmithL);
}

float3 CookTorranceSpecular(float3 F0, float Roughness,
    float NoL, float NoV, float NoH, float VoH)
{
    float alpha = Roughness * Roughness;
    float a2 = alpha * alpha;

    float D = D_GGX(NoH, a2);
    float G = G_Smith(NoV, NoL, alpha);
    float3 F = F_Schlick(F0, VoH);
    
    float Vis = Vis_SmithJoint(a2, NoV, NoL);
    
    return D * Vis * F;
}

float BlinnPhongSpecular(float NoH, float Shininess, float SpecularStrength = 1.0)
{
    return pow(NoH, Shininess) * SpecularStrength;
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
    return float2(float(i) / float(N), RadicalInverse_VdC(i)); // flaot2(Linear, Random)
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


////////
/// BRDF
////////
struct FBRDFResult
{
    float3 DiffuseContribution;
    float3 SpecularContribution;
};

FBRDFResult CalculateBRDF(float3 L, float3 V, float3 N,
#ifdef LIGHTING_MODEL_PBR
    float3 BaseColor, float Metallic, float Roughness
#else
    float3 DiffuseColor, float3 SpecularColor, float Shininess
#endif
)
{
    FBRDFResult Result = (FBRDFResult)0;
    
    float3 H = normalize(V + L);
    float VoL = saturate(dot(V, L));
    float NoL = saturate(dot(N, L));
    float NoV = saturate(dot(N, V));
    float VoH = saturate(dot(V, H));
    float NoH = saturate(dot(N, H));

#ifdef LIGHTING_MODEL_PBR
    float3 F0 = lerp(0.04, BaseColor, Metallic);

    float KdScale = 1.0 - Metallic;
    float3 DiffuseColor = BaseColor * KdScale;
    
    //Result.DiffuseContribution = Diffuse_Lambert(DiffuseColor);
    //Result.DiffuseContribution = Diffuse_Burley(DiffuseColor, Roughness, NoV, NoL, VoH);
    Result.DiffuseContribution = Diffuse_EON(DiffuseColor, Roughness, NoV, NoL, VoL);
    Result.SpecularContribution = CookTorranceSpecular(F0, Roughness, NoL, NoV, NoH, VoH);
#else
    Result.DiffuseContribution = DiffuseColor / PI; // Lambert Diffuse
#ifdef LIGHTING_MODEL_BLINN_PHONG
    Result.SpecularContribution = BlinnPhongSpecular(NoH, Shininess) * SpecularColor;
#endif
#endif
    
    return Result;
}

////////
/// Calculate Light
////////
struct FLightOutput
{
    float3 DiffuseContribution;
    float3 SpecularContribution;
};

FLightOutput PointLight(int Index, float3 WorldPosition, float3 WorldNormal, float3 WorldViewPosition,
#ifdef LIGHTING_MODEL_PBR
    float3 BaseColor, float Metallic, float Roughness
#else
    float3 DiffuseColor, float3 SpecularColor, float Shininess
#endif
)
{
    FLightOutput Output = (FLightOutput)0;
    
    FLightData Data = LightData[Index];
    
    float3 ToLight = Data.Position - WorldPosition;
    float Distance = length(ToLight);
    
    float Attenuation = GetDistanceAttenuation(Distance, Data.Radius);
    if (Attenuation <= 0.0)
    {
        return Output;
    }
    
    // --- 그림자 계산
    float Shadow = 1.0;
    if (Data.Type > 1 && IsShadow)
    {
        // 그림자 계산
        // Shadow = CalculatePointShadowFactor(WorldPosition, LightInfo, PointShadowMapArray, ShadowSamplerCmp);
        // 그림자 계수가 0 이하면 더 이상 계산 불필요
        if (Shadow <= 0.0)
        {
            return Output;
        }
    }
    
    float3 L = normalize(ToLight);
    float NoL = saturate(dot(WorldNormal, L));
    if (NoL <= 0.0)
    {
        return Output;
    }
    
    float3 V = normalize(WorldViewPosition - WorldPosition);

    FBRDFResult BRDF = CalculateBRDF(L, V, WorldNormal,
#ifdef LIGHTING_MODEL_PBR
        BaseColor, Metallic, Roughness
#else
        DiffuseColor, SpecularColor, Shininess
#endif
    );

    float3 LightEnergy = Data.LightColor.rgb * Data.Intensity;

    Output.DiffuseContribution  = BRDF.DiffuseContribution * LightEnergy * Attenuation * Shadow * NoL;
    Output.SpecularContribution = BRDF.SpecularContribution * LightEnergy * Attenuation * Shadow * NoL;

    return Output;
}

FLightOutput SpotLight(int Index, float3 WorldPosition, float3 WorldNormal, float3 WorldViewPosition,
#ifdef LIGHTING_MODEL_PBR
    float3 BaseColor, float Metallic, float Roughness
#else
    float3 DiffuseColor, float3 SpecularColor, float Shininess
#endif
)
{
    FLightOutput Output = (FLightOutput)0;
    
    FLightData Data = LightData[Index];
    
    float3 ToLight = Data.Position - WorldPosition;
    float Distance = length(ToLight);
    float3 LightDir = normalize(ToLight);

    float OuterRad = Data.SpotRadians.x;
    float InnerRad = Data.SpotRadians.y;
    
    float SpotlightFactor = GetSpotLightAttenuation(Distance, Data.Radius, LightDir, normalize(Data.Direction), InnerRad, OuterRad);
    if (SpotlightFactor <= 0.0)
    {
        return Output;
    }

    // --- 그림자 계산
    float Shadow = 1.0;
    if (Data.Type > 1 && IsShadow)
    {
        // 그림자 계산
        // Shadow = CalculateSpotShadowFactor(WorldPosition, Data, SpotShadowMapArray, ShadowSamplerCmp);
        // 그림자 계수가 0 이하면 더 이상 계산 불필요
        if (Shadow <= 0.0)
        {
            return Output;
        }
    }

    float3 L = normalize(ToLight);
    float NoL = saturate(dot(WorldNormal, L));
    if (NoL <= 0.0)
    {
        return Output;
    }
    
    float3 V = normalize(WorldViewPosition - WorldPosition);
    
    FBRDFResult BRDF = CalculateBRDF(L, V, WorldNormal,
#ifdef LIGHTING_MODEL_PBR
        BaseColor, Metallic, Roughness
#else
        DiffuseColor, SpecularColor, Shininess
#endif
    );
    
    float3 LightEnergy = Data.LightColor.rgb * Data.Intensity;

    Output.DiffuseContribution  = BRDF.DiffuseContribution * LightEnergy * SpotlightFactor * Shadow * NoL;
    Output.SpecularContribution = BRDF.SpecularContribution * LightEnergy * SpotlightFactor * Shadow * NoL;

    return Output;
}

FLightOutput DirectionalLight(float3 WorldPosition, float3 WorldNormal, float3 WorldViewPosition,
#ifdef LIGHTING_MODEL_PBR
    float3 BaseColor, float Metallic, float Roughness
#else
    float3 DiffuseColor, float3 SpecularColor, float Shininess
#endif
)
{
    FLightOutput Output = (FLightOutput)0;
    
    FDirectionalLightInfo LightInfo = DirectionalLightInfo;
    
    float4 posCam = mul(float4(WorldPosition, 1), ViewMatrix);
    float depthCam = posCam.z / posCam.w;
    uint csmIndex = GetCascadeIndex(depthCam); // 시각적 디버깅용
    
    // --- 그림자 계산
    float Shadow = 1.0;
    if (IsShadow)
    {
        Shadow = CalculateDirectionalShadowFactor(WorldPosition, WorldNormal, LightInfo, DirectionShadowMapArray, ShadowSamplerCmp);
        // 그림자 계수가 0 이하면 더 이상 계산 불필요
        if (Shadow <= 0.0)
        {
            return Output;
        }
    }

    float3 L = normalize(-LightInfo.Direction);
    float NoL = saturate(dot(WorldNormal, L));
    if (NoL <= 0.0)
    {
        return Output;
    }
    
    float3 V = normalize(WorldViewPosition - WorldPosition);
    
    FBRDFResult BRDF = CalculateBRDF(L, V, WorldNormal,
#ifdef LIGHTING_MODEL_PBR
        BaseColor, Metallic, Roughness
#else
        DiffuseColor, SpecularColor, Shininess
#endif
    );

    float3 LightEnergy = LightInfo.LightColor.rgb * LightInfo.Intensity;

    Output.DiffuseContribution  = BRDF.DiffuseContribution * LightEnergy * Shadow * NoL /* DebugCSMColor(csmIndex) */;
    Output.SpecularContribution = BRDF.SpecularContribution * LightEnergy * Shadow * NoL;

    return Output;
}


float GetFinalAlpha(float BaseAlpha, float SpecularLuminance, float3 V, float3 N,
#ifdef LIGHTING_MODEL_PBR
    float3 BaseColor, float Metallic
#else
    float3 SpecularColor
#endif
)
{
    float3 F0_View = float3(0.0, 0.0, 0.0);
#ifdef LIGHTING_MODEL_PBR
    F0_View = lerp(0.04, BaseColor, Metallic);
#else
    F0_View = SpecularColor; // Phong에서는 Specular 색상을 F0로 사용하거나 고정값 사용. 또는 float3(0.04, 0.04, 0.04);
#endif
    float ViewAngleFresnel = F_Schlick(F0_View, saturate(dot(N, V))).r;

    float HighlightOpacityContribution = saturate(SpecularLuminance);
    float TotalReflectanceInfluence = max(ViewAngleFresnel, HighlightOpacityContribution);

    float FinalAlpha = lerp(BaseAlpha, 1.0f, TotalReflectanceInfluence);
    
    return saturate(FinalAlpha); // [0, 1]
}


float4 Lighting(float3 WorldPosition, float3 WorldNormal, float3 WorldViewPosition,
#ifdef LIGHTING_MODEL_PBR
    float3 BaseColor, float Metallic, float Roughness,
#else
    float3 DiffuseColor, float3 SpecularColor, float Shininess,
#endif
    float BaseAlpha,
    uint TileIndex
)
{
    /**
     * RGB 색상 값을 인간이 인지하는 밝기로 변환하기 위한 가중치.
     * Rec.709(ITU-R BT.709) 표준.
     */
    /*
    float3 LUMINANCE = float3(0.299, 0.587, 0.114);
    
    float3 AccumulatedDiffuseColor = float3(0.0, 0.0, 0.0);
    float3 AccumulatedSpecularColor = float3(0.0, 0.0, 0.0);
    
    // 광원으로부터 계산된 스페큘러 값들 중 최대값
    float MaxObservedSpecularLuminance = 0.0f;

    
    // 조명 계산
    int BucketsPerTile = MAX_LIGHT_PER_TILE / 32;
    int StartIndex = TileIndex * BucketsPerTile;
    for (int Bucket = 0; Bucket < BucketsPerTile; ++Bucket)
    {
        int PointMask = PerTilePointLightIndexBuffer[StartIndex + Bucket];
        int SpotMask = PerTileSpotLightIndexBuffer[StartIndex + Bucket];
        for (int bit = 0; bit < 32; ++bit)
        {
            if (PointMask & (1u << bit))
            {
                // 전역 조명 인덱스는 bucket * 32 + bit 로 계산됨.
                // 전역 조명 인덱스가 총 조명 수보다 작은 경우에만 추가
                int GlobalPointLightIndex = Bucket * 32 + bit;
                if (GlobalPointLightIndex < MAX_LIGHT_PER_TILE) // TODO: MAX_LIGHT_PER_TILE 대신 실제 포인트 라이트 개수와 비교해야 함. (중요)
                {
                    FLightOutput Result = PointLight(
                        GlobalPointLightIndex, WorldPosition, WorldNormal, WorldViewPosition,
#ifdef LIGHTING_MODEL_PBR
                        BaseColor, Metallic, Roughness
#else
                        DiffuseColor, SpecularColor, Shininess
#endif
                    );
                    AccumulatedDiffuseColor += Result.DiffuseContribution;
                    AccumulatedSpecularColor += Result.SpecularContribution;
                    
                    MaxObservedSpecularLuminance = max(MaxObservedSpecularLuminance, dot(Result.SpecularContribution, LUMINANCE));
                }
            }
            if (SpotMask & (1u << bit))
            {
                int GlobalSpotLightIndex = Bucket * 32 + bit;
                if (GlobalSpotLightIndex < MAX_LIGHT_PER_TILE) // TODO: MAX_LIGHT_PER_TILE 대신 실제 스팟 라이트 개수와 비교해야 함. (중요)
                {
                    FLightOutput Result = SpotLight(
                        GlobalSpotLightIndex, WorldPosition, WorldNormal, WorldViewPosition,
#ifdef LIGHTING_MODEL_PBR
                        BaseColor, Metallic, Roughness
#else
                        DiffuseColor, SpecularColor, Shininess
#endif
                    );
                    AccumulatedDiffuseColor += Result.DiffuseContribution;
                    AccumulatedSpecularColor += Result.SpecularContribution;
                    
                    MaxObservedSpecularLuminance = max(MaxObservedSpecularLuminance, dot(Result.SpecularContribution, LUMINANCE));
                }
            }
        }
    }
    
    [unroll(MAX_DIRECTIONAL_LIGHT)]
    for (int k = 0; k < 1; k++) // TODO: 그림자는 0번 인덱스만 사용하더라도 실제 디렉셔널 라이트 개수만큼 루프해야함. (중요)
    {
         FLightOutput Result = DirectionalLight(
            k, WorldPosition, WorldNormal, WorldViewPosition,
#ifdef LIGHTING_MODEL_PBR
            BaseColor, Metallic, Roughness
#else
            DiffuseColor, SpecularColor, Shininess
#endif
        );
        AccumulatedDiffuseColor += Result.DiffuseContribution;
        AccumulatedSpecularColor += Result.SpecularContribution;

        MaxObservedSpecularLuminance = max(MaxObservedSpecularLuminance, dot(Result.SpecularContribution, LUMINANCE));
    }
    
    
    // 앰비언트
#ifdef LIGHTING_MODEL_PBR
    float3 IBL_DiffuseColor = float3(0.01, 0.01, 0.01); // TODO: 임시 값으로, 추후 IBL 적용

    if (AmbientLightsCount > 0)
    {
        IBL_DiffuseColor = AmbientLightInfo.AmbientColor.rgb;
    }
    AccumulatedDiffuseColor += BaseColor * (1.0 - Metallic) * IBL_DiffuseColor;
#else
    float3 AmbientLightColor = float3(0.01, 0.01, 0.01);
    
    if (AmbientLightsCount > 0)
    {
        AmbientLightColor = AmbientLightInfo.AmbientColor.rgb;
    }
    AccumulatedDiffuseColor += DiffuseColor * AmbientLightColor;
#endif

    float3 FinalRGB = AccumulatedDiffuseColor + AccumulatedSpecularColor;

    
    // 알파
    float3 V = normalize(WorldViewPosition - WorldPosition);
    float FinalAlpha = GetFinalAlpha(BaseAlpha, MaxObservedSpecularLuminance, V, WorldNormal,
#ifdef LIGHTING_MODEL_PBR
        BaseColor, Metallic
#else
        SpecularColor
#endif
    );

    return float4(FinalRGB, FinalAlpha);
    */
    return float4(0.0, 0.0, 0.0, 0.0);
}


float4 Lighting(float3 WorldPosition, float3 WorldNormal, float3 WorldViewPosition,
#ifdef LIGHTING_MODEL_PBR
    float3 BaseColor, float Metallic, float Roughness,
#else
    float3 DiffuseColor, float3 SpecularColor, float Shininess,
#endif
    float BaseAlpha
)
{
    /**
     * RGB 색상 값을 인간이 인지하는 밝기로 변환하기 위한 가중치.
     * Rec.709(ITU-R BT.709) 표준.
     */
    float3 LUMINANCE = float3(0.299, 0.587, 0.114);
    
    float3 AccumulatedDiffuseColor = float3(0.0, 0.0, 0.0);
    float3 AccumulatedSpecularColor = float3(0.0, 0.0, 0.0);
    
    // 광원으로부터 계산된 스페큘러 값들 중 최대값
    float MaxObservedSpecularLuminance = 0.0f;
    
/*
    // 조명 계산
    // 다소 비효율적일 수도 있음.
    [unroll(MAX_POINT_LIGHT)]
    for (int i = 0; i < PointLightsCount; i++)
    {
        FLightOutput Result = PointLight(
            i, WorldPosition, WorldNormal, WorldViewPosition,
#ifdef LIGHTING_MODEL_PBR
            BaseColor, Metallic, Roughness
#else
            DiffuseColor, SpecularColor, Shininess
#endif
        );
        AccumulatedDiffuseColor += Result.DiffuseContribution;
        AccumulatedSpecularColor += Result.SpecularContribution;

        MaxObservedSpecularLuminance = max(MaxObservedSpecularLuminance, dot(Result.SpecularContribution, LUMINANCE));
    }

    [unroll(MAX_SPOT_LIGHT)]
    for (int j = 0; j < SpotLightsCount; j++)
    {
        FLightOutput Result = SpotLight(
            j, WorldPosition, WorldNormal, WorldViewPosition,
#ifdef LIGHTING_MODEL_PBR
            BaseColor, Metallic, Roughness
#else
            DiffuseColor, SpecularColor, Shininess
#endif
        );
        AccumulatedDiffuseColor += Result.DiffuseContribution;
        AccumulatedSpecularColor += Result.SpecularContribution;

        MaxObservedSpecularLuminance = max(MaxObservedSpecularLuminance, dot(Result.SpecularContribution, LUMINANCE));
    }
    
    [unroll(MAX_DIRECTIONAL_LIGHT)]
    for (int k = 0; k < 1; k++) 
    {
        FLightOutput Result = DirectionalLight(
            k, WorldPosition, WorldNormal, WorldViewPosition,
#ifdef LIGHTING_MODEL_PBR
            BaseColor, Metallic, Roughness
#else
            DiffuseColor, SpecularColor, Shininess
#endif
        );
        AccumulatedDiffuseColor += Result.DiffuseContribution;
        AccumulatedSpecularColor += Result.SpecularContribution;

        MaxObservedSpecularLuminance = max(MaxObservedSpecularLuminance, dot(Result.SpecularContribution, LUMINANCE));
    }
*/

    for (int i = 0; i < TotalActiveLightCount; i++)
    {
        FLightOutput Result = (FLightOutput)0;
        if (LightData[i].Type == 0 || LightData[i].Type == 2) // SpotLight
        {
            Result = SpotLight(
                i, WorldPosition, WorldNormal, WorldViewPosition,
#ifdef LIGHTING_MODEL_PBR
                BaseColor, Metallic, Roughness
#else
                DiffuseColor, SpecularColor, Shininess
#endif
            );
        }
        else // PointLight
        {
            Result = PointLight(
                i, WorldPosition, WorldNormal, WorldViewPosition,
#ifdef LIGHTING_MODEL_PBR
                BaseColor, Metallic, Roughness
#else
                DiffuseColor, SpecularColor, Shininess
#endif
            );
        }
        AccumulatedDiffuseColor += Result.DiffuseContribution;
        AccumulatedSpecularColor += Result.SpecularContribution;

        MaxObservedSpecularLuminance = max(MaxObservedSpecularLuminance, dot(Result.SpecularContribution, LUMINANCE));
    }

    
    if (DirectionalLightCount > 0)
    {
        FLightOutput Result = DirectionalLight(
            WorldPosition, WorldNormal, WorldViewPosition,
#ifdef LIGHTING_MODEL_PBR
            BaseColor, Metallic, Roughness
#else
            DiffuseColor, SpecularColor, Shininess
#endif
        );
        AccumulatedDiffuseColor += Result.DiffuseContribution;
        AccumulatedSpecularColor += Result.SpecularContribution;

        MaxObservedSpecularLuminance = max(MaxObservedSpecularLuminance, dot(Result.SpecularContribution, LUMINANCE));
    }

    
    // 앰비언트
#ifdef LIGHTING_MODEL_PBR
    float3 IBL_DiffuseColor = 0; // TODO: 임시 값으로, 추후 IBL 적용

    if (AmbientLightCount > 0)
    {
        IBL_DiffuseColor = AmbientLightInfo.AmbientColor.rgb;
    }
    AccumulatedDiffuseColor += BaseColor * (1.0 - Metallic) * IBL_DiffuseColor;
#else
    float3 AmbientLightColor = float3(0.01, 0.01, 0.01);
    
    if (AmbientLightCount > 0)
    {
        AmbientLightColor = AmbientLightInfo.AmbientColor.rgb;
    }
    AccumulatedDiffuseColor += DiffuseColor * AmbientLightColor;
#endif

    float3 FinalRGB = AccumulatedDiffuseColor + AccumulatedSpecularColor;

    
    // 알파
    float3 V = normalize(WorldViewPosition - WorldPosition);
    float FinalAlpha = GetFinalAlpha(BaseAlpha, MaxObservedSpecularLuminance, V, WorldNormal,
#ifdef LIGHTING_MODEL_PBR
        BaseColor, Metallic
#else
        SpecularColor
#endif
    );

    
    return float4(FinalRGB, FinalAlpha);
}
