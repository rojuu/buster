// Copyright (c) 2022-2023, Roni Juppi <roni.juppi@gmail.com>

#pragma once

#include "def.h"
#include "containers/string_view.h"
#include "non_null.h"
#include "out_ptr.h"
#include "smart_ptr.h"

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

struct RendererResource {
    RendererResource() = default;
    virtual ~RendererResource() = default;

    RendererResource(const RendererResource&) = delete;
    RendererResource& operator=(const RendererResource&) = delete;
    RendererResource(RendererResource&&) = delete;
    RendererResource& operator=(RendererResource&&) = delete;
};

struct Texture : RendererResource
{
};

struct Font : RendererResource
{
};

template<typename T>
using RendererResourceHandle = shared_ptr<T>;

using TextureHandle = RendererResourceHandle<Texture>;
using FontHandle = RendererResourceHandle<Font>;

class Renderer {
public:
    Renderer() = default;
    virtual ~Renderer() = default;
    
    Renderer(const Renderer&) = delete;
    Renderer &operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer &operator=(Renderer&&) = delete;

    virtual void begin_frame(Color clear_color) = 0;
    virtual void end_frame(bool use_vsync, out_ptr<u64> out_draw_calls) = 0;

    virtual TextureHandle create_texture(non_null<u32> pixels, u32 width, u32 height) = 0;
    virtual TextureHandle create_texture_from_file(non_null<const char> filename) = 0;
    virtual FontHandle create_font(non_null<const char> font_file, f32 size) = 0;

    virtual void draw_sprite(const TextureHandle &texture, Rect src, Rect dst, Color tint_color = {1,1,1,1}) = 0;
    virtual void draw_text(const FontHandle &font, string_view text, f32 x, f32 y, Color tint_color = {1,1,1,1}) = 0;
};

::core::unique_ptr<Renderer> create_renderer();

}


