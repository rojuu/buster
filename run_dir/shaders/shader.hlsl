// Copyright (c) 2022-2023, Roni Juppi <roni.juppi@gmail.com>

cbuffer constants : register(b0)
{
    float2 window_size;
    float2 texture_size;
}

struct InputData
{
    float2 position   : POS;
    float2 uv         : TEX;
    
    uint   vertex_id  : SV_VertexID;

    float4 color            : COL;
    float4 src_rect         : SRC;
    float4 dst_rect         : DST;
};

struct Interpolators
{
    float4 dst_position      : SV_POSITION;
    
    float2 pixel         : TEXCOORD6;
    float4 color         : TEXCOORD7;
};

Interpolators vs_main(InputData i)
{
    Interpolators interp = (Interpolators)0;

    float2 pos = i.position;
    pos.y *= -1;
    pos = (pos + 1) / 2;
    pos *= window_size;
    pos.x = clamp(pos.x, i.dst_rect.x, i.dst_rect.x + i.dst_rect.z);
    pos.y = clamp(pos.y, i.dst_rect.y, i.dst_rect.y + i.dst_rect.w);
    pos /= window_size;
    pos *= 2;
    pos -= 1;
    pos.y *= -1;
    interp.dst_position = float4(pos, 0, 1);

    float2 pixel = i.uv * texture_size;
    pixel.x = clamp(pixel.x, i.src_rect.x, i.src_rect.x + i.src_rect.z);
    pixel.y = clamp(pixel.y, i.src_rect.y, i.src_rect.y + i.src_rect.w);
    interp.pixel = pixel;

    interp.color = i.color;
    return interp;
}

Texture2D    tex  : register(t0);
SamplerState samp : register(s0);

float4 ps_main(Interpolators interp) : SV_TARGET
{
#ifndef ERROR_SHADER
    // TODO: Toggle allow switching between fat pixel and regular bilinear
    // "fat pixel" or smooth pixel art shader courtesy of https://www.shadertoy.com/view/MlB3D3
    float2 pixel = floor(interp.pixel) + 0.5; // emulate point filtering
#if 0
    pixel += (1.0 - clamp((1.0 - frac(interp.pixel)) * 4, 0.0, 1.0)); // add some aa
#else
    float2 gradient = fwidth(interp.pixel);
    float2 clampedGradient = clamp((1.0 - abs(frac(interp.pixel) - 0.5) / gradient), 0.0, 1.0);
    pixel += clampedGradient * gradient * 0.5; // add some aa
#endif

    float4 tex_col = tex.Sample(samp, pixel / texture_size);

    float4 color = interp.color * tex_col;
    
    return color;
#else
    return float4(1, 0, 1, 1);
#endif
}
