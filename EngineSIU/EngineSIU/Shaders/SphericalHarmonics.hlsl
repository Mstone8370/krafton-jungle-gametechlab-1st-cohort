
cbuffer SHBuffer : register(b9)
{
    float4 SH[9];
}

float3 EvaluateSH(float3 N)
{
    // UE(왼손, X-fwd) → 표준 SH(오른손)
    float X = -N.x;
    float Y = -N.y;
    float Z =  N.z;

    // Condon-Shortley 위상 포함
    float B[9];
    B[0] =  0.282095;
    B[1] = -0.488603 * Y;
    B[2] =  0.488603 * Z;
    B[3] = -0.488603 * X;
    B[4] =  1.092548 * X * Y;
    B[5] = -1.092548 * Y * Z;
    B[6] =  0.315392 * (3.0 * Z * Z - 1.0);
    B[7] = -1.092548 * X * Z;
    B[8] =  0.546274 * (X * X - Y * Y);

    float3 E_over_pi = 0;
    for (int i = 0; i < 9; ++i)
    {
        E_over_pi += SH[i].xyz * B[i];
    }

    return E_over_pi;
}
