#pragma once
#include "UnrealClient.h"
#include "Components/SceneComponent.h"

#define NUM_FACES 6

class ULightComponentBase : public USceneComponent
{
    DECLARE_CLASS(ULightComponentBase, USceneComponent)

public:
    ULightComponentBase();
    virtual ~ULightComponentBase() override = default;
    
    virtual UObject* Duplicate(UObject* InOuter) override;

    virtual void UpdateViewMatrix();
    virtual void UpdateProjectionMatrix();

    virtual float GetRadius() const { return 0.f; }
    virtual float GetIntensity() const { return 0.f; }
    virtual bool GetCastShadows() const { return false; }

};
