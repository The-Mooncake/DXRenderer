#pragma once

cbuffer CB_WVP : register(b0)
{
    row_major float4x4 ModelMatrix : packoffset(c4);
    row_major float4x4 ViewMatrix : packoffset(c8);
    row_major float4x4 ProjectionMatrix : packoffset(c0);
};

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
    Out.Position = mul(float4(In.Position, 1.0f), ModelMatrix);
    Out.Position = mul(Out.Position, ViewMatrix);
    Out.Position = mul(Out.Position, ProjectionMatrix);
    
    Out.Colour = In.Colour;
    
    return Out;
}

float4 PSMain(VS_OUTPUT In) : SV_TARGET
{
    return float4(In.Colour, 1.0f);
}
