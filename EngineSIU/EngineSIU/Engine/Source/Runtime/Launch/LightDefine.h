#pragma once
#include "Math/Vector.h"
#include "Math/Vector4.h"
#include "Math/Matrix.h"

// for SoptLight and PointLight
struct FLightData
{
    FLinearColor LightColor = FLinearColor::White;

    FVector Location = FVector::ZeroVector;
    float Radius = 1000.f; // cm

    FVector Direction = FVector::ForwardVector;
    float Intensity = 100000.f;

    FVector UpVector = FVector::UpVector;
    float ShadowBias = 0.005f;

    // x: Outer, y: Inner
    FVector2D SpotRadians = FVector2D(0.2618f, 0.5236f); 
    /**
     * 0: SpotLight (no shadow),   1: PointLight (no shadow),\n
     * 2: SpotLight (cast shadow), 3: PointLight (cast shadow)
     */
    uint32 Type = 0; 
    int32 ShadowMapIndex = -1;

    FMatrix ProjectionMatrix = FMatrix::Identity;
};

#define MAX_AMBIENT_LIGHT 16
#define MAX_DIRECTIONAL_LIGHT 16
#define MAX_POINT_LIGHT 16
#define MAX_SPOT_LIGHT 16

struct FAmbientLightInfo
{
    FLinearColor AmbientColor;         // RGB + alpha
};

struct FDirectionalLightInfo
{
    FLinearColor LightColor;         // RGB + alpha

    FVector Direction;   // 정규화된 광선 방향 (월드 공간 기준)
    float   Intensity;   // 밝기

    // --- Shadow Info ---
    FMatrix LightViewProj; // 섀도우맵 생성 시 사용한 VP 행렬
    FMatrix LightInvProj;  // Light 광원 입장에서의 InvProj

    uint32 ShadowMapArrayIndex = 0 ;//캐스캐이드전 임시 배열
    uint32 CastShadows;
    float ShadowBias;
    float Padding3; // 필요시

    // --- 직교 투영 파라미터 ---
    // 직교 투영 볼륨의 월드 단위 너비 (섀도우 영역)
    float OrthoWidth = 100.0f;

    // 직교 투영 볼륨의 월드 단위 높이 (섀도우 영역)
    float OrthoHeight = 100.0f;

    // 섀도우 계산을 위한 라이트 시점의 Near Plane (음수 가능)
    float ShadowNearPlane = 1.0F;

    // 섀도우 계산을 위한 라이트 시점의 Far Plane
    float ShadowFarPlane = 1000.0f;

};

struct FPointLightInfo
{
    FLinearColor LightColor = FLinearColor::White;         // RGB + alpha

    FVector Position = FVector::ZeroVector;    // 월드 공간 위치
    float   Radius = 0.f;      // 감쇠가 0이 되는 거리

    int32   Type = 0;        // 라이트 타입 구분용 (예: 1 = Point)
    float   Intensity = 0.f;   // 밝기
    float   Attenuation = 0.f;
    float   Padding = 0.f;  // 16바이트 정렬

    // --- Shadow Info ---
    FMatrix LightViewProjs[6]; // 섀도우맵 생성 시 사용한 VP 행렬
    
    uint32 CastShadows = 0;
    float ShadowBias = 0.005f;
    uint32 ShadowMapArrayIndex = 0;
    float Padding2; // 필요시
};

struct FSpotLightInfo
{
    FLinearColor LightColor = FLinearColor::White;         // RGB + alpha

    FVector Position = FVector::ZeroVector;       // 월드 공간 위치
    float   Radius = 0.f;         // 감쇠 거리

    FVector Direction = FVector::ForwardVector;      // 빛이 향하는 방향 (normalize)
    float   Intensity = 0.f;      // 밝기

    int32   Type = 0;           // 라이트 타입 구분용 (예: 2 = Spot)
    float   InnerRad = 0.f; // cos(inner angle)
    float   OuterRad = 0.f; // cos(outer angle)
    float   Attenuation = 0.f;

    // --- Shadow Info ---
    FMatrix LightViewProj; // 섀도우맵 생성 시 사용한 VP 행렬
    
    uint32 CastShadows = 0;
    float ShadowBias = 0.005f;
    uint32 ShadowMapArrayIndex;
    float Padding2; // 필요시
};

struct FSceneLightConstants
{
    FDirectionalLightInfo DirectionalLightInfo;
    FAmbientLightInfo AmbientLightInfo;
    
    int32 DirectionalLightsCount = 0;
    int32 AmbientLightsCount = 0;
    
    int32 TotalActiveLightCount = 0;
    int32 Padding = 0;
};
