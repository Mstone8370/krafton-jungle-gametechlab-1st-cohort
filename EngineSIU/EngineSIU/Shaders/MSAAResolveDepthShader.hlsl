Texture2DMS<float> MultisampledDepth : register(t99);

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD;
};

float main(PSInput Input) : SV_TARGET
{
    uint Width;
    uint Height;
    uint SampleCount;
    MultisampledDepth.GetDimensions(Width, Height, SampleCount);

    const uint2 Pixel = min(uint2(Input.Position.xy), uint2(Width - 1, Height - 1));
    float ResolvedDepth = 1.0f;

    [loop]
    for (uint SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
    {
        ResolvedDepth = min(ResolvedDepth, MultisampledDepth.Load(Pixel, SampleIndex));
    }

    return ResolvedDepth;
}
