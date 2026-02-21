
#ifndef BRDF_H
#define BRDF_H

#define PI 3.14159265359

float Pow5(float x)
{
    return x * x * x * x * x;
}


////////
/// Specular
////////
float3 F_Schlick(float3 F0, float VoH)
{
    float Fc = Pow5(1 - VoH);
    return Fc + (1 - Fc) * F0;
}

float3 F_Schlick(float3 F0, float3 F90, float VoH)
{
    float Fc = Pow5(1 - VoH);
    return F90 * Fc + (1 - Fc) * F0;
}

float3 F_SchlickRoughness(float3 F0, float CosTheta, float Roughness)
{
    return F0 + (max(float3(1.0 - Roughness, 1.0 - Roughness, 1.0 - Roughness), F0) - F0) * Pow5(1.0 - CosTheta);
}

float D_GGX(float NoH, float a2)
{
    float d = ( NoH * a2 - NoH ) * NoH + 1;
    return a2 / (PI * d * d);
}

// Smith 기법 적용된 Schlick 근사
float G_Schlick(float a2, float NoV, float NoL)
{
    float k = a2 * 0.5;
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
    float3 F = F_Schlick(F0, VoH);
    
    float Vis = Vis_SmithJoint(a2, NoV, NoL);
    
    return D * Vis * F;
}

float BlinnPhongSpecular(float NoH, float Shininess, float SpecularStrength = 1.0)
{
    return pow(NoH, Shininess) * SpecularStrength;
}


////////
/// Diffuse
////////
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

float3 Diffuse_Burley_2( float3 DiffuseColor, float Roughness, float NoV, float NoL, float VoH )
{
    float F90 = 0.5 + 2 * VoH * VoH * Roughness;
    float LightScatter = F_Schlick(1.0, F90, NoL);
    float ViewScatter = F_Schlick(1.0, F90, NoV);
    return DiffuseColor * LightScatter * ViewScatter * (1.0 / PI);
}

// [Gotanda 2012, "Beyond a Simple Physically Based Blinn-Phong Model in Real-Time"]
float3 Diffuse_OrenNayar( float3 DiffuseColor, float Roughness, float NoV, float NoL, float VoH )
{
    float a = Roughness * Roughness;
    float s = a;// / ( 1.29 + 0.5 * a );
    float s2 = s * s;
    float VoL = 2 * VoH * VoH - 1;		// double angle identity
    float Cosri = VoL - NoV * NoL;
    float C1 = 1 - 0.5 * s2 / (s2 + 0.33);
    float C2 = 0.45 * s2 / (s2 + 0.09) * Cosri * ( Cosri >= 0 ? rcp( max( NoL, NoV ) ) : 1 );
    return DiffuseColor / PI * ( C1 + C2 ) * ( 1 + Roughness * 0.5 );
}

// [ Chan 2024, "Multiscattering Diffuse and Specular BRDFs", Unpublished manuscript ]
float3 Diffuse_Chan( float3 DiffuseColor, float Roughness, float NoH, float VoH )
{
    const float Alpha = Roughness * Roughness;
    // The original writeup uses an FSmooth term inspired by Burley diffuse to balance energy between spec/diffuse.
    // However in our implementation the energy balance between diffuse and spec is handled externally, so we stick
    // to a plain lambertian for the Roughness=0 limit.
    const float FSmooth = 1;
    const float Scale = max(0.55 - 0.2 * Roughness, 1.25 - 1.6 * Roughness);
    const float Bias = saturate(4 * Alpha);
    const float FRough = Scale * (NoH + Bias) * rcp(NoH + 0.025) * VoH * VoH;
    const float DiffuseSS = lerp(FSmooth, FRough, Roughness);
    const float DiffuseMS = Alpha * 0.38;
    return (1 / PI) * DiffuseColor * (DiffuseSS + DiffuseMS);
}

// [Portsmouth et al. 2025, "EON: A Practical Energy-Preserving Rough Diffuse BRDF"]
float3 Diffuse_EON( float3 DiffuseColor, float Roughness, float NoV, float NoL, float VoL )
{
    Roughness = Roughness * 0.4; // 언리얼 엔진의 Roughness 보정 식
    
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
    //Result.DiffuseContribution = Diffuse_Chan(DiffuseColor, Roughness, NoH, VoH);
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

#endif
