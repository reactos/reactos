/*
 * Copyright 1997-2000 Marcus Meissner
 * Copyright 1998-2000 Lionel Ulmer
 * Copyright 2000-2001 TransGaming Technologies Inc.
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006-2011, 2013-2014 Stefan Dösinger for CodeWeavers
 * Copyright 2007-2008 Henri Verbeet
 * Copyright 2006-2008 Roderick Colenbrander
 * Copyright 2009-2011 Henri Verbeet for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "wined3d_private.h"
#include "wined3d_gl.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(d3d_perf);

/* Works correctly only for <= 4 bpp formats. */
static void get_color_masks(const struct wined3d_format *format, uint32_t *masks)
{
    masks[0] = wined3d_mask_from_size(format->red_size) << format->red_offset;
    masks[1] = wined3d_mask_from_size(format->green_size) << format->green_offset;
    masks[2] = wined3d_mask_from_size(format->blue_size) << format->blue_offset;
}

static void convert_r32_float_r16_float(const BYTE *src, BYTE *dst,
        unsigned int pitch_in, unsigned int pitch_out, unsigned int w, unsigned int h)
{
    unsigned short *dst_s;
    const float *src_f;
    unsigned int x, y;

    TRACE("Converting %ux%u pixels, pitches %u %u.\n", w, h, pitch_in, pitch_out);

    for (y = 0; y < h; ++y)
    {
        src_f = (const float *)(src + y * pitch_in);
        dst_s = (unsigned short *) (dst + y * pitch_out);
        for (x = 0; x < w; ++x)
        {
            dst_s[x] = float_32_to_16(src_f + x);
        }
    }
}

static void convert_r5g6b5_x8r8g8b8(const BYTE *src, BYTE *dst,
        unsigned int pitch_in, unsigned int pitch_out, unsigned int w, unsigned int h)
{
    static const unsigned char convert_5to8[] =
    {
        0x00, 0x08, 0x10, 0x19, 0x21, 0x29, 0x31, 0x3a,
        0x42, 0x4a, 0x52, 0x5a, 0x63, 0x6b, 0x73, 0x7b,
        0x84, 0x8c, 0x94, 0x9c, 0xa5, 0xad, 0xb5, 0xbd,
        0xc5, 0xce, 0xd6, 0xde, 0xe6, 0xef, 0xf7, 0xff,
    };
    static const unsigned char convert_6to8[] =
    {
        0x00, 0x04, 0x08, 0x0c, 0x10, 0x14, 0x18, 0x1c,
        0x20, 0x24, 0x28, 0x2d, 0x31, 0x35, 0x39, 0x3d,
        0x41, 0x45, 0x49, 0x4d, 0x51, 0x55, 0x59, 0x5d,
        0x61, 0x65, 0x69, 0x6d, 0x71, 0x75, 0x79, 0x7d,
        0x82, 0x86, 0x8a, 0x8e, 0x92, 0x96, 0x9a, 0x9e,
        0xa2, 0xa6, 0xaa, 0xae, 0xb2, 0xb6, 0xba, 0xbe,
        0xc2, 0xc6, 0xca, 0xce, 0xd2, 0xd7, 0xdb, 0xdf,
        0xe3, 0xe7, 0xeb, 0xef, 0xf3, 0xf7, 0xfb, 0xff,
    };
    unsigned int x, y;

    TRACE("Converting %ux%u pixels, pitches %u %u.\n", w, h, pitch_in, pitch_out);

    for (y = 0; y < h; ++y)
    {
        const WORD *src_line = (const WORD *)(src + y * pitch_in);
        DWORD *dst_line = (DWORD *)(dst + y * pitch_out);
        for (x = 0; x < w; ++x)
        {
            WORD pixel = src_line[x];
            dst_line[x] = 0xff000000u
                    | convert_5to8[(pixel & 0xf800u) >> 11] << 16
                    | convert_6to8[(pixel & 0x07e0u) >> 5] << 8
                    | convert_5to8[(pixel & 0x001fu)];
        }
    }
}

/* We use this for both B8G8R8A8 -> B8G8R8X8 and B8G8R8X8 -> B8G8R8A8, since
 * in both cases we're just setting the X / Alpha channel to 0xff. */
static void convert_a8r8g8b8_x8r8g8b8(const BYTE *src, BYTE *dst,
        unsigned int pitch_in, unsigned int pitch_out, unsigned int w, unsigned int h)
{
    unsigned int x, y;

    TRACE("Converting %ux%u pixels, pitches %u %u.\n", w, h, pitch_in, pitch_out);

    for (y = 0; y < h; ++y)
    {
        const DWORD *src_line = (const DWORD *)(src + y * pitch_in);
        DWORD *dst_line = (DWORD *)(dst + y * pitch_out);

        for (x = 0; x < w; ++x)
        {
            dst_line[x] = 0xff000000 | (src_line[x] & 0xffffff);
        }
    }
}

static inline BYTE cliptobyte(int x)
{
    return (BYTE)((x < 0) ? 0 : ((x > 255) ? 255 : x));
}

static void convert_yuy2_x8r8g8b8(const BYTE *src, BYTE *dst,
        unsigned int pitch_in, unsigned int pitch_out, unsigned int w, unsigned int h)
{
    int c2, d, e, r2 = 0, g2 = 0, b2 = 0;
    unsigned int x, y;

    TRACE("Converting %ux%u pixels, pitches %u %u.\n", w, h, pitch_in, pitch_out);

    for (y = 0; y < h; ++y)
    {
        const BYTE *src_line = src + y * pitch_in;
        DWORD *dst_line = (DWORD *)(dst + y * pitch_out);
        for (x = 0; x < w; ++x)
        {
            /* YUV to RGB conversion formulas from http://en.wikipedia.org/wiki/YUV:
             *     C = Y - 16; D = U - 128; E = V - 128;
             *     R = cliptobyte((298 * C + 409 * E + 128) >> 8);
             *     G = cliptobyte((298 * C - 100 * D - 208 * E + 128) >> 8);
             *     B = cliptobyte((298 * C + 516 * D + 128) >> 8);
             * Two adjacent YUY2 pixels are stored as four bytes: Y0 U Y1 V .
             * U and V are shared between the pixels. */
            if (!(x & 1)) /* For every even pixel, read new U and V. */
            {
                d = (int) src_line[1] - 128;
                e = (int) src_line[3] - 128;
                r2 = 409 * e + 128;
                g2 = - 100 * d - 208 * e + 128;
                b2 = 516 * d + 128;
            }
            c2 = 298 * ((int) src_line[0] - 16);
            dst_line[x] = 0xff000000
                | cliptobyte((c2 + r2) >> 8) << 16    /* red   */
                | cliptobyte((c2 + g2) >> 8) << 8     /* green */
                | cliptobyte((c2 + b2) >> 8);         /* blue  */
                /* Scale RGB values to 0..255 range,
                 * then clip them if still not in range (may be negative),
                 * then shift them within DWORD if necessary. */
            src_line += 2;
        }
    }
}

static void convert_yuy2_r5g6b5(const BYTE *src, BYTE *dst,
        unsigned int pitch_in, unsigned int pitch_out, unsigned int w, unsigned int h)
{
    unsigned int x, y;
    int c2, d, e, r2 = 0, g2 = 0, b2 = 0;

    TRACE("Converting %ux%u pixels, pitches %u %u\n", w, h, pitch_in, pitch_out);

    for (y = 0; y < h; ++y)
    {
        const BYTE *src_line = src + y * pitch_in;
        WORD *dst_line = (WORD *)(dst + y * pitch_out);
        for (x = 0; x < w; ++x)
        {
            /* YUV to RGB conversion formulas from http://en.wikipedia.org/wiki/YUV:
             *     C = Y - 16; D = U - 128; E = V - 128;
             *     R = cliptobyte((298 * C + 409 * E + 128) >> 8);
             *     G = cliptobyte((298 * C - 100 * D - 208 * E + 128) >> 8);
             *     B = cliptobyte((298 * C + 516 * D + 128) >> 8);
             * Two adjacent YUY2 pixels are stored as four bytes: Y0 U Y1 V .
             * U and V are shared between the pixels. */
            if (!(x & 1)) /* For every even pixel, read new U and V. */
            {
                d = (int) src_line[1] - 128;
                e = (int) src_line[3] - 128;
                r2 = 409 * e + 128;
                g2 = - 100 * d - 208 * e + 128;
                b2 = 516 * d + 128;
            }
            c2 = 298 * ((int) src_line[0] - 16);
            dst_line[x] = (cliptobyte((c2 + r2) >> 8) >> 3) << 11   /* red   */
                | (cliptobyte((c2 + g2) >> 8) >> 2) << 5            /* green */
                | (cliptobyte((c2 + b2) >> 8) >> 3);                /* blue  */
                /* Scale RGB values to 0..255 range,
                 * then clip them if still not in range (may be negative),
                 * then shift them within DWORD if necessary. */
            src_line += 2;
        }
    }
}

struct d3dfmt_converter_desc
{
    enum wined3d_format_id from, to;
    void (*convert)(const BYTE *src, BYTE *dst,
                    unsigned int pitch_in, unsigned int pitch_out,
                    unsigned int w, unsigned int h);
};

static const struct d3dfmt_converter_desc converters[] =
{
    {WINED3DFMT_R32_FLOAT,      WINED3DFMT_R16_FLOAT,       convert_r32_float_r16_float},
    {WINED3DFMT_B5G6R5_UNORM,   WINED3DFMT_B8G8R8X8_UNORM,  convert_r5g6b5_x8r8g8b8},
    {WINED3DFMT_B8G8R8A8_UNORM, WINED3DFMT_B8G8R8X8_UNORM,  convert_a8r8g8b8_x8r8g8b8},
    {WINED3DFMT_B8G8R8X8_UNORM, WINED3DFMT_B8G8R8A8_UNORM,  convert_a8r8g8b8_x8r8g8b8},
    {WINED3DFMT_YUY2,           WINED3DFMT_B8G8R8X8_UNORM,  convert_yuy2_x8r8g8b8},
    {WINED3DFMT_YUY2,           WINED3DFMT_B5G6R5_UNORM,    convert_yuy2_r5g6b5},
};

static inline const struct d3dfmt_converter_desc *find_converter(enum wined3d_format_id from,
        enum wined3d_format_id to)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(converters); ++i)
    {
        if (converters[i].from == from && converters[i].to == to)
            return &converters[i];
    }

    return NULL;
}

static struct wined3d_texture *surface_convert_format(struct wined3d_texture *src_texture,
        unsigned int sub_resource_idx, const struct wined3d_format *dst_format)
{
    unsigned int texture_level = sub_resource_idx % src_texture->level_count;
    const struct wined3d_format *src_format = src_texture->resource.format;
    struct wined3d_device *device = src_texture->resource.device;
    const struct d3dfmt_converter_desc *conv = NULL;
    unsigned int src_row_pitch, src_slice_pitch;
    struct wined3d_texture *dst_texture;
    struct wined3d_bo_address src_data;
    struct wined3d_resource_desc desc;
    struct wined3d_context *context;
    DWORD map_binding;

    if (!(conv = find_converter(src_format->id, dst_format->id)) && ((device->wined3d->flags & WINED3D_NO3D)
            || !is_identity_fixup(src_format->color_fixup) || src_format->conv_byte_count
            || !is_identity_fixup(dst_format->color_fixup) || dst_format->conv_byte_count
            || ((src_format->attrs & WINED3D_FORMAT_ATTR_COMPRESSED)
            && !src_format->decompress)))
    {
        FIXME("Cannot find a conversion function from format %s to %s.\n",
                debug_d3dformat(src_format->id), debug_d3dformat(dst_format->id));
        return NULL;
    }

    /* FIXME: Multisampled conversion? */
    desc.resource_type = WINED3D_RTYPE_TEXTURE_2D;
    desc.format = dst_format->id;
    desc.multisample_type = WINED3D_MULTISAMPLE_NONE;
    desc.multisample_quality = 0;
    desc.usage = WINED3DUSAGE_SCRATCH | WINED3DUSAGE_CS;
    desc.bind_flags = 0;
    desc.access = WINED3D_RESOURCE_ACCESS_CPU | WINED3D_RESOURCE_ACCESS_MAP_R | WINED3D_RESOURCE_ACCESS_MAP_W;
    desc.width = wined3d_texture_get_level_width(src_texture, texture_level);
    desc.height = wined3d_texture_get_level_height(src_texture, texture_level);
    desc.depth = 1;
    desc.size = 0;
    if (FAILED(wined3d_texture_create(device, &desc, 1, 1, WINED3D_TEXTURE_CREATE_DISCARD,
            NULL, NULL, &wined3d_null_parent_ops, &dst_texture)))
    {
        ERR("Failed to create a destination texture for conversion.\n");
        return NULL;
    }

    context = context_acquire(device, NULL, 0);

    map_binding = src_texture->resource.map_binding;
    if (!wined3d_texture_load_location(src_texture, sub_resource_idx, context, map_binding))
        ERR("Failed to load the source sub-resource into %s.\n", wined3d_debug_location(map_binding));
    wined3d_texture_get_pitch(src_texture, texture_level, &src_row_pitch, &src_slice_pitch);
    wined3d_texture_get_bo_address(src_texture, sub_resource_idx, &src_data, map_binding);

    if (conv)
    {
        unsigned int dst_row_pitch, dst_slice_pitch;
        struct wined3d_bo_address dst_data;
        struct wined3d_range range;
        const BYTE *src;
        BYTE *dst;

        map_binding = dst_texture->resource.map_binding;
        if (!wined3d_texture_load_location(dst_texture, 0, context, map_binding))
            ERR("Failed to load the destination sub-resource into %s.\n", wined3d_debug_location(map_binding));
        wined3d_texture_get_pitch(dst_texture, 0, &dst_row_pitch, &dst_slice_pitch);
        wined3d_texture_get_bo_address(dst_texture, 0, &dst_data, map_binding);

        src = wined3d_context_map_bo_address(context, &src_data,
                src_texture->sub_resources[sub_resource_idx].size, WINED3D_MAP_READ);
        dst = wined3d_context_map_bo_address(context, &dst_data,
                dst_texture->sub_resources[0].size, WINED3D_MAP_WRITE);

        conv->convert(src, dst, src_row_pitch, dst_row_pitch, desc.width, desc.height);

        range.offset = 0;
        range.size = dst_texture->sub_resources[0].size;
        wined3d_texture_invalidate_location(dst_texture, 0, ~map_binding);
        wined3d_context_unmap_bo_address(context, &dst_data, 1, &range);
        wined3d_context_unmap_bo_address(context, &src_data, 0, NULL);
    }
    else
    {
        struct wined3d_box src_box = {0, 0, desc.width, desc.height, 0, 1};

        TRACE("Using upload conversion.\n");

        wined3d_texture_prepare_location(dst_texture, 0, context, WINED3D_LOCATION_TEXTURE_RGB);
        dst_texture->texture_ops->texture_upload_data(context, wined3d_const_bo_address(&src_data),
                src_format, &src_box, src_row_pitch, src_slice_pitch,
                dst_texture, 0, WINED3D_LOCATION_TEXTURE_RGB, 0, 0, 0);

        wined3d_texture_validate_location(dst_texture, 0, WINED3D_LOCATION_TEXTURE_RGB);
        wined3d_texture_invalidate_location(dst_texture, 0, ~WINED3D_LOCATION_TEXTURE_RGB);
    }

    context_release(context);

    return dst_texture;
}

void texture2d_read_from_framebuffer(struct wined3d_texture *texture, unsigned int sub_resource_idx,
        struct wined3d_context *context, DWORD src_location, DWORD dst_location)
{
    struct wined3d_resource *resource = &texture->resource;
    struct wined3d_device *device = resource->device;
    const struct wined3d_format_gl *format_gl;
    struct wined3d_texture *restore_texture;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context_gl *context_gl;
    unsigned int row_pitch, slice_pitch;
    unsigned int width, height, level;
    struct wined3d_bo_address data;
    bool restore_context = false;
    unsigned int restore_idx;
    BYTE *row, *top, *bottom;
    BOOL src_is_upside_down;
    BYTE *mem = NULL;
    uint8_t *offset;
    unsigned int i;

    TRACE("texture %p, sub_resource_idx %u, context %p, src_location %s, dst_location %s.\n",
            texture, sub_resource_idx, context, wined3d_debug_location(src_location), wined3d_debug_location(dst_location));

    /* dst_location was already prepared by the caller. */
    wined3d_texture_get_bo_address(texture, sub_resource_idx, &data, dst_location);
    offset = data.addr;

    restore_texture = context->current_rt.texture;
    restore_idx = context->current_rt.sub_resource_idx;
    if (!wined3d_resource_is_offscreen(resource) && (restore_texture != texture || restore_idx != sub_resource_idx))
    {
        context = context_acquire(device, texture, sub_resource_idx);
        restore_context = true;
    }
    context_gl = wined3d_context_gl(context);
    gl_info = context_gl->gl_info;

    if (resource->format->depth_size || resource->format->stencil_size)
        wined3d_context_gl_apply_fbo_state_explicit(context_gl, GL_READ_FRAMEBUFFER,
                NULL, 0, resource, sub_resource_idx, src_location);
    else
        wined3d_context_gl_apply_fbo_state_explicit(context_gl, GL_READ_FRAMEBUFFER,
                resource, sub_resource_idx, NULL, 0, src_location);

    /* Select the correct read buffer, and give some debug output. There is no
     * need to keep track of the current read buffer or reset it, every part
     * of the code that reads pixels sets the read buffer as desired. */
    if (src_location != WINED3D_LOCATION_DRAWABLE || wined3d_resource_is_offscreen(resource))
    {
        /* Mapping the primary render target which is not on a swapchain.
         * Read from the back buffer. */
        TRACE("Mapping offscreen render target.\n");
        gl_info->gl_ops.gl.p_glReadBuffer(wined3d_context_gl_get_offscreen_gl_buffer(context_gl));
        src_is_upside_down = TRUE;
    }
    else
    {
        /* Onscreen surfaces are always part of a swapchain */
        GLenum buffer = wined3d_texture_get_gl_buffer(texture);
        TRACE("Mapping %#x buffer.\n", buffer);
        gl_info->gl_ops.gl.p_glReadBuffer(buffer);
        src_is_upside_down = FALSE;
    }
    checkGLcall("glReadBuffer");
    wined3d_context_gl_check_fbo_status(context_gl, GL_READ_FRAMEBUFFER);

    if (data.buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, wined3d_bo_gl(data.buffer_object)->id));
        checkGLcall("glBindBuffer");
        offset += data.buffer_object->buffer_offset;
    }
    else
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
        checkGLcall("glBindBuffer");
    }

    level = sub_resource_idx % texture->level_count;
    wined3d_texture_get_pitch(texture, level, &row_pitch, &slice_pitch);
    format_gl = wined3d_format_gl(resource->format);

    /* Setup pixel store pack state -- to glReadPixels into the correct place */
    gl_info->gl_ops.gl.p_glPixelStorei(GL_PACK_ROW_LENGTH, row_pitch / format_gl->f.byte_count);
    checkGLcall("glPixelStorei");

    width = wined3d_texture_get_level_width(texture, level);
    height = wined3d_texture_get_level_height(texture, level);
    gl_info->gl_ops.gl.p_glReadPixels(0, 0, width, height,
            format_gl->format, format_gl->type, offset);
    checkGLcall("glReadPixels");

    /* Reset previous pixel store pack state */
    gl_info->gl_ops.gl.p_glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    checkGLcall("glPixelStorei");

    if (!src_is_upside_down)
    {
        /* glReadPixels returns the image upside down, and there is no way to
         * prevent this. Flip the lines in software. */

        if (!(row = malloc(row_pitch)))
            goto error;

        if (data.buffer_object)
        {
            mem = GL_EXTCALL(glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_WRITE));
            checkGLcall("glMapBuffer");
        }
        mem += (uintptr_t)offset;

        top = mem;
        bottom = mem + row_pitch * (height - 1);
        for (i = 0; i < height / 2; i++)
        {
            memcpy(row, top, row_pitch);
            memcpy(top, bottom, row_pitch);
            memcpy(bottom, row, row_pitch);
            top += row_pitch;
            bottom -= row_pitch;
        }
        free(row);

        if (data.buffer_object)
            GL_EXTCALL(glUnmapBuffer(GL_PIXEL_PACK_BUFFER));
    }

error:
    if (data.buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
        wined3d_context_gl_reference_bo(context_gl, wined3d_bo_gl(data.buffer_object));
        checkGLcall("glBindBuffer");
    }

    if (restore_context)
        context_restore(context, restore_texture, restore_idx);
}

/* Context activation is done by the caller. */
static void cpu_blitter_destroy(struct wined3d_blitter *blitter, struct wined3d_context *context)
{
    struct wined3d_blitter *next;

    if ((next = blitter->next))
        next->ops->blitter_destroy(next, context);

    free(blitter);
}

static HRESULT surface_cpu_blt_compressed(const BYTE *src_data, BYTE *dst_data,
        UINT src_pitch, UINT dst_pitch, UINT update_w, UINT update_h,
        const struct wined3d_format *format, uint32_t flags, const struct wined3d_blt_fx *fx)
{
    UINT row_block_count;
    const BYTE *src_row;
    BYTE *dst_row;
    UINT x, y;

    src_row = src_data;
    dst_row = dst_data;

    row_block_count = (update_w + format->block_width - 1) / format->block_width;

    if (!flags)
    {
        for (y = 0; y < update_h; y += format->block_height)
        {
            memcpy(dst_row, src_row, row_block_count * format->block_byte_count);
            src_row += src_pitch;
            dst_row += dst_pitch;
        }

        return WINED3D_OK;
    }

    if (flags == WINED3D_BLT_FX && fx->fx == WINEDDBLTFX_MIRRORUPDOWN)
    {
        src_row += (((update_h / format->block_height) - 1) * src_pitch);

        switch (format->id)
        {
            case WINED3DFMT_DXT1:
                for (y = 0; y < update_h; y += format->block_height)
                {
                    struct block
                    {
                        WORD color[2];
                        BYTE control_row[4];
                    };

                    const struct block *s = (const struct block *)src_row;
                    struct block *d = (struct block *)dst_row;

                    for (x = 0; x < row_block_count; ++x)
                    {
                        d[x].color[0] = s[x].color[0];
                        d[x].color[1] = s[x].color[1];
                        d[x].control_row[0] = s[x].control_row[3];
                        d[x].control_row[1] = s[x].control_row[2];
                        d[x].control_row[2] = s[x].control_row[1];
                        d[x].control_row[3] = s[x].control_row[0];
                    }
                    src_row -= src_pitch;
                    dst_row += dst_pitch;
                }
                return WINED3D_OK;

            case WINED3DFMT_DXT2:
            case WINED3DFMT_DXT3:
                for (y = 0; y < update_h; y += format->block_height)
                {
                    struct block
                    {
                        WORD alpha_row[4];
                        WORD color[2];
                        BYTE control_row[4];
                    };

                    const struct block *s = (const struct block *)src_row;
                    struct block *d = (struct block *)dst_row;

                    for (x = 0; x < row_block_count; ++x)
                    {
                        d[x].alpha_row[0] = s[x].alpha_row[3];
                        d[x].alpha_row[1] = s[x].alpha_row[2];
                        d[x].alpha_row[2] = s[x].alpha_row[1];
                        d[x].alpha_row[3] = s[x].alpha_row[0];
                        d[x].color[0] = s[x].color[0];
                        d[x].color[1] = s[x].color[1];
                        d[x].control_row[0] = s[x].control_row[3];
                        d[x].control_row[1] = s[x].control_row[2];
                        d[x].control_row[2] = s[x].control_row[1];
                        d[x].control_row[3] = s[x].control_row[0];
                    }
                    src_row -= src_pitch;
                    dst_row += dst_pitch;
                }
                return WINED3D_OK;

            default:
                FIXME("Compressed flip not implemented for format %s.\n",
                        debug_d3dformat(format->id));
                return E_NOTIMPL;
        }
    }

    FIXME("Unsupported blit on compressed surface (format %s, flags %#x, DDFX %#x).\n",
            debug_d3dformat(format->id), flags, flags & WINED3D_BLT_FX ? fx->fx : 0);

    return E_NOTIMPL;
}

static HRESULT surface_cpu_blt(struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx,
        const struct wined3d_box *dst_box, struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx,
        const struct wined3d_box *src_box, uint32_t flags, const struct wined3d_blt_fx *fx,
        enum wined3d_texture_filter_type filter)
{
    unsigned int bpp, src_height, src_width, dst_height, dst_width, row_byte_count;
    struct wined3d_device *device = dst_texture->resource.device;
    const struct wined3d_format *src_format, *dst_format;
    struct wined3d_texture *converted_texture = NULL;
    struct wined3d_bo_address src_data, dst_data;
    unsigned int src_fmt_attrs, dst_fmt_attrs;
    struct wined3d_map_desc dst_map, src_map;
    unsigned int x, sx, xinc, y, sy, yinc;
    struct wined3d_context *context;
    struct wined3d_range dst_range;
    unsigned int texture_level;
    HRESULT hr = WINED3D_OK;
    BOOL same_sub_resource;
    BOOL upload = FALSE;
    DWORD map_binding;
    const BYTE *sbase;
    const BYTE *sbuf;
    BYTE *dbuf;

    TRACE("dst_texture %p, dst_sub_resource_idx %u, dst_box %s, src_texture %p, "
            "src_sub_resource_idx %u, src_box %s, flags %#x, fx %p, filter %s.\n",
            dst_texture, dst_sub_resource_idx, debug_box(dst_box), src_texture,
            src_sub_resource_idx, debug_box(src_box), flags, fx, debug_d3dtexturefiltertype(filter));

    context = context_acquire(device, NULL, 0);

    src_format = src_texture->resource.format;
    dst_format = dst_texture->resource.format;

    if (wined3d_format_is_typeless(src_format) && src_format->id == dst_format->typeless_id)
        src_format = dst_format;
    if (wined3d_format_is_typeless(dst_format) && dst_format->id == src_format->typeless_id)
        dst_format = src_format;

    src_height = src_box->bottom - src_box->top;
    src_width = src_box->right - src_box->left;
    dst_height = dst_box->bottom - dst_box->top;
    dst_width = dst_box->right - dst_box->left;

    dst_range.offset = 0;
    dst_range.size = dst_texture->sub_resources[dst_sub_resource_idx].size;
    if (src_texture == dst_texture && src_sub_resource_idx == dst_sub_resource_idx)
    {
        same_sub_resource = TRUE;

        map_binding = dst_texture->resource.map_binding;
        texture_level = dst_sub_resource_idx % dst_texture->level_count;
        if (!wined3d_texture_load_location(dst_texture, dst_sub_resource_idx, context, map_binding))
            ERR("Failed to load the destination sub-resource into %s.\n", wined3d_debug_location(map_binding));
        wined3d_texture_invalidate_location(dst_texture, dst_sub_resource_idx, ~map_binding);
        wined3d_texture_get_pitch(dst_texture, texture_level, &dst_map.row_pitch, &dst_map.slice_pitch);
        wined3d_texture_get_bo_address(dst_texture, dst_sub_resource_idx, &dst_data, map_binding);
        dst_map.data = wined3d_context_map_bo_address(context, &dst_data,
                dst_texture->sub_resources[dst_sub_resource_idx].size, WINED3D_MAP_READ | WINED3D_MAP_WRITE);

        src_map = dst_map;
    }
    else
    {
        same_sub_resource = FALSE;
        upload = dst_format->attrs & WINED3D_FORMAT_ATTR_BLOCKS
                && (dst_width != src_width || dst_height != src_height);

        if (upload)
        {
            dst_format = src_format->attrs & WINED3D_FORMAT_ATTR_BLOCKS
                    ? wined3d_get_format(device->adapter, WINED3DFMT_B8G8R8A8_UNORM, 0) : src_format;
        }

        if (!(flags & WINED3D_BLT_RAW) && dst_format->id != src_format->id)
        {
            if (!(converted_texture = surface_convert_format(src_texture, src_sub_resource_idx, dst_format)))
            {
                FIXME("Cannot convert %s to %s.\n", debug_d3dformat(src_format->id),
                        debug_d3dformat(dst_format->id));
                context_release(context);
                return WINED3DERR_NOTAVAILABLE;
            }
            src_texture = converted_texture;
            src_sub_resource_idx = 0;
            src_format = src_texture->resource.format;
        }

        map_binding = src_texture->resource.map_binding;
        texture_level = src_sub_resource_idx % src_texture->level_count;
        if (!wined3d_texture_load_location(src_texture, src_sub_resource_idx, context, map_binding))
            ERR("Failed to load the source sub-resource into %s.\n", wined3d_debug_location(map_binding));
        wined3d_texture_get_pitch(src_texture, texture_level, &src_map.row_pitch, &src_map.slice_pitch);
        wined3d_texture_get_bo_address(src_texture, src_sub_resource_idx, &src_data, map_binding);
        src_map.data = wined3d_context_map_bo_address(context, &src_data,
                src_texture->sub_resources[src_sub_resource_idx].size, WINED3D_MAP_READ);

        if (upload)
        {
            wined3d_format_calculate_pitch(dst_format, 1, dst_box->right, dst_box->bottom,
                    &dst_map.row_pitch, &dst_map.slice_pitch);
            dst_map.data = malloc(dst_map.slice_pitch);
        }
        else
        {
            map_binding = dst_texture->resource.map_binding;
            texture_level = dst_sub_resource_idx % dst_texture->level_count;
            if (!wined3d_texture_load_location(dst_texture, dst_sub_resource_idx, context, map_binding))
                ERR("Failed to load the destination sub-resource into %s.\n", wined3d_debug_location(map_binding));

            wined3d_texture_invalidate_location(dst_texture, dst_sub_resource_idx, ~map_binding);
            wined3d_texture_get_pitch(dst_texture, texture_level, &dst_map.row_pitch, &dst_map.slice_pitch);
            wined3d_texture_get_bo_address(dst_texture, dst_sub_resource_idx, &dst_data, map_binding);
            dst_map.data = wined3d_context_map_bo_address(context, &dst_data,
                    dst_texture->sub_resources[dst_sub_resource_idx].size, WINED3D_MAP_WRITE);
        }
    }
    src_fmt_attrs = src_format->attrs;
    dst_fmt_attrs = dst_format->attrs;
    flags &= ~WINED3D_BLT_RAW;

    bpp = dst_format->byte_count;
    row_byte_count = dst_width * bpp;

    sbase = (BYTE *)src_map.data
            + ((src_box->top / src_format->block_height) * src_map.row_pitch)
            + ((src_box->left / src_format->block_width) * src_format->block_byte_count);
    dbuf = (BYTE *)dst_map.data
            + ((dst_box->top / dst_format->block_height) * dst_map.row_pitch)
            + ((dst_box->left / dst_format->block_width) * dst_format->block_byte_count);

    if (src_fmt_attrs & dst_fmt_attrs & WINED3D_FORMAT_ATTR_BLOCKS)
    {
        TRACE("%s -> %s copy.\n", debug_d3dformat(src_format->id), debug_d3dformat(dst_format->id));

        if (same_sub_resource)
        {
            FIXME("Only plain blits supported on compressed surfaces.\n");
            hr = E_NOTIMPL;
            goto release;
        }

        hr = surface_cpu_blt_compressed(sbase, dbuf,
                src_map.row_pitch, dst_map.row_pitch, dst_width, dst_height,
                src_format, flags, fx);
        goto release;
    }

    if ((src_fmt_attrs | dst_fmt_attrs) & WINED3D_FORMAT_ATTR_HEIGHT_SCALE)
    {
        FIXME("Unsupported blit between height-scaled formats (src %s, dst %s).\n",
                debug_d3dformat(src_format->id), debug_d3dformat(dst_format->id));
        hr = E_NOTIMPL;
        goto release;
    }

    if (filter != WINED3D_TEXF_NONE && filter != WINED3D_TEXF_POINT
            && (src_width != dst_width || src_height != dst_height))
    {
        /* Can happen when d3d9 apps do a StretchRect() call which isn't handled in GL. */
        FIXME("Filter %s not supported in software blit.\n", debug_d3dtexturefiltertype(filter));
    }

    xinc = (src_width << 16) / dst_width;
    yinc = (src_height << 16) / dst_height;

    if (!flags)
    {
        /* No effects, we can cheat here. */
        if (dst_width == src_width)
        {
            if (dst_height == src_height)
            {
                /* No stretching in either direction. This needs to be as fast
                 * as possible. */
                sbuf = sbase;

                /* Check for overlapping surfaces. */
                if (!same_sub_resource || dst_box->top < src_box->top
                        || dst_box->right <= src_box->left || src_box->right <= dst_box->left)
                {
                    /* No overlap, or dst above src, so copy from top downwards. */
                    for (y = 0; y < dst_height; ++y)
                    {
                        memcpy(dbuf, sbuf, row_byte_count);
                        sbuf += src_map.row_pitch;
                        dbuf += dst_map.row_pitch;
                    }
                }
                else if (dst_box->top > src_box->top)
                {
                    /* Copy from bottom upwards. */
                    sbuf += src_map.row_pitch * dst_height;
                    dbuf += dst_map.row_pitch * dst_height;
                    for (y = 0; y < dst_height; ++y)
                    {
                        sbuf -= src_map.row_pitch;
                        dbuf -= dst_map.row_pitch;
                        memcpy(dbuf, sbuf, row_byte_count);
                    }
                }
                else
                {
                    /* Src and dst overlapping on the same line, use memmove. */
                    for (y = 0; y < dst_height; ++y)
                    {
                        memmove(dbuf, sbuf, row_byte_count);
                        sbuf += src_map.row_pitch;
                        dbuf += dst_map.row_pitch;
                    }
                }
            }
            else
            {
                /* Stretching in y direction only. */
                for (y = sy = 0; y < dst_height; ++y, sy += yinc)
                {
                    sbuf = sbase + (sy >> 16) * src_map.row_pitch;
                    memcpy(dbuf, sbuf, row_byte_count);
                    dbuf += dst_map.row_pitch;
                }
            }
        }
        else
        {
            /* Stretching in X direction. */
            unsigned int last_sy = ~0u;
            for (y = sy = 0; y < dst_height; ++y, sy += yinc)
            {
                sbuf = sbase + (sy >> 16) * src_map.row_pitch;

                if ((sy >> 16) == (last_sy >> 16))
                {
                    /* This source row is the same as last source row -
                     * Copy the already stretched row. */
                    memcpy(dbuf, dbuf - dst_map.row_pitch, row_byte_count);
                }
                else
                {
#define STRETCH_ROW(type) \
do { \
    const type *s = (const type *)sbuf; \
    type *d = (type *)dbuf; \
    for (x = sx = 0; x < dst_width; ++x, sx += xinc) \
        d[x] = s[sx >> 16]; \
} while(0)

                    switch(bpp)
                    {
                        case 1:
                            STRETCH_ROW(BYTE);
                            break;
                        case 2:
                            STRETCH_ROW(WORD);
                            break;
                        case 4:
                            STRETCH_ROW(DWORD);
                            break;
                        case 3:
                        {
                            const BYTE *s;
                            BYTE *d = dbuf;
                            for (x = sx = 0; x < dst_width; x++, sx+= xinc)
                            {
                                DWORD pixel;

                                s = sbuf + 3 * (sx >> 16);
                                pixel = s[0] | (s[1] << 8) | (s[2] << 16);
                                d[0] = (pixel      ) & 0xff;
                                d[1] = (pixel >>  8) & 0xff;
                                d[2] = (pixel >> 16) & 0xff;
                                d += 3;
                            }
                            break;
                        }
                        default:
                            FIXME("Stretched blit not implemented for bpp %u.\n", bpp * 8);
                            hr = WINED3DERR_NOTAVAILABLE;
                            goto error;
                    }
#undef STRETCH_ROW
                }
                dbuf += dst_map.row_pitch;
                last_sy = sy;
            }
        }
    }
    else
    {
        LONG dstyinc = dst_map.row_pitch, dstxinc = bpp;
        DWORD keylow = 0xffffffff, keyhigh = 0, keymask = 0xffffffff;
        DWORD destkeylow = 0x0, destkeyhigh = 0xffffffff, destkeymask = 0xffffffff;
        if (flags & (WINED3D_BLT_SRC_CKEY | WINED3D_BLT_DST_CKEY
                | WINED3D_BLT_SRC_CKEY_OVERRIDE | WINED3D_BLT_DST_CKEY_OVERRIDE))
        {
            /* The color keying flags are checked for correctness in ddraw. */
            if (flags & WINED3D_BLT_SRC_CKEY)
            {
                keylow  = src_texture->async.src_blt_color_key.color_space_low_value;
                keyhigh = src_texture->async.src_blt_color_key.color_space_high_value;
            }
            else if (flags & WINED3D_BLT_SRC_CKEY_OVERRIDE)
            {
                keylow = fx->src_color_key.color_space_low_value;
                keyhigh = fx->src_color_key.color_space_high_value;
            }

            if (flags & WINED3D_BLT_DST_CKEY)
            {
                /* Destination color keys are taken from the source surface! */
                destkeylow = src_texture->async.dst_blt_color_key.color_space_low_value;
                destkeyhigh = src_texture->async.dst_blt_color_key.color_space_high_value;
            }
            else if (flags & WINED3D_BLT_DST_CKEY_OVERRIDE)
            {
                destkeylow = fx->dst_color_key.color_space_low_value;
                destkeyhigh = fx->dst_color_key.color_space_high_value;
            }

            if (bpp == 1)
            {
                keymask = 0xff;
            }
            else
            {
                uint32_t masks[3];
                get_color_masks(src_format, masks);
                keymask = masks[0] | masks[1] | masks[2];
            }
            flags &= ~(WINED3D_BLT_SRC_CKEY | WINED3D_BLT_DST_CKEY
                    | WINED3D_BLT_SRC_CKEY_OVERRIDE | WINED3D_BLT_DST_CKEY_OVERRIDE);
        }

        if (flags & WINED3D_BLT_FX)
        {
            BYTE *dTopLeft, *dTopRight, *dBottomLeft, *dBottomRight, *tmp;
            LONG tmpxy;
            dTopLeft     = dbuf;
            dTopRight    = dbuf + ((dst_width - 1) * bpp);
            dBottomLeft  = dTopLeft + ((dst_height - 1) * dst_map.row_pitch);
            dBottomRight = dBottomLeft + ((dst_width - 1) * bpp);

            if (fx->fx & WINEDDBLTFX_ARITHSTRETCHY)
            {
                /* I don't think we need to do anything about this flag. */
                WARN("Nothing done for WINEDDBLTFX_ARITHSTRETCHY.\n");
            }
            if (fx->fx & WINEDDBLTFX_MIRRORLEFTRIGHT)
            {
                tmp          = dTopRight;
                dTopRight    = dTopLeft;
                dTopLeft     = tmp;
                tmp          = dBottomRight;
                dBottomRight = dBottomLeft;
                dBottomLeft  = tmp;
                dstxinc = dstxinc * -1;
            }
            if (fx->fx & WINEDDBLTFX_MIRRORUPDOWN)
            {
                tmp          = dTopLeft;
                dTopLeft     = dBottomLeft;
                dBottomLeft  = tmp;
                tmp          = dTopRight;
                dTopRight    = dBottomRight;
                dBottomRight = tmp;
                dstyinc = dstyinc * -1;
            }
            if (fx->fx & WINEDDBLTFX_NOTEARING)
            {
                /* I don't think we need to do anything about this flag. */
                WARN("Nothing done for WINEDDBLTFX_NOTEARING.\n");
            }
            if (fx->fx & WINEDDBLTFX_ROTATE180)
            {
                tmp          = dBottomRight;
                dBottomRight = dTopLeft;
                dTopLeft     = tmp;
                tmp          = dBottomLeft;
                dBottomLeft  = dTopRight;
                dTopRight    = tmp;
                dstxinc = dstxinc * -1;
                dstyinc = dstyinc * -1;
            }
            if (fx->fx & WINEDDBLTFX_ROTATE270)
            {
                tmp          = dTopLeft;
                dTopLeft     = dBottomLeft;
                dBottomLeft  = dBottomRight;
                dBottomRight = dTopRight;
                dTopRight    = tmp;
                tmpxy   = dstxinc;
                dstxinc = dstyinc;
                dstyinc = tmpxy;
                dstxinc = dstxinc * -1;
            }
            if (fx->fx & WINEDDBLTFX_ROTATE90)
            {
                tmp          = dTopLeft;
                dTopLeft     = dTopRight;
                dTopRight    = dBottomRight;
                dBottomRight = dBottomLeft;
                dBottomLeft  = tmp;
                tmpxy   = dstxinc;
                dstxinc = dstyinc;
                dstyinc = tmpxy;
                dstyinc = dstyinc * -1;
            }
            if (fx->fx & WINEDDBLTFX_ZBUFFERBASEDEST)
            {
                /* I don't think we need to do anything about this flag. */
                WARN("Nothing done for WINEDDBLTFX_ZBUFFERBASEDEST.\n");
            }
            dbuf = dTopLeft;
            flags &= ~(WINED3D_BLT_FX);
        }

#define COPY_COLORKEY_FX(type) \
do { \
    const type *s; \
    type *d = (type *)dbuf, *dx, tmp; \
    for (y = sy = 0; y < dst_height; ++y, sy += yinc) \
    { \
        s = (const type *)(sbase + (sy >> 16) * src_map.row_pitch); \
        dx = d; \
        for (x = sx = 0; x < dst_width; ++x, sx += xinc) \
        { \
            tmp = s[sx >> 16]; \
            if (((tmp & keymask) < keylow || (tmp & keymask) > keyhigh) \
                    && ((dx[0] & destkeymask) >= destkeylow && (dx[0] & destkeymask) <= destkeyhigh)) \
            { \
                dx[0] = tmp; \
            } \
            dx = (type *)(((BYTE *)dx) + dstxinc); \
        } \
        d = (type *)(((BYTE *)d) + dstyinc); \
    } \
} while(0)

        switch (bpp)
        {
            case 1:
                COPY_COLORKEY_FX(BYTE);
                break;
            case 2:
                COPY_COLORKEY_FX(WORD);
                break;
            case 4:
                COPY_COLORKEY_FX(DWORD);
                break;
            case 3:
            {
                const BYTE *s;
                BYTE *d = dbuf, *dx;
                for (y = sy = 0; y < dst_height; ++y, sy += yinc)
                {
                    sbuf = sbase + (sy >> 16) * src_map.row_pitch;
                    dx = d;
                    for (x = sx = 0; x < dst_width; ++x, sx+= xinc)
                    {
                        DWORD pixel, dpixel = 0;
                        s = sbuf + 3 * (sx>>16);
                        pixel = s[0] | (s[1] << 8) | (s[2] << 16);
                        dpixel = dx[0] | (dx[1] << 8 ) | (dx[2] << 16);
                        if (((pixel & keymask) < keylow || (pixel & keymask) > keyhigh)
                                && ((dpixel & keymask) >= destkeylow || (dpixel & keymask) <= keyhigh))
                        {
                            dx[0] = (pixel      ) & 0xff;
                            dx[1] = (pixel >>  8) & 0xff;
                            dx[2] = (pixel >> 16) & 0xff;
                        }
                        dx += dstxinc;
                    }
                    d += dstyinc;
                }
                break;
            }
            default:
                FIXME("%s color-keyed blit not implemented for bpp %u.\n",
                      (flags & WINED3D_BLT_SRC_CKEY) ? "Source" : "Destination", bpp * 8);
                hr = WINED3DERR_NOTAVAILABLE;
                goto error;
#undef COPY_COLORKEY_FX
        }
    }

error:
    if (flags)
        FIXME("    Unsupported flags %#x.\n", flags);

release:
    if (upload && hr == WINED3D_OK)
    {
        struct wined3d_bo_address data;

        data.buffer_object = 0;
        data.addr = dst_map.data;

        texture_level = dst_sub_resource_idx % dst_texture->level_count;

        wined3d_texture_prepare_location(dst_texture, texture_level, context, WINED3D_LOCATION_TEXTURE_RGB);
        dst_texture->texture_ops->texture_upload_data(context, wined3d_const_bo_address(&data), dst_format,
                dst_box, dst_map.row_pitch, dst_map.slice_pitch, dst_texture, texture_level,
                WINED3D_LOCATION_TEXTURE_RGB, dst_box->left, dst_box->top, 0);

        wined3d_texture_validate_location(dst_texture, texture_level, WINED3D_LOCATION_TEXTURE_RGB);
        wined3d_texture_invalidate_location(dst_texture, texture_level, ~WINED3D_LOCATION_TEXTURE_RGB);
    }

    if (upload)
    {
        free(dst_map.data);
    }
    else
    {
        wined3d_context_unmap_bo_address(context, &dst_data, 1, &dst_range);
    }

    if (!same_sub_resource)
        wined3d_context_unmap_bo_address(context, &src_data, 0, NULL);
    if (SUCCEEDED(hr) && dst_texture->swapchain && dst_texture->swapchain->front_buffer == dst_texture)
    {
        SetRect(&dst_texture->swapchain->front_buffer_update,
                dst_box->left, dst_box->top, dst_box->right, dst_box->bottom);
        dst_texture->swapchain->swapchain_ops->swapchain_frontbuffer_updated(dst_texture->swapchain);
    }
    if (converted_texture)
        wined3d_texture_decref(converted_texture);
    context_release(context);

    return hr;
}

void cpu_blitter_clear_texture(struct wined3d_texture *texture, unsigned int sub_resource_idx,
        const struct wined3d_box *box, const struct wined3d_color *colour)
{
    struct wined3d_device *device = texture->resource.device;
    struct wined3d_context *context;
    struct wined3d_bo_address data;
    struct wined3d_box level_box;
    struct wined3d_map_desc map;
    struct wined3d_range range;
    bool full_subresource;
    unsigned int level;
    DWORD map_binding;

    TRACE("texture %p, sub_resource_idx %u, box %s, colour %s.\n",
            texture, sub_resource_idx, debug_box(box), debug_color(colour));

    if (texture->resource.format_attrs & WINED3D_FORMAT_ATTR_BLOCKS)
    {
        FIXME("Not implemented for format %s.\n", debug_d3dformat(texture->resource.format->id));
        return;
    }

    context = context_acquire(device, NULL, 0);

    level = sub_resource_idx % texture->level_count;
    wined3d_texture_get_level_box(texture, level, &level_box);
    full_subresource = !memcmp(box, &level_box, sizeof(*box));

    map_binding = texture->resource.map_binding;
    if (!wined3d_texture_load_location(texture, sub_resource_idx, context, map_binding))
        ERR("Failed to load the sub-resource into %s.\n", wined3d_debug_location(map_binding));
    wined3d_texture_invalidate_location(texture, sub_resource_idx, ~map_binding);
    wined3d_texture_get_pitch(texture, level, &map.row_pitch, &map.slice_pitch);
    wined3d_texture_get_bo_address(texture, sub_resource_idx, &data, map_binding);
    map.data = wined3d_context_map_bo_address(context, &data,
            texture->sub_resources[sub_resource_idx].size, WINED3D_MAP_WRITE);
    range.offset = 0;
    range.size = texture->sub_resources[sub_resource_idx].size;

    wined3d_resource_memory_colour_fill(&texture->resource, &map, colour, box, full_subresource);

    wined3d_context_unmap_bo_address(context, &data, 1, &range);
    context_release(context);
}

static void cpu_blitter_clear_rtv(struct wined3d_rendertarget_view *view,
        const struct wined3d_box *box, const struct wined3d_color *colour)
{
    if (view->format->id != view->resource->format->id)
        FIXME("View format %s doesn't match resource format %s.\n",
                debug_d3dformat(view->format->id), debug_d3dformat(view->resource->format->id));

    if (view->resource->type == WINED3D_RTYPE_BUFFER)
    {
        FIXME("Not implemented for buffers.\n");
        return;
    }

    cpu_blitter_clear_texture(texture_from_resource(view->resource), view->sub_resource_idx, box, colour);
}

static bool wined3d_box_intersect(struct wined3d_box *ret, const struct wined3d_box *b1,
        const struct wined3d_box *b2)
{
    wined3d_box_set(ret, max(b1->left, b2->left), max(b1->top, b2->top),
            min(b1->right, b2->right), min(b1->bottom, b2->bottom),
            max(b1->front, b2->front), min(b1->back, b2->back));
    return ret->right > ret->left && ret->bottom > ret->top && ret->back > ret->front;
}

static void cpu_blitter_clear(struct wined3d_blitter *blitter, struct wined3d_device *device,
        unsigned int rt_count, const struct wined3d_fb_state *fb, unsigned int rect_count, const RECT *clear_rects,
        const RECT *draw_rect, uint32_t flags, const struct wined3d_color *colour, float depth, unsigned int stencil)
{
    struct wined3d_color c = {depth, 0.0f, 0.0f, 0.0f};
    struct wined3d_box box, box_clip, box_view;
    struct wined3d_rendertarget_view *view;
    unsigned int i, j;

    if (!rect_count)
    {
        rect_count = 1;
        clear_rects = draw_rect;
    }

    for (i = 0; i < rect_count; ++i)
    {
        box.left = max(clear_rects[i].left, draw_rect->left);
        box.top = max(clear_rects[i].top, draw_rect->top);
        box.right = min(clear_rects[i].right, draw_rect->right);
        box.bottom = min(clear_rects[i].bottom, draw_rect->bottom);
        box.front = 0;
        box.back = ~0u;

        if (box.left >= box.right || box.top >= box.bottom)
            continue;

        if (flags & WINED3DCLEAR_TARGET)
        {
            for (j = 0; j < rt_count; ++j)
            {
                if ((view = fb->render_targets[j]))
                {
                    wined3d_rendertarget_view_get_box(view, &box_view);
                    if (wined3d_box_intersect(&box_clip, &box_view, &box))
                        cpu_blitter_clear_rtv(view, &box_clip, colour);
                }
            }
        }

        if ((flags & (WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL)) && (view = fb->depth_stencil))
        {
            if ((view->format->depth_size && !(flags & WINED3DCLEAR_ZBUFFER))
                    || (view->format->stencil_size && !(flags & WINED3DCLEAR_STENCIL)))
                FIXME("Clearing %#x on %s.\n", flags, debug_d3dformat(view->format->id));

            wined3d_rendertarget_view_get_box(view, &box_view);
            if (wined3d_box_intersect(&box_clip, &box_view, &box))
                cpu_blitter_clear_rtv(view, &box_clip, &c);
        }
    }
}

static DWORD cpu_blitter_blit(struct wined3d_blitter *blitter, enum wined3d_blit_op op,
        struct wined3d_context *context, struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx,
        DWORD src_location, const RECT *src_rect, struct wined3d_texture *dst_texture,
        unsigned int dst_sub_resource_idx, DWORD dst_location, const RECT *dst_rect,
        const struct wined3d_color_key *color_key, enum wined3d_texture_filter_type filter,
        const struct wined3d_format *resolve_format)
{
    struct wined3d_box dst_box = {dst_rect->left, dst_rect->top, dst_rect->right, dst_rect->bottom, 0, 1};
    struct wined3d_box src_box = {src_rect->left, src_rect->top, src_rect->right, src_rect->bottom, 0, 1};
    struct wined3d_blt_fx fx;
    DWORD flags = 0;

    memset(&fx, 0, sizeof(fx));
    switch (op)
    {
        case WINED3D_BLIT_OP_COLOR_BLIT:
        case WINED3D_BLIT_OP_DEPTH_BLIT:
            break;
        case WINED3D_BLIT_OP_RAW_BLIT:
            flags |= WINED3D_BLT_RAW;
            break;
        case WINED3D_BLIT_OP_COLOR_BLIT_ALPHATEST:
            flags |= WINED3D_BLT_ALPHA_TEST;
            break;
        case WINED3D_BLIT_OP_COLOR_BLIT_CKEY:
            flags |= WINED3D_BLT_SRC_CKEY_OVERRIDE | WINED3D_BLT_FX;
            fx.src_color_key = *color_key;
            break;
        default:
            FIXME("Unhandled op %#x.\n", op);
            break;
    }

    if (FAILED(surface_cpu_blt(dst_texture, dst_sub_resource_idx, &dst_box,
            src_texture, src_sub_resource_idx, &src_box, flags, &fx, filter)))
        ERR("Failed to blit.\n");
    wined3d_texture_load_location(dst_texture, dst_sub_resource_idx, context, dst_location);

    return dst_location | (dst_texture->sub_resources[dst_sub_resource_idx].locations
            & dst_texture->resource.map_binding);
}

static const struct wined3d_blitter_ops cpu_blitter_ops =
{
    cpu_blitter_destroy,
    cpu_blitter_clear,
    cpu_blitter_blit,
};

struct wined3d_blitter *wined3d_cpu_blitter_create(void)
{
    struct wined3d_blitter *blitter;

    if (!(blitter = malloc(sizeof(*blitter))))
        return NULL;

    TRACE("Created blitter %p.\n", blitter);

    blitter->ops = &cpu_blitter_ops;
    blitter->next = NULL;

    return blitter;
}

static bool wined3d_is_colour_blit(enum wined3d_blit_op blit_op)
{
    switch (blit_op)
    {
        case WINED3D_BLIT_OP_COLOR_BLIT:
        case WINED3D_BLIT_OP_COLOR_BLIT_ALPHATEST:
        case WINED3D_BLIT_OP_COLOR_BLIT_CKEY:
            return true;

        default:
            return false;
    }
}

static bool sub_resource_is_on_cpu(const struct wined3d_texture *texture, unsigned int sub_resource_idx)
{
    DWORD locations = texture->sub_resources[sub_resource_idx].locations;

    if (locations & (WINED3D_LOCATION_BUFFER | WINED3D_LOCATION_SYSMEM))
        return true;

    if (!(texture->resource.access & WINED3D_RESOURCE_ACCESS_GPU) && (locations & WINED3D_LOCATION_CLEARED))
        return true;

    return false;
}

HRESULT texture2d_blt(struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx,
        const struct wined3d_box *dst_box, struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx,
        const struct wined3d_box *src_box, uint32_t flags, const struct wined3d_blt_fx *fx,
        enum wined3d_texture_filter_type filter)
{
    struct wined3d_texture_sub_resource *src_sub_resource, *dst_sub_resource;
    struct wined3d_device *device = dst_texture->resource.device;
    struct wined3d_swapchain *src_swapchain, *dst_swapchain;
    BOOL scale, convert, resolve, resolve_typeless = FALSE;
    const struct wined3d_format *resolve_format = NULL;
    const struct wined3d_color_key *colour_key = NULL;
    DWORD src_location, dst_location, valid_locations;
    struct wined3d_context *context;
    enum wined3d_blit_op blit_op;
    RECT src_rect, dst_rect;
    bool src_ds, dst_ds;

    static const DWORD simple_blit = WINED3D_BLT_SRC_CKEY
            | WINED3D_BLT_SRC_CKEY_OVERRIDE
            | WINED3D_BLT_ALPHA_TEST
            | WINED3D_BLT_RAW;

    TRACE("dst_texture %p, dst_sub_resource_idx %u, dst_box %s, src_texture %p, "
            "src_sub_resource_idx %u, src_box %s, flags %#x, fx %p, filter %s.\n",
            dst_texture, dst_sub_resource_idx, debug_box(dst_box), src_texture, src_sub_resource_idx,
            debug_box(src_box), flags, fx, debug_d3dtexturefiltertype(filter));
    TRACE("Usage is %s.\n", debug_d3dusage(dst_texture->resource.usage));

    if (fx)
    {
        TRACE("fx %#x.\n", fx->fx);
        TRACE("dst_color_key {0x%08x, 0x%08x}.\n",
                fx->dst_color_key.color_space_low_value,
                fx->dst_color_key.color_space_high_value);
        TRACE("src_color_key {0x%08x, 0x%08x}.\n",
                fx->src_color_key.color_space_low_value,
                fx->src_color_key.color_space_high_value);
        TRACE("resolve_format_id %s.\n", debug_d3dformat(fx->resolve_format_id));

        if (fx->resolve_format_id != WINED3DFMT_UNKNOWN)
            resolve_format = wined3d_get_format(device->adapter, fx->resolve_format_id, 0);
    }

    dst_sub_resource = &dst_texture->sub_resources[dst_sub_resource_idx];
    src_sub_resource = &src_texture->sub_resources[src_sub_resource_idx];

    if (src_sub_resource->locations & WINED3D_LOCATION_DISCARDED)
    {
        WARN("Source sub-resource is discarded, nothing to do.\n");
        return WINED3D_OK;
    }

    SetRect(&src_rect, src_box->left, src_box->top, src_box->right, src_box->bottom);
    SetRect(&dst_rect, dst_box->left, dst_box->top, dst_box->right, dst_box->bottom);

    if (!fx || !(fx->fx))
        flags &= ~WINED3D_BLT_FX;

    /* WINED3D_BLT_DO_NOT_WAIT appeared in DX7. */
    if (flags & WINED3D_BLT_DO_NOT_WAIT)
    {
        static unsigned int once;

        if (!once++)
            FIXME("Can't handle WINED3D_BLT_DO_NOT_WAIT flag.\n");
    }

    flags &= ~(WINED3D_BLT_SYNCHRONOUS | WINED3D_BLT_DO_NOT_WAIT | WINED3D_BLT_WAIT);

    if (flags & ~simple_blit)
    {
        WARN_(d3d_perf)("Using CPU fallback for complex blit (%#x).\n", flags);
        goto cpu;
    }

    src_swapchain = src_texture->swapchain;
    dst_swapchain = dst_texture->swapchain;

    if (src_swapchain && dst_swapchain && src_swapchain != dst_swapchain
            && src_texture == src_swapchain->front_buffer)
    {
        /* TODO: We could support cross-swapchain blits by first downloading
         * the source to a texture. */
        FIXME("Cross-swapchain blit not supported.\n");
        return WINED3DERR_INVALIDCALL;
    }

    scale = src_box->right - src_box->left != dst_box->right - dst_box->left
            || src_box->bottom - src_box->top != dst_box->bottom - dst_box->top;
    convert = src_texture->resource.format->id != dst_texture->resource.format->id;
    resolve = src_texture->resource.multisample_type != dst_texture->resource.multisample_type;
    if (resolve)
    {
        resolve_typeless = (wined3d_format_is_typeless(src_texture->resource.format)
                || wined3d_format_is_typeless(dst_texture->resource.format))
                && (src_texture->resource.format->typeless_id == dst_texture->resource.format->typeless_id);
        if (resolve_typeless && !resolve_format)
            WARN("Resolve format for typeless resolve not specified.\n");
    }

    dst_ds = dst_texture->resource.format->depth_size || dst_texture->resource.format->stencil_size;
    src_ds = src_texture->resource.format->depth_size || src_texture->resource.format->stencil_size;

    if (src_ds || dst_ds)
    {
        TRACE("Depth/stencil blit.\n");

        if (dst_texture->resource.access & WINED3D_RESOURCE_ACCESS_GPU)
            dst_location = dst_texture->resource.draw_binding;
        else
            dst_location = dst_texture->resource.map_binding;

        if ((flags & WINED3D_BLT_RAW) || (!scale && !convert && !resolve))
            blit_op = WINED3D_BLIT_OP_RAW_BLIT;
        else
            blit_op = WINED3D_BLIT_OP_DEPTH_BLIT;

        context = context_acquire(device, dst_texture, dst_sub_resource_idx);
        valid_locations = device->blitter->ops->blitter_blit(device->blitter, blit_op, context,
                src_texture, src_sub_resource_idx, src_texture->resource.draw_binding, &src_rect,
                dst_texture, dst_sub_resource_idx, dst_location, &dst_rect, NULL, filter, resolve_format);
        context_release(context);

        wined3d_texture_validate_location(dst_texture, dst_sub_resource_idx, valid_locations);
        wined3d_texture_invalidate_location(dst_texture, dst_sub_resource_idx, ~valid_locations);

        return WINED3D_OK;
    }

    TRACE("Colour blit.\n");

    /* In principle this would apply to depth blits as well, but we don't
     * implement those in the CPU blitter at the moment. */
    if ((dst_sub_resource->locations & dst_texture->resource.map_binding)
            && (src_sub_resource->locations & src_texture->resource.map_binding))
    {
        if (scale)
            TRACE("Not doing sysmem blit because of scaling.\n");
        else if (convert)
            TRACE("Not doing sysmem blit because of format conversion.\n");
        else
            goto cpu;
    }

    blit_op = WINED3D_BLIT_OP_COLOR_BLIT;
    if (flags & WINED3D_BLT_SRC_CKEY_OVERRIDE)
    {
        colour_key = &fx->src_color_key;
        blit_op = WINED3D_BLIT_OP_COLOR_BLIT_CKEY;
    }
    else if (flags & WINED3D_BLT_SRC_CKEY)
    {
        colour_key = &src_texture->async.src_blt_color_key;
        blit_op = WINED3D_BLIT_OP_COLOR_BLIT_CKEY;
    }
    else if (flags & WINED3D_BLT_ALPHA_TEST)
    {
        blit_op = WINED3D_BLIT_OP_COLOR_BLIT_ALPHATEST;
    }
    else if (sub_resource_is_on_cpu(src_texture, src_sub_resource_idx)
            && !sub_resource_is_on_cpu(dst_texture, dst_sub_resource_idx)
            && (dst_texture->resource.access & WINED3D_RESOURCE_ACCESS_GPU))
    {
        /* Upload */
        if (scale)
            TRACE("Not doing upload because of scaling.\n");
        else if (convert)
            TRACE("Not doing upload because of format conversion.\n");
        else if (dst_texture->resource.format->conv_byte_count)
            TRACE("Not doing upload because the destination format needs conversion.\n");
        else
        {
            wined3d_texture_upload_from_texture(dst_texture, dst_sub_resource_idx, dst_box->left,
                    dst_box->top, dst_box->front, src_texture, src_sub_resource_idx, src_box);
            if (!wined3d_resource_is_offscreen(&dst_texture->resource))
            {
                context = context_acquire(device, dst_texture, dst_sub_resource_idx);
                wined3d_texture_load_location(dst_texture, dst_sub_resource_idx,
                        context, dst_texture->resource.draw_binding);
                context_release(context);
            }
            return WINED3D_OK;
        }
    }
    else if (!sub_resource_is_on_cpu(src_texture, src_sub_resource_idx)
            && !(dst_texture->resource.access & WINED3D_RESOURCE_ACCESS_GPU))
    {
        /* Download */
        if (scale)
            TRACE("Not doing download because of scaling.\n");
        else if (convert)
            TRACE("Not doing download because of format conversion.\n");
        else if (src_texture->resource.format->conv_byte_count)
            TRACE("Not doing download because the source format needs conversion.\n");
        else if (!(src_texture->flags & WINED3D_TEXTURE_DOWNLOADABLE))
            TRACE("Not doing download because texture is not downloadable.\n");
        else if (!wined3d_texture_is_full_rect(src_texture, src_sub_resource_idx % src_texture->level_count, &src_rect))
            TRACE("Not doing download because of partial download (src).\n");
        else if (!wined3d_texture_is_full_rect(dst_texture, dst_sub_resource_idx % dst_texture->level_count, &dst_rect))
            TRACE("Not doing download because of partial download (dst).\n");
        else
        {
            wined3d_texture_download_from_texture(dst_texture, dst_sub_resource_idx, src_texture,
                    src_sub_resource_idx);
            return WINED3D_OK;
        }
    }
    else if (dst_swapchain && dst_swapchain->back_buffers
            && dst_texture == dst_swapchain->front_buffer
            && src_texture == dst_swapchain->back_buffers[0])
    {
        /* Use present for back -> front blits. The idea behind this is that
         * present is potentially faster than a blit, in particular when FBO
         * blits aren't available. Some ddraw applications like Half-Life and
         * Prince of Persia 3D use Blt() from the backbuffer to the
         * frontbuffer instead of doing a Flip(). D3d8 and d3d9 applications
         * can't blit directly to the frontbuffer. */
        enum wined3d_swap_effect swap_effect = dst_swapchain->state.desc.swap_effect;

        TRACE("Using present for backbuffer -> frontbuffer blit.\n");

        /* Set the swap effect to COPY, we don't want the backbuffer to become
         * undefined. */
        dst_swapchain->state.desc.swap_effect = WINED3D_SWAP_EFFECT_COPY;
        wined3d_swapchain_present(dst_swapchain, NULL, NULL,
                dst_swapchain->win_handle, dst_swapchain->swap_interval, 0);
        dst_swapchain->state.desc.swap_effect = swap_effect;

        return WINED3D_OK;
    }

    if ((flags & WINED3D_BLT_RAW) || (blit_op == WINED3D_BLIT_OP_COLOR_BLIT && !scale && !convert && !resolve))
        blit_op = WINED3D_BLIT_OP_RAW_BLIT;

    context = context_acquire(device, dst_texture, dst_sub_resource_idx);

    if (src_texture->resource.multisample_type != WINED3D_MULTISAMPLE_NONE && !resolve_typeless
            && ((scale && !context->d3d_info->scaled_resolve)
            || convert || !wined3d_is_colour_blit(blit_op)))
        src_location = WINED3D_LOCATION_RB_RESOLVED;
    else
        src_location = src_texture->resource.draw_binding;

    if (!(dst_texture->resource.access & WINED3D_RESOURCE_ACCESS_GPU))
        dst_location = dst_texture->resource.map_binding;
    else if (dst_texture->resource.multisample_type != WINED3D_MULTISAMPLE_NONE
            && (scale || convert || !wined3d_is_colour_blit(blit_op)))
        dst_location = WINED3D_LOCATION_RB_RESOLVED;
    else
        dst_location = dst_texture->resource.draw_binding;

    valid_locations = device->blitter->ops->blitter_blit(device->blitter, blit_op, context,
            src_texture, src_sub_resource_idx, src_location, &src_rect,
            dst_texture, dst_sub_resource_idx, dst_location, &dst_rect, colour_key, filter, resolve_format);

    context_release(context);

    wined3d_texture_validate_location(dst_texture, dst_sub_resource_idx, valid_locations);
    wined3d_texture_invalidate_location(dst_texture, dst_sub_resource_idx, ~valid_locations);

    return WINED3D_OK;

cpu:
    return surface_cpu_blt(dst_texture, dst_sub_resource_idx, dst_box,
            src_texture, src_sub_resource_idx, src_box, flags, fx, filter);
}
