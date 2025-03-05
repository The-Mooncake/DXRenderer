#pragma once

struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Colour : COLOR;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float3 Colour : COLOR;
};

VS_OUTPUT VSMain(VS_INPUT In)
{
    VS_OUTPUT Out;
    Out.Position = float4(In.Position, 0.0f);
    Out.Colour = float3(0.65f, 0.65f, 0.65f);
    return Out;
}

float4 PSMain(VS_OUTPUT In) : SV_TARGET
{
    return float4(In.Colour, 1.0f);
}
