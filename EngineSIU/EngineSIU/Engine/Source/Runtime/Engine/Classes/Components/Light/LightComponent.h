#pragma once
#include "LightComponentBase.h"


class ULightComponent : public ULightComponentBase
{
    DECLARE_CLASS(ULightComponent, ULightComponentBase)

public:
    ULightComponent() = default;
    virtual ~ULightComponent() override = default;

    UPROPERTY(float, AttenuationRadius, = 1000.f)
};
