// Copyright (c) 2021-2023, Roni Juppi <roni.juppi@gmail.com>

#include "renderer.h"
#include "platform.h"
#include "utils.h"
#include "containers/common.h"
#include "containers/list.h"

#pragma warning(push, 0)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#pragma warning(pop)

#pragma warning(push, 0)
#define STBRP_ASSERT(cond) ASSERT(cond, "stb_rect_pack");
#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#pragma warning(pop)

#pragma warning(push, 0)
#define STBTT_assert(cond) ASSERT(cond, "stb_truetype")
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#pragma warning(pop)

#pragma warning(push, 0)
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#pragma warning(pop)

#include "imgui_impl_dx11.h"

#undef min
#undef max

using namespace bstr::core;
using namespace bstr::core::containers;
using namespace bstr::core::platform;

namespace bstr::renderer {

static void d3d11_print_last_error(const char* msg)
{
    char err[256] = { 0 };
    auto last_error = GetLastError();
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, last_error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), err, ARRAY_COUNT(err) - 1, NULL);
    LOG_ERROR("{}: {}", msg, err);
}


struct D3D11_ShaderData : public RendererResource
{
    D3D11_ShaderData() = default;

    ~D3D11_ShaderData()
    {
        if(input_layout) input_layout->Release();
        if (vertex_shader) vertex_shader->Release();
        if (pixel_shader) pixel_shader->Release();
    }

    ID3D11VertexShader* vertex_shader{};
    ID3D11PixelShader* pixel_shader{};
    ID3D11InputLayout* input_layout{};

    bool is_valid() {
        bool result = vertex_shader && pixel_shader && input_layout;
        return result;
    }
};

struct D3D11_Texture : public Texture
{
    D3D11_Texture() = default;

    ~D3D11_Texture()
    {
        if (sampler_state) sampler_state->Release();
        if (texture) texture->Release();
        if (texture_view) texture_view->Release();
    }

    ID3D11SamplerState* sampler_state{};
    ID3D11Texture2D* texture{};
    ID3D11ShaderResourceView* texture_view{};
};

struct D3D11_Font : public Font
{
    shared_ptr<D3D11_Texture> atlas{};

    vector<stbtt_packedchar> packed_chars;
    u32 num_chars{};
    u32 first_char{};
    u32 last_char{};

    f32 ascent{};
    f32 descent{};
    f32 line_gap{};

    u32 char_index(u32 character)
    {
        u32 result = character - first_char;
        return result;
    }

    f32 y_advance()
    {
        f32 result = ascent - descent + line_gap;
        return result;
    }
};

struct SpriteDrawCmd
{
    Rect src{};
    Rect dst{};
    Color color{};
};

struct D3D11_SpriteBatch
{
    shared_ptr<D3D11_Texture> texture{};
    vector<SpriteDrawCmd> sprite_commands;

    bool is_valid()
    {
        bool result = !(sprite_commands.empty() && texture == nullptr);
        return result;
    }
};


class D3D11_RendererState;

class D3D11_Renderer : public Renderer {
public:
    ~D3D11_Renderer();

    void begin_frame(Color clear_color) override;
    void end_frame(bool use_vsync, out_ptr<u64> out_draw_calls) override;

    TextureHandle create_texture(non_null<u32> pixels, u32 width, u32 height) override;
    TextureHandle create_texture_from_file(non_null<const char> filename) override;
    FontHandle create_font(const char* font_file, f32 size) override;

    void draw_sprite(const TextureHandle& texture, Rect src, Rect dst, Color tint_color) override;
    void draw_text(const FontHandle& font, string_view text, f32 x, f32 y, Color tint_color) override;

    void end_sprite_batch(D3D11_SpriteBatch& sprite_batch);
    TextureHandle create_font_texture(non_null<u8> pixels, u32 width, u32 height);
    void draw_glyph_and_advance(const shared_ptr<D3D11_Font> &font, u32 glyph, f32* x, f32* y, Color tint_color);
    
    
    shared_ptr<D3D11_ShaderData> create_shader(
        const char* shader_filename,
        D3D11_INPUT_ELEMENT_DESC const* input_elem_descs,
        usz input_elem_descs_count,
        D3D_SHADER_MACRO const* defines);

public:
    HDC m_hdc{};
    HINSTANCE m_hinstance{};
    HWND m_hwnd{};

    ID3D11Device* m_device{};
    ID3D11DeviceContext* m_device_context{};
    IDXGISwapChain* m_swap_chain{};

    D3D11_SpriteBatch m_current_sprite_batch;

    u32 m_window_width{};
    u32 m_window_height{};

    u64 m_draw_calls_in_frame{};

    TextureHandle m_white_texture{};

    ID3D11RenderTargetView* m_render_target_view{};
    ID3D11RasterizerState* m_rasterizer_state{};

    ID3D11BlendState* m_blend_state{};

    shared_ptr<D3D11_ShaderData> m_error_shader{};
    shared_ptr<D3D11_ShaderData> m_shader{};
   
    u64 m_last_shader_write_time{};
    f64 m_file_check_time{};

    ID3D11Buffer* m_constant_buffer{};

    ID3D11Buffer* m_per_vertex_buffer{};
    ID3D11Buffer* m_per_instance_buffer{};
    ID3D11Buffer* m_index_buffer{};
};

struct VertexData
{
    f32 x{}, y{};
    f32 u{}, v{};
};

struct InstanceData
{
    f32 color[4]{};
    f32 src_rect[4]{};
    f32 dst_rect[4]{};
};

// We allocate one large buffer for vertices needed in drawing. We cannot have more draw
// commands in a batch than can fit into that buffer.
// For 6*MiB buffer we can have 131072 commands per sprite batch assuming sizeof(InstanceData) is 48
static constexpr usz MAX_COMMANDS_PER_SPRITE_BATCH = ((6 * MiB) / sizeof(InstanceData));
static_assert(sizeof(InstanceData) == 48); // This is an assumption in the comment above

struct ShaderConstants
{
    f32 window_size_x{};
    f32 window_size_y{};
    f32 texture_size_x{};
    f32 texture_size_y{};
};


shared_ptr<D3D11_ShaderData> D3D11_Renderer::create_shader(
    const char *shader_filename,
    D3D11_INPUT_ELEMENT_DESC const* input_elem_descs,
    usz input_elem_descs_count,
    D3D_SHADER_MACRO const* defines)
{
    shared_ptr<D3D11_ShaderData> result;;
    HRESULT hr;

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(IS_DEBUG_BUILD)
    flags |= D3DCOMPILE_DEBUG; // add more debug output
#endif
    ID3DBlob* vs_blob = NULL, * ps_blob = NULL, * error_blob = NULL;

    auto shader_file = read_entire_file_as_bytes(shader_filename);

    hr = D3DCompile(
        shader_file.data(),
        shader_file.size(),
        shader_filename,
        defines,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "vs_main",
        "vs_5_0",
        flags, 0,
        &vs_blob,
        &error_blob);
    if (FAILED(hr))
    {
        if (error_blob)
        {
            LOG_ERROR("Vertex compile error: {}", (char*)error_blob->GetBufferPointer());
            error_blob->Release();
        }

        if (vs_blob)
        {
            vs_blob->Release();
            vs_blob = NULL;
        }
    }

    hr = D3DCompile(
        shader_file.data(),
        shader_file.size(),
        shader_filename,
        defines,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "ps_main",
        "ps_5_0",
        flags, 0,
        &ps_blob,
        &error_blob);
    if (FAILED(hr))
    {
        if (error_blob)
        {

            LOG_ERROR("Fragment compile error: {}", (char*)error_blob->GetBufferPointer());
            error_blob->Release();
        }
        if (ps_blob)
        {
            ps_blob->Release();
            ps_blob = NULL;
        }
    }

    result = make_shared<D3D11_ShaderData>();

    if (vs_blob)
    {
        hr = m_device->CreateVertexShader(
            vs_blob->GetBufferPointer(),
            vs_blob->GetBufferSize(),
            NULL,
            &result->vertex_shader);
        ASSERT_UNCHECKED(SUCCEEDED(hr), "");
    }

    if (ps_blob)
    {
        hr = m_device->CreatePixelShader(
            ps_blob->GetBufferPointer(),
            ps_blob->GetBufferSize(),
            NULL,
            &result->pixel_shader);
        ASSERT_UNCHECKED(SUCCEEDED(hr), "");
    }

    if (vs_blob)
    {
        hr = m_device->CreateInputLayout(
            input_elem_descs,
            (UINT)input_elem_descs_count,
            vs_blob->GetBufferPointer(),
            vs_blob->GetBufferSize(),
            &result->input_layout);
        ASSERT_UNCHECKED(SUCCEEDED(hr), "");
    }

    return result;
}

static const D3D11_INPUT_ELEMENT_DESC shader_input_element_descs[] = {
    { "POS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },

    { "COL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
    { "SRC", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
    { "DST", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
};

static string make_shader_filename(const char *shader_name)
{
    string result = string("shaders/") + shader_name + string(".hlsl");
    return result;
}

void D3D11_Renderer::begin_frame(Color clear_color)
{
    m_draw_calls_in_frame = 0;

    ImGui_ImplDX11_NewFrame();

    // hot reloading
    f64 current_time = get_highresolution_time_seconds();
    if (current_time - m_file_check_time > 0.5)
    {
        m_file_check_time = current_time;
        auto main_shader_filename = make_shader_filename("shader");
        u64 last_shader_write_time = get_file_modify_time(main_shader_filename.c_str());
        if (last_shader_write_time != m_last_shader_write_time)
        {
            m_last_shader_write_time = last_shader_write_time;
            auto shader_ptr = create_shader(main_shader_filename.c_str(), shader_input_element_descs, ARRAY_COUNT(shader_input_element_descs), NULL);
            auto& shader = *shader_ptr;
            if (shader.is_valid())
            {
                m_shader = move(shader_ptr);
            }
            else
            {
                m_shader = move(m_error_shader);
            }
        }
    }

    f32 background_colour[4] = { clear_color.r, clear_color.g, clear_color.b, clear_color.a };
    m_device_context->ClearRenderTargetView(m_render_target_view, background_colour);

    RECT window_rect;
    GetWindowRect(m_hwnd, &window_rect);
    m_window_width = window_rect.right - window_rect.left;
    m_window_height = window_rect.bottom - window_rect.top;
    D3D11_VIEWPORT viewport = {0};
    viewport.Width = (f32)m_window_width;
    viewport.Height = (f32)m_window_height;
    m_device_context->RSSetViewports(1, &viewport);
    m_device_context->OMSetRenderTargets(1, &m_render_target_view, NULL);

    m_device_context->OMSetBlendState(m_blend_state, NULL, 0xffffffff);

    m_device_context->RSSetState(m_rasterizer_state);
}

void D3D11_Renderer::end_sprite_batch(D3D11_SpriteBatch& sprite_batch)
{
    ASSERT(sprite_batch.is_valid(), "");

    shared_ptr<D3D11_Texture> texture = sprite_batch.texture;

    // Set instance buffer data
    {
        D3D11_MAPPED_SUBRESOURCE mapped_subresource;
        m_device_context->Map(
            (ID3D11Resource*)m_per_instance_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
        InstanceData* instance_datas = (InstanceData*)mapped_subresource.pData;

        for (usz cmd_idx = 0; cmd_idx < sprite_batch.sprite_commands.size(); ++cmd_idx)
        {
            auto& sprite_cmd = sprite_batch.sprite_commands[cmd_idx];
            InstanceData& instance = *(instance_datas + cmd_idx);

            instance.color[0] = sprite_cmd.color.r;
            instance.color[1] = sprite_cmd.color.g;
            instance.color[2] = sprite_cmd.color.b;
            instance.color[3] = sprite_cmd.color.a;
            instance.src_rect[0] = sprite_cmd.src.x;
            instance.src_rect[1] = sprite_cmd.src.y;
            instance.src_rect[2] = sprite_cmd.src.w;
            instance.src_rect[3] = sprite_cmd.src.h;
            instance.dst_rect[0] = sprite_cmd.dst.x;
            instance.dst_rect[1] = sprite_cmd.dst.y;
            instance.dst_rect[2] = sprite_cmd.dst.w;
            instance.dst_rect[3] = sprite_cmd.dst.h;
        }

        m_device_context->Unmap((ID3D11Resource*)m_per_instance_buffer, 0);
    }

    m_device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_device_context->IASetInputLayout(m_shader->input_layout);

    u32 vertex_offsets[2] = { 0, 0 };
    u32 vertex_strides[2] = { sizeof(VertexData), sizeof(InstanceData) };
    ID3D11Buffer* vertex_buffers[2] = { m_per_vertex_buffer, m_per_instance_buffer };
    m_device_context->IASetVertexBuffers(
        0,
        2,
        vertex_buffers,
        vertex_strides,
        vertex_offsets);

    m_device_context->IASetIndexBuffer(
        m_index_buffer, DXGI_FORMAT_R16_UINT, 0);

    // Set constant buffer data
    {
        D3D11_MAPPED_SUBRESOURCE mapped_subresource;
        m_device_context->Map(
            (ID3D11Resource*)m_constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);

        ShaderConstants* shader_constants = (ShaderConstants*)mapped_subresource.pData;
        shader_constants->window_size_x = (f32)m_window_width;
        shader_constants->window_size_y = (f32)m_window_height;
        shader_constants->texture_size_x = (f32)texture->width;
        shader_constants->texture_size_y = (f32)texture->height;

        m_device_context->Unmap(
            (ID3D11Resource*)m_constant_buffer, 0);
    }

    m_device_context->VSSetShader(
        m_shader->vertex_shader, NULL, 0);
    m_device_context->VSSetConstantBuffers(
        0, 1, &m_constant_buffer);

    m_device_context->PSSetShader(
        m_shader->pixel_shader, NULL, 0);
    m_device_context->PSSetConstantBuffers(
        0, 1, &m_constant_buffer);
    m_device_context->PSSetShaderResources(
        0, 1, &texture->texture_view);
    m_device_context->PSSetSamplers(
        0, 1, &texture->sampler_state);

    u32 index_count = 6;
    u32 instance_count = (u32)sprite_batch.sprite_commands.size();
    m_device_context->DrawIndexedInstanced(
        index_count, instance_count,
        0, 0, 0);

    m_draw_calls_in_frame += 1;
}

void D3D11_Renderer::end_frame(bool use_vsync, u64* out_draw_calls)
{
    if (m_current_sprite_batch.is_valid()) {
        end_sprite_batch(m_current_sprite_batch);
        m_current_sprite_batch.texture = nullptr;
        m_current_sprite_batch.sprite_commands.clear();
    }

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    UINT sync_interval = 0;
    m_swap_chain->Present(sync_interval, 0);
    *out_draw_calls = m_draw_calls_in_frame;
}

TextureHandle D3D11_Renderer::create_texture(u32* pixels, u32 width, u32 height)
{
    HRESULT hr;

    auto tex_ptr = make_shared<D3D11_Texture>();
    auto& tex = *tex_ptr;
    
    tex.width = width;
    tex.height = height;

    D3D11_SAMPLER_DESC sampler_desc = {};
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;

    m_device->CreateSamplerState(&sampler_desc, &tex.sampler_state);

    D3D11_TEXTURE2D_DESC texture_desc = {0};
    texture_desc.Width = width;
    texture_desc.Height = height;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.Usage = D3D11_USAGE_IMMUTABLE;
    texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA texture_subresource_data = {0};
    texture_subresource_data.pSysMem = pixels;
    texture_subresource_data.SysMemPitch = width * sizeof(*pixels);

    hr = m_device->CreateTexture2D(
        &texture_desc, &texture_subresource_data, &tex.texture);
    ASSERT_UNCHECKED(SUCCEEDED(hr), "");

    hr = m_device->CreateShaderResourceView(
        (ID3D11Resource*)tex.texture, NULL, &tex.texture_view);
    ASSERT_UNCHECKED(SUCCEEDED(hr), "");

    return tex_ptr;
}

TextureHandle D3D11_Renderer::create_texture_from_file(const char* filename)
{
    int req_comps = 4;
    int width, height, num_comps;
    u8* pixels = stbi_load(filename, &width, &height, &num_comps, req_comps);
    TextureHandle result = create_texture((u32*)pixels, width, height);
    stbi_image_free(pixels);
    return result;
}

TextureHandle D3D11_Renderer::create_font_texture(non_null<u8> pixels, u32 width, u32 height)
{
    usz pixels_len = width * height;
    vector<u32> gpu_pixels(pixels_len);

    u32 r = 0xFF;
    u32 g = 0xFF;
    u32 b = 0xFF;
    for (usz pixel_idx = 0; pixel_idx < pixels_len; ++pixel_idx)
    {
        u32 a = (u32)pixels[pixel_idx];
        u32 pixel = (a << 24) | (b << 16) | (g << 8) | (r << 0);
        gpu_pixels[pixel_idx] = pixel;
    }

    auto result = create_texture(gpu_pixels.data(), width, height);
    return result;
}

void D3D11_Renderer::draw_sprite(const TextureHandle &texture, Rect src, Rect dst, Color tint_color)
{
    // TODO: More intelligent batching? now it just batches if you draw the same texture multiple times in a row,
    // but maybe sometimes it could batch even if the user doesn't know to do that 

    D3D11_SpriteBatch &batch = m_current_sprite_batch;

    auto d3d11_tex = static_pointer_cast<D3D11_Texture>(texture);

    if (batch.texture != d3d11_tex || batch.sprite_commands.size() >= MAX_COMMANDS_PER_SPRITE_BATCH) {
        if (batch.texture) {
            end_sprite_batch(batch);
        }
        batch.sprite_commands.clear();
        batch.texture = d3d11_tex;
    }

    ASSERT(batch.sprite_commands.size() + 1 <= MAX_COMMANDS_PER_SPRITE_BATCH, "");
    SpriteDrawCmd& cmd = batch.sprite_commands.emplace_back();
    cmd.src = src;
    cmd.dst = dst;
    cmd.color = tint_color;
}


FontHandle D3D11_Renderer::create_font(non_null<const char> font_file, f32 size)
{
    shared_ptr<D3D11_Font> result;
    
    auto font_data = read_entire_file_as_bytes(font_file);
    ASSERT_UNCHECKED(!font_data.empty(), "");

    f32 ascent;
    f32 descent;
    f32 line_gap;
    stbtt_GetScaledFontVMetrics(font_data.data(), 0, size, &ascent, &descent, &line_gap);

    // ASCII range
    u32 first_char = 0;
    u32 last_char = 255;
    u32 num_chars = last_char - first_char;

    // TODO: Adjust atlas size somehow based on font size etc?
    u32 width = 512;
    u32 height = 512;
    u32 stride = width;

    vector<u8> pixels(width * height);

    stbtt_pack_context pc;
    if (stbtt_PackBegin(&pc, pixels.data(), width, height, stride, 1, 0) != 0)
    {
        vector<stbtt_packedchar> temp_packed_chars(num_chars);
        if (stbtt_PackFontRange(&pc, font_data.data(), 0, size, first_char, num_chars, temp_packed_chars.data()) != 0)
        {
            result = make_shared<D3D11_Font>();
           
            result->packed_chars = move(temp_packed_chars);

            result->first_char = first_char;
            result->last_char = last_char;
            result->num_chars = num_chars;

            result->ascent = ascent;
            result->descent = descent;
            result->line_gap = line_gap;

            result->atlas = static_pointer_cast<D3D11_Texture>(
                create_font_texture(pixels.data(), width, height));
        }
        else
        {
            LOG_ERROR("stbtt_PackFontRange failed");
        }

        stbtt_PackEnd(&pc);
    }
    else
    {
        LOG_ERROR("stbtt_PackBegin failed");
    }

    return result;
}

void D3D11_Renderer::draw_glyph_and_advance(
    const shared_ptr<D3D11_Font> &font,
    u32 glyph,
    f32* x, f32* y,
    Color tint_color)
{
    stbtt_aligned_quad quad;
    int align_to_integer = true;
    stbtt_GetPackedQuad(font->packed_chars.data(), font->atlas->width, font->atlas->height, font->char_index(glyph), x, y, &quad, align_to_integer);

    Rect source;
    {
        f32 x0 = quad.s0 * font->atlas->width;
        f32 y0 = quad.t0 * font->atlas->height;
        f32 x1 = quad.s1 * font->atlas->width;
        f32 y1 = quad.t1 * font->atlas->height;

        source.x = x0;
        source.y = y0;
        source.w = (x1 - x0);
        source.h = (y1 - y0);
    }

    Rect dest;
    {
        f32 x0 = quad.x0;
        f32 x1 = quad.x1;
        f32 y0 = quad.y0;
        f32 y1 = quad.y1;

        dest.x = x0;
        dest.y = y0;
        dest.w = (x1 - x0);
        dest.h = (y1 - y0);
    }

    draw_sprite(font->atlas, source, dest, tint_color);
}

void D3D11_Renderer::draw_text(const FontHandle& font_, string_view text, f32 x, f32 y, Color tint_color)
{
    auto font = static_pointer_cast<D3D11_Font>(font_);
    
    f32 line_begin_x = x;
    y += font->ascent + font->descent; // I want text to have top-left as origin
    for (char ch : text)
    {
        // TODO: UTF-8
        u32 codepoint = ch;
        if (codepoint == '\r')
        {
            continue;
        }
        if (codepoint == '\n')
        {
            x = line_begin_x;
            y += font->y_advance();
            continue;
        }
        if (codepoint == '\t') // tab as 4 spaces
        {
            for (int i = 0; i < 4; ++i)
            {
                draw_glyph_and_advance(font, ' ', &x, &y, tint_color);
            }
            continue;
        }
        // else just draw the thing
        draw_glyph_and_advance(font, codepoint, &x, &y, tint_color);
    }
}

::bstr::core::unique_ptr<Renderer> create_renderer()
{
    HINSTANCE instance = GetModuleHandle(0);

    WindowHandle window_handle = get_native_window_handle();
    HWND window = (HWND)window_handle.nwh;
    HDC hdc = GetDC(window);
    if (!hdc) {
        d3d11_print_last_error("GetDC");
        return nullptr;
    }

    unique_ptr<D3D11_Renderer> renderer;

    renderer = make_unique<D3D11_Renderer>();

    renderer->m_hdc = hdc;
    renderer->m_hinstance = instance;
    renderer->m_hwnd = window;

    DXGI_SWAP_CHAIN_DESC swap_chain_descr = {};
    swap_chain_descr.BufferDesc.RefreshRate.Numerator = 0;
    swap_chain_descr.BufferDesc.RefreshRate.Denominator = 1;
    swap_chain_descr.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swap_chain_descr.SampleDesc.Count = 1;
    swap_chain_descr.SampleDesc.Quality = 0;
    swap_chain_descr.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_descr.BufferCount = 1;
    swap_chain_descr.OutputWindow = renderer->m_hwnd;
    swap_chain_descr.Windowed = true;

    HRESULT hr;

    D3D_FEATURE_LEVEL feature_level;
    UINT flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#if defined(IS_DEBUG_BUILD)
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    hr = D3D11CreateDeviceAndSwapChain(
        NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        flags,
        NULL,
        0,
        D3D11_SDK_VERSION,
        &swap_chain_descr,
        &renderer->m_swap_chain,
        &renderer->m_device,
        &feature_level,
        &renderer->m_device_context);
    ASSERT_UNCHECKED(S_OK == hr && renderer->m_swap_chain && renderer->m_device && renderer->m_device_context, "");

    if (!ImGui_ImplDX11_Init(renderer->m_device, renderer->m_device_context)) {
        LOG_ERROR("Failed to initialize dear imgui");
    }

#if defined(IS_INTERNAL_BUILD) && IS_INTERNAL_BUILD
    // Set up debug layer to break on D3D11 errors
    ID3D11Debug* d3d_debug = NULL;

    renderer->m_device->QueryInterface(__uuidof(ID3D11Debug), (void**)&d3d_debug);

    if (d3d_debug)
    {
        ID3D11InfoQueue* d3d_info_queue = NULL;
        hr = d3d_debug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&d3d_info_queue);
        if (SUCCEEDED(hr))
        {
            d3d_info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
            d3d_info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
            d3d_info_queue->Release();
        }
        d3d_debug->Release();
    }
#endif


    ID3D11Texture2D* framebuffer;
    hr = renderer->m_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&framebuffer);
    ASSERT_UNCHECKED(SUCCEEDED(hr), "");
    hr = renderer->m_device->CreateRenderTargetView(
        (ID3D11Resource*)framebuffer,
        0,
        &renderer->m_render_target_view);
    ASSERT_UNCHECKED(SUCCEEDED(hr), "");
    framebuffer->Release();

    D3D11_RASTERIZER_DESC rasterizer_desc = {};
    rasterizer_desc.FillMode = D3D11_FILL_SOLID;
    rasterizer_desc.CullMode = D3D11_CULL_BACK;
    rasterizer_desc.FrontCounterClockwise = true;
    //rasterizer_desc.MultisampleEnable = true;
    //rasterizer_desc.AntialiasedLineEnable = true;
    hr = renderer->m_device->CreateRasterizerState(&rasterizer_desc, &renderer->m_rasterizer_state);
    ASSERT_UNCHECKED(SUCCEEDED(hr), "");

    D3D11_BLEND_DESC blend_desc = {};
    blend_desc.RenderTarget[0].BlendEnable = true;
    blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
    blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D10_COLOR_WRITE_ENABLE_ALL;
    hr = renderer->m_device->CreateBlendState(&blend_desc, &renderer->m_blend_state);
    ASSERT_UNCHECKED(SUCCEEDED(hr), "");

    D3D_SHADER_MACRO error_shader_defines[2] = {}; // +1 for nul termination
    error_shader_defines[0].Name = "ERROR_SHADER";
    error_shader_defines[0].Definition = "1";
    auto main_shader_filename = make_shader_filename("shader");
    renderer->m_error_shader = renderer->create_shader(main_shader_filename.c_str(), shader_input_element_descs, ARRAY_COUNT(shader_input_element_descs), error_shader_defines);
    ASSERT_UNCHECKED(renderer->m_error_shader->is_valid(), "");

    renderer->m_shader = renderer->create_shader(main_shader_filename.c_str(), shader_input_element_descs, ARRAY_COUNT(shader_input_element_descs), NULL);
    if (!renderer->m_shader->is_valid())
    {
        renderer->m_shader = renderer->m_error_shader;
    }

    renderer->m_last_shader_write_time = get_file_modify_time(main_shader_filename.c_str());
    renderer->m_file_check_time = get_highresolution_time_seconds();

    static const VertexData vertices[] = {
        // pos           tex
        -1.f, -1.f,      0, 1,
         1.f, -1.f,      1, 1,
         1.f,  1.f,      1, 0,
        -1.f,  1.f,      0, 0,
    };
    D3D11_BUFFER_DESC per_vertex_buffer_desc = {};
    per_vertex_buffer_desc.ByteWidth = sizeof(vertices);
    per_vertex_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    per_vertex_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA per_vertex_sr = {};
    per_vertex_sr.pSysMem = vertices;
    hr = renderer->m_device->CreateBuffer(
        &per_vertex_buffer_desc,
        &per_vertex_sr,
        &renderer->m_per_vertex_buffer);
    ASSERT_UNCHECKED(SUCCEEDED(hr), "");

    D3D11_BUFFER_DESC per_instance_buffer_desc = {};
    per_instance_buffer_desc.ByteWidth = sizeof(InstanceData) * MAX_COMMANDS_PER_SPRITE_BATCH;
    per_instance_buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
    per_instance_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    per_instance_buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = renderer->m_device->CreateBuffer(
        &per_instance_buffer_desc,
        NULL,
        &renderer->m_per_instance_buffer);
    ASSERT_UNCHECKED(SUCCEEDED(hr), "");

    static const u16 indices[] = {
        0, 1, 2,
        0, 2, 3,
    };
    D3D11_BUFFER_DESC index_buffer_desc = { 0 };
    index_buffer_desc.ByteWidth = sizeof(indices);
    index_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    index_buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA index_sr_data = { 0 };
    index_sr_data.pSysMem = indices;
    hr = renderer->m_device->CreateBuffer(
        &index_buffer_desc,
        &index_sr_data,
        &renderer->m_index_buffer);
    ASSERT_UNCHECKED(SUCCEEDED(hr), "");

    D3D11_BUFFER_DESC constant_buffer_desc = { 0 };
    constant_buffer_desc.ByteWidth = (UINT)align_forwards(sizeof(ShaderConstants), 16);
    constant_buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
    constant_buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constant_buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    renderer->m_device->CreateBuffer(&constant_buffer_desc, NULL, &renderer->m_constant_buffer);

    {
        u32 pixels[1] = { ~0u };
        renderer->m_white_texture = renderer->create_texture(pixels, 1, 1);
    }

    renderer->m_current_sprite_batch.sprite_commands.reserve(MAX_COMMANDS_PER_SPRITE_BATCH);

    return renderer;
}

D3D11_Renderer::~D3D11_Renderer()
{
    ImGui_ImplDX11_Shutdown();

    m_blend_state->Release();
    m_constant_buffer->Release();
    m_per_vertex_buffer->Release();
    m_per_instance_buffer->Release();
    m_render_target_view->Release();
    m_rasterizer_state->Release();

    m_swap_chain->Release();
    m_device_context->Release();
    m_device->Release();
}

}
