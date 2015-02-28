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

WINE_DEFAULT_DEBUG_CHANNEL(d3d_surface);
WINE_DECLARE_DEBUG_CHANNEL(d3d_perf);
WINE_DECLARE_DEBUG_CHANNEL(d3d);

#define MAXLOCKCOUNT 50 /* After this amount of locks do not free the sysmem copy. */

static const DWORD surface_simple_locations =
        WINED3D_LOCATION_SYSMEM | WINED3D_LOCATION_USER_MEMORY
        | WINED3D_LOCATION_DIB | WINED3D_LOCATION_BUFFER;

static void surface_cleanup(struct wined3d_surface *surface)
{
    struct wined3d_surface *overlay, *cur;

    TRACE("surface %p.\n", surface);

    if (surface->pbo || surface->rb_multisample
            || surface->rb_resolved || !list_empty(&surface->renderbuffers))
    {
        struct wined3d_renderbuffer_entry *entry, *entry2;
        const struct wined3d_gl_info *gl_info;
        struct wined3d_context *context;

        context = context_acquire(surface->resource.device, NULL);
        gl_info = context->gl_info;

        if (surface->pbo)
        {
            TRACE("Deleting PBO %u.\n", surface->pbo);
            GL_EXTCALL(glDeleteBuffers(1, &surface->pbo));
        }

        if (surface->rb_multisample)
        {
            TRACE("Deleting multisample renderbuffer %u.\n", surface->rb_multisample);
            gl_info->fbo_ops.glDeleteRenderbuffers(1, &surface->rb_multisample);
        }

        if (surface->rb_resolved)
        {
            TRACE("Deleting resolved renderbuffer %u.\n", surface->rb_resolved);
            gl_info->fbo_ops.glDeleteRenderbuffers(1, &surface->rb_resolved);
        }

        LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &surface->renderbuffers, struct wined3d_renderbuffer_entry, entry)
        {
            TRACE("Deleting renderbuffer %u.\n", entry->id);
            gl_info->fbo_ops.glDeleteRenderbuffers(1, &entry->id);
            HeapFree(GetProcessHeap(), 0, entry);
        }

        context_release(context);
    }

    if (surface->flags & SFLAG_DIBSECTION)
    {
        DeleteDC(surface->hDC);
        DeleteObject(surface->dib.DIBsection);
        surface->dib.bitmap_data = NULL;
    }

    if (surface->overlay_dest)
        list_remove(&surface->overlay_entry);

    LIST_FOR_EACH_ENTRY_SAFE(overlay, cur, &surface->overlays, struct wined3d_surface, overlay_entry)
    {
        list_remove(&overlay->overlay_entry);
        overlay->overlay_dest = NULL;
    }

    resource_cleanup(&surface->resource);
}

void wined3d_surface_destroy(struct wined3d_surface *surface)
{
    TRACE("surface %p.\n", surface);

    surface_cleanup(surface);
    surface->resource.parent_ops->wined3d_object_destroyed(surface->resource.parent);
    HeapFree(GetProcessHeap(), 0, surface);
}

void surface_get_drawable_size(const struct wined3d_surface *surface, const struct wined3d_context *context,
        unsigned int *width, unsigned int *height)
{
    if (surface->container->swapchain)
    {
        /* The drawable size of an onscreen drawable is the surface size.
         * (Actually: The window size, but the surface is created in window
         * size.) */
        *width = context->current_rt->resource.width;
        *height = context->current_rt->resource.height;
    }
    else if (wined3d_settings.offscreen_rendering_mode == ORM_BACKBUFFER)
    {
        const struct wined3d_swapchain *swapchain = context->swapchain;

        /* The drawable size of a backbuffer / aux buffer offscreen target is
         * the size of the current context's drawable, which is the size of
         * the back buffer of the swapchain the active context belongs to. */
        *width = swapchain->desc.backbuffer_width;
        *height = swapchain->desc.backbuffer_height;
    }
    else
    {
        /* The drawable size of an FBO target is the OpenGL texture size,
         * which is the power of two size. */
        *width = context->current_rt->pow2Width;
        *height = context->current_rt->pow2Height;
    }
}

struct blt_info
{
    GLenum binding;
    GLenum bind_target;
    enum tex_types tex_type;
    GLfloat coords[4][3];
};

struct float_rect
{
    float l;
    float t;
    float r;
    float b;
};

static inline void cube_coords_float(const RECT *r, UINT w, UINT h, struct float_rect *f)
{
    f->l = ((r->left * 2.0f) / w) - 1.0f;
    f->t = ((r->top * 2.0f) / h) - 1.0f;
    f->r = ((r->right * 2.0f) / w) - 1.0f;
    f->b = ((r->bottom * 2.0f) / h) - 1.0f;
}

static void surface_get_blt_info(GLenum target, const RECT *rect, GLsizei w, GLsizei h, struct blt_info *info)
{
    GLfloat (*coords)[3] = info->coords;
    struct float_rect f;

    switch (target)
    {
        default:
            FIXME("Unsupported texture target %#x\n", target);
            /* Fall back to GL_TEXTURE_2D */
        case GL_TEXTURE_2D:
            info->binding = GL_TEXTURE_BINDING_2D;
            info->bind_target = GL_TEXTURE_2D;
            info->tex_type = tex_2d;
            coords[0][0] = (float)rect->left / w;
            coords[0][1] = (float)rect->top / h;
            coords[0][2] = 0.0f;

            coords[1][0] = (float)rect->right / w;
            coords[1][1] = (float)rect->top / h;
            coords[1][2] = 0.0f;

            coords[2][0] = (float)rect->left / w;
            coords[2][1] = (float)rect->bottom / h;
            coords[2][2] = 0.0f;

            coords[3][0] = (float)rect->right / w;
            coords[3][1] = (float)rect->bottom / h;
            coords[3][2] = 0.0f;
            break;

        case GL_TEXTURE_RECTANGLE_ARB:
            info->binding = GL_TEXTURE_BINDING_RECTANGLE_ARB;
            info->bind_target = GL_TEXTURE_RECTANGLE_ARB;
            info->tex_type = tex_rect;
            coords[0][0] = rect->left;  coords[0][1] = rect->top;       coords[0][2] = 0.0f;
            coords[1][0] = rect->right; coords[1][1] = rect->top;       coords[1][2] = 0.0f;
            coords[2][0] = rect->left;  coords[2][1] = rect->bottom;    coords[2][2] = 0.0f;
            coords[3][0] = rect->right; coords[3][1] = rect->bottom;    coords[3][2] = 0.0f;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
            info->binding = GL_TEXTURE_BINDING_CUBE_MAP_ARB;
            info->bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            info->tex_type = tex_cube;
            cube_coords_float(rect, w, h, &f);

            coords[0][0] =  1.0f;   coords[0][1] = -f.t;   coords[0][2] = -f.l;
            coords[1][0] =  1.0f;   coords[1][1] = -f.t;   coords[1][2] = -f.r;
            coords[2][0] =  1.0f;   coords[2][1] = -f.b;   coords[2][2] = -f.l;
            coords[3][0] =  1.0f;   coords[3][1] = -f.b;   coords[3][2] = -f.r;
            break;

        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
            info->binding = GL_TEXTURE_BINDING_CUBE_MAP_ARB;
            info->bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            info->tex_type = tex_cube;
            cube_coords_float(rect, w, h, &f);

            coords[0][0] = -1.0f;   coords[0][1] = -f.t;   coords[0][2] = f.l;
            coords[1][0] = -1.0f;   coords[1][1] = -f.t;   coords[1][2] = f.r;
            coords[2][0] = -1.0f;   coords[2][1] = -f.b;   coords[2][2] = f.l;
            coords[3][0] = -1.0f;   coords[3][1] = -f.b;   coords[3][2] = f.r;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
            info->binding = GL_TEXTURE_BINDING_CUBE_MAP_ARB;
            info->bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            info->tex_type = tex_cube;
            cube_coords_float(rect, w, h, &f);

            coords[0][0] = f.l;   coords[0][1] =  1.0f;   coords[0][2] = f.t;
            coords[1][0] = f.r;   coords[1][1] =  1.0f;   coords[1][2] = f.t;
            coords[2][0] = f.l;   coords[2][1] =  1.0f;   coords[2][2] = f.b;
            coords[3][0] = f.r;   coords[3][1] =  1.0f;   coords[3][2] = f.b;
            break;

        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
            info->binding = GL_TEXTURE_BINDING_CUBE_MAP_ARB;
            info->bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            info->tex_type = tex_cube;
            cube_coords_float(rect, w, h, &f);

            coords[0][0] = f.l;   coords[0][1] = -1.0f;   coords[0][2] = -f.t;
            coords[1][0] = f.r;   coords[1][1] = -1.0f;   coords[1][2] = -f.t;
            coords[2][0] = f.l;   coords[2][1] = -1.0f;   coords[2][2] = -f.b;
            coords[3][0] = f.r;   coords[3][1] = -1.0f;   coords[3][2] = -f.b;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
            info->binding = GL_TEXTURE_BINDING_CUBE_MAP_ARB;
            info->bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            info->tex_type = tex_cube;
            cube_coords_float(rect, w, h, &f);

            coords[0][0] = f.l;   coords[0][1] = -f.t;   coords[0][2] =  1.0f;
            coords[1][0] = f.r;   coords[1][1] = -f.t;   coords[1][2] =  1.0f;
            coords[2][0] = f.l;   coords[2][1] = -f.b;   coords[2][2] =  1.0f;
            coords[3][0] = f.r;   coords[3][1] = -f.b;   coords[3][2] =  1.0f;
            break;

        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            info->binding = GL_TEXTURE_BINDING_CUBE_MAP_ARB;
            info->bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            info->tex_type = tex_cube;
            cube_coords_float(rect, w, h, &f);

            coords[0][0] = -f.l;   coords[0][1] = -f.t;   coords[0][2] = -1.0f;
            coords[1][0] = -f.r;   coords[1][1] = -f.t;   coords[1][2] = -1.0f;
            coords[2][0] = -f.l;   coords[2][1] = -f.b;   coords[2][2] = -1.0f;
            coords[3][0] = -f.r;   coords[3][1] = -f.b;   coords[3][2] = -1.0f;
            break;
    }
}

static void surface_get_rect(const struct wined3d_surface *surface, const RECT *rect_in, RECT *rect_out)
{
    if (rect_in)
        *rect_out = *rect_in;
    else
    {
        rect_out->left = 0;
        rect_out->top = 0;
        rect_out->right = surface->resource.width;
        rect_out->bottom = surface->resource.height;
    }
}

/* Context activation is done by the caller. */
void draw_textured_quad(const struct wined3d_surface *src_surface, struct wined3d_context *context,
        const RECT *src_rect, const RECT *dst_rect, enum wined3d_texture_filter_type filter)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    struct wined3d_texture *texture = src_surface->container;
    struct blt_info info;

    surface_get_blt_info(src_surface->texture_target, src_rect, src_surface->pow2Width, src_surface->pow2Height, &info);

    gl_info->gl_ops.gl.p_glEnable(info.bind_target);
    checkGLcall("glEnable(bind_target)");

    context_bind_texture(context, info.bind_target, texture->texture_rgb.name);

    /* Filtering for StretchRect */
    gl_info->gl_ops.gl.p_glTexParameteri(info.bind_target, GL_TEXTURE_MAG_FILTER, wined3d_gl_mag_filter(filter));
    checkGLcall("glTexParameteri");
    gl_info->gl_ops.gl.p_glTexParameteri(info.bind_target, GL_TEXTURE_MIN_FILTER,
            wined3d_gl_min_mip_filter(filter, WINED3D_TEXF_NONE));
    checkGLcall("glTexParameteri");
    gl_info->gl_ops.gl.p_glTexParameteri(info.bind_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl_info->gl_ops.gl.p_glTexParameteri(info.bind_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (context->gl_info->supported[EXT_TEXTURE_SRGB_DECODE])
        gl_info->gl_ops.gl.p_glTexParameteri(info.bind_target, GL_TEXTURE_SRGB_DECODE_EXT, GL_SKIP_DECODE_EXT);
    gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    checkGLcall("glTexEnvi");

    /* Draw a quad */
    gl_info->gl_ops.gl.p_glBegin(GL_TRIANGLE_STRIP);
    gl_info->gl_ops.gl.p_glTexCoord3fv(info.coords[0]);
    gl_info->gl_ops.gl.p_glVertex2i(dst_rect->left, dst_rect->top);

    gl_info->gl_ops.gl.p_glTexCoord3fv(info.coords[1]);
    gl_info->gl_ops.gl.p_glVertex2i(dst_rect->right, dst_rect->top);

    gl_info->gl_ops.gl.p_glTexCoord3fv(info.coords[2]);
    gl_info->gl_ops.gl.p_glVertex2i(dst_rect->left, dst_rect->bottom);

    gl_info->gl_ops.gl.p_glTexCoord3fv(info.coords[3]);
    gl_info->gl_ops.gl.p_glVertex2i(dst_rect->right, dst_rect->bottom);
    gl_info->gl_ops.gl.p_glEnd();

    /* Unbind the texture */
    context_bind_texture(context, info.bind_target, 0);

    /* We changed the filtering settings on the texture. Inform the
     * container about this to get the filters reset properly next draw. */
    texture->texture_rgb.sampler_desc.mag_filter = WINED3D_TEXF_POINT;
    texture->texture_rgb.sampler_desc.min_filter = WINED3D_TEXF_POINT;
    texture->texture_rgb.sampler_desc.mip_filter = WINED3D_TEXF_NONE;
    texture->texture_rgb.sampler_desc.srgb_decode = FALSE;
}

/* Works correctly only for <= 4 bpp formats. */
static void get_color_masks(const struct wined3d_format *format, DWORD *masks)
{
    masks[0] = ((1 << format->red_size) - 1) << format->red_offset;
    masks[1] = ((1 << format->green_size) - 1) << format->green_offset;
    masks[2] = ((1 << format->blue_size) - 1) << format->blue_offset;
}

static HRESULT surface_create_dib_section(struct wined3d_surface *surface)
{
    const struct wined3d_format *format = surface->resource.format;
    SYSTEM_INFO sysInfo;
    BITMAPINFO *b_info;
    int extraline = 0;
    DWORD *masks;

    TRACE("surface %p.\n", surface);

    if (!(format->flags & WINED3DFMT_FLAG_GETDC))
    {
        WARN("Cannot use GetDC on a %s surface.\n", debug_d3dformat(format->id));
        return WINED3DERR_INVALIDCALL;
    }

    switch (format->byte_count)
    {
        case 2:
        case 4:
            /* Allocate extra space to store the RGB bit masks. */
            b_info = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BITMAPINFOHEADER) + 3 * sizeof(DWORD));
            break;

        case 3:
            b_info = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BITMAPINFOHEADER));
            break;

        default:
            /* Allocate extra space for a palette. */
            b_info = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                    sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (1 << (format->byte_count * 8)));
            break;
    }

    if (!b_info)
        return E_OUTOFMEMORY;

    /* Some applications access the surface in via DWORDs, and do not take
     * the necessary care at the end of the surface. So we need at least
     * 4 extra bytes at the end of the surface. Check against the page size,
     * if the last page used for the surface has at least 4 spare bytes we're
     * safe, otherwise add an extra line to the DIB section. */
    GetSystemInfo(&sysInfo);
    if( ((surface->resource.size + 3) % sysInfo.dwPageSize) < 4)
    {
        extraline = 1;
        TRACE("Adding an extra line to the DIB section.\n");
    }

    b_info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    /* TODO: Is there a nicer way to force a specific alignment? (8 byte for ddraw) */
    b_info->bmiHeader.biWidth = wined3d_surface_get_pitch(surface) / format->byte_count;
    b_info->bmiHeader.biHeight = 0 - surface->resource.height - extraline;
    b_info->bmiHeader.biSizeImage = (surface->resource.height + extraline)
            * wined3d_surface_get_pitch(surface);
    b_info->bmiHeader.biPlanes = 1;
    b_info->bmiHeader.biBitCount = format->byte_count * 8;

    b_info->bmiHeader.biXPelsPerMeter = 0;
    b_info->bmiHeader.biYPelsPerMeter = 0;
    b_info->bmiHeader.biClrUsed = 0;
    b_info->bmiHeader.biClrImportant = 0;

    /* Get the bit masks */
    masks = (DWORD *)b_info->bmiColors;
    switch (surface->resource.format->id)
    {
        case WINED3DFMT_B8G8R8_UNORM:
            b_info->bmiHeader.biCompression = BI_RGB;
            break;

        case WINED3DFMT_B5G5R5X1_UNORM:
        case WINED3DFMT_B5G5R5A1_UNORM:
        case WINED3DFMT_B4G4R4A4_UNORM:
        case WINED3DFMT_B4G4R4X4_UNORM:
        case WINED3DFMT_B2G3R3_UNORM:
        case WINED3DFMT_B2G3R3A8_UNORM:
        case WINED3DFMT_R10G10B10A2_UNORM:
        case WINED3DFMT_R8G8B8A8_UNORM:
        case WINED3DFMT_R8G8B8X8_UNORM:
        case WINED3DFMT_B10G10R10A2_UNORM:
        case WINED3DFMT_B5G6R5_UNORM:
        case WINED3DFMT_R16G16B16A16_UNORM:
            b_info->bmiHeader.biCompression = BI_BITFIELDS;
            get_color_masks(format, masks);
            break;

        default:
            /* Don't know palette */
            b_info->bmiHeader.biCompression = BI_RGB;
            break;
    }

    TRACE("Creating a DIB section with size %dx%dx%d, size=%d.\n",
            b_info->bmiHeader.biWidth, b_info->bmiHeader.biHeight,
            b_info->bmiHeader.biBitCount, b_info->bmiHeader.biSizeImage);
    surface->dib.DIBsection = CreateDIBSection(0, b_info, DIB_RGB_COLORS, &surface->dib.bitmap_data, 0, 0);

    if (!surface->dib.DIBsection)
    {
        ERR("Failed to create DIB section.\n");
        HeapFree(GetProcessHeap(), 0, b_info);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    TRACE("DIBSection at %p.\n", surface->dib.bitmap_data);
    surface->dib.bitmap_size = b_info->bmiHeader.biSizeImage;

    HeapFree(GetProcessHeap(), 0, b_info);

    /* Now allocate a DC. */
    surface->hDC = CreateCompatibleDC(0);
    SelectObject(surface->hDC, surface->dib.DIBsection);

    surface->flags |= SFLAG_DIBSECTION;

    return WINED3D_OK;
}

static void surface_get_memory(const struct wined3d_surface *surface, struct wined3d_bo_address *data,
        DWORD location)
{
    if (location & WINED3D_LOCATION_BUFFER)
    {
        data->addr = NULL;
        data->buffer_object = surface->pbo;
        return;
    }
    if (location & WINED3D_LOCATION_USER_MEMORY)
    {
        data->addr = surface->user_memory;
        data->buffer_object = 0;
        return;
    }
    if (location & WINED3D_LOCATION_DIB)
    {
        data->addr = surface->dib.bitmap_data;
        data->buffer_object = 0;
        return;
    }
    if (location & WINED3D_LOCATION_SYSMEM)
    {
        data->addr = surface->resource.heap_memory;
        data->buffer_object = 0;
        return;
    }

    ERR("Unexpected locations %s.\n", wined3d_debug_location(location));
    data->addr = NULL;
    data->buffer_object = 0;
}

static void surface_prepare_buffer(struct wined3d_surface *surface)
{
    struct wined3d_context *context;
    GLenum error;
    const struct wined3d_gl_info *gl_info;

    if (surface->pbo)
        return;

    context = context_acquire(surface->resource.device, NULL);
    gl_info = context->gl_info;

    GL_EXTCALL(glGenBuffers(1, &surface->pbo));
    error = gl_info->gl_ops.gl.p_glGetError();
    if (!surface->pbo || error != GL_NO_ERROR)
        ERR("Failed to create a PBO with error %s (%#x).\n", debug_glerror(error), error);

    TRACE("Binding PBO %u.\n", surface->pbo);

    GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, surface->pbo));
    checkGLcall("glBindBuffer");

    GL_EXTCALL(glBufferData(GL_PIXEL_UNPACK_BUFFER, surface->resource.size + 4,
            NULL, GL_STREAM_DRAW));
    checkGLcall("glBufferData");

    GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
    checkGLcall("glBindBuffer");

    context_release(context);
}

static void surface_prepare_system_memory(struct wined3d_surface *surface)
{
    TRACE("surface %p.\n", surface);

    if (surface->resource.heap_memory)
        return;

    /* Whatever surface we have, make sure that there is memory allocated
     * for the downloaded copy, or a PBO to map. */
    if (!wined3d_resource_allocate_sysmem(&surface->resource))
        ERR("Failed to allocate system memory.\n");

    if (surface->locations & WINED3D_LOCATION_SYSMEM)
        ERR("Surface without system memory has WINED3D_LOCATION_SYSMEM set.\n");
}

void surface_prepare_map_memory(struct wined3d_surface *surface)
{
    switch (surface->resource.map_binding)
    {
        case WINED3D_LOCATION_SYSMEM:
            surface_prepare_system_memory(surface);
            break;

        case WINED3D_LOCATION_USER_MEMORY:
            if (!surface->user_memory)
                ERR("Map binding is set to WINED3D_LOCATION_USER_MEMORY but surface->user_memory is NULL.\n");
            break;

        case WINED3D_LOCATION_DIB:
            if (!surface->dib.bitmap_data)
                ERR("Map binding is set to WINED3D_LOCATION_DIB but surface->dib.bitmap_data is NULL.\n");
            break;

        case WINED3D_LOCATION_BUFFER:
            surface_prepare_buffer(surface);
            break;

        default:
            ERR("Unexpected map binding %s.\n", wined3d_debug_location(surface->resource.map_binding));
    }
}

static void surface_evict_sysmem(struct wined3d_surface *surface)
{
    /* In some conditions the surface memory must not be freed:
     * WINED3D_TEXTURE_CONVERTED: Converting the data back would take too long
     * WINED3D_TEXTURE_DYNAMIC_MAP: Avoid freeing the data for performance
     * SFLAG_CLIENT: OpenGL uses our memory as backup */
    if (surface->resource.map_count || surface->flags & SFLAG_CLIENT
            || surface->container->flags & (WINED3D_TEXTURE_CONVERTED | WINED3D_TEXTURE_PIN_SYSMEM
            | WINED3D_TEXTURE_DYNAMIC_MAP))
        return;

    wined3d_resource_free_sysmem(&surface->resource);
    surface_invalidate_location(surface, WINED3D_LOCATION_SYSMEM);
}

static void surface_release_client_storage(struct wined3d_surface *surface)
{
    struct wined3d_context *context = context_acquire(surface->resource.device, NULL);
    const struct wined3d_gl_info *gl_info = context->gl_info;

    if (surface->container->texture_rgb.name)
    {
        wined3d_texture_bind_and_dirtify(surface->container, context, FALSE);
        gl_info->gl_ops.gl.p_glTexImage2D(surface->texture_target, surface->texture_level,
                GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    }
    if (surface->container->texture_srgb.name)
    {
        wined3d_texture_bind_and_dirtify(surface->container, context, TRUE);
        gl_info->gl_ops.gl.p_glTexImage2D(surface->texture_target, surface->texture_level,
                GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    }
    wined3d_texture_force_reload(surface->container);

    context_release(context);
}

static BOOL surface_use_pbo(const struct wined3d_surface *surface)
{
    const struct wined3d_gl_info *gl_info = &surface->resource.device->adapter->gl_info;
    struct wined3d_texture *texture = surface->container;

    return texture->resource.pool == WINED3D_POOL_DEFAULT
                && surface->resource.access_flags & WINED3D_RESOURCE_ACCESS_CPU
                && gl_info->supported[ARB_PIXEL_BUFFER_OBJECT]
                && !texture->resource.format->convert
                && !(texture->flags & WINED3D_TEXTURE_PIN_SYSMEM)
                && !(surface->flags & SFLAG_NONPOW2);
}

static HRESULT surface_private_setup(struct wined3d_surface *surface)
{
    /* TODO: Check against the maximum texture sizes supported by the video card. */
    const struct wined3d_gl_info *gl_info = &surface->resource.device->adapter->gl_info;
    unsigned int pow2Width, pow2Height;

    TRACE("surface %p.\n", surface);

    /* Non-power2 support */
    if (gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO] || gl_info->supported[WINED3D_GL_NORMALIZED_TEXRECT]
            || gl_info->supported[ARB_TEXTURE_RECTANGLE])
    {
        pow2Width = surface->resource.width;
        pow2Height = surface->resource.height;
    }
    else
    {
        /* Find the nearest pow2 match */
        pow2Width = pow2Height = 1;
        while (pow2Width < surface->resource.width)
            pow2Width <<= 1;
        while (pow2Height < surface->resource.height)
            pow2Height <<= 1;
    }
    surface->pow2Width = pow2Width;
    surface->pow2Height = pow2Height;

    if (pow2Width > surface->resource.width || pow2Height > surface->resource.height)
    {
        /* TODO: Add support for non power two compressed textures. */
        if (surface->resource.format->flags & (WINED3DFMT_FLAG_COMPRESSED | WINED3DFMT_FLAG_HEIGHT_SCALE))
        {
            FIXME("(%p) Compressed or height scaled non-power-two textures are not supported w(%d) h(%d)\n",
                  surface, surface->resource.width, surface->resource.height);
            return WINED3DERR_NOTAVAILABLE;
        }
    }

    if (pow2Width != surface->resource.width
            || pow2Height != surface->resource.height)
    {
        surface->flags |= SFLAG_NONPOW2;
    }

    if ((surface->pow2Width > gl_info->limits.texture_size || surface->pow2Height > gl_info->limits.texture_size)
            && !(surface->resource.usage & (WINED3DUSAGE_RENDERTARGET | WINED3DUSAGE_DEPTHSTENCIL)))
    {
        /* One of three options:
         * 1: Do the same as we do with NPOT and scale the texture, (any
         *    texture ops would require the texture to be scaled which is
         *    potentially slow)
         * 2: Set the texture to the maximum size (bad idea).
         * 3: WARN and return WINED3DERR_NOTAVAILABLE;
         * 4: Create the surface, but allow it to be used only for DirectDraw
         *    Blts. Some apps (e.g. Swat 3) create textures with a Height of
         *    16 and a Width > 3000 and blt 16x16 letter areas from them to
         *    the render target. */
        if (surface->resource.pool == WINED3D_POOL_DEFAULT || surface->resource.pool == WINED3D_POOL_MANAGED)
        {
            WARN("Unable to allocate a surface which exceeds the maximum OpenGL texture size.\n");
            return WINED3DERR_NOTAVAILABLE;
        }

        /* We should never use this surface in combination with OpenGL! */
        TRACE("Creating an oversized surface: %ux%u.\n",
                surface->pow2Width, surface->pow2Height);
    }

    if (surface->resource.usage & WINED3DUSAGE_DEPTHSTENCIL)
        surface->locations = WINED3D_LOCATION_DISCARDED;

    if (surface_use_pbo(surface))
        surface->resource.map_binding = WINED3D_LOCATION_BUFFER;

    return WINED3D_OK;
}

static void surface_unmap(struct wined3d_surface *surface)
{
    struct wined3d_device *device = surface->resource.device;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;

    TRACE("surface %p.\n", surface);

    memset(&surface->lockedRect, 0, sizeof(surface->lockedRect));

    switch (surface->resource.map_binding)
    {
        case WINED3D_LOCATION_SYSMEM:
        case WINED3D_LOCATION_USER_MEMORY:
        case WINED3D_LOCATION_DIB:
            break;

        case WINED3D_LOCATION_BUFFER:
            context = context_acquire(device, NULL);
            gl_info = context->gl_info;

            GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, surface->pbo));
            GL_EXTCALL(glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER));
            GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
            checkGLcall("glUnmapBuffer");
            context_release(context);
            break;

        default:
            ERR("Unexpected map binding %s.\n", wined3d_debug_location(surface->resource.map_binding));
    }

    if (surface->locations & (WINED3D_LOCATION_DRAWABLE | WINED3D_LOCATION_TEXTURE_RGB))
    {
        TRACE("Not dirtified, nothing to do.\n");
        return;
    }

    if (surface->container->swapchain && surface->container->swapchain->front_buffer == surface->container)
        surface_load_location(surface, surface->container->resource.draw_binding);
    else if (surface->resource.format->flags & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL))
        FIXME("Depth / stencil buffer locking is not implemented.\n");
}

static BOOL surface_is_full_rect(const struct wined3d_surface *surface, const RECT *r)
{
    if ((r->left && r->right) || abs(r->right - r->left) != surface->resource.width)
        return FALSE;
    if ((r->top && r->bottom) || abs(r->bottom - r->top) != surface->resource.height)
        return FALSE;
    return TRUE;
}

static void surface_depth_blt_fbo(const struct wined3d_device *device,
        struct wined3d_surface *src_surface, DWORD src_location, const RECT *src_rect,
        struct wined3d_surface *dst_surface, DWORD dst_location, const RECT *dst_rect)
{
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;
    DWORD src_mask, dst_mask;
    GLbitfield gl_mask;

    TRACE("device %p\n", device);
    TRACE("src_surface %p, src_location %s, src_rect %s,\n",
            src_surface, wined3d_debug_location(src_location), wine_dbgstr_rect(src_rect));
    TRACE("dst_surface %p, dst_location %s, dst_rect %s.\n",
            dst_surface, wined3d_debug_location(dst_location), wine_dbgstr_rect(dst_rect));

    src_mask = src_surface->resource.format->flags & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL);
    dst_mask = dst_surface->resource.format->flags & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL);

    if (src_mask != dst_mask)
    {
        ERR("Incompatible formats %s and %s.\n",
                debug_d3dformat(src_surface->resource.format->id),
                debug_d3dformat(dst_surface->resource.format->id));
        return;
    }

    if (!src_mask)
    {
        ERR("Not a depth / stencil format: %s.\n",
                debug_d3dformat(src_surface->resource.format->id));
        return;
    }

    gl_mask = 0;
    if (src_mask & WINED3DFMT_FLAG_DEPTH)
        gl_mask |= GL_DEPTH_BUFFER_BIT;
    if (src_mask & WINED3DFMT_FLAG_STENCIL)
        gl_mask |= GL_STENCIL_BUFFER_BIT;

    /* Make sure the locations are up-to-date. Loading the destination
     * surface isn't required if the entire surface is overwritten. */
    surface_load_location(src_surface, src_location);
    if (!surface_is_full_rect(dst_surface, dst_rect))
        surface_load_location(dst_surface, dst_location);

    context = context_acquire(device, NULL);
    if (!context->valid)
    {
        context_release(context);
        WARN("Invalid context, skipping blit.\n");
        return;
    }

    gl_info = context->gl_info;

    context_apply_fbo_state_blit(context, GL_READ_FRAMEBUFFER, NULL, src_surface, src_location);
    context_check_fbo_status(context, GL_READ_FRAMEBUFFER);

    context_apply_fbo_state_blit(context, GL_DRAW_FRAMEBUFFER, NULL, dst_surface, dst_location);
    context_set_draw_buffer(context, GL_NONE);
    context_check_fbo_status(context, GL_DRAW_FRAMEBUFFER);
    context_invalidate_state(context, STATE_FRAMEBUFFER);

    if (gl_mask & GL_DEPTH_BUFFER_BIT)
    {
        gl_info->gl_ops.gl.p_glDepthMask(GL_TRUE);
        context_invalidate_state(context, STATE_RENDER(WINED3D_RS_ZWRITEENABLE));
    }
    if (gl_mask & GL_STENCIL_BUFFER_BIT)
    {
        if (context->gl_info->supported[EXT_STENCIL_TWO_SIDE])
        {
            gl_info->gl_ops.gl.p_glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
            context_invalidate_state(context, STATE_RENDER(WINED3D_RS_TWOSIDEDSTENCILMODE));
        }
        gl_info->gl_ops.gl.p_glStencilMask(~0U);
        context_invalidate_state(context, STATE_RENDER(WINED3D_RS_STENCILWRITEMASK));
    }

    gl_info->gl_ops.gl.p_glDisable(GL_SCISSOR_TEST);
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_SCISSORTESTENABLE));

    gl_info->fbo_ops.glBlitFramebuffer(src_rect->left, src_rect->top, src_rect->right, src_rect->bottom,
            dst_rect->left, dst_rect->top, dst_rect->right, dst_rect->bottom, gl_mask, GL_NEAREST);
    checkGLcall("glBlitFramebuffer()");

    if (wined3d_settings.strict_draw_ordering)
        gl_info->gl_ops.gl.p_glFlush(); /* Flush to ensure ordering across contexts. */

    context_release(context);
}

/* Blit between surface locations. Onscreen on different swapchains is not supported.
 * Depth / stencil is not supported. */
static void surface_blt_fbo(const struct wined3d_device *device, enum wined3d_texture_filter_type filter,
        struct wined3d_surface *src_surface, DWORD src_location, const RECT *src_rect_in,
        struct wined3d_surface *dst_surface, DWORD dst_location, const RECT *dst_rect_in)
{
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;
    RECT src_rect, dst_rect;
    GLenum gl_filter;
    GLenum buffer;

    TRACE("device %p, filter %s,\n", device, debug_d3dtexturefiltertype(filter));
    TRACE("src_surface %p, src_location %s, src_rect %s,\n",
            src_surface, wined3d_debug_location(src_location), wine_dbgstr_rect(src_rect_in));
    TRACE("dst_surface %p, dst_location %s, dst_rect %s.\n",
            dst_surface, wined3d_debug_location(dst_location), wine_dbgstr_rect(dst_rect_in));

    src_rect = *src_rect_in;
    dst_rect = *dst_rect_in;

    switch (filter)
    {
        case WINED3D_TEXF_LINEAR:
            gl_filter = GL_LINEAR;
            break;

        default:
            FIXME("Unsupported filter mode %s (%#x).\n", debug_d3dtexturefiltertype(filter), filter);
        case WINED3D_TEXF_NONE:
        case WINED3D_TEXF_POINT:
            gl_filter = GL_NEAREST;
            break;
    }

    /* Resolve the source surface first if needed. */
    if (src_location == WINED3D_LOCATION_RB_MULTISAMPLE
            && (src_surface->resource.format->id != dst_surface->resource.format->id
                || abs(src_rect.bottom - src_rect.top) != abs(dst_rect.bottom - dst_rect.top)
                || abs(src_rect.right - src_rect.left) != abs(dst_rect.right - dst_rect.left)))
        src_location = WINED3D_LOCATION_RB_RESOLVED;

    /* Make sure the locations are up-to-date. Loading the destination
     * surface isn't required if the entire surface is overwritten. (And is
     * in fact harmful if we're being called by surface_load_location() with
     * the purpose of loading the destination surface.) */
    surface_load_location(src_surface, src_location);
    if (!surface_is_full_rect(dst_surface, &dst_rect))
        surface_load_location(dst_surface, dst_location);

    if (src_location == WINED3D_LOCATION_DRAWABLE) context = context_acquire(device, src_surface);
    else if (dst_location == WINED3D_LOCATION_DRAWABLE) context = context_acquire(device, dst_surface);
    else context = context_acquire(device, NULL);

    if (!context->valid)
    {
        context_release(context);
        WARN("Invalid context, skipping blit.\n");
        return;
    }

    gl_info = context->gl_info;

    if (src_location == WINED3D_LOCATION_DRAWABLE)
    {
        TRACE("Source surface %p is onscreen.\n", src_surface);
        buffer = surface_get_gl_buffer(src_surface);
        surface_translate_drawable_coords(src_surface, context->win_handle, &src_rect);
    }
    else
    {
        TRACE("Source surface %p is offscreen.\n", src_surface);
        buffer = GL_COLOR_ATTACHMENT0;
    }

    context_apply_fbo_state_blit(context, GL_READ_FRAMEBUFFER, src_surface, NULL, src_location);
    gl_info->gl_ops.gl.p_glReadBuffer(buffer);
    checkGLcall("glReadBuffer()");
    context_check_fbo_status(context, GL_READ_FRAMEBUFFER);

    if (dst_location == WINED3D_LOCATION_DRAWABLE)
    {
        TRACE("Destination surface %p is onscreen.\n", dst_surface);
        buffer = surface_get_gl_buffer(dst_surface);
        surface_translate_drawable_coords(dst_surface, context->win_handle, &dst_rect);
    }
    else
    {
        TRACE("Destination surface %p is offscreen.\n", dst_surface);
        buffer = GL_COLOR_ATTACHMENT0;
    }

    context_apply_fbo_state_blit(context, GL_DRAW_FRAMEBUFFER, dst_surface, NULL, dst_location);
    context_set_draw_buffer(context, buffer);
    context_check_fbo_status(context, GL_DRAW_FRAMEBUFFER);
    context_invalidate_state(context, STATE_FRAMEBUFFER);

    gl_info->gl_ops.gl.p_glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_COLORWRITEENABLE));
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_COLORWRITEENABLE1));
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_COLORWRITEENABLE2));
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_COLORWRITEENABLE3));

    gl_info->gl_ops.gl.p_glDisable(GL_SCISSOR_TEST);
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_SCISSORTESTENABLE));

    gl_info->fbo_ops.glBlitFramebuffer(src_rect.left, src_rect.top, src_rect.right, src_rect.bottom,
            dst_rect.left, dst_rect.top, dst_rect.right, dst_rect.bottom, GL_COLOR_BUFFER_BIT, gl_filter);
    checkGLcall("glBlitFramebuffer()");

    if (wined3d_settings.strict_draw_ordering
            || (dst_location == WINED3D_LOCATION_DRAWABLE
            && dst_surface->container->swapchain->front_buffer == dst_surface->container))
        gl_info->gl_ops.gl.p_glFlush();

    context_release(context);
}

static BOOL fbo_blit_supported(const struct wined3d_gl_info *gl_info, enum wined3d_blit_op blit_op,
        const RECT *src_rect, DWORD src_usage, enum wined3d_pool src_pool, const struct wined3d_format *src_format,
        const RECT *dst_rect, DWORD dst_usage, enum wined3d_pool dst_pool, const struct wined3d_format *dst_format)
{
    if ((wined3d_settings.offscreen_rendering_mode != ORM_FBO) || !gl_info->fbo_ops.glBlitFramebuffer)
        return FALSE;

    /* Source and/or destination need to be on the GL side */
    if (src_pool == WINED3D_POOL_SYSTEM_MEM || dst_pool == WINED3D_POOL_SYSTEM_MEM)
        return FALSE;

    switch (blit_op)
    {
        case WINED3D_BLIT_OP_COLOR_BLIT:
            if (!((src_format->flags & WINED3DFMT_FLAG_FBO_ATTACHABLE) || (src_usage & WINED3DUSAGE_RENDERTARGET)))
                return FALSE;
            if (!((dst_format->flags & WINED3DFMT_FLAG_FBO_ATTACHABLE) || (dst_usage & WINED3DUSAGE_RENDERTARGET)))
                return FALSE;
            break;

        case WINED3D_BLIT_OP_DEPTH_BLIT:
            if (!(src_format->flags & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL)))
                return FALSE;
            if (!(dst_format->flags & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL)))
                return FALSE;
            break;

        default:
            return FALSE;
    }

    if (!(src_format->id == dst_format->id
            || (is_identity_fixup(src_format->color_fixup)
            && is_identity_fixup(dst_format->color_fixup))))
        return FALSE;

    return TRUE;
}

static BOOL surface_convert_color_to_float(const struct wined3d_surface *surface,
        DWORD color, struct wined3d_color *float_color)
{
    const struct wined3d_format *format = surface->resource.format;
    const struct wined3d_palette *palette;

    switch (format->id)
    {
        case WINED3DFMT_P8_UINT:
            palette = surface->container->swapchain ? surface->container->swapchain->palette : NULL;

            if (palette)
            {
                float_color->r = palette->colors[color].rgbRed / 255.0f;
                float_color->g = palette->colors[color].rgbGreen / 255.0f;
                float_color->b = palette->colors[color].rgbBlue / 255.0f;
            }
            else
            {
                float_color->r = 0.0f;
                float_color->g = 0.0f;
                float_color->b = 0.0f;
            }
            float_color->a = color / 255.0f;
            break;

        case WINED3DFMT_B5G6R5_UNORM:
            float_color->r = ((color >> 11) & 0x1f) / 31.0f;
            float_color->g = ((color >> 5) & 0x3f) / 63.0f;
            float_color->b = (color & 0x1f) / 31.0f;
            float_color->a = 1.0f;
            break;

        case WINED3DFMT_B8G8R8_UNORM:
        case WINED3DFMT_B8G8R8X8_UNORM:
            float_color->r = D3DCOLOR_R(color);
            float_color->g = D3DCOLOR_G(color);
            float_color->b = D3DCOLOR_B(color);
            float_color->a = 1.0f;
            break;

        case WINED3DFMT_B8G8R8A8_UNORM:
            float_color->r = D3DCOLOR_R(color);
            float_color->g = D3DCOLOR_G(color);
            float_color->b = D3DCOLOR_B(color);
            float_color->a = D3DCOLOR_A(color);
            break;

        default:
            ERR("Unhandled conversion from %s to floating point.\n", debug_d3dformat(format->id));
            return FALSE;
    }

    return TRUE;
}

static BOOL surface_convert_depth_to_float(const struct wined3d_surface *surface, DWORD depth, float *float_depth)
{
    const struct wined3d_format *format = surface->resource.format;

    switch (format->id)
    {
        case WINED3DFMT_S1_UINT_D15_UNORM:
            *float_depth = depth / (float)0x00007fff;
            break;

        case WINED3DFMT_D16_UNORM:
            *float_depth = depth / (float)0x0000ffff;
            break;

        case WINED3DFMT_D24_UNORM_S8_UINT:
        case WINED3DFMT_X8D24_UNORM:
            *float_depth = depth / (float)0x00ffffff;
            break;

        case WINED3DFMT_D32_UNORM:
            *float_depth = depth / (float)0xffffffff;
            break;

        default:
            ERR("Unhandled conversion from %s to floating point.\n", debug_d3dformat(format->id));
            return FALSE;
    }

    return TRUE;
}

static HRESULT wined3d_surface_depth_fill(struct wined3d_surface *surface, const RECT *rect, float depth)
{
    const struct wined3d_resource *resource = &surface->container->resource;
    struct wined3d_device *device = resource->device;
    const struct blit_shader *blitter;

    blitter = wined3d_select_blitter(&device->adapter->gl_info, WINED3D_BLIT_OP_DEPTH_FILL,
            NULL, 0, 0, NULL, rect, resource->usage, resource->pool, resource->format);
    if (!blitter)
    {
        FIXME("No blitter is capable of performing the requested depth fill operation.\n");
        return WINED3DERR_INVALIDCALL;
    }

    return blitter->depth_fill(device, surface, rect, depth);
}

static HRESULT wined3d_surface_depth_blt(struct wined3d_surface *src_surface, DWORD src_location, const RECT *src_rect,
        struct wined3d_surface *dst_surface, DWORD dst_location, const RECT *dst_rect)
{
    struct wined3d_device *device = src_surface->resource.device;

    if (!fbo_blit_supported(&device->adapter->gl_info, WINED3D_BLIT_OP_DEPTH_BLIT,
            src_rect, src_surface->resource.usage, src_surface->resource.pool, src_surface->resource.format,
            dst_rect, dst_surface->resource.usage, dst_surface->resource.pool, dst_surface->resource.format))
        return WINED3DERR_INVALIDCALL;

    surface_depth_blt_fbo(device, src_surface, src_location, src_rect, dst_surface, dst_location, dst_rect);

    surface_modify_ds_location(dst_surface, dst_location,
            dst_surface->ds_current_size.cx, dst_surface->ds_current_size.cy);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_surface_get_render_target_data(struct wined3d_surface *surface,
        struct wined3d_surface *render_target)
{
    TRACE("surface %p, render_target %p.\n", surface, render_target);

    /* TODO: Check surface sizes, pools, etc. */

    if (render_target->resource.multisample_type)
        return WINED3DERR_INVALIDCALL;

    return wined3d_surface_blt(surface, NULL, render_target, NULL, 0, NULL, WINED3D_TEXF_POINT);
}

/* Context activation is done by the caller. */
static void surface_remove_pbo(struct wined3d_surface *surface, const struct wined3d_gl_info *gl_info)
{
    GL_EXTCALL(glDeleteBuffers(1, &surface->pbo));
    checkGLcall("glDeleteBuffers(1, &surface->pbo)");

    surface->pbo = 0;
    surface_invalidate_location(surface, WINED3D_LOCATION_BUFFER);
}

static ULONG surface_resource_incref(struct wined3d_resource *resource)
{
    return wined3d_surface_incref(surface_from_resource(resource));
}

static ULONG surface_resource_decref(struct wined3d_resource *resource)
{
    return wined3d_surface_decref(surface_from_resource(resource));
}

static void surface_unload(struct wined3d_resource *resource)
{
    struct wined3d_surface *surface = surface_from_resource(resource);
    struct wined3d_renderbuffer_entry *entry, *entry2;
    struct wined3d_device *device = resource->device;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;

    TRACE("surface %p.\n", surface);

    if (resource->pool == WINED3D_POOL_DEFAULT)
    {
        /* Default pool resources are supposed to be destroyed before Reset is called.
         * Implicit resources stay however. So this means we have an implicit render target
         * or depth stencil. The content may be destroyed, but we still have to tear down
         * opengl resources, so we cannot leave early.
         *
         * Put the surfaces into sysmem, and reset the content. The D3D content is undefined,
         * but we can't set the sysmem INDRAWABLE because when we're rendering the swapchain
         * or the depth stencil into an FBO the texture or render buffer will be removed
         * and all flags get lost */
        surface_prepare_system_memory(surface);
        memset(surface->resource.heap_memory, 0, surface->resource.size);
        surface_validate_location(surface, WINED3D_LOCATION_SYSMEM);
        surface_invalidate_location(surface, ~WINED3D_LOCATION_SYSMEM);

        /* We also get here when the ddraw swapchain is destroyed, for example
         * for a mode switch. In this case this surface won't necessarily be
         * an implicit surface. We have to mark it lost so that the
         * application can restore it after the mode switch. */
        surface->flags |= SFLAG_LOST;
    }
    else
    {
        surface_prepare_map_memory(surface);
        surface_load_location(surface, surface->resource.map_binding);
        surface_invalidate_location(surface, ~surface->resource.map_binding);
    }

    context = context_acquire(device, NULL);
    gl_info = context->gl_info;

    /* Destroy PBOs, but load them into real sysmem before */
    if (surface->pbo)
        surface_remove_pbo(surface, gl_info);

    /* Destroy fbo render buffers. This is needed for implicit render targets, for
     * all application-created targets the application has to release the surface
     * before calling _Reset
     */
    LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &surface->renderbuffers, struct wined3d_renderbuffer_entry, entry)
    {
        gl_info->fbo_ops.glDeleteRenderbuffers(1, &entry->id);
        list_remove(&entry->entry);
        HeapFree(GetProcessHeap(), 0, entry);
    }
    list_init(&surface->renderbuffers);
    surface->current_renderbuffer = NULL;

    if (surface->rb_multisample)
    {
        gl_info->fbo_ops.glDeleteRenderbuffers(1, &surface->rb_multisample);
        surface->rb_multisample = 0;
    }
    if (surface->rb_resolved)
    {
        gl_info->fbo_ops.glDeleteRenderbuffers(1, &surface->rb_resolved);
        surface->rb_resolved = 0;
    }

    context_release(context);

    resource_unload(resource);
}

static const struct wined3d_resource_ops surface_resource_ops =
{
    surface_resource_incref,
    surface_resource_decref,
    surface_unload,
};

static const struct wined3d_surface_ops surface_ops =
{
    surface_private_setup,
    surface_unmap,
};

/*****************************************************************************
 * Initializes the GDI surface, aka creates the DIB section we render to
 * The DIB section creation is done by calling GetDC, which will create the
 * section and releasing the dc to allow the app to use it. The dib section
 * will stay until the surface is released
 *
 * GDI surfaces do not need to be a power of 2 in size, so the pow2 sizes
 * are set to the real sizes to save memory. The NONPOW2 flag is unset to
 * avoid confusion in the shared surface code.
 *
 * Returns:
 *  WINED3D_OK on success
 *  The return values of called methods on failure
 *
 *****************************************************************************/
static HRESULT gdi_surface_private_setup(struct wined3d_surface *surface)
{
    HRESULT hr;

    TRACE("surface %p.\n", surface);

    if (surface->resource.usage & WINED3DUSAGE_OVERLAY)
    {
        ERR("Overlays not yet supported by GDI surfaces.\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* Sysmem textures have memory already allocated - release it,
     * this avoids an unnecessary memcpy. */
    hr = surface_create_dib_section(surface);
    if (FAILED(hr))
        return hr;
    surface->resource.map_binding = WINED3D_LOCATION_DIB;

    /* We don't mind the nonpow2 stuff in GDI. */
    surface->pow2Width = surface->resource.width;
    surface->pow2Height = surface->resource.height;

    return WINED3D_OK;
}

static void gdi_surface_unmap(struct wined3d_surface *surface)
{
    TRACE("surface %p.\n", surface);

    /* Tell the swapchain to update the screen. */
    if (surface->container->swapchain && surface->container == surface->container->swapchain->front_buffer)
        x11_copy_to_screen(surface->container->swapchain, &surface->lockedRect);

    memset(&surface->lockedRect, 0, sizeof(RECT));
}

static const struct wined3d_surface_ops gdi_surface_ops =
{
    gdi_surface_private_setup,
    gdi_surface_unmap,
};

/* This call just downloads data, the caller is responsible for binding the
 * correct texture. */
/* Context activation is done by the caller. */
static void surface_download_data(struct wined3d_surface *surface, const struct wined3d_gl_info *gl_info,
        DWORD dst_location)
{
    const struct wined3d_format *format = surface->resource.format;
    struct wined3d_bo_address data;

    /* Only support read back of converted P8 surfaces. */
    if (surface->container->flags & WINED3D_TEXTURE_CONVERTED && format->id != WINED3DFMT_P8_UINT)
    {
        ERR("Trying to read back converted surface %p with format %s.\n", surface, debug_d3dformat(format->id));
        return;
    }

    surface_get_memory(surface, &data, dst_location);

    if (format->flags & WINED3DFMT_FLAG_COMPRESSED)
    {
        TRACE("(%p) : Calling glGetCompressedTexImage level %d, format %#x, type %#x, data %p.\n",
                surface, surface->texture_level, format->glFormat, format->glType, data.addr);

        if (data.buffer_object)
        {
            GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, data.buffer_object));
            checkGLcall("glBindBuffer");
            GL_EXTCALL(glGetCompressedTexImage(surface->texture_target, surface->texture_level, NULL));
            checkGLcall("glGetCompressedTexImage");
            GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
            checkGLcall("glBindBuffer");
        }
        else
        {
            GL_EXTCALL(glGetCompressedTexImage(surface->texture_target,
                    surface->texture_level, data.addr));
            checkGLcall("glGetCompressedTexImage");
        }
    }
    else
    {
        void *mem;
        GLenum gl_format = format->glFormat;
        GLenum gl_type = format->glType;
        int src_pitch = 0;
        int dst_pitch = 0;

        if (surface->flags & SFLAG_NONPOW2)
        {
            unsigned char alignment = surface->resource.device->surface_alignment;
            src_pitch = format->byte_count * surface->pow2Width;
            dst_pitch = wined3d_surface_get_pitch(surface);
            src_pitch = (src_pitch + alignment - 1) & ~(alignment - 1);
            mem = HeapAlloc(GetProcessHeap(), 0, src_pitch * surface->pow2Height);
        }
        else
        {
            mem = data.addr;
        }

        TRACE("(%p) : Calling glGetTexImage level %d, format %#x, type %#x, data %p\n",
                surface, surface->texture_level, gl_format, gl_type, mem);

        if (data.buffer_object)
        {
            GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, data.buffer_object));
            checkGLcall("glBindBuffer");

            gl_info->gl_ops.gl.p_glGetTexImage(surface->texture_target, surface->texture_level,
                    gl_format, gl_type, NULL);
            checkGLcall("glGetTexImage");

            GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
            checkGLcall("glBindBuffer");
        }
        else
        {
            gl_info->gl_ops.gl.p_glGetTexImage(surface->texture_target, surface->texture_level,
                    gl_format, gl_type, mem);
            checkGLcall("glGetTexImage");
        }

        if (surface->flags & SFLAG_NONPOW2)
        {
            const BYTE *src_data;
            BYTE *dst_data;
            UINT y;
            /*
             * Some games (e.g. warhammer 40k) don't work properly with the odd pitches, preventing
             * the surface pitch from being used to box non-power2 textures. Instead we have to use a hack to
             * repack the texture so that the bpp * width pitch can be used instead of bpp * pow2width.
             *
             * We're doing this...
             *
             * instead of boxing the texture :
             * |<-texture width ->|  -->pow2width|   /\
             * |111111111111111111|              |   |
             * |222 Texture 222222| boxed empty  | texture height
             * |3333 Data 33333333|              |   |
             * |444444444444444444|              |   \/
             * -----------------------------------   |
             * |     boxed  empty | boxed empty  | pow2height
             * |                  |              |   \/
             * -----------------------------------
             *
             *
             * we're repacking the data to the expected texture width
             *
             * |<-texture width ->|  -->pow2width|   /\
             * |111111111111111111222222222222222|   |
             * |222333333333333333333444444444444| texture height
             * |444444                           |   |
             * |                                 |   \/
             * |                                 |   |
             * |            empty                | pow2height
             * |                                 |   \/
             * -----------------------------------
             *
             * == is the same as
             *
             * |<-texture width ->|    /\
             * |111111111111111111|
             * |222222222222222222|texture height
             * |333333333333333333|
             * |444444444444444444|    \/
             * --------------------
             *
             * This also means that any references to surface memory should work with the data as if it were a
             * standard texture with a non-power2 width instead of a texture boxed up to be a power2 texture.
             *
             * internally the texture is still stored in a boxed format so any references to textureName will
             * get a boxed texture with width pow2width and not a texture of width resource.width.
             *
             * Performance should not be an issue, because applications normally do not lock the surfaces when
             * rendering. If an app does, the WINED3D_TEXTURE_DYNAMIC_MAP flag will kick in and the memory copy
             * won't be released, and doesn't have to be re-read. */
            src_data = mem;
            dst_data = data.addr;
            TRACE("(%p) : Repacking the surface data from pitch %d to pitch %d\n", surface, src_pitch, dst_pitch);
            for (y = 0; y < surface->resource.height; ++y)
            {
                memcpy(dst_data, src_data, dst_pitch);
                src_data += src_pitch;
                dst_data += dst_pitch;
            }

            HeapFree(GetProcessHeap(), 0, mem);
        }
    }
}

/* This call just uploads data, the caller is responsible for binding the
 * correct texture. */
/* Context activation is done by the caller. */
void wined3d_surface_upload_data(struct wined3d_surface *surface, const struct wined3d_gl_info *gl_info,
        const struct wined3d_format *format, const RECT *src_rect, UINT src_pitch, const POINT *dst_point,
        BOOL srgb, const struct wined3d_const_bo_address *data)
{
    UINT update_w = src_rect->right - src_rect->left;
    UINT update_h = src_rect->bottom - src_rect->top;

    TRACE("surface %p, gl_info %p, format %s, src_rect %s, src_pitch %u, dst_point %s, srgb %#x, data {%#x:%p}.\n",
            surface, gl_info, debug_d3dformat(format->id), wine_dbgstr_rect(src_rect), src_pitch,
            wine_dbgstr_point(dst_point), srgb, data->buffer_object, data->addr);

    if (surface->resource.map_count)
    {
        WARN("Uploading a surface that is currently mapped, setting WINED3D_TEXTURE_PIN_SYSMEM.\n");
        surface->container->flags |= WINED3D_TEXTURE_PIN_SYSMEM;
    }

    if (format->flags & WINED3DFMT_FLAG_HEIGHT_SCALE)
    {
        update_h *= format->height_scale.numerator;
        update_h /= format->height_scale.denominator;
    }

    if (data->buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, data->buffer_object));
        checkGLcall("glBindBuffer");
    }

    if (format->flags & WINED3DFMT_FLAG_COMPRESSED)
    {
        UINT row_length = wined3d_format_calculate_size(format, 1, update_w, 1, 1);
        UINT row_count = (update_h + format->block_height - 1) / format->block_height;
        const BYTE *addr = data->addr;
        GLenum internal;

        addr += (src_rect->top / format->block_height) * src_pitch;
        addr += (src_rect->left / format->block_width) * format->block_byte_count;

        if (srgb)
            internal = format->glGammaInternal;
        else if (surface->resource.usage & WINED3DUSAGE_RENDERTARGET
                && wined3d_resource_is_offscreen(&surface->container->resource))
            internal = format->rtInternal;
        else
            internal = format->glInternal;

        TRACE("glCompressedTexSubImage2D, target %#x, level %d, x %d, y %d, w %d, h %d, "
                "format %#x, image_size %#x, addr %p.\n", surface->texture_target, surface->texture_level,
                dst_point->x, dst_point->y, update_w, update_h, internal, row_count * row_length, addr);

        if (row_length == src_pitch)
        {
            GL_EXTCALL(glCompressedTexSubImage2D(surface->texture_target, surface->texture_level,
                    dst_point->x, dst_point->y, update_w, update_h, internal, row_count * row_length, addr));
        }
        else
        {
            UINT row, y;

            /* glCompressedTexSubImage2D() ignores pixel store state, so we
             * can't use the unpack row length like for glTexSubImage2D. */
            for (row = 0, y = dst_point->y; row < row_count; ++row)
            {
                GL_EXTCALL(glCompressedTexSubImage2D(surface->texture_target, surface->texture_level,
                        dst_point->x, y, update_w, format->block_height, internal, row_length, addr));
                y += format->block_height;
                addr += src_pitch;
            }
        }
        checkGLcall("glCompressedTexSubImage2D");
    }
    else
    {
        const BYTE *addr = data->addr;

        addr += src_rect->top * src_pitch;
        addr += src_rect->left * format->byte_count;

        TRACE("glTexSubImage2D, target %#x, level %d, x %d, y %d, w %d, h %d, format %#x, type %#x, addr %p.\n",
                surface->texture_target, surface->texture_level, dst_point->x, dst_point->y,
                update_w, update_h, format->glFormat, format->glType, addr);

        gl_info->gl_ops.gl.p_glPixelStorei(GL_UNPACK_ROW_LENGTH, src_pitch / format->byte_count);
        gl_info->gl_ops.gl.p_glTexSubImage2D(surface->texture_target, surface->texture_level,
                dst_point->x, dst_point->y, update_w, update_h, format->glFormat, format->glType, addr);
        gl_info->gl_ops.gl.p_glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        checkGLcall("glTexSubImage2D");
    }

    if (data->buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
        checkGLcall("glBindBuffer");
    }

    if (wined3d_settings.strict_draw_ordering)
        gl_info->gl_ops.gl.p_glFlush();

    if (gl_info->quirks & WINED3D_QUIRK_FBO_TEX_UPDATE)
    {
        struct wined3d_device *device = surface->resource.device;
        unsigned int i;

        for (i = 0; i < device->context_count; ++i)
        {
            context_surface_update(device->contexts[i], surface);
        }
    }
}

static BOOL surface_check_block_align(struct wined3d_surface *surface, const RECT *rect)
{
    UINT width_mask, height_mask;

    if (!rect->left && !rect->top
            && rect->right == surface->resource.width
            && rect->bottom == surface->resource.height)
        return TRUE;

    /* This assumes power of two block sizes, but NPOT block sizes would be
     * silly anyway. */
    width_mask = surface->resource.format->block_width - 1;
    height_mask = surface->resource.format->block_height - 1;

    if (!(rect->left & width_mask) && !(rect->top & height_mask)
            && !(rect->right & width_mask) && !(rect->bottom & height_mask))
        return TRUE;

    return FALSE;
}

HRESULT surface_upload_from_surface(struct wined3d_surface *dst_surface, const POINT *dst_point,
        struct wined3d_surface *src_surface, const RECT *src_rect)
{
    const struct wined3d_format *src_format;
    const struct wined3d_format *dst_format;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;
    struct wined3d_bo_address data;
    UINT update_w, update_h;
    UINT dst_w, dst_h;
    RECT r, dst_rect;
    UINT src_pitch;
    POINT p;

    TRACE("dst_surface %p, dst_point %s, src_surface %p, src_rect %s.\n",
            dst_surface, wine_dbgstr_point(dst_point),
            src_surface, wine_dbgstr_rect(src_rect));

    src_format = src_surface->resource.format;
    dst_format = dst_surface->resource.format;

    if (src_format->id != dst_format->id)
    {
        WARN("Source and destination surfaces should have the same format.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (!dst_point)
    {
        p.x = 0;
        p.y = 0;
        dst_point = &p;
    }
    else if (dst_point->x < 0 || dst_point->y < 0)
    {
        WARN("Invalid destination point.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (!src_rect)
    {
        r.left = 0;
        r.top = 0;
        r.right = src_surface->resource.width;
        r.bottom = src_surface->resource.height;
        src_rect = &r;
    }
    else if (src_rect->left < 0 || src_rect->left >= src_rect->right
            || src_rect->top < 0 || src_rect->top >= src_rect->bottom)
    {
        WARN("Invalid source rectangle.\n");
        return WINED3DERR_INVALIDCALL;
    }

    dst_w = dst_surface->resource.width;
    dst_h = dst_surface->resource.height;

    update_w = src_rect->right - src_rect->left;
    update_h = src_rect->bottom - src_rect->top;

    if (update_w > dst_w || dst_point->x > dst_w - update_w
            || update_h > dst_h || dst_point->y > dst_h - update_h)
    {
        WARN("Destination out of bounds.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if ((src_format->flags & WINED3DFMT_FLAG_BLOCKS) && !surface_check_block_align(src_surface, src_rect))
    {
        WARN("Source rectangle not block-aligned.\n");
        return WINED3DERR_INVALIDCALL;
    }

    SetRect(&dst_rect, dst_point->x, dst_point->y, dst_point->x + update_w, dst_point->y + update_h);
    if ((dst_format->flags & WINED3DFMT_FLAG_BLOCKS) && !surface_check_block_align(dst_surface, &dst_rect))
    {
        WARN("Destination rectangle not block-aligned.\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* Use wined3d_surface_blt() instead of uploading directly if we need conversion. */
    if (dst_format->convert || wined3d_format_get_color_key_conversion(dst_surface->container, FALSE))
        return wined3d_surface_blt(dst_surface, &dst_rect, src_surface, src_rect, 0, NULL, WINED3D_TEXF_POINT);

    context = context_acquire(dst_surface->resource.device, NULL);
    gl_info = context->gl_info;

    /* Only load the surface for partial updates. For newly allocated texture
     * the texture wouldn't be the current location, and we'd upload zeroes
     * just to overwrite them again. */
    if (update_w == dst_w && update_h == dst_h)
        wined3d_texture_prepare_texture(dst_surface->container, context, FALSE);
    else
        surface_load_location(dst_surface, WINED3D_LOCATION_TEXTURE_RGB);
    wined3d_texture_bind(dst_surface->container, context, FALSE);

    surface_get_memory(src_surface, &data, src_surface->locations);
    src_pitch = wined3d_surface_get_pitch(src_surface);

    wined3d_surface_upload_data(dst_surface, gl_info, src_format, src_rect,
            src_pitch, dst_point, FALSE, wined3d_const_bo_address(&data));

    context_invalidate_active_texture(context);

    context_release(context);

    surface_validate_location(dst_surface, WINED3D_LOCATION_TEXTURE_RGB);
    surface_invalidate_location(dst_surface, ~WINED3D_LOCATION_TEXTURE_RGB);

    return WINED3D_OK;
}

/* In D3D the depth stencil dimensions have to be greater than or equal to the
 * render target dimensions. With FBOs, the dimensions have to be an exact match. */
/* TODO: We should synchronize the renderbuffer's content with the texture's content. */
/* Context activation is done by the caller. */
void surface_set_compatible_renderbuffer(struct wined3d_surface *surface, const struct wined3d_surface *rt)
{
    const struct wined3d_gl_info *gl_info = &surface->resource.device->adapter->gl_info;
    struct wined3d_renderbuffer_entry *entry;
    GLuint renderbuffer = 0;
    unsigned int src_width, src_height;
    unsigned int width, height;

    if (rt && rt->resource.format->id != WINED3DFMT_NULL)
    {
        width = rt->pow2Width;
        height = rt->pow2Height;
    }
    else
    {
        width = surface->pow2Width;
        height = surface->pow2Height;
    }

    src_width = surface->pow2Width;
    src_height = surface->pow2Height;

    /* A depth stencil smaller than the render target is not valid */
    if (width > src_width || height > src_height) return;

    /* Remove any renderbuffer set if the sizes match */
    if (gl_info->supported[ARB_FRAMEBUFFER_OBJECT]
            || (width == src_width && height == src_height))
    {
        surface->current_renderbuffer = NULL;
        return;
    }

    /* Look if we've already got a renderbuffer of the correct dimensions */
    LIST_FOR_EACH_ENTRY(entry, &surface->renderbuffers, struct wined3d_renderbuffer_entry, entry)
    {
        if (entry->width == width && entry->height == height)
        {
            renderbuffer = entry->id;
            surface->current_renderbuffer = entry;
            break;
        }
    }

    if (!renderbuffer)
    {
        gl_info->fbo_ops.glGenRenderbuffers(1, &renderbuffer);
        gl_info->fbo_ops.glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
        gl_info->fbo_ops.glRenderbufferStorage(GL_RENDERBUFFER,
                surface->resource.format->glInternal, width, height);

        entry = HeapAlloc(GetProcessHeap(), 0, sizeof(*entry));
        entry->width = width;
        entry->height = height;
        entry->id = renderbuffer;
        list_add_head(&surface->renderbuffers, &entry->entry);

        surface->current_renderbuffer = entry;
    }

    checkGLcall("set_compatible_renderbuffer");
}

GLenum surface_get_gl_buffer(const struct wined3d_surface *surface)
{
    const struct wined3d_swapchain *swapchain = surface->container->swapchain;

    TRACE("surface %p.\n", surface);

    if (!swapchain)
    {
        ERR("Surface %p is not on a swapchain.\n", surface);
        return GL_NONE;
    }

    if (swapchain->back_buffers && swapchain->back_buffers[0] == surface->container)
    {
        if (swapchain->render_to_fbo)
        {
            TRACE("Returning GL_COLOR_ATTACHMENT0\n");
            return GL_COLOR_ATTACHMENT0;
        }
        TRACE("Returning GL_BACK\n");
        return GL_BACK;
    }
    else if (surface->container == swapchain->front_buffer)
    {
        TRACE("Returning GL_FRONT\n");
        return GL_FRONT;
    }

    FIXME("Higher back buffer, returning GL_BACK\n");
    return GL_BACK;
}

void surface_load(struct wined3d_surface *surface, BOOL srgb)
{
    DWORD location = srgb ? WINED3D_LOCATION_TEXTURE_SRGB : WINED3D_LOCATION_TEXTURE_RGB;

    TRACE("surface %p, srgb %#x.\n", surface, srgb);

    if (surface->resource.pool == WINED3D_POOL_SCRATCH)
        ERR("Not supported on scratch surfaces.\n");

    if (surface->locations & location)
    {
        TRACE("surface is already in texture\n");
        return;
    }
    TRACE("Reloading because surface is dirty.\n");

    surface_load_location(surface, location);
    surface_evict_sysmem(surface);
}

/* See also float_16_to_32() in wined3d_private.h */
static inline unsigned short float_32_to_16(const float *in)
{
    int exp = 0;
    float tmp = fabsf(*in);
    unsigned int mantissa;
    unsigned short ret;

    /* Deal with special numbers */
    if (*in == 0.0f)
        return 0x0000;
    if (isnan(*in))
        return 0x7c01;
    if (isinf(*in))
        return (*in < 0.0f ? 0xfc00 : 0x7c00);

    if (tmp < powf(2, 10))
    {
        do
        {
            tmp = tmp * 2.0f;
            exp--;
        } while (tmp < powf(2, 10));
    }
    else if (tmp >= powf(2, 11))
    {
        do
        {
            tmp /= 2.0f;
            exp++;
        } while (tmp >= powf(2, 11));
    }

    mantissa = (unsigned int)tmp;
    if (tmp - mantissa >= 0.5f)
        ++mantissa; /* Round to nearest, away from zero. */

    exp += 10;  /* Normalize the mantissa. */
    exp += 15;  /* Exponent is encoded with excess 15. */

    if (exp > 30) /* too big */
    {
        ret = 0x7c00; /* INF */
    }
    else if (exp <= 0)
    {
        /* exp == 0: Non-normalized mantissa. Returns 0x0000 (=0.0) for too small numbers. */
        while (exp <= 0)
        {
            mantissa = mantissa >> 1;
            ++exp;
        }
        ret = mantissa & 0x3ff;
    }
    else
    {
        ret = (exp << 10) | (mantissa & 0x3ff);
    }

    ret |= ((*in < 0.0f ? 1 : 0) << 15); /* Add the sign */
    return ret;
}

ULONG CDECL wined3d_surface_incref(struct wined3d_surface *surface)
{
    TRACE("surface %p, container %p.\n", surface, surface->container);

    return wined3d_texture_incref(surface->container);
}

ULONG CDECL wined3d_surface_decref(struct wined3d_surface *surface)
{
    TRACE("surface %p, container %p.\n", surface, surface->container);

    return wined3d_texture_decref(surface->container);
}

void CDECL wined3d_surface_preload(struct wined3d_surface *surface)
{
    TRACE("surface %p.\n", surface);

    if (!surface->resource.device->d3d_initialized)
    {
        ERR("D3D not initialized.\n");
        return;
    }

    wined3d_texture_preload(surface->container);
}

void * CDECL wined3d_surface_get_parent(const struct wined3d_surface *surface)
{
    TRACE("surface %p.\n", surface);

    return surface->resource.parent;
}

struct wined3d_resource * CDECL wined3d_surface_get_resource(struct wined3d_surface *surface)
{
    TRACE("surface %p.\n", surface);

    return &surface->resource;
}

HRESULT CDECL wined3d_surface_get_blt_status(const struct wined3d_surface *surface, DWORD flags)
{
    TRACE("surface %p, flags %#x.\n", surface, flags);

    switch (flags)
    {
        case WINEDDGBS_CANBLT:
        case WINEDDGBS_ISBLTDONE:
            return WINED3D_OK;

        default:
            return WINED3DERR_INVALIDCALL;
    }
}

HRESULT CDECL wined3d_surface_get_flip_status(const struct wined3d_surface *surface, DWORD flags)
{
    TRACE("surface %p, flags %#x.\n", surface, flags);

    /* XXX: DDERR_INVALIDSURFACETYPE */

    switch (flags)
    {
        case WINEDDGFS_CANFLIP:
        case WINEDDGFS_ISFLIPDONE:
            return WINED3D_OK;

        default:
            return WINED3DERR_INVALIDCALL;
    }
}

HRESULT CDECL wined3d_surface_is_lost(const struct wined3d_surface *surface)
{
    TRACE("surface %p.\n", surface);

    /* D3D8 and 9 loose full devices, ddraw only surfaces. */
    return surface->flags & SFLAG_LOST ? WINED3DERR_DEVICELOST : WINED3D_OK;
}

HRESULT CDECL wined3d_surface_restore(struct wined3d_surface *surface)
{
    TRACE("surface %p.\n", surface);

    surface->flags &= ~SFLAG_LOST;
    return WINED3D_OK;
}

DWORD CDECL wined3d_surface_get_pitch(const struct wined3d_surface *surface)
{
    unsigned int alignment;
    DWORD pitch;

    TRACE("surface %p.\n", surface);

    if (surface->pitch)
        return surface->pitch;

    alignment = surface->resource.device->surface_alignment;
    pitch = wined3d_format_calculate_pitch(surface->resource.format, surface->resource.width);
    pitch = (pitch + alignment - 1) & ~(alignment - 1);

    TRACE("Returning %u.\n", pitch);

    return pitch;
}

HRESULT CDECL wined3d_surface_set_overlay_position(struct wined3d_surface *surface, LONG x, LONG y)
{
    LONG w, h;

    TRACE("surface %p, x %d, y %d.\n", surface, x, y);

    if (!(surface->resource.usage & WINED3DUSAGE_OVERLAY))
    {
        WARN("Not an overlay surface.\n");
        return WINEDDERR_NOTAOVERLAYSURFACE;
    }

    w = surface->overlay_destrect.right - surface->overlay_destrect.left;
    h = surface->overlay_destrect.bottom - surface->overlay_destrect.top;
    surface->overlay_destrect.left = x;
    surface->overlay_destrect.top = y;
    surface->overlay_destrect.right = x + w;
    surface->overlay_destrect.bottom = y + h;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_surface_get_overlay_position(const struct wined3d_surface *surface, LONG *x, LONG *y)
{
    TRACE("surface %p, x %p, y %p.\n", surface, x, y);

    if (!(surface->resource.usage & WINED3DUSAGE_OVERLAY))
    {
        TRACE("Not an overlay surface.\n");
        return WINEDDERR_NOTAOVERLAYSURFACE;
    }

    if (!surface->overlay_dest)
    {
        TRACE("Overlay not visible.\n");
        *x = 0;
        *y = 0;
        return WINEDDERR_OVERLAYNOTVISIBLE;
    }

    *x = surface->overlay_destrect.left;
    *y = surface->overlay_destrect.top;

    TRACE("Returning position %d, %d.\n", *x, *y);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_surface_update_overlay_z_order(struct wined3d_surface *surface,
        DWORD flags, struct wined3d_surface *ref)
{
    FIXME("surface %p, flags %#x, ref %p stub!\n", surface, flags, ref);

    if (!(surface->resource.usage & WINED3DUSAGE_OVERLAY))
    {
        TRACE("Not an overlay surface.\n");
        return WINEDDERR_NOTAOVERLAYSURFACE;
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_surface_update_overlay(struct wined3d_surface *surface, const RECT *src_rect,
        struct wined3d_surface *dst_surface, const RECT *dst_rect, DWORD flags, const WINEDDOVERLAYFX *fx)
{
    TRACE("surface %p, src_rect %s, dst_surface %p, dst_rect %s, flags %#x, fx %p.\n",
            surface, wine_dbgstr_rect(src_rect), dst_surface, wine_dbgstr_rect(dst_rect), flags, fx);

    if (!(surface->resource.usage & WINED3DUSAGE_OVERLAY))
    {
        WARN("Not an overlay surface.\n");
        return WINEDDERR_NOTAOVERLAYSURFACE;
    }
    else if (!dst_surface)
    {
        WARN("Dest surface is NULL.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (src_rect)
    {
        surface->overlay_srcrect = *src_rect;
    }
    else
    {
        surface->overlay_srcrect.left = 0;
        surface->overlay_srcrect.top = 0;
        surface->overlay_srcrect.right = surface->resource.width;
        surface->overlay_srcrect.bottom = surface->resource.height;
    }

    if (dst_rect)
    {
        surface->overlay_destrect = *dst_rect;
    }
    else
    {
        surface->overlay_destrect.left = 0;
        surface->overlay_destrect.top = 0;
        surface->overlay_destrect.right = dst_surface ? dst_surface->resource.width : 0;
        surface->overlay_destrect.bottom = dst_surface ? dst_surface->resource.height : 0;
    }

    if (surface->overlay_dest && (surface->overlay_dest != dst_surface || flags & WINEDDOVER_HIDE))
    {
        surface->overlay_dest = NULL;
        list_remove(&surface->overlay_entry);
    }

    if (flags & WINEDDOVER_SHOW)
    {
        if (surface->overlay_dest != dst_surface)
        {
            surface->overlay_dest = dst_surface;
            list_add_tail(&dst_surface->overlays, &surface->overlay_entry);
        }
    }
    else if (flags & WINEDDOVER_HIDE)
    {
        /* tests show that the rectangles are erased on hide */
        surface->overlay_srcrect.left = 0; surface->overlay_srcrect.top = 0;
        surface->overlay_srcrect.right = 0; surface->overlay_srcrect.bottom = 0;
        surface->overlay_destrect.left = 0; surface->overlay_destrect.top = 0;
        surface->overlay_destrect.right = 0; surface->overlay_destrect.bottom = 0;
        surface->overlay_dest = NULL;
    }

    return WINED3D_OK;
}

HRESULT wined3d_surface_update_desc(struct wined3d_surface *surface,
        const struct wined3d_gl_info *gl_info, void *mem, unsigned int pitch)
{
    struct wined3d_resource *texture_resource = &surface->container->resource;
    unsigned int width, height;
    BOOL create_dib = FALSE;
    DWORD valid_location = 0;
    HRESULT hr;

    if (surface->flags & SFLAG_DIBSECTION)
    {
        DeleteDC(surface->hDC);
        DeleteObject(surface->dib.DIBsection);
        surface->dib.bitmap_data = NULL;
        surface->flags &= ~SFLAG_DIBSECTION;
        create_dib = TRUE;
    }

    surface->locations = 0;
    wined3d_resource_free_sysmem(&surface->resource);

    width = texture_resource->width;
    height = texture_resource->height;
    surface->resource.width = width;
    surface->resource.height = height;
    if (gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO] || gl_info->supported[ARB_TEXTURE_RECTANGLE]
            || gl_info->supported[WINED3D_GL_NORMALIZED_TEXRECT])
    {
        surface->pow2Width = width;
        surface->pow2Height = height;
    }
    else
    {
        surface->pow2Width = surface->pow2Height = 1;
        while (surface->pow2Width < width)
            surface->pow2Width <<= 1;
        while (surface->pow2Height < height)
            surface->pow2Height <<= 1;
    }

    if (surface->pow2Width != width || surface->pow2Height != height)
        surface->flags |= SFLAG_NONPOW2;
    else
        surface->flags &= ~SFLAG_NONPOW2;

    if ((surface->user_memory = mem))
    {
        surface->resource.map_binding = WINED3D_LOCATION_USER_MEMORY;
        valid_location = WINED3D_LOCATION_USER_MEMORY;
    }
    surface->pitch = pitch;
    surface->resource.format = texture_resource->format;
    surface->resource.multisample_type = texture_resource->multisample_type;
    surface->resource.multisample_quality = texture_resource->multisample_quality;
    if (surface->pitch)
    {
        surface->resource.size = height * surface->pitch;
    }
    else
    {
        /* User memory surfaces don't have the regular surface alignment. */
        surface->resource.size = wined3d_format_calculate_size(texture_resource->format,
                1, width, height, 1);
        surface->pitch = wined3d_format_calculate_pitch(texture_resource->format, width);
    }

    /* The format might be changed to a format that needs conversion.
     * If the surface didn't use PBOs previously but could now, don't
     * change it - whatever made us not use PBOs might come back, e.g.
     * color keys. */
    if (surface->resource.map_binding == WINED3D_LOCATION_BUFFER && !surface_use_pbo(surface))
        surface->resource.map_binding = create_dib ? WINED3D_LOCATION_DIB : WINED3D_LOCATION_SYSMEM;

    if (create_dib)
    {
        if (FAILED(hr = surface_create_dib_section(surface)))
        {
            ERR("Failed to create dib section, hr %#x.\n", hr);
            return hr;
        }
        if (!valid_location)
            valid_location = WINED3D_LOCATION_DIB;
    }

    if (!valid_location)
    {
        surface_prepare_system_memory(surface);
        valid_location = WINED3D_LOCATION_SYSMEM;
    }

    surface_validate_location(surface, valid_location);

    return WINED3D_OK;
}

static void convert_r32_float_r16_float(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
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
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
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
            dst_line[x] = 0xff000000
                    | convert_5to8[(pixel & 0xf800) >> 11] << 16
                    | convert_6to8[(pixel & 0x07e0) >> 5] << 8
                    | convert_5to8[(pixel & 0x001f)];
        }
    }
}

/* We use this for both B8G8R8A8 -> B8G8R8X8 and B8G8R8X8 -> B8G8R8A8, since
 * in both cases we're just setting the X / Alpha channel to 0xff. */
static void convert_a8r8g8b8_x8r8g8b8(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
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
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
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
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
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

static void convert_dxt1_a8r8g8b8(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
{
    wined3d_dxt1_decode(src, dst, pitch_in, pitch_out, WINED3DFMT_B8G8R8A8_UNORM, w, h);
}

static void convert_dxt1_x8r8g8b8(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
{
    wined3d_dxt1_decode(src, dst, pitch_in, pitch_out, WINED3DFMT_B8G8R8X8_UNORM, w, h);
}

static void convert_dxt1_a4r4g4b4(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
{
    wined3d_dxt1_decode(src, dst, pitch_in, pitch_out, WINED3DFMT_B4G4R4A4_UNORM, w, h);
}

static void convert_dxt1_x4r4g4b4(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
{
    wined3d_dxt1_decode(src, dst, pitch_in, pitch_out, WINED3DFMT_B4G4R4X4_UNORM, w, h);
}

static void convert_dxt1_a1r5g5b5(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
{
    wined3d_dxt1_decode(src, dst, pitch_in, pitch_out, WINED3DFMT_B5G5R5A1_UNORM, w, h);
}

static void convert_dxt1_x1r5g5b5(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
{
    wined3d_dxt1_decode(src, dst, pitch_in, pitch_out, WINED3DFMT_B5G5R5X1_UNORM, w, h);
}

static void convert_dxt3_a8r8g8b8(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
{
    wined3d_dxt3_decode(src, dst, pitch_in, pitch_out, WINED3DFMT_B8G8R8A8_UNORM, w, h);
}

static void convert_dxt3_x8r8g8b8(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
{
    wined3d_dxt3_decode(src, dst, pitch_in, pitch_out, WINED3DFMT_B8G8R8X8_UNORM, w, h);
}

static void convert_dxt3_a4r4g4b4(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
{
    wined3d_dxt3_decode(src, dst, pitch_in, pitch_out, WINED3DFMT_B4G4R4A4_UNORM, w, h);
}

static void convert_dxt3_x4r4g4b4(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
{
    wined3d_dxt3_decode(src, dst, pitch_in, pitch_out, WINED3DFMT_B4G4R4X4_UNORM, w, h);
}

static void convert_dxt5_a8r8g8b8(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
{
    wined3d_dxt5_decode(src, dst, pitch_in, pitch_out, WINED3DFMT_B8G8R8A8_UNORM, w, h);
}

static void convert_dxt5_x8r8g8b8(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
{
    wined3d_dxt5_decode(src, dst, pitch_in, pitch_out, WINED3DFMT_B8G8R8X8_UNORM, w, h);
}

static void convert_a8r8g8b8_dxt1(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
{
    wined3d_dxt1_encode(src, dst, pitch_in, pitch_out, WINED3DFMT_B8G8R8A8_UNORM, w, h);
}

static void convert_x8r8g8b8_dxt1(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
{
    wined3d_dxt1_encode(src, dst, pitch_in, pitch_out, WINED3DFMT_B8G8R8X8_UNORM, w, h);
}

static void convert_a1r5g5b5_dxt1(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
{
    wined3d_dxt1_encode(src, dst, pitch_in, pitch_out, WINED3DFMT_B5G5R5A1_UNORM, w, h);
}

static void convert_x1r5g5b5_dxt1(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
{
    wined3d_dxt1_encode(src, dst, pitch_in, pitch_out, WINED3DFMT_B5G5R5X1_UNORM, w, h);
}

static void convert_a8r8g8b8_dxt3(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
{
    wined3d_dxt3_encode(src, dst, pitch_in, pitch_out, WINED3DFMT_B8G8R8A8_UNORM, w, h);
}

static void convert_x8r8g8b8_dxt3(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
{
    wined3d_dxt3_encode(src, dst, pitch_in, pitch_out, WINED3DFMT_B8G8R8X8_UNORM, w, h);
}

static void convert_a8r8g8b8_dxt5(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
{
    wined3d_dxt5_encode(src, dst, pitch_in, pitch_out, WINED3DFMT_B8G8R8A8_UNORM, w, h);
}

static void convert_x8r8g8b8_dxt5(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
{
    wined3d_dxt5_encode(src, dst, pitch_in, pitch_out, WINED3DFMT_B8G8R8X8_UNORM, w, h);
}

struct d3dfmt_converter_desc
{
    enum wined3d_format_id from, to;
    void (*convert)(const BYTE *src, BYTE *dst, DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h);
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

static const struct d3dfmt_converter_desc dxtn_converters[] =
{
    /* decode DXT */
    {WINED3DFMT_DXT1,           WINED3DFMT_B8G8R8A8_UNORM,  convert_dxt1_a8r8g8b8},
    {WINED3DFMT_DXT1,           WINED3DFMT_B8G8R8X8_UNORM,  convert_dxt1_x8r8g8b8},
    {WINED3DFMT_DXT1,           WINED3DFMT_B4G4R4A4_UNORM,  convert_dxt1_a4r4g4b4},
    {WINED3DFMT_DXT1,           WINED3DFMT_B4G4R4X4_UNORM,  convert_dxt1_x4r4g4b4},
    {WINED3DFMT_DXT1,           WINED3DFMT_B5G5R5A1_UNORM,  convert_dxt1_a1r5g5b5},
    {WINED3DFMT_DXT1,           WINED3DFMT_B5G5R5X1_UNORM,  convert_dxt1_x1r5g5b5},
    {WINED3DFMT_DXT3,           WINED3DFMT_B8G8R8A8_UNORM,  convert_dxt3_a8r8g8b8},
    {WINED3DFMT_DXT3,           WINED3DFMT_B8G8R8X8_UNORM,  convert_dxt3_x8r8g8b8},
    {WINED3DFMT_DXT3,           WINED3DFMT_B4G4R4A4_UNORM,  convert_dxt3_a4r4g4b4},
    {WINED3DFMT_DXT3,           WINED3DFMT_B4G4R4X4_UNORM,  convert_dxt3_x4r4g4b4},
    {WINED3DFMT_DXT5,           WINED3DFMT_B8G8R8A8_UNORM,  convert_dxt5_a8r8g8b8},
    {WINED3DFMT_DXT5,           WINED3DFMT_B8G8R8X8_UNORM,  convert_dxt5_x8r8g8b8},

    /* encode DXT */
    {WINED3DFMT_B8G8R8A8_UNORM, WINED3DFMT_DXT1,            convert_a8r8g8b8_dxt1},
    {WINED3DFMT_B8G8R8X8_UNORM, WINED3DFMT_DXT1,            convert_x8r8g8b8_dxt1},
    {WINED3DFMT_B5G5R5A1_UNORM, WINED3DFMT_DXT1,            convert_a1r5g5b5_dxt1},
    {WINED3DFMT_B5G5R5X1_UNORM, WINED3DFMT_DXT1,            convert_x1r5g5b5_dxt1},
    {WINED3DFMT_B8G8R8A8_UNORM, WINED3DFMT_DXT3,            convert_a8r8g8b8_dxt3},
    {WINED3DFMT_B8G8R8X8_UNORM, WINED3DFMT_DXT3,            convert_x8r8g8b8_dxt3},
    {WINED3DFMT_B8G8R8A8_UNORM, WINED3DFMT_DXT5,            convert_a8r8g8b8_dxt5},
    {WINED3DFMT_B8G8R8X8_UNORM, WINED3DFMT_DXT5,            convert_x8r8g8b8_dxt5}
};

static inline const struct d3dfmt_converter_desc *find_converter(enum wined3d_format_id from,
        enum wined3d_format_id to)
{
    unsigned int i;

    for (i = 0; i < (sizeof(converters) / sizeof(*converters)); ++i)
    {
        if (converters[i].from == from && converters[i].to == to)
            return &converters[i];
    }

    for (i = 0; i < (sizeof(dxtn_converters) / sizeof(*dxtn_converters)); ++i)
    {
        if (dxtn_converters[i].from == from && dxtn_converters[i].to == to)
            return wined3d_dxtn_supported() ? &dxtn_converters[i] : NULL;
    }

    return NULL;
}

static struct wined3d_texture *surface_convert_format(struct wined3d_surface *source, enum wined3d_format_id to_fmt)
{
    struct wined3d_map_desc src_map, dst_map;
    const struct d3dfmt_converter_desc *conv;
    struct wined3d_texture *ret = NULL;
    struct wined3d_resource_desc desc;
    struct wined3d_surface *dst;

    conv = find_converter(source->resource.format->id, to_fmt);
    if (!conv)
    {
        FIXME("Cannot find a conversion function from format %s to %s.\n",
                debug_d3dformat(source->resource.format->id), debug_d3dformat(to_fmt));
        return NULL;
    }

    /* FIXME: Multisampled conversion? */
    wined3d_resource_get_desc(&source->resource, &desc);
    desc.resource_type = WINED3D_RTYPE_TEXTURE;
    desc.format = to_fmt;
    desc.usage = 0;
    desc.pool = WINED3D_POOL_SCRATCH;
    if (FAILED(wined3d_texture_create(source->resource.device, &desc, 1,
            WINED3D_SURFACE_MAPPABLE | WINED3D_SURFACE_DISCARD, NULL, NULL, &wined3d_null_parent_ops, &ret)))
    {
        ERR("Failed to create a destination surface for conversion.\n");
        return NULL;
    }
    dst = surface_from_resource(wined3d_texture_get_sub_resource(ret, 0));

    memset(&src_map, 0, sizeof(src_map));
    memset(&dst_map, 0, sizeof(dst_map));

    if (FAILED(wined3d_surface_map(source, &src_map, NULL, WINED3D_MAP_READONLY)))
    {
        ERR("Failed to lock the source surface.\n");
        wined3d_texture_decref(ret);
        return NULL;
    }
    if (FAILED(wined3d_surface_map(dst, &dst_map, NULL, 0)))
    {
        ERR("Failed to lock the destination surface.\n");
        wined3d_surface_unmap(source);
        wined3d_texture_decref(ret);
        return NULL;
    }

    conv->convert(src_map.data, dst_map.data, src_map.row_pitch, dst_map.row_pitch,
            source->resource.width, source->resource.height);

    wined3d_surface_unmap(dst);
    wined3d_surface_unmap(source);

    return ret;
}

static HRESULT _Blt_ColorFill(BYTE *buf, unsigned int width, unsigned int height,
        unsigned int bpp, UINT pitch, DWORD color)
{
    BYTE *first;
    unsigned int x, y;

    /* Do first row */

#define COLORFILL_ROW(type) \
do { \
    type *d = (type *)buf; \
    for (x = 0; x < width; ++x) \
        d[x] = (type)color; \
} while(0)

    switch (bpp)
    {
        case 1:
            COLORFILL_ROW(BYTE);
            break;

        case 2:
            COLORFILL_ROW(WORD);
            break;

        case 3:
        {
            BYTE *d = buf;
            for (x = 0; x < width; ++x, d += 3)
            {
                d[0] = (color      ) & 0xff;
                d[1] = (color >>  8) & 0xff;
                d[2] = (color >> 16) & 0xff;
            }
            break;
        }
        case 4:
            COLORFILL_ROW(DWORD);
            break;

        default:
            FIXME("Color fill not implemented for bpp %u!\n", bpp * 8);
            return WINED3DERR_NOTAVAILABLE;
    }

#undef COLORFILL_ROW

    /* Now copy first row. */
    first = buf;
    for (y = 1; y < height; ++y)
    {
        buf += pitch;
        memcpy(buf, first, width * bpp);
    }

    return WINED3D_OK;
}

struct wined3d_surface * CDECL wined3d_surface_from_resource(struct wined3d_resource *resource)
{
    return surface_from_resource(resource);
}

HRESULT CDECL wined3d_surface_unmap(struct wined3d_surface *surface)
{
    TRACE("surface %p.\n", surface);

    if (!surface->resource.map_count)
    {
        WARN("Trying to unmap unmapped surface.\n");
        return WINEDDERR_NOTLOCKED;
    }
    --surface->resource.map_count;

    surface->surface_ops->surface_unmap(surface);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_surface_map(struct wined3d_surface *surface,
        struct wined3d_map_desc *map_desc, const RECT *rect, DWORD flags)
{
    const struct wined3d_format *format = surface->resource.format;
    struct wined3d_device *device = surface->resource.device;
    struct wined3d_context *context;
    const struct wined3d_gl_info *gl_info;
    BYTE *base_memory;

    TRACE("surface %p, map_desc %p, rect %s, flags %#x.\n",
            surface, map_desc, wine_dbgstr_rect(rect), flags);

    if (surface->resource.map_count)
    {
        WARN("Surface is already mapped.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if ((format->flags & WINED3DFMT_FLAG_BLOCKS) && rect
            && !surface_check_block_align(surface, rect))
    {
        WARN("Map rect %s is misaligned for %ux%u blocks.\n",
                wine_dbgstr_rect(rect), format->block_width, format->block_height);

        if (surface->resource.pool == WINED3D_POOL_DEFAULT)
            return WINED3DERR_INVALIDCALL;
    }

    ++surface->resource.map_count;

    if (!(surface->resource.access_flags & WINED3D_RESOURCE_ACCESS_CPU))
        WARN("Trying to lock unlockable surface.\n");

    /* Performance optimization: Count how often a surface is mapped, if it is
     * mapped regularly do not throw away the system memory copy. This avoids
     * the need to download the surface from OpenGL all the time. The surface
     * is still downloaded if the OpenGL texture is changed. Note that this
     * only really makes sense for managed textures.*/
    if (!(surface->container->flags & WINED3D_TEXTURE_DYNAMIC_MAP)
            && surface->resource.map_binding == WINED3D_LOCATION_SYSMEM)
    {
        if (++surface->lockCount > MAXLOCKCOUNT)
        {
            TRACE("Surface is mapped regularly, not freeing the system memory copy any more.\n");
            surface->container->flags |= WINED3D_TEXTURE_DYNAMIC_MAP;
        }
    }

    surface_prepare_map_memory(surface);
    if (flags & WINED3D_MAP_DISCARD)
    {
        TRACE("WINED3D_MAP_DISCARD flag passed, marking %s as up to date.\n",
                wined3d_debug_location(surface->resource.map_binding));
        surface_validate_location(surface, surface->resource.map_binding);
    }
    else
    {
        if (surface->resource.usage & WINED3DUSAGE_DYNAMIC)
            WARN_(d3d_perf)("Mapping a dynamic surface without WINED3D_MAP_DISCARD.\n");

        surface_load_location(surface, surface->resource.map_binding);
    }

    if (!(flags & (WINED3D_MAP_NO_DIRTY_UPDATE | WINED3D_MAP_READONLY)))
        surface_invalidate_location(surface, ~surface->resource.map_binding);

    switch (surface->resource.map_binding)
    {
        case WINED3D_LOCATION_SYSMEM:
            base_memory = surface->resource.heap_memory;
            break;

        case WINED3D_LOCATION_USER_MEMORY:
            base_memory = surface->user_memory;
            break;

        case WINED3D_LOCATION_DIB:
            base_memory = surface->dib.bitmap_data;
            break;

        case WINED3D_LOCATION_BUFFER:
            context = context_acquire(device, NULL);
            gl_info = context->gl_info;

            GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, surface->pbo));
            base_memory = GL_EXTCALL(glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_READ_WRITE));
            GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
            checkGLcall("map PBO");

            context_release(context);
            break;

        default:
            ERR("Unexpected map binding %s.\n", wined3d_debug_location(surface->resource.map_binding));
            base_memory = NULL;
    }

    if (format->flags & WINED3DFMT_FLAG_BROKEN_PITCH)
        map_desc->row_pitch = surface->resource.width * format->byte_count;
    else
        map_desc->row_pitch = wined3d_surface_get_pitch(surface);
    map_desc->slice_pitch = 0;

    if (!rect)
    {
        map_desc->data = base_memory;
        surface->lockedRect.left = 0;
        surface->lockedRect.top = 0;
        surface->lockedRect.right = surface->resource.width;
        surface->lockedRect.bottom = surface->resource.height;
    }
    else
    {
        if ((format->flags & (WINED3DFMT_FLAG_BLOCKS | WINED3DFMT_FLAG_BROKEN_PITCH)) == WINED3DFMT_FLAG_BLOCKS)
        {
            /* Compressed textures are block based, so calculate the offset of
             * the block that contains the top-left pixel of the locked rectangle. */
            map_desc->data = base_memory
                    + ((rect->top / format->block_height) * map_desc->row_pitch)
                    + ((rect->left / format->block_width) * format->block_byte_count);
        }
        else
        {
            map_desc->data = base_memory
                    + (map_desc->row_pitch * rect->top)
                    + (rect->left * format->byte_count);
        }
        surface->lockedRect.left = rect->left;
        surface->lockedRect.top = rect->top;
        surface->lockedRect.right = rect->right;
        surface->lockedRect.bottom = rect->bottom;
    }

    TRACE("Locked rect %s.\n", wine_dbgstr_rect(&surface->lockedRect));
    TRACE("Returning memory %p, pitch %u.\n", map_desc->data, map_desc->row_pitch);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_surface_getdc(struct wined3d_surface *surface, HDC *dc)
{
    HRESULT hr;

    TRACE("surface %p, dc %p.\n", surface, dc);

    /* Give more detailed info for ddraw. */
    if (surface->flags & SFLAG_DCINUSE)
        return WINEDDERR_DCALREADYCREATED;

    /* Can't GetDC if the surface is locked. */
    if (surface->resource.map_count)
        return WINED3DERR_INVALIDCALL;

    /* Create a DIB section if there isn't a dc yet. */
    if (!surface->hDC)
    {
        if (surface->flags & SFLAG_CLIENT)
        {
            surface_load_location(surface, WINED3D_LOCATION_SYSMEM);
            surface_release_client_storage(surface);
        }
        hr = surface_create_dib_section(surface);
        if (FAILED(hr))
            return WINED3DERR_INVALIDCALL;
        if (!(surface->resource.map_binding == WINED3D_LOCATION_USER_MEMORY
                || surface->container->flags & WINED3D_TEXTURE_PIN_SYSMEM
                || surface->pbo))
            surface->resource.map_binding = WINED3D_LOCATION_DIB;
    }

    surface_load_location(surface, WINED3D_LOCATION_DIB);
    surface_invalidate_location(surface, ~WINED3D_LOCATION_DIB);

    surface->flags |= SFLAG_DCINUSE;
    surface->resource.map_count++;

    *dc = surface->hDC;
    TRACE("Returning dc %p.\n", *dc);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_surface_releasedc(struct wined3d_surface *surface, HDC dc)
{
    TRACE("surface %p, dc %p.\n", surface, dc);

    if (!(surface->flags & SFLAG_DCINUSE))
        return WINEDDERR_NODC;

    if (surface->hDC != dc)
    {
        WARN("Application tries to release invalid DC %p, surface DC is %p.\n",
                dc, surface->hDC);
        return WINEDDERR_NODC;
    }

    surface->resource.map_count--;
    surface->flags &= ~SFLAG_DCINUSE;

    if (surface->resource.map_binding == WINED3D_LOCATION_USER_MEMORY
            || (surface->container->flags & WINED3D_TEXTURE_PIN_SYSMEM
            && surface->resource.map_binding != WINED3D_LOCATION_DIB))
    {
        /* The game Salammbo modifies the surface contents without mapping the surface between
         * a GetDC/ReleaseDC operation and flipping the surface. If the DIB remains the active
         * copy and is copied to the screen, this update, which draws the mouse pointer, is lost.
         * Do not only copy the DIB to the map location, but also make sure the map location is
         * copied back to the DIB in the next getdc call.
         *
         * The same consideration applies to user memory surfaces. */
        surface_load_location(surface, surface->resource.map_binding);
        surface_invalidate_location(surface, WINED3D_LOCATION_DIB);
    }

    return WINED3D_OK;
}

static void read_from_framebuffer(struct wined3d_surface *surface, DWORD dst_location)
{
    struct wined3d_device *device = surface->resource.device;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;
    BYTE *mem;
    BYTE *row, *top, *bottom;
    int i;
    BOOL srcIsUpsideDown;
    struct wined3d_bo_address data;

    surface_get_memory(surface, &data, dst_location);

    context = context_acquire(device, surface);
    context_apply_blit_state(context, device);
    gl_info = context->gl_info;

    /* Select the correct read buffer, and give some debug output.
     * There is no need to keep track of the current read buffer or reset it, every part of the code
     * that reads sets the read buffer as desired.
     */
    if (wined3d_resource_is_offscreen(&surface->container->resource))
    {
        /* Mapping the primary render target which is not on a swapchain.
         * Read from the back buffer. */
        TRACE("Mapping offscreen render target.\n");
        gl_info->gl_ops.gl.p_glReadBuffer(device->offscreenBuffer);
        srcIsUpsideDown = TRUE;
    }
    else
    {
        /* Onscreen surfaces are always part of a swapchain */
        GLenum buffer = surface_get_gl_buffer(surface);
        TRACE("Mapping %#x buffer.\n", buffer);
        gl_info->gl_ops.gl.p_glReadBuffer(buffer);
        checkGLcall("glReadBuffer");
        srcIsUpsideDown = FALSE;
    }

    if (data.buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, data.buffer_object));
        checkGLcall("glBindBuffer");
    }

    /* Setup pixel store pack state -- to glReadPixels into the correct place */
    gl_info->gl_ops.gl.p_glPixelStorei(GL_PACK_ROW_LENGTH,
            wined3d_surface_get_pitch(surface) / surface->resource.format->byte_count);
    checkGLcall("glPixelStorei");

    gl_info->gl_ops.gl.p_glReadPixels(0, 0,
            surface->resource.width, surface->resource.height,
            surface->resource.format->glFormat,
            surface->resource.format->glType, data.addr);
    checkGLcall("glReadPixels");

    /* Reset previous pixel store pack state */
    gl_info->gl_ops.gl.p_glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    checkGLcall("glPixelStorei");

    if (!srcIsUpsideDown)
    {
        /* glReadPixels returns the image upside down, and there is no way to prevent this.
         * Flip the lines in software. */
        UINT pitch = wined3d_surface_get_pitch(surface);

        if (!(row = HeapAlloc(GetProcessHeap(), 0, pitch)))
            goto error;

        if (data.buffer_object)
        {
            mem = GL_EXTCALL(glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_WRITE));
            checkGLcall("glMapBuffer");
        }
        else
            mem = data.addr;

        top = mem;
        bottom = mem + pitch * (surface->resource.height - 1);
        for (i = 0; i < surface->resource.height / 2; i++)
        {
            memcpy(row, top, pitch);
            memcpy(top, bottom, pitch);
            memcpy(bottom, row, pitch);
            top += pitch;
            bottom -= pitch;
        }
        HeapFree(GetProcessHeap(), 0, row);

        if (data.buffer_object)
            GL_EXTCALL(glUnmapBuffer(GL_PIXEL_PACK_BUFFER));
    }

error:
    if (data.buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
        checkGLcall("glBindBuffer");
    }

    context_release(context);
}

/* Read the framebuffer contents into a texture. Note that this function
 * doesn't do any kind of flipping. Using this on an onscreen surface will
 * result in a flipped D3D texture. */
void surface_load_fb_texture(struct wined3d_surface *surface, BOOL srgb)
{
    struct wined3d_device *device = surface->resource.device;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;

    context = context_acquire(device, surface);
    gl_info = context->gl_info;
    device_invalidate_state(device, STATE_FRAMEBUFFER);

    wined3d_texture_prepare_texture(surface->container, context, srgb);
    wined3d_texture_bind_and_dirtify(surface->container, context, srgb);

    TRACE("Reading back offscreen render target %p.\n", surface);

    if (wined3d_resource_is_offscreen(&surface->container->resource))
        gl_info->gl_ops.gl.p_glReadBuffer(device->offscreenBuffer);
    else
        gl_info->gl_ops.gl.p_glReadBuffer(surface_get_gl_buffer(surface));
    checkGLcall("glReadBuffer");

    gl_info->gl_ops.gl.p_glCopyTexSubImage2D(surface->texture_target, surface->texture_level,
            0, 0, 0, 0, surface->resource.width, surface->resource.height);
    checkGLcall("glCopyTexSubImage2D");

    context_release(context);
}

void surface_prepare_rb(struct wined3d_surface *surface, const struct wined3d_gl_info *gl_info, BOOL multisample)
{
    if (multisample)
    {
        if (surface->rb_multisample)
            return;

        gl_info->fbo_ops.glGenRenderbuffers(1, &surface->rb_multisample);
        gl_info->fbo_ops.glBindRenderbuffer(GL_RENDERBUFFER, surface->rb_multisample);
        gl_info->fbo_ops.glRenderbufferStorageMultisample(GL_RENDERBUFFER, surface->resource.multisample_type,
                surface->resource.format->glInternal, surface->pow2Width, surface->pow2Height);
        TRACE("Created multisample rb %u.\n", surface->rb_multisample);
    }
    else
    {
        if (surface->rb_resolved)
            return;

        gl_info->fbo_ops.glGenRenderbuffers(1, &surface->rb_resolved);
        gl_info->fbo_ops.glBindRenderbuffer(GL_RENDERBUFFER, surface->rb_resolved);
        gl_info->fbo_ops.glRenderbufferStorage(GL_RENDERBUFFER, surface->resource.format->glInternal,
                surface->pow2Width, surface->pow2Height);
        TRACE("Created resolved rb %u.\n", surface->rb_resolved);
    }
}

void flip_surface(struct wined3d_surface *front, struct wined3d_surface *back)
{
    if (front->container->level_count != 1 || front->container->layer_count != 1
            || back->container->level_count != 1 || back->container->layer_count != 1)
        ERR("Flip between surfaces %p and %p not supported.\n", front, back);

    /* Flip the surface contents */
    /* Flip the DC */
    {
        HDC tmp;
        tmp = front->hDC;
        front->hDC = back->hDC;
        back->hDC = tmp;
    }

    /* Flip the DIBsection */
    {
        HBITMAP tmp = front->dib.DIBsection;
        front->dib.DIBsection = back->dib.DIBsection;
        back->dib.DIBsection = tmp;
    }

    /* Flip the surface data */
    {
        void* tmp;

        tmp = front->dib.bitmap_data;
        front->dib.bitmap_data = back->dib.bitmap_data;
        back->dib.bitmap_data = tmp;

        tmp = front->resource.heap_memory;
        front->resource.heap_memory = back->resource.heap_memory;
        back->resource.heap_memory = tmp;
    }

    /* Flip the PBO */
    {
        GLuint tmp_pbo = front->pbo;
        front->pbo = back->pbo;
        back->pbo = tmp_pbo;
    }

    /* Flip the opengl texture */
    {
        GLuint tmp;

        tmp = back->container->texture_rgb.name;
        back->container->texture_rgb.name = front->container->texture_rgb.name;
        front->container->texture_rgb.name = tmp;

        tmp = back->container->texture_srgb.name;
        back->container->texture_srgb.name = front->container->texture_srgb.name;
        front->container->texture_srgb.name = tmp;

        tmp = back->rb_multisample;
        back->rb_multisample = front->rb_multisample;
        front->rb_multisample = tmp;

        tmp = back->rb_resolved;
        back->rb_resolved = front->rb_resolved;
        front->rb_resolved = tmp;

        resource_unload(&back->resource);
        resource_unload(&front->resource);
    }

    {
        DWORD tmp_flags = back->flags;
        back->flags = front->flags;
        front->flags = tmp_flags;

        tmp_flags = back->locations;
        back->locations = front->locations;
        front->locations = tmp_flags;
    }
}

/* Does a direct frame buffer -> texture copy. Stretching is done with single
 * pixel copy calls. */
static void fb_copy_to_texture_direct(struct wined3d_surface *dst_surface, struct wined3d_surface *src_surface,
        const RECT *src_rect, const RECT *dst_rect_in, enum wined3d_texture_filter_type filter)
{
    struct wined3d_device *device = dst_surface->resource.device;
    const struct wined3d_gl_info *gl_info;
    float xrel, yrel;
    struct wined3d_context *context;
    BOOL upsidedown = FALSE;
    RECT dst_rect = *dst_rect_in;

    /* Make sure that the top pixel is always above the bottom pixel, and keep a separate upside down flag
     * glCopyTexSubImage is a bit picky about the parameters we pass to it
     */
    if(dst_rect.top > dst_rect.bottom) {
        UINT tmp = dst_rect.bottom;
        dst_rect.bottom = dst_rect.top;
        dst_rect.top = tmp;
        upsidedown = TRUE;
    }

    context = context_acquire(device, src_surface);
    gl_info = context->gl_info;
    context_apply_blit_state(context, device);
    wined3d_texture_load(dst_surface->container, context, FALSE);

    /* Bind the target texture */
    context_bind_texture(context, dst_surface->container->target, dst_surface->container->texture_rgb.name);
    if (wined3d_resource_is_offscreen(&src_surface->container->resource))
    {
        TRACE("Reading from an offscreen target\n");
        upsidedown = !upsidedown;
        gl_info->gl_ops.gl.p_glReadBuffer(device->offscreenBuffer);
    }
    else
    {
        gl_info->gl_ops.gl.p_glReadBuffer(surface_get_gl_buffer(src_surface));
    }
    checkGLcall("glReadBuffer");

    xrel = (float) (src_rect->right - src_rect->left) / (float) (dst_rect.right - dst_rect.left);
    yrel = (float) (src_rect->bottom - src_rect->top) / (float) (dst_rect.bottom - dst_rect.top);

    if ((xrel - 1.0f < -eps) || (xrel - 1.0f > eps))
    {
        FIXME("Doing a pixel by pixel copy from the framebuffer to a texture, expect major performance issues\n");

        if (filter != WINED3D_TEXF_NONE && filter != WINED3D_TEXF_POINT)
            ERR("Texture filtering not supported in direct blit.\n");
    }
    else if ((filter != WINED3D_TEXF_NONE && filter != WINED3D_TEXF_POINT)
            && ((yrel - 1.0f < -eps) || (yrel - 1.0f > eps)))
    {
        ERR("Texture filtering not supported in direct blit\n");
    }

    if (upsidedown
            && !((xrel - 1.0f < -eps) || (xrel - 1.0f > eps))
            && !((yrel - 1.0f < -eps) || (yrel - 1.0f > eps)))
    {
        /* Upside down copy without stretching is nice, one glCopyTexSubImage call will do. */
        gl_info->gl_ops.gl.p_glCopyTexSubImage2D(dst_surface->texture_target, dst_surface->texture_level,
                dst_rect.left /*xoffset */, dst_rect.top /* y offset */,
                src_rect->left, src_surface->resource.height - src_rect->bottom,
                dst_rect.right - dst_rect.left, dst_rect.bottom - dst_rect.top);
    }
    else
    {
        LONG row;
        UINT yoffset = src_surface->resource.height - src_rect->top + dst_rect.top - 1;
        /* I have to process this row by row to swap the image,
         * otherwise it would be upside down, so stretching in y direction
         * doesn't cost extra time
         *
         * However, stretching in x direction can be avoided if not necessary
         */
        for(row = dst_rect.top; row < dst_rect.bottom; row++) {
            if ((xrel - 1.0f < -eps) || (xrel - 1.0f > eps))
            {
                /* Well, that stuff works, but it's very slow.
                 * find a better way instead
                 */
                LONG col;

                for (col = dst_rect.left; col < dst_rect.right; ++col)
                {
                    gl_info->gl_ops.gl.p_glCopyTexSubImage2D(dst_surface->texture_target, dst_surface->texture_level,
                            dst_rect.left + col /* x offset */, row /* y offset */,
                            src_rect->left + col * xrel, yoffset - (int) (row * yrel), 1, 1);
                }
            }
            else
            {
                gl_info->gl_ops.gl.p_glCopyTexSubImage2D(dst_surface->texture_target, dst_surface->texture_level,
                        dst_rect.left /* x offset */, row /* y offset */,
                        src_rect->left, yoffset - (int) (row * yrel), dst_rect.right - dst_rect.left, 1);
            }
        }
    }
    checkGLcall("glCopyTexSubImage2D");

    context_release(context);

    /* The texture is now most up to date - If the surface is a render target
     * and has a drawable, this path is never entered. */
    surface_validate_location(dst_surface, WINED3D_LOCATION_TEXTURE_RGB);
    surface_invalidate_location(dst_surface, ~WINED3D_LOCATION_TEXTURE_RGB);
}

/* Uses the hardware to stretch and flip the image */
static void fb_copy_to_texture_hwstretch(struct wined3d_surface *dst_surface, struct wined3d_surface *src_surface,
        const RECT *src_rect, const RECT *dst_rect_in, enum wined3d_texture_filter_type filter)
{
    struct wined3d_device *device = dst_surface->resource.device;
    GLuint src, backup = 0;
    float left, right, top, bottom; /* Texture coordinates */
    UINT fbwidth = src_surface->resource.width;
    UINT fbheight = src_surface->resource.height;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;
    GLenum drawBuffer = GL_BACK;
    GLenum texture_target;
    BOOL noBackBufferBackup;
    BOOL src_offscreen;
    BOOL upsidedown = FALSE;
    RECT dst_rect = *dst_rect_in;

    TRACE("Using hwstretch blit\n");
    /* Activate the Proper context for reading from the source surface, set it up for blitting */
    context = context_acquire(device, src_surface);
    gl_info = context->gl_info;
    context_apply_blit_state(context, device);
    wined3d_texture_load(dst_surface->container, context, FALSE);

    src_offscreen = wined3d_resource_is_offscreen(&src_surface->container->resource);
    noBackBufferBackup = src_offscreen && wined3d_settings.offscreen_rendering_mode == ORM_FBO;
    if (!noBackBufferBackup && !src_surface->container->texture_rgb.name)
    {
        /* Get it a description */
        wined3d_texture_load(src_surface->container, context, FALSE);
    }

    /* Try to use an aux buffer for drawing the rectangle. This way it doesn't need restoring.
     * This way we don't have to wait for the 2nd readback to finish to leave this function.
     */
    if (context->aux_buffers >= 2)
    {
        /* Got more than one aux buffer? Use the 2nd aux buffer */
        drawBuffer = GL_AUX1;
    }
    else if ((!src_offscreen || device->offscreenBuffer == GL_BACK) && context->aux_buffers >= 1)
    {
        /* Only one aux buffer, but it isn't used (Onscreen rendering, or non-aux orm)? Use it! */
        drawBuffer = GL_AUX0;
    }

    if (noBackBufferBackup)
    {
        gl_info->gl_ops.gl.p_glGenTextures(1, &backup);
        checkGLcall("glGenTextures");
        context_bind_texture(context, GL_TEXTURE_2D, backup);
        texture_target = GL_TEXTURE_2D;
    }
    else
    {
        /* Backup the back buffer and copy the source buffer into a texture to draw an upside down stretched quad. If
         * we are reading from the back buffer, the backup can be used as source texture
         */
        texture_target = src_surface->texture_target;
        context_bind_texture(context, texture_target, src_surface->container->texture_rgb.name);
        gl_info->gl_ops.gl.p_glEnable(texture_target);
        checkGLcall("glEnable(texture_target)");

        /* For now invalidate the texture copy of the back buffer. Drawable and sysmem copy are untouched */
        src_surface->locations &= ~WINED3D_LOCATION_TEXTURE_RGB;
    }

    /* Make sure that the top pixel is always above the bottom pixel, and keep a separate upside down flag
     * glCopyTexSubImage is a bit picky about the parameters we pass to it
     */
    if(dst_rect.top > dst_rect.bottom) {
        UINT tmp = dst_rect.bottom;
        dst_rect.bottom = dst_rect.top;
        dst_rect.top = tmp;
        upsidedown = TRUE;
    }

    if (src_offscreen)
    {
        TRACE("Reading from an offscreen target\n");
        upsidedown = !upsidedown;
        gl_info->gl_ops.gl.p_glReadBuffer(device->offscreenBuffer);
    }
    else
    {
        gl_info->gl_ops.gl.p_glReadBuffer(surface_get_gl_buffer(src_surface));
    }

    /* TODO: Only back up the part that will be overwritten */
    gl_info->gl_ops.gl.p_glCopyTexSubImage2D(texture_target, 0, 0, 0, 0, 0, fbwidth, fbheight);

    checkGLcall("glCopyTexSubImage2D");

    /* No issue with overriding these - the sampler is dirty due to blit usage */
    gl_info->gl_ops.gl.p_glTexParameteri(texture_target, GL_TEXTURE_MAG_FILTER, wined3d_gl_mag_filter(filter));
    checkGLcall("glTexParameteri");
    gl_info->gl_ops.gl.p_glTexParameteri(texture_target, GL_TEXTURE_MIN_FILTER,
            wined3d_gl_min_mip_filter(filter, WINED3D_TEXF_NONE));
    checkGLcall("glTexParameteri");

    if (!src_surface->container->swapchain
            || src_surface->container == src_surface->container->swapchain->back_buffers[0])
    {
        src = backup ? backup : src_surface->container->texture_rgb.name;
    }
    else
    {
        gl_info->gl_ops.gl.p_glReadBuffer(GL_FRONT);
        checkGLcall("glReadBuffer(GL_FRONT)");

        gl_info->gl_ops.gl.p_glGenTextures(1, &src);
        checkGLcall("glGenTextures(1, &src)");
        context_bind_texture(context, GL_TEXTURE_2D, src);

        /* TODO: Only copy the part that will be read. Use src_rect->left, src_rect->bottom as origin, but with the width watch
         * out for power of 2 sizes
         */
        gl_info->gl_ops.gl.p_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, src_surface->pow2Width,
                src_surface->pow2Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        checkGLcall("glTexImage2D");
        gl_info->gl_ops.gl.p_glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, fbwidth, fbheight);

        gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        checkGLcall("glTexParameteri");
        gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        checkGLcall("glTexParameteri");

        gl_info->gl_ops.gl.p_glReadBuffer(GL_BACK);
        checkGLcall("glReadBuffer(GL_BACK)");

        if (texture_target != GL_TEXTURE_2D)
        {
            gl_info->gl_ops.gl.p_glDisable(texture_target);
            gl_info->gl_ops.gl.p_glEnable(GL_TEXTURE_2D);
            texture_target = GL_TEXTURE_2D;
        }
    }
    checkGLcall("glEnd and previous");

    left = src_rect->left;
    right = src_rect->right;

    if (!upsidedown)
    {
        top = src_surface->resource.height - src_rect->top;
        bottom = src_surface->resource.height - src_rect->bottom;
    }
    else
    {
        top = src_surface->resource.height - src_rect->bottom;
        bottom = src_surface->resource.height - src_rect->top;
    }

    if (src_surface->container->flags & WINED3D_TEXTURE_NORMALIZED_COORDS)
    {
        left /= src_surface->pow2Width;
        right /= src_surface->pow2Width;
        top /= src_surface->pow2Height;
        bottom /= src_surface->pow2Height;
    }

    /* draw the source texture stretched and upside down. The correct surface is bound already */
    gl_info->gl_ops.gl.p_glTexParameteri(texture_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl_info->gl_ops.gl.p_glTexParameteri(texture_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    context_set_draw_buffer(context, drawBuffer);
    gl_info->gl_ops.gl.p_glReadBuffer(drawBuffer);

    gl_info->gl_ops.gl.p_glBegin(GL_QUADS);
        /* bottom left */
        gl_info->gl_ops.gl.p_glTexCoord2f(left, bottom);
        gl_info->gl_ops.gl.p_glVertex2i(0, 0);

        /* top left */
        gl_info->gl_ops.gl.p_glTexCoord2f(left, top);
        gl_info->gl_ops.gl.p_glVertex2i(0, dst_rect.bottom - dst_rect.top);

        /* top right */
        gl_info->gl_ops.gl.p_glTexCoord2f(right, top);
        gl_info->gl_ops.gl.p_glVertex2i(dst_rect.right - dst_rect.left, dst_rect.bottom - dst_rect.top);

        /* bottom right */
        gl_info->gl_ops.gl.p_glTexCoord2f(right, bottom);
        gl_info->gl_ops.gl.p_glVertex2i(dst_rect.right - dst_rect.left, 0);
    gl_info->gl_ops.gl.p_glEnd();
    checkGLcall("glEnd and previous");

    if (texture_target != dst_surface->texture_target)
    {
        gl_info->gl_ops.gl.p_glDisable(texture_target);
        gl_info->gl_ops.gl.p_glEnable(dst_surface->texture_target);
        texture_target = dst_surface->texture_target;
    }

    /* Now read the stretched and upside down image into the destination texture */
    context_bind_texture(context, texture_target, dst_surface->container->texture_rgb.name);
    gl_info->gl_ops.gl.p_glCopyTexSubImage2D(texture_target,
                        0,
                        dst_rect.left, dst_rect.top, /* xoffset, yoffset */
                        0, 0, /* We blitted the image to the origin */
                        dst_rect.right - dst_rect.left, dst_rect.bottom - dst_rect.top);
    checkGLcall("glCopyTexSubImage2D");

    if (drawBuffer == GL_BACK)
    {
        /* Write the back buffer backup back. */
        if (backup)
        {
            if (texture_target != GL_TEXTURE_2D)
            {
                gl_info->gl_ops.gl.p_glDisable(texture_target);
                gl_info->gl_ops.gl.p_glEnable(GL_TEXTURE_2D);
                texture_target = GL_TEXTURE_2D;
            }
            context_bind_texture(context, GL_TEXTURE_2D, backup);
        }
        else
        {
            if (texture_target != src_surface->texture_target)
            {
                gl_info->gl_ops.gl.p_glDisable(texture_target);
                gl_info->gl_ops.gl.p_glEnable(src_surface->texture_target);
                texture_target = src_surface->texture_target;
            }
            context_bind_texture(context, src_surface->texture_target, src_surface->container->texture_rgb.name);
        }

        gl_info->gl_ops.gl.p_glBegin(GL_QUADS);
            /* top left */
            gl_info->gl_ops.gl.p_glTexCoord2f(0.0f, 0.0f);
            gl_info->gl_ops.gl.p_glVertex2i(0, fbheight);

            /* bottom left */
            gl_info->gl_ops.gl.p_glTexCoord2f(0.0f, (float)fbheight / (float)src_surface->pow2Height);
            gl_info->gl_ops.gl.p_glVertex2i(0, 0);

            /* bottom right */
            gl_info->gl_ops.gl.p_glTexCoord2f((float)fbwidth / (float)src_surface->pow2Width,
                    (float)fbheight / (float)src_surface->pow2Height);
            gl_info->gl_ops.gl.p_glVertex2i(fbwidth, 0);

            /* top right */
            gl_info->gl_ops.gl.p_glTexCoord2f((float)fbwidth / (float)src_surface->pow2Width, 0.0f);
            gl_info->gl_ops.gl.p_glVertex2i(fbwidth, fbheight);
        gl_info->gl_ops.gl.p_glEnd();
    }
    gl_info->gl_ops.gl.p_glDisable(texture_target);
    checkGLcall("glDisable(texture_target)");

    /* Cleanup */
    if (src != src_surface->container->texture_rgb.name && src != backup)
    {
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &src);
        checkGLcall("glDeleteTextures(1, &src)");
    }
    if (backup)
    {
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &backup);
        checkGLcall("glDeleteTextures(1, &backup)");
    }

    if (wined3d_settings.strict_draw_ordering)
        gl_info->gl_ops.gl.p_glFlush(); /* Flush to ensure ordering across contexts. */

    context_release(context);

    /* The texture is now most up to date - If the surface is a render target
     * and has a drawable, this path is never entered. */
    surface_validate_location(dst_surface, WINED3D_LOCATION_TEXTURE_RGB);
    surface_invalidate_location(dst_surface, ~WINED3D_LOCATION_TEXTURE_RGB);
}

/* Front buffer coordinates are always full screen coordinates, but our GL
 * drawable is limited to the window's client area. The sysmem and texture
 * copies do have the full screen size. Note that GL has a bottom-left
 * origin, while D3D has a top-left origin. */
void surface_translate_drawable_coords(const struct wined3d_surface *surface, HWND window, RECT *rect)
{
    UINT drawable_height;

    if (surface->container->swapchain && surface->container == surface->container->swapchain->front_buffer)
    {
        POINT offset = {0, 0};
        RECT windowsize;

        ScreenToClient(window, &offset);
        OffsetRect(rect, offset.x, offset.y);

        GetClientRect(window, &windowsize);
        drawable_height = windowsize.bottom - windowsize.top;
    }
    else
    {
        drawable_height = surface->resource.height;
    }

    rect->top = drawable_height - rect->top;
    rect->bottom = drawable_height - rect->bottom;
}

static void surface_blt_to_drawable(const struct wined3d_device *device,
        enum wined3d_texture_filter_type filter, BOOL alpha_test,
        struct wined3d_surface *src_surface, const RECT *src_rect_in,
        struct wined3d_surface *dst_surface, const RECT *dst_rect_in)
{
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;
    RECT src_rect, dst_rect;

    src_rect = *src_rect_in;
    dst_rect = *dst_rect_in;

    context = context_acquire(device, dst_surface);
    gl_info = context->gl_info;

    /* Make sure the surface is up-to-date. This should probably use
     * surface_load_location() and worry about the destination surface too,
     * unless we're overwriting it completely. */
    wined3d_texture_load(src_surface->container, context, FALSE);

    /* Activate the destination context, set it up for blitting */
    context_apply_blit_state(context, device);

    if (!wined3d_resource_is_offscreen(&dst_surface->container->resource))
        surface_translate_drawable_coords(dst_surface, context->win_handle, &dst_rect);

    device->blitter->set_shader(device->blit_priv, context, src_surface);

    if (alpha_test)
    {
        gl_info->gl_ops.gl.p_glEnable(GL_ALPHA_TEST);
        checkGLcall("glEnable(GL_ALPHA_TEST)");

        /* For P8 surfaces, the alpha component contains the palette index.
         * Which means that the colorkey is one of the palette entries. In
         * other cases pixels that should be masked away have alpha set to 0. */
        if (src_surface->resource.format->id == WINED3DFMT_P8_UINT)
            gl_info->gl_ops.gl.p_glAlphaFunc(GL_NOTEQUAL,
                    (float)src_surface->container->src_blt_color_key.color_space_low_value / 255.0f);
        else
            gl_info->gl_ops.gl.p_glAlphaFunc(GL_NOTEQUAL, 0.0f);
        checkGLcall("glAlphaFunc");
    }
    else
    {
        gl_info->gl_ops.gl.p_glDisable(GL_ALPHA_TEST);
        checkGLcall("glDisable(GL_ALPHA_TEST)");
    }

    draw_textured_quad(src_surface, context, &src_rect, &dst_rect, filter);

    if (alpha_test)
    {
        gl_info->gl_ops.gl.p_glDisable(GL_ALPHA_TEST);
        checkGLcall("glDisable(GL_ALPHA_TEST)");
    }

    /* Leave the opengl state valid for blitting */
    device->blitter->unset_shader(context->gl_info);

    if (wined3d_settings.strict_draw_ordering
            || (dst_surface->container->swapchain
            && dst_surface->container->swapchain->front_buffer == dst_surface->container))
        gl_info->gl_ops.gl.p_glFlush(); /* Flush to ensure ordering across contexts. */

    context_release(context);
}

HRESULT surface_color_fill(struct wined3d_surface *s, const RECT *rect, const struct wined3d_color *color)
{
    struct wined3d_device *device = s->resource.device;
    const struct blit_shader *blitter;

    blitter = wined3d_select_blitter(&device->adapter->gl_info, WINED3D_BLIT_OP_COLOR_FILL,
            NULL, 0, 0, NULL, rect, s->resource.usage, s->resource.pool, s->resource.format);
    if (!blitter)
    {
        FIXME("No blitter is capable of performing the requested color fill operation.\n");
        return WINED3DERR_INVALIDCALL;
    }

    return blitter->color_fill(device, s, rect, color);
}

static HRESULT surface_blt_special(struct wined3d_surface *dst_surface, const RECT *dst_rect,
        struct wined3d_surface *src_surface, const RECT *src_rect, DWORD flags, const WINEDDBLTFX *DDBltFx,
        enum wined3d_texture_filter_type filter)
{
    struct wined3d_device *device = dst_surface->resource.device;
    const struct wined3d_surface *rt = wined3d_rendertarget_view_get_surface(device->fb.render_targets[0]);
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_swapchain *src_swapchain, *dst_swapchain;

    TRACE("dst_surface %p, dst_rect %s, src_surface %p, src_rect %s, flags %#x, blt_fx %p, filter %s.\n",
            dst_surface, wine_dbgstr_rect(dst_rect), src_surface, wine_dbgstr_rect(src_rect),
            flags, DDBltFx, debug_d3dtexturefiltertype(filter));

    /* Get the swapchain. One of the surfaces has to be a primary surface */
    if (dst_surface->resource.pool == WINED3D_POOL_SYSTEM_MEM)
    {
        WARN("Destination is in sysmem, rejecting gl blt\n");
        return WINED3DERR_INVALIDCALL;
    }

    dst_swapchain = dst_surface->container->swapchain;

    if (src_surface)
    {
        if (src_surface->resource.pool == WINED3D_POOL_SYSTEM_MEM)
        {
            WARN("Src is in sysmem, rejecting gl blt\n");
            return WINED3DERR_INVALIDCALL;
        }

        src_swapchain = src_surface->container->swapchain;
    }
    else
    {
        src_swapchain = NULL;
    }

    /* Early sort out of cases where no render target is used */
    if (!dst_swapchain && !src_swapchain && src_surface != rt && dst_surface != rt)
    {
        TRACE("No surface is render target, not using hardware blit.\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* No destination color keying supported */
    if (flags & (WINEDDBLT_KEYDEST | WINEDDBLT_KEYDESTOVERRIDE))
    {
        /* Can we support that with glBlendFunc if blitting to the frame buffer? */
        TRACE("Destination color key not supported in accelerated Blit, falling back to software\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (dst_swapchain && dst_swapchain == src_swapchain)
    {
        FIXME("Implement hardware blit between two surfaces on the same swapchain\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (dst_swapchain && src_swapchain)
    {
        FIXME("Implement hardware blit between two different swapchains\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (dst_swapchain)
    {
        /* Handled with regular texture -> swapchain blit */
        if (src_surface == rt)
            TRACE("Blit from active render target to a swapchain\n");
    }
    else if (src_swapchain && dst_surface == rt)
    {
        FIXME("Implement blit from a swapchain to the active render target\n");
        return WINED3DERR_INVALIDCALL;
    }

    if ((src_swapchain || src_surface == rt) && !dst_swapchain)
    {
        /* Blit from render target to texture */
        BOOL stretchx;

        /* P8 read back is not implemented */
        if (src_surface->resource.format->id == WINED3DFMT_P8_UINT
                || dst_surface->resource.format->id == WINED3DFMT_P8_UINT)
        {
            TRACE("P8 read back not supported by frame buffer to texture blit\n");
            return WINED3DERR_INVALIDCALL;
        }

        if (flags & (WINEDDBLT_KEYSRC | WINEDDBLT_KEYSRCOVERRIDE))
        {
            TRACE("Color keying not supported by frame buffer to texture blit\n");
            return WINED3DERR_INVALIDCALL;
            /* Destination color key is checked above */
        }

        if (dst_rect->right - dst_rect->left != src_rect->right - src_rect->left)
            stretchx = TRUE;
        else
            stretchx = FALSE;

        /* Blt is a pretty powerful call, while glCopyTexSubImage2D is not. glCopyTexSubImage cannot
         * flip the image nor scale it.
         *
         * -> If the app asks for an unscaled, upside down copy, just perform one glCopyTexSubImage2D call
         * -> If the app wants an image width an unscaled width, copy it line per line
         * -> If the app wants an image that is scaled on the x axis, and the destination rectangle is smaller
         *    than the frame buffer, draw an upside down scaled image onto the fb, read it back and restore the
         *    back buffer. This is slower than reading line per line, thus not used for flipping
         * -> If the app wants a scaled image with a dest rect that is bigger than the fb, it has to be copied
         *    pixel by pixel. */
        if (!stretchx || dst_rect->right - dst_rect->left > src_surface->resource.width
                || dst_rect->bottom - dst_rect->top > src_surface->resource.height)
        {
            TRACE("No stretching in x direction, using direct framebuffer -> texture copy.\n");
            fb_copy_to_texture_direct(dst_surface, src_surface, src_rect, dst_rect, filter);
        }
        else
        {
            TRACE("Using hardware stretching to flip / stretch the texture.\n");
            fb_copy_to_texture_hwstretch(dst_surface, src_surface, src_rect, dst_rect, filter);
        }

        surface_evict_sysmem(dst_surface);

        return WINED3D_OK;
    }
    else if (src_surface)
    {
        /* Blit from offscreen surface to render target */
        struct wined3d_color_key old_blt_key = src_surface->container->src_blt_color_key;
        DWORD old_color_key_flags = src_surface->container->color_key_flags;

        TRACE("Blt from surface %p to rendertarget %p\n", src_surface, dst_surface);

        if (!device->blitter->blit_supported(gl_info, WINED3D_BLIT_OP_COLOR_BLIT,
                src_rect, src_surface->resource.usage, src_surface->resource.pool, src_surface->resource.format,
                dst_rect, dst_surface->resource.usage, dst_surface->resource.pool, dst_surface->resource.format))
        {
            FIXME("Unsupported blit operation falling back to software\n");
            return WINED3DERR_INVALIDCALL;
        }

        /* Color keying: Check if we have to do a color keyed blt,
         * and if not check if a color key is activated.
         *
         * Just modify the color keying parameters in the surface and restore them afterwards
         * The surface keeps track of the color key last used to load the opengl surface.
         * PreLoad will catch the change to the flags and color key and reload if necessary.
         */
        if (flags & WINEDDBLT_KEYSRC)
        {
            /* Use color key from surface */
        }
        else if (flags & WINEDDBLT_KEYSRCOVERRIDE)
        {
            /* Use color key from DDBltFx */
            wined3d_texture_set_color_key(src_surface->container, WINED3D_CKEY_SRC_BLT, &DDBltFx->ddckSrcColorkey);
        }
        else
        {
            /* Do not use color key */
            wined3d_texture_set_color_key(src_surface->container, WINED3D_CKEY_SRC_BLT, NULL);
        }

        surface_blt_to_drawable(device, filter,
                flags & (WINEDDBLT_KEYSRC | WINEDDBLT_KEYSRCOVERRIDE | WINEDDBLT_ALPHATEST),
                src_surface, src_rect, dst_surface, dst_rect);

        /* Restore the color key parameters */
        wined3d_texture_set_color_key(src_surface->container, WINED3D_CKEY_SRC_BLT,
                (old_color_key_flags & WINED3D_CKEY_SRC_BLT) ? &old_blt_key : NULL);

        surface_validate_location(dst_surface, dst_surface->container->resource.draw_binding);
        surface_invalidate_location(dst_surface, ~dst_surface->container->resource.draw_binding);

        return WINED3D_OK;
    }

    /* Default: Fall back to the generic blt. Not an error, a TRACE is enough */
    TRACE("Didn't find any usable render target setup for hw blit, falling back to software\n");
    return WINED3DERR_INVALIDCALL;
}

/* Context activation is done by the caller. */
static void surface_depth_blt(const struct wined3d_surface *surface, struct wined3d_context *context,
        GLuint texture, GLint x, GLint y, GLsizei w, GLsizei h, GLenum target)
{
    struct wined3d_device *device = surface->resource.device;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    GLint compare_mode = GL_NONE;
    struct blt_info info;
    GLint old_binding = 0;
    RECT rect;

    gl_info->gl_ops.gl.p_glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT);

    gl_info->gl_ops.gl.p_glDisable(GL_CULL_FACE);
    gl_info->gl_ops.gl.p_glDisable(GL_BLEND);
    gl_info->gl_ops.gl.p_glDisable(GL_ALPHA_TEST);
    gl_info->gl_ops.gl.p_glDisable(GL_SCISSOR_TEST);
    gl_info->gl_ops.gl.p_glDisable(GL_STENCIL_TEST);
    gl_info->gl_ops.gl.p_glEnable(GL_DEPTH_TEST);
    gl_info->gl_ops.gl.p_glDepthFunc(GL_ALWAYS);
    gl_info->gl_ops.gl.p_glDepthMask(GL_TRUE);
    gl_info->gl_ops.gl.p_glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    gl_info->gl_ops.gl.p_glViewport(x, y, w, h);
    gl_info->gl_ops.gl.p_glDepthRange(0.0, 1.0);

    SetRect(&rect, 0, h, w, 0);
    surface_get_blt_info(target, &rect, surface->pow2Width, surface->pow2Height, &info);
    context_active_texture(context, context->gl_info, 0);
    gl_info->gl_ops.gl.p_glGetIntegerv(info.binding, &old_binding);
    gl_info->gl_ops.gl.p_glBindTexture(info.bind_target, texture);
    if (gl_info->supported[ARB_SHADOW])
    {
        gl_info->gl_ops.gl.p_glGetTexParameteriv(info.bind_target, GL_TEXTURE_COMPARE_MODE_ARB, &compare_mode);
        if (compare_mode != GL_NONE)
            gl_info->gl_ops.gl.p_glTexParameteri(info.bind_target, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
    }

    device->shader_backend->shader_select_depth_blt(device->shader_priv,
            gl_info, info.tex_type, &surface->ds_current_size);

    gl_info->gl_ops.gl.p_glBegin(GL_TRIANGLE_STRIP);
    gl_info->gl_ops.gl.p_glTexCoord3fv(info.coords[0]);
    gl_info->gl_ops.gl.p_glVertex2f(-1.0f, -1.0f);
    gl_info->gl_ops.gl.p_glTexCoord3fv(info.coords[1]);
    gl_info->gl_ops.gl.p_glVertex2f(1.0f, -1.0f);
    gl_info->gl_ops.gl.p_glTexCoord3fv(info.coords[2]);
    gl_info->gl_ops.gl.p_glVertex2f(-1.0f, 1.0f);
    gl_info->gl_ops.gl.p_glTexCoord3fv(info.coords[3]);
    gl_info->gl_ops.gl.p_glVertex2f(1.0f, 1.0f);
    gl_info->gl_ops.gl.p_glEnd();

    if (compare_mode != GL_NONE)
        gl_info->gl_ops.gl.p_glTexParameteri(info.bind_target, GL_TEXTURE_COMPARE_MODE_ARB, compare_mode);
    gl_info->gl_ops.gl.p_glBindTexture(info.bind_target, old_binding);

    gl_info->gl_ops.gl.p_glPopAttrib();

    device->shader_backend->shader_deselect_depth_blt(device->shader_priv, gl_info);
}

void surface_modify_ds_location(struct wined3d_surface *surface,
        DWORD location, UINT w, UINT h)
{
    TRACE("surface %p, new location %#x, w %u, h %u.\n", surface, location, w, h);

    if (((surface->locations & WINED3D_LOCATION_TEXTURE_RGB) && !(location & WINED3D_LOCATION_TEXTURE_RGB))
            || (!(surface->locations & WINED3D_LOCATION_TEXTURE_RGB) && (location & WINED3D_LOCATION_TEXTURE_RGB)))
        wined3d_texture_set_dirty(surface->container);

    surface->ds_current_size.cx = w;
    surface->ds_current_size.cy = h;
    surface->locations = location;
}

/* Context activation is done by the caller. */
void surface_load_ds_location(struct wined3d_surface *surface, struct wined3d_context *context, DWORD location)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    struct wined3d_device *device = surface->resource.device;
    GLsizei w, h;

    TRACE("surface %p, new location %#x.\n", surface, location);

    /* TODO: Make this work for modes other than FBO */
    if (wined3d_settings.offscreen_rendering_mode != ORM_FBO) return;

    if (!(surface->locations & location))
    {
        w = surface->ds_current_size.cx;
        h = surface->ds_current_size.cy;
        surface->ds_current_size.cx = 0;
        surface->ds_current_size.cy = 0;
    }
    else
    {
        w = surface->resource.width;
        h = surface->resource.height;
    }

    if (surface->ds_current_size.cx == surface->resource.width
            && surface->ds_current_size.cy == surface->resource.height)
    {
        TRACE("Location (%#x) is already up to date.\n", location);
        return;
    }

    if (surface->current_renderbuffer)
    {
        FIXME("Not supported with fixed up depth stencil.\n");
        return;
    }

    if (surface->locations & WINED3D_LOCATION_DISCARDED)
    {
        TRACE("Surface was discarded, no need copy data.\n");
        switch (location)
        {
            case WINED3D_LOCATION_TEXTURE_RGB:
                wined3d_texture_prepare_texture(surface->container, context, FALSE);
                break;
            case WINED3D_LOCATION_RB_MULTISAMPLE:
                surface_prepare_rb(surface, gl_info, TRUE);
                break;
            case WINED3D_LOCATION_DRAWABLE:
                /* Nothing to do */
                break;
            default:
                FIXME("Unhandled location %#x\n", location);
        }
        surface->locations &= ~WINED3D_LOCATION_DISCARDED;
        surface->locations |= location;
        surface->ds_current_size.cx = surface->resource.width;
        surface->ds_current_size.cy = surface->resource.height;
        return;
    }

    if (!surface->locations)
    {
        FIXME("No up to date depth stencil location.\n");
        surface->locations |= location;
        surface->ds_current_size.cx = surface->resource.width;
        surface->ds_current_size.cy = surface->resource.height;
        return;
    }

    if (location == WINED3D_LOCATION_TEXTURE_RGB)
    {
        GLint old_binding = 0;
        GLenum bind_target;

        /* The render target is allowed to be smaller than the depth/stencil
         * buffer, so the onscreen depth/stencil buffer is potentially smaller
         * than the offscreen surface. Don't overwrite the offscreen surface
         * with undefined data. */
        w = min(w, context->swapchain->desc.backbuffer_width);
        h = min(h, context->swapchain->desc.backbuffer_height);

        TRACE("Copying onscreen depth buffer to depth texture.\n");

        if (!device->depth_blt_texture)
            gl_info->gl_ops.gl.p_glGenTextures(1, &device->depth_blt_texture);

        /* Note that we use depth_blt here as well, rather than glCopyTexImage2D
         * directly on the FBO texture. That's because we need to flip. */
        context_apply_fbo_state_blit(context, GL_FRAMEBUFFER,
                surface_from_resource(wined3d_texture_get_sub_resource(context->swapchain->front_buffer, 0)),
                NULL, WINED3D_LOCATION_DRAWABLE);
        if (surface->texture_target == GL_TEXTURE_RECTANGLE_ARB)
        {
            gl_info->gl_ops.gl.p_glGetIntegerv(GL_TEXTURE_BINDING_RECTANGLE_ARB, &old_binding);
            bind_target = GL_TEXTURE_RECTANGLE_ARB;
        }
        else
        {
            gl_info->gl_ops.gl.p_glGetIntegerv(GL_TEXTURE_BINDING_2D, &old_binding);
            bind_target = GL_TEXTURE_2D;
        }
        gl_info->gl_ops.gl.p_glBindTexture(bind_target, device->depth_blt_texture);
        /* We use GL_DEPTH_COMPONENT instead of the surface's specific
         * internal format, because the internal format might include stencil
         * data. In principle we should copy stencil data as well, but unless
         * the driver supports stencil export it's hard to do, and doesn't
         * seem to be needed in practice. If the hardware doesn't support
         * writing stencil data, the glCopyTexImage2D() call might trigger
         * software fallbacks. */
        gl_info->gl_ops.gl.p_glCopyTexImage2D(bind_target, 0, GL_DEPTH_COMPONENT, 0, 0, w, h, 0);
        gl_info->gl_ops.gl.p_glTexParameteri(bind_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        gl_info->gl_ops.gl.p_glTexParameteri(bind_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        gl_info->gl_ops.gl.p_glTexParameteri(bind_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl_info->gl_ops.gl.p_glTexParameteri(bind_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        gl_info->gl_ops.gl.p_glTexParameteri(bind_target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        gl_info->gl_ops.gl.p_glTexParameteri(bind_target, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
        gl_info->gl_ops.gl.p_glBindTexture(bind_target, old_binding);

        context_apply_fbo_state_blit(context, GL_FRAMEBUFFER,
                NULL, surface, WINED3D_LOCATION_TEXTURE_RGB);
        context_set_draw_buffer(context, GL_NONE);

        /* Do the actual blit */
        surface_depth_blt(surface, context, device->depth_blt_texture, 0, 0, w, h, bind_target);
        checkGLcall("depth_blt");

        context_invalidate_state(context, STATE_FRAMEBUFFER);

        if (wined3d_settings.strict_draw_ordering)
            gl_info->gl_ops.gl.p_glFlush(); /* Flush to ensure ordering across contexts. */
    }
    else if (location == WINED3D_LOCATION_DRAWABLE)
    {
        TRACE("Copying depth texture to onscreen depth buffer.\n");

        context_apply_fbo_state_blit(context, GL_FRAMEBUFFER,
                surface_from_resource(wined3d_texture_get_sub_resource(context->swapchain->front_buffer, 0)),
                NULL, WINED3D_LOCATION_DRAWABLE);
        surface_depth_blt(surface, context, surface->container->texture_rgb.name,
                0, surface->pow2Height - h, w, h, surface->texture_target);
        checkGLcall("depth_blt");

        context_invalidate_state(context, STATE_FRAMEBUFFER);

        if (wined3d_settings.strict_draw_ordering)
            gl_info->gl_ops.gl.p_glFlush(); /* Flush to ensure ordering across contexts. */
    }
    else
    {
        ERR("Invalid location (%#x) specified.\n", location);
    }

    surface->locations |= location;
    surface->ds_current_size.cx = surface->resource.width;
    surface->ds_current_size.cy = surface->resource.height;
}

void surface_validate_location(struct wined3d_surface *surface, DWORD location)
{
    TRACE("surface %p, location %s.\n", surface, wined3d_debug_location(location));

    surface->locations |= location;
}

void surface_invalidate_location(struct wined3d_surface *surface, DWORD location)
{
    TRACE("surface %p, location %s.\n", surface, wined3d_debug_location(location));

    if (location & (WINED3D_LOCATION_TEXTURE_RGB | WINED3D_LOCATION_TEXTURE_SRGB))
        wined3d_texture_set_dirty(surface->container);
    surface->locations &= ~location;

    if (!surface->locations)
        ERR("Surface %p does not have any up to date location.\n", surface);
}

static DWORD resource_access_from_location(DWORD location)
{
    switch (location)
    {
        case WINED3D_LOCATION_SYSMEM:
        case WINED3D_LOCATION_USER_MEMORY:
        case WINED3D_LOCATION_DIB:
        case WINED3D_LOCATION_BUFFER:
            return WINED3D_RESOURCE_ACCESS_CPU;

        case WINED3D_LOCATION_DRAWABLE:
        case WINED3D_LOCATION_TEXTURE_SRGB:
        case WINED3D_LOCATION_TEXTURE_RGB:
        case WINED3D_LOCATION_RB_MULTISAMPLE:
        case WINED3D_LOCATION_RB_RESOLVED:
            return WINED3D_RESOURCE_ACCESS_GPU;

        default:
            FIXME("Unhandled location %#x.\n", location);
            return 0;
    }
}

static void surface_copy_simple_location(struct wined3d_surface *surface, DWORD location)
{
    struct wined3d_device *device = surface->resource.device;
    struct wined3d_context *context;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_bo_address dst, src;
    UINT size = surface->resource.size;

    surface_get_memory(surface, &dst, location);
    surface_get_memory(surface, &src, surface->locations);

    if (dst.buffer_object)
    {
        context = context_acquire(device, NULL);
        gl_info = context->gl_info;
        GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, dst.buffer_object));
        GL_EXTCALL(glBufferSubData(GL_PIXEL_UNPACK_BUFFER, 0, size, src.addr));
        GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
        checkGLcall("Upload PBO");
        context_release(context);
        return;
    }
    if (src.buffer_object)
    {
        context = context_acquire(device, NULL);
        gl_info = context->gl_info;
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, src.buffer_object));
        GL_EXTCALL(glGetBufferSubData(GL_PIXEL_PACK_BUFFER, 0, size, dst.addr));
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
        checkGLcall("Download PBO");
        context_release(context);
        return;
    }
    memcpy(dst.addr, src.addr, size);
}

static void surface_load_sysmem(struct wined3d_surface *surface,
        const struct wined3d_gl_info *gl_info, DWORD dst_location)
{
    if (surface->locations & surface_simple_locations)
    {
        surface_copy_simple_location(surface, dst_location);
        return;
    }

    if (surface->locations & (WINED3D_LOCATION_RB_MULTISAMPLE | WINED3D_LOCATION_RB_RESOLVED))
        surface_load_location(surface, WINED3D_LOCATION_TEXTURE_RGB);

    /* Download the surface to system memory. */
    if (surface->locations & (WINED3D_LOCATION_TEXTURE_RGB | WINED3D_LOCATION_TEXTURE_SRGB))
    {
        struct wined3d_device *device = surface->resource.device;
        struct wined3d_context *context;

        /* TODO: Use already acquired context when possible. */
        context = context_acquire(device, NULL);

        wined3d_texture_bind_and_dirtify(surface->container, context,
                !(surface->locations & WINED3D_LOCATION_TEXTURE_RGB));
        surface_download_data(surface, gl_info, dst_location);

        context_release(context);

        return;
    }

    if (surface->locations & WINED3D_LOCATION_DRAWABLE)
    {
        read_from_framebuffer(surface, dst_location);
        return;
    }

    FIXME("Can't load surface %p with location flags %s into sysmem.\n",
            surface, wined3d_debug_location(surface->locations));
}

static HRESULT surface_load_drawable(struct wined3d_surface *surface,
        const struct wined3d_gl_info *gl_info)
{
    RECT r;

    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO
            && wined3d_resource_is_offscreen(&surface->container->resource))
    {
        ERR("Trying to load offscreen surface into WINED3D_LOCATION_DRAWABLE.\n");
        return WINED3DERR_INVALIDCALL;
    }

    surface_get_rect(surface, NULL, &r);
    surface_load_location(surface, WINED3D_LOCATION_TEXTURE_RGB);
    surface_blt_to_drawable(surface->resource.device,
            WINED3D_TEXF_POINT, FALSE, surface, &r, surface, &r);

    return WINED3D_OK;
}

static HRESULT surface_load_texture(struct wined3d_surface *surface,
        const struct wined3d_gl_info *gl_info, BOOL srgb)
{
    RECT src_rect = {0, 0, surface->resource.width, surface->resource.height};
    struct wined3d_device *device = surface->resource.device;
    const struct wined3d_color_key_conversion *conversion;
    struct wined3d_texture *texture = surface->container;
    struct wined3d_context *context;
    UINT width, src_pitch, dst_pitch;
    struct wined3d_bo_address data;
    struct wined3d_format format;
    POINT dst_point = {0, 0};
    BYTE *mem = NULL;

    if (wined3d_settings.offscreen_rendering_mode != ORM_FBO
            && wined3d_resource_is_offscreen(&texture->resource)
            && (surface->locations & WINED3D_LOCATION_DRAWABLE))
    {
        surface_load_fb_texture(surface, srgb);

        return WINED3D_OK;
    }

    if (surface->locations & (WINED3D_LOCATION_TEXTURE_SRGB | WINED3D_LOCATION_TEXTURE_RGB)
            && (surface->resource.format->flags & WINED3DFMT_FLAG_FBO_ATTACHABLE_SRGB)
            && fbo_blit_supported(gl_info, WINED3D_BLIT_OP_COLOR_BLIT,
                NULL, surface->resource.usage, surface->resource.pool, surface->resource.format,
                NULL, surface->resource.usage, surface->resource.pool, surface->resource.format))
    {
        if (srgb)
            surface_blt_fbo(device, WINED3D_TEXF_POINT, surface, WINED3D_LOCATION_TEXTURE_RGB,
                    &src_rect, surface, WINED3D_LOCATION_TEXTURE_SRGB, &src_rect);
        else
            surface_blt_fbo(device, WINED3D_TEXF_POINT, surface, WINED3D_LOCATION_TEXTURE_SRGB,
                    &src_rect, surface, WINED3D_LOCATION_TEXTURE_RGB, &src_rect);

        return WINED3D_OK;
    }

    if (surface->locations & (WINED3D_LOCATION_RB_MULTISAMPLE | WINED3D_LOCATION_RB_RESOLVED)
            && (!srgb || (surface->resource.format->flags & WINED3DFMT_FLAG_FBO_ATTACHABLE_SRGB))
            && fbo_blit_supported(gl_info, WINED3D_BLIT_OP_COLOR_BLIT,
                NULL, surface->resource.usage, surface->resource.pool, surface->resource.format,
                NULL, surface->resource.usage, surface->resource.pool, surface->resource.format))
    {
        DWORD src_location = surface->locations & WINED3D_LOCATION_RB_RESOLVED ?
                WINED3D_LOCATION_RB_RESOLVED : WINED3D_LOCATION_RB_MULTISAMPLE;
        DWORD dst_location = srgb ? WINED3D_LOCATION_TEXTURE_SRGB : WINED3D_LOCATION_TEXTURE_RGB;
        RECT rect = {0, 0, surface->resource.width, surface->resource.height};

        surface_blt_fbo(device, WINED3D_TEXF_POINT, surface, src_location,
                &rect, surface, dst_location, &rect);

        return WINED3D_OK;
    }

    /* Upload from system memory */

    if (srgb)
    {
        if ((surface->locations & (WINED3D_LOCATION_TEXTURE_RGB | surface->resource.map_binding))
                == WINED3D_LOCATION_TEXTURE_RGB)
        {
            /* Performance warning... */
            FIXME("Downloading RGB surface %p to reload it as sRGB.\n", surface);
            surface_prepare_map_memory(surface);
            surface_load_location(surface, surface->resource.map_binding);
        }
    }
    else
    {
        if ((surface->locations & (WINED3D_LOCATION_TEXTURE_SRGB | surface->resource.map_binding))
                == WINED3D_LOCATION_TEXTURE_SRGB)
        {
            /* Performance warning... */
            FIXME("Downloading sRGB surface %p to reload it as RGB.\n", surface);
            surface_prepare_map_memory(surface);
            surface_load_location(surface, surface->resource.map_binding);
        }
    }

    if (!(surface->locations & surface_simple_locations))
    {
        WARN("Trying to load a texture from sysmem, but no simple location is valid.\n");
        /* Lets hope we get it from somewhere... */
        surface_prepare_system_memory(surface);
        surface_load_location(surface, WINED3D_LOCATION_SYSMEM);
    }

    /* TODO: Use already acquired context when possible. */
    context = context_acquire(device, NULL);

    wined3d_texture_prepare_texture(texture, context, srgb);
    wined3d_texture_bind_and_dirtify(texture, context, srgb);

    width = surface->resource.width;
    src_pitch = wined3d_surface_get_pitch(surface);

    format = *texture->resource.format;
    if ((conversion = wined3d_format_get_color_key_conversion(texture, TRUE)))
        format = *wined3d_get_format(gl_info, conversion->dst_format);

    /* Don't use PBOs for converted surfaces. During PBO conversion we look at
     * WINED3D_TEXTURE_CONVERTED but it isn't set (yet) in all cases it is
     * getting called. */
    if ((format.convert || conversion) && surface->pbo)
    {
        TRACE("Removing the pbo attached to surface %p.\n", surface);

        if (surface->flags & SFLAG_DIBSECTION)
            surface->resource.map_binding = WINED3D_LOCATION_DIB;
        else
            surface->resource.map_binding = WINED3D_LOCATION_SYSMEM;

        surface_prepare_map_memory(surface);
        surface_load_location(surface, surface->resource.map_binding);
        surface_remove_pbo(surface, gl_info);
    }

    surface_get_memory(surface, &data, surface->locations);
    if (format.convert)
    {
        /* This code is entered for texture formats which need a fixup. */
        UINT height = surface->resource.height;

        format.byte_count = format.conv_byte_count;
        dst_pitch = wined3d_format_calculate_pitch(&format, width);

        if (!(mem = HeapAlloc(GetProcessHeap(), 0, dst_pitch * height)))
        {
            ERR("Out of memory (%u).\n", dst_pitch * height);
            context_release(context);
            return E_OUTOFMEMORY;
        }
        format.convert(data.addr, mem, src_pitch, src_pitch * height,
                dst_pitch, dst_pitch * height, width, height, 1);
        src_pitch = dst_pitch;
        data.addr = mem;
    }
    else if (conversion)
    {
        /* This code is only entered for color keying fixups */
        struct wined3d_palette *palette = NULL;
        UINT height = surface->resource.height;

        dst_pitch = wined3d_format_calculate_pitch(&format, width);
        dst_pitch = (dst_pitch + device->surface_alignment - 1) & ~(device->surface_alignment - 1);

        if (!(mem = HeapAlloc(GetProcessHeap(), 0, dst_pitch * height)))
        {
            ERR("Out of memory (%u).\n", dst_pitch * height);
            context_release(context);
            return E_OUTOFMEMORY;
        }
        if (texture->swapchain && texture->swapchain->palette)
            palette = texture->swapchain->palette;
        conversion->convert(data.addr, src_pitch, mem, dst_pitch,
                width, height, palette, &texture->gl_color_key);
        src_pitch = dst_pitch;
        data.addr = mem;
    }

    wined3d_surface_upload_data(surface, gl_info, &format, &src_rect,
            src_pitch, &dst_point, srgb, wined3d_const_bo_address(&data));

    context_release(context);

    HeapFree(GetProcessHeap(), 0, mem);

    return WINED3D_OK;
}

static void surface_multisample_resolve(struct wined3d_surface *surface)
{
    RECT rect = {0, 0, surface->resource.width, surface->resource.height};

    if (!(surface->locations & WINED3D_LOCATION_RB_MULTISAMPLE))
        ERR("Trying to resolve multisampled surface %p, but location WINED3D_LOCATION_RB_MULTISAMPLE not current.\n",
                surface);

    surface_blt_fbo(surface->resource.device, WINED3D_TEXF_POINT,
            surface, WINED3D_LOCATION_RB_MULTISAMPLE, &rect, surface, WINED3D_LOCATION_RB_RESOLVED, &rect);
}

HRESULT surface_load_location(struct wined3d_surface *surface, DWORD location)
{
    struct wined3d_device *device = surface->resource.device;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    HRESULT hr;

    TRACE("surface %p, location %s.\n", surface, wined3d_debug_location(location));

    if (surface->resource.usage & WINED3DUSAGE_DEPTHSTENCIL)
    {
        if (location == WINED3D_LOCATION_TEXTURE_RGB
                && surface->locations & (WINED3D_LOCATION_DRAWABLE | WINED3D_LOCATION_DISCARDED))
        {
            struct wined3d_context *context = context_acquire(device, NULL);
            surface_load_ds_location(surface, context, location);
            context_release(context);
            return WINED3D_OK;
        }
        else if (location & surface->locations
                && surface->container->resource.draw_binding != WINED3D_LOCATION_DRAWABLE)
        {
            /* Already up to date, nothing to do. */
            return WINED3D_OK;
        }
        else
        {
            FIXME("Unimplemented copy from %s to %s for depth/stencil buffers.\n",
                    wined3d_debug_location(surface->locations), wined3d_debug_location(location));
            return WINED3DERR_INVALIDCALL;
        }
    }

    if (surface->locations & location)
    {
        TRACE("Location already up to date.\n");
        return WINED3D_OK;
    }

    if (WARN_ON(d3d_surface))
    {
        DWORD required_access = resource_access_from_location(location);
        if ((surface->resource.access_flags & required_access) != required_access)
            WARN("Operation requires %#x access, but surface only has %#x.\n",
                    required_access, surface->resource.access_flags);
    }

    if (!surface->locations)
    {
        ERR("Surface %p does not have any up to date location.\n", surface);
        surface->flags |= SFLAG_LOST;
        return WINED3DERR_DEVICELOST;
    }

    switch (location)
    {
        case WINED3D_LOCATION_DIB:
        case WINED3D_LOCATION_USER_MEMORY:
        case WINED3D_LOCATION_SYSMEM:
        case WINED3D_LOCATION_BUFFER:
            surface_load_sysmem(surface, gl_info, location);
            break;

        case WINED3D_LOCATION_DRAWABLE:
            if (FAILED(hr = surface_load_drawable(surface, gl_info)))
                return hr;
            break;

        case WINED3D_LOCATION_RB_RESOLVED:
            surface_multisample_resolve(surface);
            break;

        case WINED3D_LOCATION_TEXTURE_RGB:
        case WINED3D_LOCATION_TEXTURE_SRGB:
            if (FAILED(hr = surface_load_texture(surface, gl_info, location == WINED3D_LOCATION_TEXTURE_SRGB)))
                return hr;
            break;

        default:
            ERR("Don't know how to handle location %#x.\n", location);
            break;
    }

    surface_validate_location(surface, location);

    if (location != WINED3D_LOCATION_SYSMEM && (surface->locations & WINED3D_LOCATION_SYSMEM))
        surface_evict_sysmem(surface);

    return WINED3D_OK;
}

static HRESULT ffp_blit_alloc(struct wined3d_device *device) { return WINED3D_OK; }
/* Context activation is done by the caller. */
static void ffp_blit_free(struct wined3d_device *device) { }

/* Context activation is done by the caller. */
static HRESULT ffp_blit_set(void *blit_priv, struct wined3d_context *context, const struct wined3d_surface *surface)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;

    gl_info->gl_ops.gl.p_glEnable(surface->container->target);
    checkGLcall("glEnable(target)");

    return WINED3D_OK;
}

/* Context activation is done by the caller. */
static void ffp_blit_unset(const struct wined3d_gl_info *gl_info)
{
    gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_2D);
    checkGLcall("glDisable(GL_TEXTURE_2D)");
    if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
    {
        gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_CUBE_MAP_ARB);
        checkGLcall("glDisable(GL_TEXTURE_CUBE_MAP_ARB)");
    }
    if (gl_info->supported[ARB_TEXTURE_RECTANGLE])
    {
        gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_RECTANGLE_ARB);
        checkGLcall("glDisable(GL_TEXTURE_RECTANGLE_ARB)");
    }
}

static BOOL ffp_blit_supported(const struct wined3d_gl_info *gl_info, enum wined3d_blit_op blit_op,
        const RECT *src_rect, DWORD src_usage, enum wined3d_pool src_pool, const struct wined3d_format *src_format,
        const RECT *dst_rect, DWORD dst_usage, enum wined3d_pool dst_pool, const struct wined3d_format *dst_format)
{
    switch (blit_op)
    {
        case WINED3D_BLIT_OP_COLOR_BLIT:
            if (src_pool == WINED3D_POOL_SYSTEM_MEM || dst_pool == WINED3D_POOL_SYSTEM_MEM)
                return FALSE;

            if (TRACE_ON(d3d_surface) && TRACE_ON(d3d))
            {
                TRACE("Checking support for fixup:\n");
                dump_color_fixup_desc(src_format->color_fixup);
            }

            /* We only support identity conversions. */
            if (!is_identity_fixup(src_format->color_fixup)
                    || !is_identity_fixup(dst_format->color_fixup))
            {
                TRACE("Fixups are not supported.\n");
                return FALSE;
            }

            return TRUE;

        case WINED3D_BLIT_OP_COLOR_FILL:
            if (dst_pool == WINED3D_POOL_SYSTEM_MEM)
                return FALSE;

            if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
            {
                if (!((dst_format->flags & WINED3DFMT_FLAG_FBO_ATTACHABLE) || (dst_usage & WINED3DUSAGE_RENDERTARGET)))
                    return FALSE;
            }
            else if (!(dst_usage & WINED3DUSAGE_RENDERTARGET))
            {
                TRACE("Color fill not supported\n");
                return FALSE;
            }

            /* FIXME: We should reject color fills on formats with fixups,
             * but this would break P8 color fills for example. */

            return TRUE;

        case WINED3D_BLIT_OP_DEPTH_FILL:
            return TRUE;

        default:
            TRACE("Unsupported blit_op=%d\n", blit_op);
            return FALSE;
    }
}

static HRESULT ffp_blit_color_fill(struct wined3d_device *device, struct wined3d_surface *dst_surface,
        const RECT *dst_rect, const struct wined3d_color *color)
{
    const RECT draw_rect = {0, 0, dst_surface->resource.width, dst_surface->resource.height};
    struct wined3d_rendertarget_view *view;
    struct wined3d_fb_state fb = {&view, NULL};
    HRESULT hr;

    if (FAILED(hr = wined3d_rendertarget_view_create_from_surface(dst_surface,
            NULL, &wined3d_null_parent_ops, &view)))
    {
        ERR("Failed to create rendertarget view, hr %#x.\n", hr);
        return hr;
    }

    device_clear_render_targets(device, 1, &fb, 1, dst_rect, &draw_rect, WINED3DCLEAR_TARGET, color, 0.0f, 0);
    wined3d_rendertarget_view_decref(view);

    return WINED3D_OK;
}

static HRESULT ffp_blit_depth_fill(struct wined3d_device *device, struct wined3d_surface *dst_surface,
        const RECT *dst_rect, float depth)
{
    const RECT draw_rect = {0, 0, dst_surface->resource.width, dst_surface->resource.height};
    struct wined3d_fb_state fb = {NULL, NULL};
    HRESULT hr;

    if (FAILED(hr = wined3d_rendertarget_view_create_from_surface(dst_surface,
            NULL, &wined3d_null_parent_ops, &fb.depth_stencil)))
    {
        ERR("Failed to create rendertarget view, hr %#x.\n", hr);
        return hr;
    }

    device_clear_render_targets(device, 0, &fb, 1, dst_rect, &draw_rect, WINED3DCLEAR_ZBUFFER, 0, depth, 0);
    wined3d_rendertarget_view_decref(fb.depth_stencil);

    return WINED3D_OK;
}

const struct blit_shader ffp_blit =  {
    ffp_blit_alloc,
    ffp_blit_free,
    ffp_blit_set,
    ffp_blit_unset,
    ffp_blit_supported,
    ffp_blit_color_fill,
    ffp_blit_depth_fill,
};

static HRESULT cpu_blit_alloc(struct wined3d_device *device)
{
    return WINED3D_OK;
}

/* Context activation is done by the caller. */
static void cpu_blit_free(struct wined3d_device *device)
{
}

/* Context activation is done by the caller. */
static HRESULT cpu_blit_set(void *blit_priv, struct wined3d_context *context, const struct wined3d_surface *surface)
{
    return WINED3D_OK;
}

/* Context activation is done by the caller. */
static void cpu_blit_unset(const struct wined3d_gl_info *gl_info)
{
}

static BOOL cpu_blit_supported(const struct wined3d_gl_info *gl_info, enum wined3d_blit_op blit_op,
        const RECT *src_rect, DWORD src_usage, enum wined3d_pool src_pool, const struct wined3d_format *src_format,
        const RECT *dst_rect, DWORD dst_usage, enum wined3d_pool dst_pool, const struct wined3d_format *dst_format)
{
    if (blit_op == WINED3D_BLIT_OP_COLOR_FILL)
    {
        return TRUE;
    }

    return FALSE;
}

static HRESULT surface_cpu_blt_compressed(const BYTE *src_data, BYTE *dst_data,
        UINT src_pitch, UINT dst_pitch, UINT update_w, UINT update_h,
        const struct wined3d_format *format, DWORD flags, const WINEDDBLTFX *fx)
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

    if (flags == WINEDDBLT_DDFX && fx->dwDDFX == WINEDDBLTFX_MIRRORUPDOWN)
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
            debug_d3dformat(format->id), flags, flags & WINEDDBLT_DDFX ? fx->dwDDFX : 0);

    return E_NOTIMPL;
}

static HRESULT surface_cpu_blt(struct wined3d_surface *dst_surface, const RECT *dst_rect,
        struct wined3d_surface *src_surface, const RECT *src_rect, DWORD flags,
        const WINEDDBLTFX *fx, enum wined3d_texture_filter_type filter)
{
    int bpp, srcheight, srcwidth, dstheight, dstwidth, width;
    const struct wined3d_format *src_format, *dst_format;
    struct wined3d_texture *src_texture = NULL;
    struct wined3d_map_desc dst_map, src_map;
    const BYTE *sbase = NULL;
    HRESULT hr = WINED3D_OK;
    const BYTE *sbuf;
    BYTE *dbuf;
    int x, y;

    TRACE("dst_surface %p, dst_rect %s, src_surface %p, src_rect %s, flags %#x, fx %p, filter %s.\n",
            dst_surface, wine_dbgstr_rect(dst_rect), src_surface, wine_dbgstr_rect(src_rect),
            flags, fx, debug_d3dtexturefiltertype(filter));

    if (src_surface == dst_surface)
    {
        wined3d_surface_map(dst_surface, &dst_map, NULL, 0);
        src_map = dst_map;
        src_format = dst_surface->resource.format;
        dst_format = src_format;
    }
    else
    {
        dst_format = dst_surface->resource.format;
        if (src_surface)
        {
            if (dst_surface->resource.format->id != src_surface->resource.format->id)
            {
                if (!(src_texture = surface_convert_format(src_surface, dst_format->id)))
                {
                    /* The conv function writes a FIXME */
                    WARN("Cannot convert source surface format to dest format.\n");
                    goto release;
                }
                src_surface = surface_from_resource(wined3d_texture_get_sub_resource(src_texture, 0));
            }
            wined3d_surface_map(src_surface, &src_map, NULL, WINED3D_MAP_READONLY);
            src_format = src_surface->resource.format;
        }
        else
        {
            src_format = dst_format;
        }

        wined3d_surface_map(dst_surface, &dst_map, dst_rect, 0);
    }

    bpp = dst_surface->resource.format->byte_count;
    srcheight = src_rect->bottom - src_rect->top;
    srcwidth = src_rect->right - src_rect->left;
    dstheight = dst_rect->bottom - dst_rect->top;
    dstwidth = dst_rect->right - dst_rect->left;
    width = (dst_rect->right - dst_rect->left) * bpp;

    if (src_surface)
        sbase = (BYTE *)src_map.data
                + ((src_rect->top / src_format->block_height) * src_map.row_pitch)
                + ((src_rect->left / src_format->block_width) * src_format->block_byte_count);
    if (src_surface != dst_surface)
        dbuf = dst_map.data;
    else
        dbuf = (BYTE *)dst_map.data
                + ((dst_rect->top / dst_format->block_height) * dst_map.row_pitch)
                + ((dst_rect->left / dst_format->block_width) * dst_format->block_byte_count);

    if (src_format->flags & dst_format->flags & WINED3DFMT_FLAG_BLOCKS)
    {
        TRACE("%s -> %s copy.\n", debug_d3dformat(src_format->id), debug_d3dformat(dst_format->id));

        if (src_surface == dst_surface)
        {
            FIXME("Only plain blits supported on compressed surfaces.\n");
            hr = E_NOTIMPL;
            goto release;
        }

        if (srcheight != dstheight || srcwidth != dstwidth)
        {
            WARN("Stretching not supported on compressed surfaces.\n");
            hr = WINED3DERR_INVALIDCALL;
            goto release;
        }

        if (!surface_check_block_align(src_surface, src_rect))
        {
            WARN("Source rectangle not block-aligned.\n");
            hr = WINED3DERR_INVALIDCALL;
            goto release;
        }

        if (!surface_check_block_align(dst_surface, dst_rect))
        {
            WARN("Destination rectangle not block-aligned.\n");
            hr = WINED3DERR_INVALIDCALL;
            goto release;
        }

        hr = surface_cpu_blt_compressed(sbase, dbuf,
                src_map.row_pitch, dst_map.row_pitch, dstwidth, dstheight,
                src_format, flags, fx);
        goto release;
    }

    /* First, all the 'source-less' blits */
    if (flags & WINEDDBLT_COLORFILL)
    {
        hr = _Blt_ColorFill(dbuf, dstwidth, dstheight, bpp, dst_map.row_pitch, fx->u5.dwFillColor);
        flags &= ~WINEDDBLT_COLORFILL;
    }

    if (flags & WINEDDBLT_DEPTHFILL)
    {
        FIXME("DDBLT_DEPTHFILL needs to be implemented!\n");
    }
    if (flags & WINEDDBLT_ROP)
    {
        /* Catch some degenerate cases here. */
        switch (fx->dwROP)
        {
            case BLACKNESS:
                hr = _Blt_ColorFill(dbuf, dstwidth, dstheight, bpp, dst_map.row_pitch, 0);
                break;
            case 0xaa0029: /* No-op */
                break;
            case WHITENESS:
                hr = _Blt_ColorFill(dbuf, dstwidth, dstheight, bpp, dst_map.row_pitch, ~0U);
                break;
            case SRCCOPY: /* Well, we do that below? */
                break;
            default:
                FIXME("Unsupported raster op: %08x Pattern: %p\n", fx->dwROP, fx->u5.lpDDSPattern);
                goto error;
        }
        flags &= ~WINEDDBLT_ROP;
    }
    if (flags & WINEDDBLT_DDROPS)
    {
        FIXME("\tDdraw Raster Ops: %08x Pattern: %p\n", fx->dwDDROP, fx->u5.lpDDSPattern);
    }
    /* Now the 'with source' blits. */
    if (src_surface)
    {
        int sx, xinc, sy, yinc;

        if (!dstwidth || !dstheight) /* Hmm... stupid program? */
            goto release;

        if (filter != WINED3D_TEXF_NONE && filter != WINED3D_TEXF_POINT
                && (srcwidth != dstwidth || srcheight != dstheight))
        {
            /* Can happen when d3d9 apps do a StretchRect() call which isn't handled in GL. */
            FIXME("Filter %s not supported in software blit.\n", debug_d3dtexturefiltertype(filter));
        }

        xinc = (srcwidth << 16) / dstwidth;
        yinc = (srcheight << 16) / dstheight;

        if (!flags)
        {
            /* No effects, we can cheat here. */
            if (dstwidth == srcwidth)
            {
                if (dstheight == srcheight)
                {
                    /* No stretching in either direction. This needs to be as
                     * fast as possible. */
                    sbuf = sbase;

                    /* Check for overlapping surfaces. */
                    if (src_surface != dst_surface || dst_rect->top < src_rect->top
                            || dst_rect->right <= src_rect->left || src_rect->right <= dst_rect->left)
                    {
                        /* No overlap, or dst above src, so copy from top downwards. */
                        for (y = 0; y < dstheight; ++y)
                        {
                            memcpy(dbuf, sbuf, width);
                            sbuf += src_map.row_pitch;
                            dbuf += dst_map.row_pitch;
                        }
                    }
                    else if (dst_rect->top > src_rect->top)
                    {
                        /* Copy from bottom upwards. */
                        sbuf += src_map.row_pitch * dstheight;
                        dbuf += dst_map.row_pitch * dstheight;
                        for (y = 0; y < dstheight; ++y)
                        {
                            sbuf -= src_map.row_pitch;
                            dbuf -= dst_map.row_pitch;
                            memcpy(dbuf, sbuf, width);
                        }
                    }
                    else
                    {
                        /* Src and dst overlapping on the same line, use memmove. */
                        for (y = 0; y < dstheight; ++y)
                        {
                            memmove(dbuf, sbuf, width);
                            sbuf += src_map.row_pitch;
                            dbuf += dst_map.row_pitch;
                        }
                    }
                }
                else
                {
                    /* Stretching in y direction only. */
                    for (y = sy = 0; y < dstheight; ++y, sy += yinc)
                    {
                        sbuf = sbase + (sy >> 16) * src_map.row_pitch;
                        memcpy(dbuf, sbuf, width);
                        dbuf += dst_map.row_pitch;
                    }
                }
            }
            else
            {
                /* Stretching in X direction. */
                int last_sy = -1;
                for (y = sy = 0; y < dstheight; ++y, sy += yinc)
                {
                    sbuf = sbase + (sy >> 16) * src_map.row_pitch;

                    if ((sy >> 16) == (last_sy >> 16))
                    {
                        /* This source row is the same as last source row -
                         * Copy the already stretched row. */
                        memcpy(dbuf, dbuf - dst_map.row_pitch, width);
                    }
                    else
                    {
#define STRETCH_ROW(type) \
do { \
    const type *s = (const type *)sbuf; \
    type *d = (type *)dbuf; \
    for (x = sx = 0; x < dstwidth; ++x, sx += xinc) \
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
                                for (x = sx = 0; x < dstwidth; x++, sx+= xinc)
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
                                FIXME("Stretched blit not implemented for bpp %u!\n", bpp * 8);
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
            if (flags & (WINEDDBLT_KEYSRC | WINEDDBLT_KEYDEST | WINEDDBLT_KEYSRCOVERRIDE | WINEDDBLT_KEYDESTOVERRIDE))
            {
                /* The color keying flags are checked for correctness in ddraw */
                if (flags & WINEDDBLT_KEYSRC)
                {
                    keylow  = src_surface->container->src_blt_color_key.color_space_low_value;
                    keyhigh = src_surface->container->src_blt_color_key.color_space_high_value;
                }
                else if (flags & WINEDDBLT_KEYSRCOVERRIDE)
                {
                    keylow = fx->ddckSrcColorkey.color_space_low_value;
                    keyhigh = fx->ddckSrcColorkey.color_space_high_value;
                }

                if (flags & WINEDDBLT_KEYDEST)
                {
                    /* Destination color keys are taken from the source surface! */
                    destkeylow = src_surface->container->dst_blt_color_key.color_space_low_value;
                    destkeyhigh = src_surface->container->dst_blt_color_key.color_space_high_value;
                }
                else if (flags & WINEDDBLT_KEYDESTOVERRIDE)
                {
                    destkeylow = fx->ddckDestColorkey.color_space_low_value;
                    destkeyhigh = fx->ddckDestColorkey.color_space_high_value;
                }

                if (bpp == 1)
                {
                    keymask = 0xff;
                }
                else
                {
                    DWORD masks[3];
                    get_color_masks(src_format, masks);
                    keymask = masks[0]
                            | masks[1]
                            | masks[2];
                }
                flags &= ~(WINEDDBLT_KEYSRC | WINEDDBLT_KEYDEST | WINEDDBLT_KEYSRCOVERRIDE | WINEDDBLT_KEYDESTOVERRIDE);
            }

            if (flags & WINEDDBLT_DDFX)
            {
                BYTE *dTopLeft, *dTopRight, *dBottomLeft, *dBottomRight, *tmp;
                LONG tmpxy;
                dTopLeft     = dbuf;
                dTopRight    = dbuf + ((dstwidth - 1) * bpp);
                dBottomLeft  = dTopLeft + ((dstheight - 1) * dst_map.row_pitch);
                dBottomRight = dBottomLeft + ((dstwidth - 1) * bpp);

                if (fx->dwDDFX & WINEDDBLTFX_ARITHSTRETCHY)
                {
                    /* I don't think we need to do anything about this flag */
                    WARN("flags=DDBLT_DDFX nothing done for WINEDDBLTFX_ARITHSTRETCHY\n");
                }
                if (fx->dwDDFX & WINEDDBLTFX_MIRRORLEFTRIGHT)
                {
                    tmp          = dTopRight;
                    dTopRight    = dTopLeft;
                    dTopLeft     = tmp;
                    tmp          = dBottomRight;
                    dBottomRight = dBottomLeft;
                    dBottomLeft  = tmp;
                    dstxinc = dstxinc * -1;
                }
                if (fx->dwDDFX & WINEDDBLTFX_MIRRORUPDOWN)
                {
                    tmp          = dTopLeft;
                    dTopLeft     = dBottomLeft;
                    dBottomLeft  = tmp;
                    tmp          = dTopRight;
                    dTopRight    = dBottomRight;
                    dBottomRight = tmp;
                    dstyinc = dstyinc * -1;
                }
                if (fx->dwDDFX & WINEDDBLTFX_NOTEARING)
                {
                    /* I don't think we need to do anything about this flag */
                    WARN("flags=DDBLT_DDFX nothing done for WINEDDBLTFX_NOTEARING\n");
                }
                if (fx->dwDDFX & WINEDDBLTFX_ROTATE180)
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
                if (fx->dwDDFX & WINEDDBLTFX_ROTATE270)
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
                if (fx->dwDDFX & WINEDDBLTFX_ROTATE90)
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
                if (fx->dwDDFX & WINEDDBLTFX_ZBUFFERBASEDEST)
                {
                    /* I don't think we need to do anything about this flag */
                    WARN("flags=WINEDDBLT_DDFX nothing done for WINEDDBLTFX_ZBUFFERBASEDEST\n");
                }
                dbuf = dTopLeft;
                flags &= ~(WINEDDBLT_DDFX);
            }

#define COPY_COLORKEY_FX(type) \
do { \
    const type *s; \
    type *d = (type *)dbuf, *dx, tmp; \
    for (y = sy = 0; y < dstheight; ++y, sy += yinc) \
    { \
        s = (const type *)(sbase + (sy >> 16) * src_map.row_pitch); \
        dx = d; \
        for (x = sx = 0; x < dstwidth; ++x, sx += xinc) \
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
                    for (y = sy = 0; y < dstheight; ++y, sy += yinc)
                    {
                        sbuf = sbase + (sy >> 16) * src_map.row_pitch;
                        dx = d;
                        for (x = sx = 0; x < dstwidth; ++x, sx+= xinc)
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
                    FIXME("%s color-keyed blit not implemented for bpp %u!\n",
                          (flags & WINEDDBLT_KEYSRC) ? "Source" : "Destination", bpp * 8);
                    hr = WINED3DERR_NOTAVAILABLE;
                    goto error;
#undef COPY_COLORKEY_FX
            }
        }
    }

error:
    if (flags && FIXME_ON(d3d_surface))
    {
        FIXME("\tUnsupported flags: %#x.\n", flags);
    }

release:
    wined3d_surface_unmap(dst_surface);
    if (src_surface && src_surface != dst_surface)
        wined3d_surface_unmap(src_surface);
    /* Release the converted surface, if any. */
    if (src_texture)
        wined3d_texture_decref(src_texture);

    return hr;
}

static HRESULT cpu_blit_color_fill(struct wined3d_device *device, struct wined3d_surface *dst_surface,
        const RECT *dst_rect, const struct wined3d_color *color)
{
    static const RECT src_rect;
    WINEDDBLTFX BltFx;

    memset(&BltFx, 0, sizeof(BltFx));
    BltFx.dwSize = sizeof(BltFx);
    BltFx.u5.dwFillColor = wined3d_format_convert_from_float(dst_surface, color);
    return surface_cpu_blt(dst_surface, dst_rect, NULL, &src_rect,
            WINEDDBLT_COLORFILL, &BltFx, WINED3D_TEXF_POINT);
}

static HRESULT cpu_blit_depth_fill(struct wined3d_device *device,
        struct wined3d_surface *surface, const RECT *rect, float depth)
{
    FIXME("Depth filling not implemented by cpu_blit.\n");
    return WINED3DERR_INVALIDCALL;
}

const struct blit_shader cpu_blit =  {
    cpu_blit_alloc,
    cpu_blit_free,
    cpu_blit_set,
    cpu_blit_unset,
    cpu_blit_supported,
    cpu_blit_color_fill,
    cpu_blit_depth_fill,
};

HRESULT CDECL wined3d_surface_blt(struct wined3d_surface *dst_surface, const RECT *dst_rect_in,
        struct wined3d_surface *src_surface, const RECT *src_rect_in, DWORD flags,
        const WINEDDBLTFX *fx, enum wined3d_texture_filter_type filter)
{
    struct wined3d_swapchain *src_swapchain, *dst_swapchain;
    struct wined3d_device *device = dst_surface->resource.device;
    DWORD src_ds_flags, dst_ds_flags;
    RECT src_rect, dst_rect;
    BOOL scale, convert;

    static const DWORD simple_blit = WINEDDBLT_ASYNC
            | WINEDDBLT_COLORFILL
            | WINEDDBLT_WAIT
            | WINEDDBLT_DEPTHFILL
            | WINEDDBLT_DONOTWAIT;

    TRACE("dst_surface %p, dst_rect %s, src_surface %p, src_rect %s, flags %#x, fx %p, filter %s.\n",
            dst_surface, wine_dbgstr_rect(dst_rect_in), src_surface, wine_dbgstr_rect(src_rect_in),
            flags, fx, debug_d3dtexturefiltertype(filter));
    TRACE("Usage is %s.\n", debug_d3dusage(dst_surface->resource.usage));

    if (fx)
    {
        TRACE("dwSize %#x.\n", fx->dwSize);
        TRACE("dwDDFX %#x.\n", fx->dwDDFX);
        TRACE("dwROP %#x.\n", fx->dwROP);
        TRACE("dwDDROP %#x.\n", fx->dwDDROP);
        TRACE("dwRotationAngle %#x.\n", fx->dwRotationAngle);
        TRACE("dwZBufferOpCode %#x.\n", fx->dwZBufferOpCode);
        TRACE("dwZBufferLow %#x.\n", fx->dwZBufferLow);
        TRACE("dwZBufferHigh %#x.\n", fx->dwZBufferHigh);
        TRACE("dwZBufferBaseDest %#x.\n", fx->dwZBufferBaseDest);
        TRACE("dwZDestConstBitDepth %#x.\n", fx->dwZDestConstBitDepth);
        TRACE("lpDDSZBufferDest %p.\n", fx->u1.lpDDSZBufferDest);
        TRACE("dwZSrcConstBitDepth %#x.\n", fx->dwZSrcConstBitDepth);
        TRACE("lpDDSZBufferSrc %p.\n", fx->u2.lpDDSZBufferSrc);
        TRACE("dwAlphaEdgeBlendBitDepth %#x.\n", fx->dwAlphaEdgeBlendBitDepth);
        TRACE("dwAlphaEdgeBlend %#x.\n", fx->dwAlphaEdgeBlend);
        TRACE("dwReserved %#x.\n", fx->dwReserved);
        TRACE("dwAlphaDestConstBitDepth %#x.\n", fx->dwAlphaDestConstBitDepth);
        TRACE("lpDDSAlphaDest %p.\n", fx->u3.lpDDSAlphaDest);
        TRACE("dwAlphaSrcConstBitDepth %#x.\n", fx->dwAlphaSrcConstBitDepth);
        TRACE("lpDDSAlphaSrc %p.\n", fx->u4.lpDDSAlphaSrc);
        TRACE("lpDDSPattern %p.\n", fx->u5.lpDDSPattern);
        TRACE("ddckDestColorkey {%#x, %#x}.\n",
                fx->ddckDestColorkey.color_space_low_value,
                fx->ddckDestColorkey.color_space_high_value);
        TRACE("ddckSrcColorkey {%#x, %#x}.\n",
                fx->ddckSrcColorkey.color_space_low_value,
                fx->ddckSrcColorkey.color_space_high_value);
    }

    if (dst_surface->resource.map_count || (src_surface && src_surface->resource.map_count))
    {
        WARN("Surface is busy, returning WINEDDERR_SURFACEBUSY.\n");
        return WINEDDERR_SURFACEBUSY;
    }

    surface_get_rect(dst_surface, dst_rect_in, &dst_rect);

    if (dst_rect.left >= dst_rect.right || dst_rect.top >= dst_rect.bottom
            || dst_rect.left > dst_surface->resource.width || dst_rect.left < 0
            || dst_rect.top > dst_surface->resource.height || dst_rect.top < 0
            || dst_rect.right > dst_surface->resource.width || dst_rect.right < 0
            || dst_rect.bottom > dst_surface->resource.height || dst_rect.bottom < 0)
    {
        WARN("The application gave us a bad destination rectangle.\n");
        return WINEDDERR_INVALIDRECT;
    }

    if (src_surface)
    {
        surface_get_rect(src_surface, src_rect_in, &src_rect);

        if (src_rect.left >= src_rect.right || src_rect.top >= src_rect.bottom
                || src_rect.left > src_surface->resource.width || src_rect.left < 0
                || src_rect.top > src_surface->resource.height || src_rect.top < 0
                || src_rect.right > src_surface->resource.width || src_rect.right < 0
                || src_rect.bottom > src_surface->resource.height || src_rect.bottom < 0)
        {
            WARN("Application gave us bad source rectangle for Blt.\n");
            return WINEDDERR_INVALIDRECT;
        }
    }
    else
    {
        memset(&src_rect, 0, sizeof(src_rect));
    }

    if (!fx || !(fx->dwDDFX))
        flags &= ~WINEDDBLT_DDFX;

    if (flags & WINEDDBLT_WAIT)
        flags &= ~WINEDDBLT_WAIT;

    if (flags & WINEDDBLT_ASYNC)
    {
        static unsigned int once;

        if (!once++)
            FIXME("Can't handle WINEDDBLT_ASYNC flag.\n");
        flags &= ~WINEDDBLT_ASYNC;
    }

    /* WINEDDBLT_DONOTWAIT appeared in DX7. */
    if (flags & WINEDDBLT_DONOTWAIT)
    {
        static unsigned int once;

        if (!once++)
            FIXME("Can't handle WINEDDBLT_DONOTWAIT flag.\n");
        flags &= ~WINEDDBLT_DONOTWAIT;
    }

    if (!device->d3d_initialized)
    {
        WARN("D3D not initialized, using fallback.\n");
        goto cpu;
    }

    /* We want to avoid invalidating the sysmem location for converted
     * surfaces, since otherwise we'd have to convert the data back when
     * locking them. */
    if (dst_surface->container->flags & WINED3D_TEXTURE_CONVERTED
            || dst_surface->container->resource.format->convert
            || wined3d_format_get_color_key_conversion(dst_surface->container, TRUE))
    {
        WARN_(d3d_perf)("Converted surface, using CPU blit.\n");
        goto cpu;
    }

    if (flags & ~simple_blit)
    {
        WARN_(d3d_perf)("Using fallback for complex blit (%#x).\n", flags);
        goto fallback;
    }

    if (src_surface)
        src_swapchain = src_surface->container->swapchain;
    else
        src_swapchain = NULL;

    dst_swapchain = dst_surface->container->swapchain;

    /* This isn't strictly needed. FBO blits for example could deal with
     * cross-swapchain blits by first downloading the source to a texture
     * before switching to the destination context. We just have this here to
     * not have to deal with the issue, since cross-swapchain blits should be
     * rare. */
    if (src_swapchain && dst_swapchain && src_swapchain != dst_swapchain)
    {
        FIXME("Using fallback for cross-swapchain blit.\n");
        goto fallback;
    }

    scale = src_surface
            && (src_rect.right - src_rect.left != dst_rect.right - dst_rect.left
            || src_rect.bottom - src_rect.top != dst_rect.bottom - dst_rect.top);
    convert = src_surface && src_surface->resource.format->id != dst_surface->resource.format->id;

    dst_ds_flags = dst_surface->resource.format->flags & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL);
    if (src_surface)
        src_ds_flags = src_surface->resource.format->flags & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL);
    else
        src_ds_flags = 0;

    if (src_ds_flags || dst_ds_flags)
    {
        if (flags & WINEDDBLT_DEPTHFILL)
        {
            float depth;

            TRACE("Depth fill.\n");

            if (!surface_convert_depth_to_float(dst_surface, fx->u5.dwFillDepth, &depth))
                return WINED3DERR_INVALIDCALL;

            if (SUCCEEDED(wined3d_surface_depth_fill(dst_surface, &dst_rect, depth)))
                return WINED3D_OK;
        }
        else
        {
            if (src_ds_flags != dst_ds_flags)
            {
                WARN("Rejecting depth / stencil blit between incompatible formats.\n");
                return WINED3DERR_INVALIDCALL;
            }

            if (SUCCEEDED(wined3d_surface_depth_blt(src_surface, src_surface->container->resource.draw_binding,
                    &src_rect, dst_surface, dst_surface->container->resource.draw_binding, &dst_rect)))
                return WINED3D_OK;
        }
    }
    else
    {
        /* In principle this would apply to depth blits as well, but we don't
         * implement those in the CPU blitter at the moment. */
        if ((dst_surface->locations & dst_surface->resource.map_binding)
                && (!src_surface || (src_surface->locations & src_surface->resource.map_binding)))
        {
            if (scale)
                TRACE("Not doing sysmem blit because of scaling.\n");
            else if (convert)
                TRACE("Not doing sysmem blit because of format conversion.\n");
            else
                goto cpu;
        }

        if (flags & WINEDDBLT_COLORFILL)
        {
            struct wined3d_color color;

            TRACE("Color fill.\n");

            if (!surface_convert_color_to_float(dst_surface, fx->u5.dwFillColor, &color))
                goto fallback;

            if (SUCCEEDED(surface_color_fill(dst_surface, &dst_rect, &color)))
                return WINED3D_OK;
        }
        else
        {
            TRACE("Color blit.\n");

            /* Upload */
            if ((src_surface->locations & WINED3D_LOCATION_SYSMEM)
                    && !(dst_surface->locations & WINED3D_LOCATION_SYSMEM))
            {
                if (scale)
                    TRACE("Not doing upload because of scaling.\n");
                else if (convert)
                    TRACE("Not doing upload because of format conversion.\n");
                else
                {
                    POINT dst_point = {dst_rect.left, dst_rect.top};

                    if (SUCCEEDED(surface_upload_from_surface(dst_surface, &dst_point, src_surface, &src_rect)))
                    {
                        if (!wined3d_resource_is_offscreen(&dst_surface->container->resource))
                            surface_load_location(dst_surface, dst_surface->container->resource.draw_binding);
                        return WINED3D_OK;
                    }
                }
            }

            /* Use present for back -> front blits. The idea behind this is
             * that present is potentially faster than a blit, in particular
             * when FBO blits aren't available. Some ddraw applications like
             * Half-Life and Prince of Persia 3D use Blt() from the backbuffer
             * to the frontbuffer instead of doing a Flip(). D3D8 and D3D9
             * applications can't blit directly to the frontbuffer. */
            if (dst_swapchain && dst_swapchain->back_buffers
                    && dst_surface->container == dst_swapchain->front_buffer
                    && src_surface->container == dst_swapchain->back_buffers[0])
            {
                enum wined3d_swap_effect swap_effect = dst_swapchain->desc.swap_effect;

                TRACE("Using present for backbuffer -> frontbuffer blit.\n");

                /* Set the swap effect to COPY, we don't want the backbuffer
                 * to become undefined. */
                dst_swapchain->desc.swap_effect = WINED3D_SWAP_EFFECT_COPY;
                wined3d_swapchain_present(dst_swapchain, NULL, NULL, dst_swapchain->win_handle, NULL, 0);
                dst_swapchain->desc.swap_effect = swap_effect;

                return WINED3D_OK;
            }

            if (fbo_blit_supported(&device->adapter->gl_info, WINED3D_BLIT_OP_COLOR_BLIT,
                    &src_rect, src_surface->resource.usage, src_surface->resource.pool, src_surface->resource.format,
                    &dst_rect, dst_surface->resource.usage, dst_surface->resource.pool, dst_surface->resource.format))
            {
                TRACE("Using FBO blit.\n");

                surface_blt_fbo(device, filter,
                        src_surface, src_surface->container->resource.draw_binding, &src_rect,
                        dst_surface, dst_surface->container->resource.draw_binding, &dst_rect);
                surface_validate_location(dst_surface, dst_surface->container->resource.draw_binding);
                surface_invalidate_location(dst_surface, ~dst_surface->container->resource.draw_binding);

                return WINED3D_OK;
            }

            if (arbfp_blit.blit_supported(&device->adapter->gl_info, WINED3D_BLIT_OP_COLOR_BLIT,
                    &src_rect, src_surface->resource.usage, src_surface->resource.pool, src_surface->resource.format,
                    &dst_rect, dst_surface->resource.usage, dst_surface->resource.pool, dst_surface->resource.format))
            {
                TRACE("Using arbfp blit.\n");

                if (SUCCEEDED(arbfp_blit_surface(device, filter, src_surface, &src_rect, dst_surface, &dst_rect)))
                    return WINED3D_OK;
            }
        }
    }

fallback:
    /* Special cases for render targets. */
    if (SUCCEEDED(surface_blt_special(dst_surface, &dst_rect, src_surface, &src_rect, flags, fx, filter)))
        return WINED3D_OK;

cpu:

    /* For the rest call the X11 surface implementation. For render targets
     * this should be implemented OpenGL accelerated in surface_blt_special(),
     * other blits are rather rare. */
    return surface_cpu_blt(dst_surface, &dst_rect, src_surface, &src_rect, flags, fx, filter);
}

static HRESULT surface_init(struct wined3d_surface *surface, struct wined3d_texture *container,
        const struct wined3d_resource_desc *desc, GLenum target, unsigned int level, unsigned int layer, DWORD flags)
{
    struct wined3d_device *device = container->resource.device;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    const struct wined3d_format *format = wined3d_get_format(gl_info, desc->format);
    UINT multisample_quality = desc->multisample_quality;
    BOOL lockable = flags & WINED3D_SURFACE_MAPPABLE;
    unsigned int resource_size;
    HRESULT hr;

    if (multisample_quality > 0)
    {
        FIXME("multisample_quality set to %u, substituting 0.\n", multisample_quality);
        multisample_quality = 0;
    }

    /* Quick lockable sanity check.
     * TODO: remove this after surfaces, usage and lockability have been debugged properly
     * this function is too deep to need to care about things like this.
     * Levels need to be checked too, since they all affect what can be done. */
    switch (desc->pool)
    {
        case WINED3D_POOL_MANAGED:
            if (desc->usage & WINED3DUSAGE_DYNAMIC)
                FIXME("Called with a pool of MANAGED and a usage of DYNAMIC which are mutually exclusive.\n");
            break;

        case WINED3D_POOL_DEFAULT:
            if (lockable && !(desc->usage & (WINED3DUSAGE_DYNAMIC
                    | WINED3DUSAGE_RENDERTARGET | WINED3DUSAGE_DEPTHSTENCIL)))
                WARN("Creating a lockable surface with a POOL of DEFAULT, that doesn't specify DYNAMIC usage.\n");
            break;

        case WINED3D_POOL_SCRATCH:
        case WINED3D_POOL_SYSTEM_MEM:
            break;

        default:
            FIXME("Unknown pool %#x.\n", desc->pool);
            break;
    };

    if (desc->usage & WINED3DUSAGE_RENDERTARGET && desc->pool != WINED3D_POOL_DEFAULT)
        FIXME("Trying to create a render target that isn't in the default pool.\n");

    /* FIXME: Check that the format is supported by the device. */

    resource_size = wined3d_format_calculate_size(format, device->surface_alignment, desc->width, desc->height, 1);
    if (!resource_size)
        return WINED3DERR_INVALIDCALL;

    if (device->wined3d->flags & WINED3D_NO3D)
        surface->surface_ops = &gdi_surface_ops;
    else
        surface->surface_ops = &surface_ops;

    if (FAILED(hr = resource_init(&surface->resource, device, WINED3D_RTYPE_SURFACE, format,
            desc->multisample_type, multisample_quality, desc->usage, desc->pool, desc->width, desc->height, 1,
            resource_size, NULL, &wined3d_null_parent_ops, &surface_resource_ops)))
    {
        WARN("Failed to initialize resource, returning %#x.\n", hr);
        return hr;
    }

    surface->container = container;
    surface_validate_location(surface, WINED3D_LOCATION_SYSMEM);
    list_init(&surface->renderbuffers);
    list_init(&surface->overlays);

    /* Flags */
    if (flags & WINED3D_SURFACE_DISCARD)
        surface->flags |= SFLAG_DISCARD;
    if (lockable || desc->format == WINED3DFMT_D16_LOCKABLE)
        surface->resource.access_flags |= WINED3D_RESOURCE_ACCESS_CPU;

    surface->texture_target = target;
    surface->texture_level = level;
    surface->texture_layer = layer;

    /* Call the private setup routine */
    if (FAILED(hr = surface->surface_ops->surface_private_setup(surface)))
    {
        ERR("Private setup failed, hr %#x.\n", hr);
        surface_cleanup(surface);
        return hr;
    }

    /* Similar to lockable rendertargets above, creating the DIB section
     * during surface initialization prevents the sysmem pointer from changing
     * after a wined3d_surface_getdc() call. */
    if ((desc->usage & WINED3DUSAGE_OWNDC) && !surface->hDC
            && SUCCEEDED(surface_create_dib_section(surface)))
        surface->resource.map_binding = WINED3D_LOCATION_DIB;

    if (surface->resource.map_binding == WINED3D_LOCATION_DIB)
    {
        wined3d_resource_free_sysmem(&surface->resource);
        surface_validate_location(surface, WINED3D_LOCATION_DIB);
        surface_invalidate_location(surface, WINED3D_LOCATION_SYSMEM);
    }

    return hr;
}

HRESULT wined3d_surface_create(struct wined3d_texture *container, const struct wined3d_resource_desc *desc,
        GLenum target, unsigned int level, unsigned int layer, DWORD flags, struct wined3d_surface **surface)
{
    struct wined3d_device_parent *device_parent = container->resource.device->device_parent;
    const struct wined3d_parent_ops *parent_ops;
    struct wined3d_surface *object;
    void *parent;
    HRESULT hr;

    TRACE("container %p, width %u, height %u, format %s, usage %s (%#x), pool %s, "
            "multisample_type %#x, multisample_quality %u, target %#x, level %u, layer %u, flags %#x, surface %p.\n",
            container, desc->width, desc->height, debug_d3dformat(desc->format),
            debug_d3dusage(desc->usage), desc->usage, debug_d3dpool(desc->pool),
            desc->multisample_type, desc->multisample_quality, target, level, layer, flags, surface);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = surface_init(object, container, desc, target, level, layer, flags)))
    {
        WARN("Failed to initialize surface, returning %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    if (FAILED(hr = device_parent->ops->surface_created(device_parent,
            wined3d_texture_get_parent(container), object, &parent, &parent_ops)))
    {
        WARN("Failed to create surface parent, hr %#x.\n", hr);
        wined3d_surface_destroy(object);
        return hr;
    }

    TRACE("Created surface %p, parent %p, parent_ops %p.\n", object, parent, parent_ops);

    object->resource.parent = parent;
    object->resource.parent_ops = parent_ops;
    *surface = object;

    return hr;
}
