#pragma once
#include "Container/Array.h"
#include "GameFramework/AIController.h"

struct FSphericalHarmonics
{
    static bool SphericalHarmonicsFromHDRI(const FWString& FilePath, TArray<FVector>& OutResult); 
    
};
