#pragma once
#include "LightComponent.h"
#include "UObject/ObjectMacros.h"

class UDirectionalLightComponent : public ULightComponentBase
{
    DECLARE_CLASS(UDirectionalLightComponent, ULightComponentBase)

public:
    UDirectionalLightComponent();
    virtual ~UDirectionalLightComponent() override = default;

    virtual UObject* Duplicate(UObject* InOuter) override;
    
    virtual void GetProperties(TMap<FString, FString>& OutProperties) const override;
    virtual void SetProperties(const TMap<FString, FString>& InProperties) override;
    FVector GetDirection();
    float GetShadowNearPlane() const;

    const FDirectionalLightInfo& GetDirectionalLightInfo() const;
    void SetDirectionalLightInfo(const FDirectionalLightInfo& InDirectionalLightInfo);

    virtual float GetIntensity() const override;
    void SetIntensity(float InIntensity);

    virtual bool GetCastShadows() const override { return DirectionalLightInfo.CastShadows; }
    void SetCastShadows(bool InCastShadows) { DirectionalLightInfo.CastShadows = InCastShadows; }

    FLinearColor GetLightColor() const;
    void SetLightColor(const FLinearColor& InColor);

    void UpdateViewMatrix(FVector TargetPosition);
    void UpdateViewMatrix() override;
    void UpdateProjectionMatrix() override;
    float GetShadowFrustumWidth() const;

private:
    FDirectionalLightInfo DirectionalLightInfo;


};

