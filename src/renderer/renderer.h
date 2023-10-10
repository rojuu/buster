// Copyright (c) 2022-2023, Roni Juppi <roni.juppi@gmail.com>

#pragma once

#include "def.h"
#include "own_ptr.h"

namespace renderer {

struct Color
{
    f32 r{}, g{}, b{}, a{};
};

struct Rect
{
    f32 x{}, y{};
    f32 w{}, h{};
};

struct Texture
{
    u32* pixels{};
    u32 width{};
    u32 height{};
};

struct Font;
typedef Texture* TextureHandle;
typedef Font* FontHandle;

struct ColorGradient {
    Color top_left;
    Color top_right;
    Color bottom_left;
    Color bottom_right;
};

class RendererState {
public:
    RendererState() = default;
    virtual ~RendererState() = default;

    RendererState(const RendererState&) = delete;
    RendererState& operator=(const RendererState&) = delete;
    RendererState(RendererState&&) = delete;
    RendererState& operator=(RendererState&&) = delete;

    virtual void begin_frame(Color color) = 0;
    virtual void end_frame(u64* out_draw_calls) = 0;
    virtual TextureHandle create_texture(u32* pixels, u32 width, u32 height) = 0;
    virtual TextureHandle create_texture_from_file(const char* filename) = 0;
    virtual void draw_rect_pro(Rect dst, f32 edge_softness, f32 corner_radius, f32 border_thickness, ColorGradient gradient) = 0;
    virtual void draw_sprite_pro(TextureHandle texture, Rect src, Rect dst, f32 edge_softness, f32 corner_radius, f32 border_thickness, ColorGradient gradient) = 0;
    virtual void draw_sprite_tint(TextureHandle texture, Rect src, Rect dst, Color tint_color) = 0;
    virtual void draw_sprite(TextureHandle texture, Rect src, Rect dst) = 0;
    virtual FontHandle create_font(const char* font_file, f32 size) = 0;
    virtual void draw_text_tint(FontHandle font, u8* text, usz text_len, f32 x, f32 y, Color tint_color) = 0;
    virtual void draw_text(FontHandle font, u8* text, usz text_len, f32 x, f32 y) = 0;
};

class Renderer {
public:
    Renderer() = default;
    virtual ~Renderer() = default;
    
    Renderer(const Renderer&) = delete;
    Renderer &operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer &operator=(Renderer&&) = delete;

    virtual core::own_ptr<RendererState> create_state() = 0;
};

::core::own_ptr<Renderer> create_renderer();

}


