#pragma once
#include "LightComponent.h"


class ULocalLightComponent : public ULightComponent
{
    DECLARE_CLASS(ULocalLightComponent, ULightComponent)

public:
    ULocalLightComponent() = default;
    virtual ~ULocalLightComponent() override = default;
    
};
