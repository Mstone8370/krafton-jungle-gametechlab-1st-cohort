#include "LightComponentBase.h"
#include "UObject/Casts.h"

ULightComponentBase::ULightComponentBase()
{
}

UObject* ULightComponentBase::Duplicate(UObject* InOuter)
{
    ThisClass* NewComponent = Cast<ThisClass>(Super::Duplicate(InOuter));
    return NewComponent;
}

void ULightComponentBase::UpdateViewMatrix()
{
}

void ULightComponentBase::UpdateProjectionMatrix()
{
}

