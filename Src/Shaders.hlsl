#pragma once

cbuffer CB_WVP : register(b0)
{
    float4x4 ModelMatrix : packoffset(c0);
    float4x4 ViewMatrix : packoffset(c4);
    float4x4 ProjectionMatrix : packoffset(c8);
};

struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float3 Colour : COLOR;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float3 Colour : COLOR;
};

static const float3 LightPosition = float3(1.0f, 0.0f, 5.0f);
static const float LightIntensity = 1.0f;

VS_OUTPUT VSMain(VS_INPUT In)
{
    VS_OUTPUT Out;
    Out.Position = mul(float4(In.Position, 1.0f), ModelMatrix);
    Out.Position = mul(Out.Position, ViewMatrix);
    Out.Position = mul(Out.Position, ProjectionMatrix);

    // Normals to WS
    Out.Normal = mul(float4(In.Normal, 0.0f), ModelMatrix); 
    
    Out.Colour = In.Colour;
    
    return Out;
}

float4 PSMain(VS_OUTPUT In) : SV_TARGET
{
    float3 OutColour = In.Colour;
    
    // Lighting
    float a = 1.0f;
    float b = 0.0f;

    // Angle
    float3 LightDir = In.Normal - LightPosition;
    float Lambert = dot(normalize(In.Normal), normalize(LightDir));

    // Falloff - not working.
    float LightDist = length(LightPosition - In.Position);
    float Attenuation = 1 / (LightDist * LightDist);
    //float LightStrength = LightIntensity / LightDist * (LightIntensity + a * LightDist + b * LightDist * LightDist);
    float Lighting = Attenuation * Lambert;

    // Final Composition
    OutColour *= Lambert;
    OutColour = saturate(OutColour);
    
    return float4(OutColour, 1.0f);
}
