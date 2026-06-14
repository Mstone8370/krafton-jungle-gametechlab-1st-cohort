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

struct FSHSample
{
    double Theta;
    double Phi;
    FVector Direction;
    TArray<double> Coefficient;
};

static double Random()
{
    static std::mt19937 rng(1234);
    static std::uniform_real_distribution<double> d(0.0, 1.0);
    return d(rng);
}

TArray<FSHSample> SetupSphericalSamples_Jittered(int32 SqrtN, int32 NumBands)
{
    TArray<FSHSample> Samples;
    Samples.Empty();
    Samples.Reserve(SqrtN * SqrtN);
    
    const int32 NumCoefficients = NumBands * NumBands;
    const double Inv = 1.0 / SqrtN;
    
    for (int32 A = 0; A < SqrtN; ++A)
    {
        for (int32 B = 0; B < SqrtN; ++B)
        {
            const double X = (A + Random()) * Inv;
            const double Y = (B + Random()) * Inv;
            
            const double Theta = 2.0 * FMath::Acos(FMath::Sqrt(1.0 - X));
            const double Phi = 2.0 * PI * Y;
            
            FSHSample Sample;
            Sample.Theta = Theta;
            Sample.Phi = Phi;
            Sample.Coefficient.SetNum(NumCoefficients);
            Sample.Direction = FVector(
                FMath::Sin(Theta) * FMath::Cos(Phi),
                FMath::Sin(Theta) * FMath::Sin(Phi),
                FMath::Cos(Theta)
            );
            
            for (int32 l = 0; l < NumBands; ++l)
            {
                for (int32 m = -l; m <= l; ++m)
                {
                    const int32 Idx = l * (l + 1) + m;
                    Sample.Coefficient[Idx] = SH(l, m, Theta, Phi);
                }
            }
            
            Samples.Emplace(Sample);
        }
    }
    
    return Samples;
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
            
            PixPtr[0] = ColorClamped.X;
            PixPtr[1] = ColorClamped.Y;
            PixPtr[2] = ColorClamped.Z;
        }
    }
    
    constexpr int32 NumBands = 3;
    constexpr int32 NumCoefficients = NumBands * NumBands;
    
    const TArray<FSHSample> Samples = SetupSphericalSamples_Jittered(100, NumBands);
    
    /*
    // 결과 검증    
    TArray<TArray<double>> Coefficients(NumCoefficients, TArray<double>(9, 0.0));
    for (const FSHSample& Sample : Samples)
    {
        for (int32 i = 0; i < NumCoefficients; ++i)
        {
            for (int32 j = 0; j < NumCoefficients; ++j)
            {
                Coefficients[i][j] += Sample.Coefficient[i] * Sample.Coefficient[j];
            }
        }
    }
    
    for (int32 i = 0; i < NumCoefficients; ++i)
    {
        for (int32 j = 0; j < NumCoefficients; ++j)
        {
            std::cout << Coefficients[i][j] * 4 * PI / static_cast<double>(Samples.Num()) << ", ";
        }
        std::cout << std::endl;
    }
    */
    
    {
        OutResult.SetNum(NumCoefficients);
    
        for (const FSHSample& Sample : Samples)
        {
            const double U = Sample.Phi / (2.0 * PI);
            const double V = Sample.Theta / PI;
        
            const int32 SampleX = U * ImageWidth;
            const int32 SampleY = V * ImageHeight;
            
            const FVector SampledColor = SampleColor(Image, SampleX, SampleY);
            for (int32 i = 0; i < NumCoefficients; ++i)
            {
                OutResult[i] += SampledColor * Sample.Coefficient[i];
            }
        }
    
        for (FVector& Result : OutResult)
        {
            Result *= (4 * PI / static_cast<double>(Samples.Num()));
        }
    }
    
    {
        int32 Band = 0;
        TArray<double> AHat_Pi = {1, 2.0 / 3.0, 1.0/ 4.0}; // A-hat_l/PI
        for (int32 i = 0; i < NumCoefficients; ++i)
        {
            const int32 NextBand = (Band + 1);
            const bool bShouldIncreaseBand = (i >= (NextBand * NextBand));
            if (bShouldIncreaseBand)
            {
                ++Band;
            }
            
            OutResult[i] *= AHat_Pi[Band];
        }
    }
    
    return true;
}
