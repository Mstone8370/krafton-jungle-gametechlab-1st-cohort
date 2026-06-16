#include "SphericalHarmonics.h"

#include <DirectXTex/DirectXTex.h>
#include "Math/MathUtility.h"
#include "Math/Vector.h"

double P(int32 l, int32 m, double X)
{
    double Pmm = 1.0;
    if (m > 0)
    {
        double Somx2 = FMath::Sqrt((1.0 - X) * (1.0 + X));
        double Fact = 1.0;
        for (int32 i = 0; i < m; ++i)
        {
            Pmm *= (-Fact) * Somx2;
            Fact += 2.0;
        }
    }
    
    if (l == m)
    {
        return Pmm;
    }
    
    double Pmmp1 = X * (2.0 * m + 1.0) * Pmm;
    if (l == m + 1)
    {
        return Pmmp1;
    }
    
    double Pll = 0.0;
    for (int32 ll = m + 2; ll <= l; ++ll)
    {
        Pll = ((2.0 * ll - 1.0) * X * Pmmp1 - (ll + m - 1.0) * Pmm) / (ll - m);
        Pmm = Pmmp1;
        Pmmp1 = Pll;
    }
    return Pll;
}

double K(int32 l, int32 m)
{
    double t = ((2.0 * l + 1.0) * FMath::Factorial(l - m)) / (4.0 * PI * FMath::Factorial(l + m));
    return FMath::Sqrt(t);
}

double SH(int32 l, int32 m, double Theta, double Phi)
{
    if (m == 0)
    {
        return K(l, 0) * P(l, m, FMath::Cos(Theta));
    }
    
    const double Sqrt2 = FMath::Sqrt(2.0);
    if (m > 0)
    {
        return Sqrt2 * K(l, m) * FMath::Cos(m * Phi) * P(l, m, FMath::Cos(Theta));
    }
    return Sqrt2 * K(l, -m) * FMath::Sin(-m * Phi) * P(l, -m, FMath::Cos(Theta));
}

FVector SampleColor(const DirectX::Image* Image, uint64 X, uint64 Y)
{
    const uint8_t* RowPtr = Image->pixels + Y * Image->rowPitch;
    const float* PixPtr = reinterpret_cast<const float*>(RowPtr) + X * 4;
    return FVector(PixPtr[0], PixPtr[1], PixPtr[2]);
}


bool FSphericalHarmonics::SphericalHarmonicsFromHDRI(const FWString& FilePath, TArray<FVector>& OutResult)
{
    OutResult.Empty();
    
    DirectX::TexMetadata MetaData;
    DirectX::ScratchImage ScratchImage;
    HRESULT hr = DirectX::LoadFromHDRFile(FilePath.c_str(), &MetaData, ScratchImage);
    if (FAILED(hr))
    {
        return false;
    }
    
    const DirectX::Image* Image = ScratchImage.GetImage(0, 0, 0);
    const uint64 ImageWidth = MetaData.width;
    const uint64 ImageHeight = MetaData.height;
    
    for (size_t Y = 0; Y < ImageHeight; ++Y)
    {
        float* Row = reinterpret_cast<float*>(Image->pixels + Y * Image->rowPitch);
        for (size_t X = 0; X < ImageWidth; ++X)
        {
            float* PixPtr = Row + X * 4;
            const FVector Color(PixPtr[0], PixPtr[1], PixPtr[2]);
            const FVector ColorClamped = FMath::SoftClampMaxChannel(Color, 10.0, 5.f);
            // const FVector ColorClamped = FVector::OneVector;
            
            PixPtr[0] = ColorClamped.X;
            PixPtr[1] = ColorClamped.Y;
            PixPtr[2] = ColorClamped.Z;
        }
    }
    
    constexpr int32 NumBands = 3;
    constexpr int32 NumCoefficients = NumBands * NumBands;
    
    {
        OutResult.SetNum(NumCoefficients);
        
        const double W = static_cast<double>(ImageWidth);
        const double H = static_cast<double>(ImageHeight);
        
        for (uint64 Y = 0; Y < ImageHeight; ++Y)
        {
            const double Theta = (static_cast<double>(Y) + 0.5) / H * PI;
            const double D_Omega = FMath::Sin(Theta) * (PI / H) * (2.0 * PI / W);
            for (uint64 X = 0; X < ImageWidth; ++X)
            {
                const double Phi = (static_cast<double>(X) + 0.5) / W * 2.0 * PI;
                const FVector L = SampleColor(Image, X, Y);
                
                for (int32 l = 0; l < NumBands; ++l)
                {
                    for (int32 m = -l; m <= l; ++m)
                    {
                        const int32 Idx = l * (l + 1) + m;
                        OutResult[Idx] += L * static_cast<float>(SH(l, m, Theta, Phi) * D_Omega);
                    }
                }
            }
        }
    }
    
    {
        TArray<double> AHat_Pi = { 1, 2.0 / 3.0, 1.0 / 4.0 }; // A-hat_l/PI
        for (int32 l = 0; l < NumBands; ++l)
        {
            for (int32 m = -l; m <= l; ++m)
            {
                const int32 Idx = l * (l + 1) + m;
                OutResult[Idx] *= AHat_Pi[l];
            }
        }
    }
    
    return true;
}
