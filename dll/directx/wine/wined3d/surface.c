/*
 * Copyright 1997-2000 Marcus Meissner
 * Copyright 1998-2000 Lionel Ulmer
 * Copyright 2000-2001 TransGaming Technologies Inc.
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006-2008 Stefan Dösinger for CodeWeavers
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

#include "config.h"
#include "wine/port.h"
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_surface);
WINE_DECLARE_DEBUG_CHANNEL(d3d);

static HRESULT surface_cpu_blt(struct wined3d_surface *dst_surface, const RECT *dst_rect,
        struct wined3d_surface *src_surface, const RECT *src_rect, DWORD flags,
        const WINEDDBLTFX *fx, WINED3DTEXTUREFILTERTYPE filter);
static HRESULT surface_cpu_bltfast(struct wined3d_surface *dst_surface, DWORD dst_x, DWORD dst_y,
        struct wined3d_surface *src_surface, const RECT *src_rect, DWORD trans);
static HRESULT IWineD3DSurfaceImpl_BltOverride(struct wined3d_surface *dst_surface, const RECT *dst_rect,
        struct wined3d_surface *src_surface, const RECT *src_rect, DWORD flags, const WINEDDBLTFX *fx,
        WINED3DTEXTUREFILTERTYPE filter);

static void surface_cleanup(struct wined3d_surface *surface)
{
    TRACE("surface %p.\n", surface);

    if (surface->texture_name || (surface->flags & SFLAG_PBO) || !list_empty(&surface->renderbuffers))
    {
        struct wined3d_renderbuffer_entry *entry, *entry2;
        const struct wined3d_gl_info *gl_info;
        struct wined3d_context *context;

        context = context_acquire(surface->resource.device, NULL);
        gl_info = context->gl_info;

        ENTER_GL();

        if (surface->texture_name)
        {
            TRACE("Deleting texture %u.\n", surface->texture_name);
            glDeleteTextures(1, &surface->texture_name);
        }

        if (surface->flags & SFLAG_PBO)
        {
            TRACE("Deleting PBO %u.\n", surface->pbo);
            GL_EXTCALL(glDeleteBuffersARB(1, &surface->pbo));
        }

        LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &surface->renderbuffers, struct wined3d_renderbuffer_entry, entry)
        {
            TRACE("Deleting renderbuffer %u.\n", entry->id);
            gl_info->fbo_ops.glDeleteRenderbuffers(1, &entry->id);
            HeapFree(GetProcessHeap(), 0, entry);
        }

        LEAVE_GL();

        context_release(context);
    }

    if (surface->flags & SFLAG_DIBSECTION)
    {
        /* Release the DC. */
        SelectObject(surface->hDC, surface->dib.holdbitmap);
        DeleteDC(surface->hDC);
        /* Release the DIB section. */
        DeleteObject(surface->dib.DIBsection);
        surface->dib.bitmap_data = NULL;
        surface->resource.allocatedMemory = NULL;
    }

    if (surface->flags & SFLAG_USERPTR)
        wined3d_surface_set_mem(surface, NULL);
    if (surface->overlay_dest)
        list_remove(&surface->overlay_entry);

    HeapFree(GetProcessHeap(), 0, surface->palette9);

    resource_cleanup(&surface->resource);
}

void surface_set_container(struct wined3d_surface *surface, enum wined3d_container_type type, void *container)
{
    TRACE("surface %p, container %p.\n", surface, container);

    if (!container && type != WINED3D_CONTAINER_NONE)
        ERR("Setting NULL container of type %#x.\n", type);

    if (type == WINED3D_CONTAINER_SWAPCHAIN)
    {
        surface->get_drawable_size = get_drawable_size_swapchain;
    }
    else
    {
        switch (wined3d_settings.offscreen_rendering_mode)
        {
            case ORM_FBO:
                surface->get_drawable_size = get_drawable_size_fbo;
                break;

            case ORM_BACKBUFFER:
                surface->get_drawable_size = get_drawable_size_backbuffer;
                break;

            default:
                ERR("Unhandled offscreen rendering mode %#x.\n", wined3d_settings.offscreen_rendering_mode);
                return;
        }
    }

    surface->container.type = type;
    surface->container.u.base = container;
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

static inline void surface_get_rect(struct wined3d_surface *surface, const RECT *rect_in, RECT *rect_out)
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

/* GL locking and context activation is done by the caller */
void draw_textured_quad(struct wined3d_surface *src_surface, const RECT *src_rect,
        const RECT *dst_rect, WINED3DTEXTUREFILTERTYPE Filter)
{
    struct blt_info info;

    surface_get_blt_info(src_surface->texture_target, src_rect, src_surface->pow2Width, src_surface->pow2Height, &info);

    glEnable(info.bind_target);
    checkGLcall("glEnable(bind_target)");

    /* Bind the texture */
    glBindTexture(info.bind_target, src_surface->texture_name);
    checkGLcall("glBindTexture");

    /* Filtering for StretchRect */
    glTexParameteri(info.bind_target, GL_TEXTURE_MAG_FILTER,
            wined3d_gl_mag_filter(magLookup, Filter));
    checkGLcall("glTexParameteri");
    glTexParameteri(info.bind_target, GL_TEXTURE_MIN_FILTER,
            wined3d_gl_min_mip_filter(minMipLookup, Filter, WINED3DTEXF_NONE));
    checkGLcall("glTexParameteri");
    glTexParameteri(info.bind_target, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(info.bind_target, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    checkGLcall("glTexEnvi");

    /* Draw a quad */
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord3fv(info.coords[0]);
    glVertex2i(dst_rect->left, dst_rect->top);

    glTexCoord3fv(info.coords[1]);
    glVertex2i(dst_rect->right, dst_rect->top);

    glTexCoord3fv(info.coords[2]);
    glVertex2i(dst_rect->left, dst_rect->bottom);

    glTexCoord3fv(info.coords[3]);
    glVertex2i(dst_rect->right, dst_rect->bottom);
    glEnd();

    /* Unbind the texture */
    glBindTexture(info.bind_target, 0);
    checkGLcall("glBindTexture(info->bind_target, 0)");

    /* We changed the filtering settings on the texture. Inform the
     * container about this to get the filters reset properly next draw. */
    if (src_surface->container.type == WINED3D_CONTAINER_TEXTURE)
    {
        struct wined3d_texture *texture = src_surface->container.u.texture;
        texture->texture_rgb.states[WINED3DTEXSTA_MAGFILTER] = WINED3DTEXF_POINT;
        texture->texture_rgb.states[WINED3DTEXSTA_MINFILTER] = WINED3DTEXF_POINT;
        texture->texture_rgb.states[WINED3DTEXSTA_MIPFILTER] = WINED3DTEXF_NONE;
    }
}

static HRESULT surface_create_dib_section(struct wined3d_surface *surface)
{
    const struct wined3d_format *format = surface->resource.format;
    SYSTEM_INFO sysInfo;
    BITMAPINFO *b_info;
    int extraline = 0;
    DWORD *masks;
    UINT usage;
    HDC dc;

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
            usage = DIB_RGB_COLORS;
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
            usage = 0;
            b_info->bmiHeader.biCompression = BI_BITFIELDS;
            masks[0] = format->red_mask;
            masks[1] = format->green_mask;
            masks[2] = format->blue_mask;
            break;

        default:
            /* Don't know palette */
            b_info->bmiHeader.biCompression = BI_RGB;
            usage = 0;
            break;
    }

    if (!(dc = GetDC(0)))
    {
        HeapFree(GetProcessHeap(), 0, b_info);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    TRACE("Creating a DIB section with size %dx%dx%d, size=%d.\n",
            b_info->bmiHeader.biWidth, b_info->bmiHeader.biHeight,
            b_info->bmiHeader.biBitCount, b_info->bmiHeader.biSizeImage);
    surface->dib.DIBsection = CreateDIBSection(dc, b_info, usage, &surface->dib.bitmap_data, 0, 0);
    ReleaseDC(0, dc);

    if (!surface->dib.DIBsection)
    {
        ERR("Failed to create DIB section.\n");
        HeapFree(GetProcessHeap(), 0, b_info);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    TRACE("DIBSection at %p.\n", surface->dib.bitmap_data);
    /* Copy the existing surface to the dib section. */
    if (surface->resource.allocatedMemory)
    {
        memcpy(surface->dib.bitmap_data, surface->resource.allocatedMemory,
                surface->resource.height * wined3d_surface_get_pitch(surface));
    }
    else
    {
        /* This is to make maps read the GL texture although memory is allocated. */
        surface->flags &= ~SFLAG_INSYSMEM;
    }
    surface->dib.bitmap_size = b_info->bmiHeader.biSizeImage;

    HeapFree(GetProcessHeap(), 0, b_info);

    /* Now allocate a DC. */
    surface->hDC = CreateCompatibleDC(0);
    surface->dib.holdbitmap = SelectObject(surface->hDC, surface->dib.DIBsection);
    TRACE("Using wined3d palette %p.\n", surface->palette);
    SelectPalette(surface->hDC, surface->palette ? surface->palette->hpal : 0, FALSE);

    surface->flags |= SFLAG_DIBSECTION;

    HeapFree(GetProcessHeap(), 0, surface->resource.heapMemory);
    surface->resource.heapMemory = NULL;

    return WINED3D_OK;
}

static void surface_prepare_system_memory(struct wined3d_surface *surface)
{
    struct wined3d_device *device = surface->resource.device;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;

    TRACE("surface %p.\n", surface);

    /* Performance optimization: Count how often a surface is locked, if it is
     * locked regularly do not throw away the system memory copy. This avoids
     * the need to download the surface from OpenGL all the time. The surface
     * is still downloaded if the OpenGL texture is changed. */
    if (!(surface->flags & SFLAG_DYNLOCK))
    {
        if (++surface->lockCount > MAXLOCKCOUNT)
        {
            TRACE("Surface is locked regularly, not freeing the system memory copy any more.\n");
            surface->flags |= SFLAG_DYNLOCK;
        }
    }

    /* Create a PBO for dynamically locked surfaces but don't do it for
     * converted or NPOT surfaces. Also don't create a PBO for systemmem
     * surfaces. */
    if (gl_info->supported[ARB_PIXEL_BUFFER_OBJECT] && (surface->flags & SFLAG_DYNLOCK)
            && !(surface->flags & (SFLAG_PBO | SFLAG_CONVERTED | SFLAG_NONPOW2))
            && (surface->resource.pool != WINED3DPOOL_SYSTEMMEM))
    {
        struct wined3d_context *context;
        GLenum error;

        context = context_acquire(device, NULL);
        ENTER_GL();

        GL_EXTCALL(glGenBuffersARB(1, &surface->pbo));
        error = glGetError();
        if (!surface->pbo || error != GL_NO_ERROR)
            ERR("Failed to create a PBO with error %s (%#x).\n", debug_glerror(error), error);

        TRACE("Binding PBO %u.\n", surface->pbo);

        GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, surface->pbo));
        checkGLcall("glBindBufferARB");

        GL_EXTCALL(glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, surface->resource.size + 4,
                surface->resource.allocatedMemory, GL_STREAM_DRAW_ARB));
        checkGLcall("glBufferDataARB");

        GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0));
        checkGLcall("glBindBufferARB");

        /* We don't need the system memory anymore and we can't even use it for PBOs. */
        if (!(surface->flags & SFLAG_CLIENT))
        {
            HeapFree(GetProcessHeap(), 0, surface->resource.heapMemory);
            surface->resource.heapMemory = NULL;
        }
        surface->resource.allocatedMemory = NULL;
        surface->flags |= SFLAG_PBO;
        LEAVE_GL();
        context_release(context);
    }
    else if (!(surface->resource.allocatedMemory || surface->flags & SFLAG_PBO))
    {
        /* Whatever surface we have, make sure that there is memory allocated
         * for the downloaded copy, or a PBO to map. */
        if (!surface->resource.heapMemory)
            surface->resource.heapMemory = HeapAlloc(GetProcessHeap(), 0, surface->resource.size + RESOURCE_ALIGNMENT);

        surface->resource.allocatedMemory = (BYTE *)(((ULONG_PTR)surface->resource.heapMemory
                + (RESOURCE_ALIGNMENT - 1)) & ~(RESOURCE_ALIGNMENT - 1));

        if (surface->flags & SFLAG_INSYSMEM)
            ERR("Surface without memory or PBO has SFLAG_INSYSMEM set.\n");
    }
}

static void surface_evict_sysmem(struct wined3d_surface *surface)
{
    if (surface->flags & SFLAG_DONOTFREE)
        return;

    HeapFree(GetProcessHeap(), 0, surface->resource.heapMemory);
    surface->resource.allocatedMemory = NULL;
    surface->resource.heapMemory = NULL;
    surface_modify_location(surface, SFLAG_INSYSMEM, FALSE);
}

/* Context activation is done by the caller. */
static void surface_bind_and_dirtify(struct wined3d_surface *surface,
        const struct wined3d_gl_info *gl_info, BOOL srgb)
{
    struct wined3d_device *device = surface->resource.device;
    DWORD active_sampler;
    GLint active_texture;

    /* We don't need a specific texture unit, but after binding the texture
     * the current unit is dirty. Read the unit back instead of switching to
     * 0, this avoids messing around with the state manager's GL states. The
     * current texture unit should always be a valid one.
     *
     * To be more specific, this is tricky because we can implicitly be
     * called from sampler() in state.c. This means we can't touch anything
     * other than whatever happens to be the currently active texture, or we
     * would risk marking already applied sampler states dirty again.
     *
     * TODO: Track the current active texture per GL context instead of using
     * glGet(). */

    ENTER_GL();
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active_texture);
    LEAVE_GL();
    active_sampler = device->rev_tex_unit_map[active_texture - GL_TEXTURE0_ARB];

    if (active_sampler != WINED3D_UNMAPPED_STAGE)
    {
        IWineD3DDeviceImpl_MarkStateDirty(device, STATE_SAMPLER(active_sampler));
    }
    surface_bind(surface, gl_info, srgb);
}

static void surface_force_reload(struct wined3d_surface *surface)
{
    surface->flags &= ~(SFLAG_ALLOCATED | SFLAG_SRGBALLOCATED);
}

static void surface_release_client_storage(struct wined3d_surface *surface)
{
    struct wined3d_context *context = context_acquire(surface->resource.device, NULL);

    ENTER_GL();
    glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE);
    if (surface->texture_name)
    {
        surface_bind_and_dirtify(surface, context->gl_info, FALSE);
        glTexImage2D(surface->texture_target, surface->texture_level,
                GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    }
    if (surface->texture_name_srgb)
    {
        surface_bind_and_dirtify(surface, context->gl_info, TRUE);
        glTexImage2D(surface->texture_target, surface->texture_level,
                GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    }
    glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
    LEAVE_GL();

    context_release(context);

    surface_modify_location(surface, SFLAG_INSRGBTEX, FALSE);
    surface_modify_location(surface, SFLAG_INTEXTURE, FALSE);
    surface_force_reload(surface);
}

static HRESULT surface_private_setup(struct wined3d_surface *surface)
{
    /* TODO: Check against the maximum texture sizes supported by the video card. */
    const struct wined3d_gl_info *gl_info = &surface->resource.device->adapter->gl_info;
    unsigned int pow2Width, pow2Height;

    TRACE("surface %p.\n", surface);

    surface->texture_name = 0;
    surface->texture_target = GL_TEXTURE_2D;

    /* Non-power2 support */
    if (gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO] || gl_info->supported[WINED3D_GL_NORMALIZED_TEXRECT])
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
        if (surface->resource.format->flags & WINED3DFMT_FLAG_COMPRESSED)
        {
            FIXME("(%p) Compressed non-power-two textures are not supported w(%d) h(%d)\n",
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
        if (surface->resource.pool == WINED3DPOOL_DEFAULT || surface->resource.pool == WINED3DPOOL_MANAGED)
        {
            WARN("Unable to allocate a surface which exceeds the maximum OpenGL texture size.\n");
            return WINED3DERR_NOTAVAILABLE;
        }

        /* We should never use this surface in combination with OpenGL! */
        TRACE("Creating an oversized surface: %ux%u.\n",
                surface->pow2Width, surface->pow2Height);
    }
    else
    {
        /* Don't use ARB_TEXTURE_RECTANGLE in case the surface format is P8
         * and EXT_PALETTED_TEXTURE is used in combination with texture
         * uploads (RTL_READTEX/RTL_TEXTEX). The reason is that
         * EXT_PALETTED_TEXTURE doesn't work in combination with
         * ARB_TEXTURE_RECTANGLE. */
        if (surface->flags & SFLAG_NONPOW2 && gl_info->supported[ARB_TEXTURE_RECTANGLE]
                && !(surface->resource.format->id == WINED3DFMT_P8_UINT
                && gl_info->supported[EXT_PALETTED_TEXTURE]
                && wined3d_settings.rendertargetlock_mode == RTL_READTEX))
        {
            surface->texture_target = GL_TEXTURE_RECTANGLE_ARB;
            surface->pow2Width = surface->resource.width;
            surface->pow2Height = surface->resource.height;
            surface->flags &= ~(SFLAG_NONPOW2 | SFLAG_NORMCOORD);
        }
    }

    switch (wined3d_settings.offscreen_rendering_mode)
    {
        case ORM_FBO:
            surface->get_drawable_size = get_drawable_size_fbo;
            break;

        case ORM_BACKBUFFER:
            surface->get_drawable_size = get_drawable_size_backbuffer;
            break;

        default:
            ERR("Unhandled offscreen rendering mode %#x.\n", wined3d_settings.offscreen_rendering_mode);
            return WINED3DERR_INVALIDCALL;
    }

    surface->flags |= SFLAG_INSYSMEM;

    return WINED3D_OK;
}

static void surface_realize_palette(struct wined3d_surface *surface)
{
    struct wined3d_palette *palette = surface->palette;

    TRACE("surface %p.\n", surface);

    if (!palette) return;

    if (surface->resource.format->id == WINED3DFMT_P8_UINT
            || surface->resource.format->id == WINED3DFMT_P8_UINT_A8_UNORM)
    {
        if (surface->resource.usage & WINED3DUSAGE_RENDERTARGET)
        {
            /* Make sure the texture is up to date. This call doesn't do
             * anything if the texture is already up to date. */
            surface_load_location(surface, SFLAG_INTEXTURE, NULL);

            /* We want to force a palette refresh, so mark the drawable as not being up to date */
            if (!surface_is_offscreen(surface))
                surface_modify_location(surface, SFLAG_INDRAWABLE, FALSE);
        }
        else
        {
            if (!(surface->flags & SFLAG_INSYSMEM))
            {
                TRACE("Palette changed with surface that does not have an up to date system memory copy.\n");
                surface_load_location(surface, SFLAG_INSYSMEM, NULL);
            }
            surface_modify_location(surface, SFLAG_INSYSMEM, TRUE);
        }
    }

    if (surface->flags & SFLAG_DIBSECTION)
    {
        RGBQUAD col[256];
        unsigned int i;

        TRACE("Updating the DC's palette.\n");

        for (i = 0; i < 256; ++i)
        {
            col[i].rgbRed   = palette->palents[i].peRed;
            col[i].rgbGreen = palette->palents[i].peGreen;
            col[i].rgbBlue  = palette->palents[i].peBlue;
            col[i].rgbReserved = 0;
        }
        SetDIBColorTable(surface->hDC, 0, 256, col);
    }

    /* Propagate the changes to the drawable when we have a palette. */
    if (surface->resource.usage & WINED3DUSAGE_RENDERTARGET)
        surface_load_location(surface, SFLAG_INDRAWABLE, NULL);
}

static HRESULT surface_draw_overlay(struct wined3d_surface *surface)
{
    HRESULT hr;

    /* If there's no destination surface there is nothing to do. */
    if (!surface->overlay_dest)
        return WINED3D_OK;

    /* Blt calls ModifyLocation on the dest surface, which in turn calls
     * DrawOverlay to update the overlay. Prevent an endless recursion. */
    if (surface->overlay_dest->flags & SFLAG_INOVERLAYDRAW)
        return WINED3D_OK;

    surface->overlay_dest->flags |= SFLAG_INOVERLAYDRAW;
    hr = wined3d_surface_blt(surface->overlay_dest, &surface->overlay_destrect, surface,
            &surface->overlay_srcrect, WINEDDBLT_WAIT, NULL, WINED3DTEXF_LINEAR);
    surface->overlay_dest->flags &= ~SFLAG_INOVERLAYDRAW;

    return hr;
}

static void surface_preload(struct wined3d_surface *surface)
{
    TRACE("surface %p.\n", surface);

    surface_internal_preload(surface, SRGB_ANY);
}

static void surface_map(struct wined3d_surface *surface, const RECT *rect, DWORD flags)
{
    struct wined3d_device *device = surface->resource.device;
    const RECT *pass_rect = rect;

    TRACE("surface %p, rect %s, flags %#x.\n",
            surface, wine_dbgstr_rect(rect), flags);

    if (flags & WINED3DLOCK_DISCARD)
    {
        TRACE("WINED3DLOCK_DISCARD flag passed, marking SYSMEM as up to date.\n");
        surface_prepare_system_memory(surface);
        surface_modify_location(surface, SFLAG_INSYSMEM, TRUE);
    }
    else
    {
        /* surface_load_location() does not check if the rectangle specifies
         * the full surface. Most callers don't need that, so do it here. */
        if (rect && !rect->top && !rect->left
                && rect->right == surface->resource.width
                && rect->bottom == surface->resource.height)
            pass_rect = NULL;

        if (!(wined3d_settings.rendertargetlock_mode == RTL_DISABLE
                && ((surface->container.type == WINED3D_CONTAINER_SWAPCHAIN)
                || surface == device->fb.render_targets[0])))
            surface_load_location(surface, SFLAG_INSYSMEM, pass_rect);
    }

    if (surface->flags & SFLAG_PBO)
    {
        const struct wined3d_gl_info *gl_info;
        struct wined3d_context *context;

        context = context_acquire(device, NULL);
        gl_info = context->gl_info;

        ENTER_GL();
        GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, surface->pbo));
        checkGLcall("glBindBufferARB");

        /* This shouldn't happen but could occur if some other function
         * didn't handle the PBO properly. */
        if (surface->resource.allocatedMemory)
            ERR("The surface already has PBO memory allocated.\n");

        surface->resource.allocatedMemory = GL_EXTCALL(glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_READ_WRITE_ARB));
        checkGLcall("glMapBufferARB");

        /* Make sure the PBO isn't set anymore in order not to break non-PBO
         * calls. */
        GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0));
        checkGLcall("glBindBufferARB");

        LEAVE_GL();
        context_release(context);
    }

    if (!(flags & (WINED3DLOCK_NO_DIRTY_UPDATE | WINED3DLOCK_READONLY)))
    {
        if (!rect)
            surface_add_dirty_rect(surface, NULL);
        else
        {
            WINED3DBOX b;

            b.Left = rect->left;
            b.Top = rect->top;
            b.Right = rect->right;
            b.Bottom = rect->bottom;
            b.Front = 0;
            b.Back = 1;
            surface_add_dirty_rect(surface, &b);
        }
    }
}

static void surface_unmap(struct wined3d_surface *surface)
{
    struct wined3d_device *device = surface->resource.device;
    BOOL fullsurface;

    TRACE("surface %p.\n", surface);

    memset(&surface->lockedRect, 0, sizeof(surface->lockedRect));

    if (surface->flags & SFLAG_PBO)
    {
        const struct wined3d_gl_info *gl_info;
        struct wined3d_context *context;

        TRACE("Freeing PBO memory.\n");

        context = context_acquire(device, NULL);
        gl_info = context->gl_info;

        ENTER_GL();
        GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, surface->pbo));
        GL_EXTCALL(glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB));
        GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0));
        checkGLcall("glUnmapBufferARB");
        LEAVE_GL();
        context_release(context);

        surface->resource.allocatedMemory = NULL;
    }

    TRACE("dirtyfied %u.\n", surface->flags & (SFLAG_INDRAWABLE | SFLAG_INTEXTURE) ? 0 : 1);

    if (surface->flags & (SFLAG_INDRAWABLE | SFLAG_INTEXTURE))
    {
        TRACE("Not dirtified, nothing to do.\n");
        goto done;
    }

    if (surface->container.type == WINED3D_CONTAINER_SWAPCHAIN
            || (device->fb.render_targets && surface == device->fb.render_targets[0]))
    {
        if (wined3d_settings.rendertargetlock_mode == RTL_DISABLE)
        {
            static BOOL warned = FALSE;
            if (!warned)
            {
                ERR("The application tries to write to the render target, but render target locking is disabled.\n");
                warned = TRUE;
            }
            goto done;
        }

        if (!surface->dirtyRect.left && !surface->dirtyRect.top
                && surface->dirtyRect.right == surface->resource.width
                && surface->dirtyRect.bottom == surface->resource.height)
        {
            fullsurface = TRUE;
        }
        else
        {
            /* TODO: Proper partial rectangle tracking. */
            fullsurface = FALSE;
            surface->flags |= SFLAG_INSYSMEM;
        }

        surface_load_location(surface, SFLAG_INDRAWABLE, fullsurface ? NULL : &surface->dirtyRect);

        /* Partial rectangle tracking is not commonly implemented, it is only
         * done for render targets. INSYSMEM was set before to tell
         * surface_load_location() where to read the rectangle from.
         * Indrawable is set because all modifications from the partial
         * sysmem copy are written back to the drawable, thus the surface is
         * merged again in the drawable. The sysmem copy is not fully up to
         * date because only a subrectangle was read in Map(). */
        if (!fullsurface)
        {
            surface_modify_location(surface, SFLAG_INDRAWABLE, TRUE);
            surface_evict_sysmem(surface);
        }

        surface->dirtyRect.left = surface->resource.width;
        surface->dirtyRect.top = surface->resource.height;
        surface->dirtyRect.right = 0;
        surface->dirtyRect.bottom = 0;
    }
    else if (surface->resource.format->flags & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL))
    {
        FIXME("Depth / stencil buffer locking is not implemented.\n");
    }

done:
    /* Overlays have to be redrawn manually after changes with the GL implementation */
    if (surface->overlay_dest)
        surface->surface_ops->surface_draw_overlay(surface);
}

static HRESULT surface_getdc(struct wined3d_surface *surface)
{
    WINED3DLOCKED_RECT lock;
    HRESULT hr;

    TRACE("surface %p.\n", surface);

    /* Create a DIB section if there isn't a dc yet. */
    if (!surface->hDC)
    {
        if (surface->flags & SFLAG_CLIENT)
        {
            surface_load_location(surface, SFLAG_INSYSMEM, NULL);
            surface_release_client_storage(surface);
        }
        hr = surface_create_dib_section(surface);
        if (FAILED(hr))
            return WINED3DERR_INVALIDCALL;

        /* Use the DIB section from now on if we are not using a PBO. */
        if (!(surface->flags & SFLAG_PBO))
            surface->resource.allocatedMemory = surface->dib.bitmap_data;
    }

    /* Map the surface. */
    hr = wined3d_surface_map(surface, &lock, NULL, 0);
    if (FAILED(hr))
        ERR("Map failed, hr %#x.\n", hr);

    /* Sync the DIB with the PBO. This can't be done earlier because Map()
     * activates the allocatedMemory. */
    if (surface->flags & SFLAG_PBO)
        memcpy(surface->dib.bitmap_data, surface->resource.allocatedMemory, surface->dib.bitmap_size);

    return hr;
}

static HRESULT surface_flip(struct wined3d_surface *surface, struct wined3d_surface *override)
{
    TRACE("surface %p, override %p.\n", surface, override);

    /* Flipping is only supported on render targets and overlays. */
    if (!(surface->resource.usage & (WINED3DUSAGE_RENDERTARGET | WINED3DUSAGE_OVERLAY)))
    {
        WARN("Tried to flip a non-render target, non-overlay surface.\n");
        return WINEDDERR_NOTFLIPPABLE;
    }

    if (surface->resource.usage & WINED3DUSAGE_OVERLAY)
    {
        flip_surface(surface, override);

        /* Update the overlay if it is visible */
        if (surface->overlay_dest)
            return surface->surface_ops->surface_draw_overlay(surface);
        else
            return WINED3D_OK;
    }

    return WINED3D_OK;
}

static BOOL surface_is_full_rect(struct wined3d_surface *surface, const RECT *r)
{
    if ((r->left && r->right) || abs(r->right - r->left) != surface->resource.width)
        return FALSE;
    if ((r->top && r->bottom) || abs(r->bottom - r->top) != surface->resource.height)
        return FALSE;
    return TRUE;
}

static void wined3d_surface_depth_blt_fbo(struct wined3d_device *device, struct wined3d_surface *src_surface,
        const RECT *src_rect, struct wined3d_surface *dst_surface, const RECT *dst_rect)
{
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;
    DWORD src_mask, dst_mask;
    GLbitfield gl_mask;

    TRACE("device %p, src_surface %p, src_rect %s, dst_surface %p, dst_rect %s.\n",
            device, src_surface, wine_dbgstr_rect(src_rect),
            dst_surface, wine_dbgstr_rect(dst_rect));

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
    surface_load_location(src_surface, SFLAG_INTEXTURE, NULL);
    if (!surface_is_full_rect(dst_surface, dst_rect))
        surface_load_location(dst_surface, SFLAG_INTEXTURE, NULL);

    context = context_acquire(device, NULL);
    if (!context->valid)
    {
        context_release(context);
        WARN("Invalid context, skipping blit.\n");
        return;
    }

    gl_info = context->gl_info;

    ENTER_GL();

    context_apply_fbo_state_blit(context, GL_READ_FRAMEBUFFER, NULL, src_surface, SFLAG_INTEXTURE);
    glReadBuffer(GL_NONE);
    checkGLcall("glReadBuffer()");
    context_check_fbo_status(context, GL_READ_FRAMEBUFFER);

    context_apply_fbo_state_blit(context, GL_DRAW_FRAMEBUFFER, NULL, dst_surface, SFLAG_INTEXTURE);
    context_set_draw_buffer(context, GL_NONE);
    context_check_fbo_status(context, GL_DRAW_FRAMEBUFFER);

    if (gl_mask & GL_DEPTH_BUFFER_BIT)
    {
        glDepthMask(GL_TRUE);
        IWineD3DDeviceImpl_MarkStateDirty(device, STATE_RENDER(WINED3DRS_ZWRITEENABLE));
    }
    if (gl_mask & GL_STENCIL_BUFFER_BIT)
    {
        if (context->gl_info->supported[EXT_STENCIL_TWO_SIDE])
        {
            glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
            IWineD3DDeviceImpl_MarkStateDirty(device, STATE_RENDER(WINED3DRS_TWOSIDEDSTENCILMODE));
        }
        glStencilMask(~0U);
        IWineD3DDeviceImpl_MarkStateDirty(device, STATE_RENDER(WINED3DRS_STENCILWRITEMASK));
    }

    glDisable(GL_SCISSOR_TEST);
    IWineD3DDeviceImpl_MarkStateDirty(device, STATE_RENDER(WINED3DRS_SCISSORTESTENABLE));

    gl_info->fbo_ops.glBlitFramebuffer(src_rect->left, src_rect->top, src_rect->right, src_rect->bottom,
            dst_rect->left, dst_rect->top, dst_rect->right, dst_rect->bottom, gl_mask, GL_NEAREST);
    checkGLcall("glBlitFramebuffer()");

    LEAVE_GL();

    if (wined3d_settings.strict_draw_ordering)
        wglFlush(); /* Flush to ensure ordering across contexts. */

    context_release(context);
}

static BOOL fbo_blit_supported(const struct wined3d_gl_info *gl_info, enum wined3d_blit_op blit_op,
        const RECT *src_rect, DWORD src_usage, WINED3DPOOL src_pool, const struct wined3d_format *src_format,
        const RECT *dst_rect, DWORD dst_usage, WINED3DPOOL dst_pool, const struct wined3d_format *dst_format)
{
    if ((wined3d_settings.offscreen_rendering_mode != ORM_FBO) || !gl_info->fbo_ops.glBlitFramebuffer)
        return FALSE;

    /* Source and/or destination need to be on the GL side */
    if (src_pool == WINED3DPOOL_SYSTEMMEM || dst_pool == WINED3DPOOL_SYSTEMMEM)
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

static BOOL surface_convert_depth_to_float(struct wined3d_surface *surface, DWORD depth, float *float_depth)
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

/* Do not call while under the GL lock. */
static HRESULT wined3d_surface_depth_fill(struct wined3d_surface *surface, const RECT *rect, float depth)
{
    const struct wined3d_resource *resource = &surface->resource;
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

static HRESULT wined3d_surface_depth_blt(struct wined3d_surface *src_surface, const RECT *src_rect,
        struct wined3d_surface *dst_surface, const RECT *dst_rect)
{
    struct wined3d_device *device = src_surface->resource.device;

    if (!fbo_blit_supported(&device->adapter->gl_info, WINED3D_BLIT_OP_DEPTH_BLIT,
            src_rect, src_surface->resource.usage, src_surface->resource.pool, src_surface->resource.format,
            dst_rect, dst_surface->resource.usage, dst_surface->resource.pool, dst_surface->resource.format))
        return WINED3DERR_INVALIDCALL;

    wined3d_surface_depth_blt_fbo(device, src_surface, src_rect, dst_surface, dst_rect);

    surface_modify_ds_location(dst_surface, SFLAG_DS_OFFSCREEN,
            dst_surface->ds_current_size.cx, dst_surface->ds_current_size.cy);
    surface_modify_location(dst_surface, SFLAG_INDRAWABLE, TRUE);

    return WINED3D_OK;
}

/* Do not call while under the GL lock. */
static HRESULT surface_blt(struct wined3d_surface *dst_surface, const RECT *dst_rect_in,
        struct wined3d_surface *src_surface, const RECT *src_rect_in, DWORD flags,
        const WINEDDBLTFX *fx, WINED3DTEXTUREFILTERTYPE filter)
{
    struct wined3d_device *device = dst_surface->resource.device;
    DWORD src_ds_flags, dst_ds_flags;

    TRACE("dst_surface %p, dst_rect %s, src_surface %p, src_rect %s, flags %#x, fx %p, filter %s.\n",
            dst_surface, wine_dbgstr_rect(dst_rect_in), src_surface, wine_dbgstr_rect(src_rect_in),
            flags, fx, debug_d3dtexturefiltertype(filter));
    TRACE("Usage is %s.\n", debug_d3dusage(dst_surface->resource.usage));

    if ((dst_surface->flags & SFLAG_LOCKED) || (src_surface && (src_surface->flags & SFLAG_LOCKED)))
    {
        WARN(" Surface is busy, returning DDERR_SURFACEBUSY\n");
        return WINEDDERR_SURFACEBUSY;
    }

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
            RECT rect;

            TRACE("Depth fill.\n");

            surface_get_rect(dst_surface, dst_rect_in, &rect);

            if (!surface_convert_depth_to_float(dst_surface, fx->u5.dwFillDepth, &depth))
                return WINED3DERR_INVALIDCALL;

            if (SUCCEEDED(wined3d_surface_depth_fill(dst_surface, &rect, depth)))
                return WINED3D_OK;
        }
        else
        {
            RECT src_rect, dst_rect;

            /* Accessing depth / stencil surfaces is supposed to fail while in
             * a scene, except for fills, which seem to work. */
            if (device->inScene)
            {
                WARN("Rejecting depth / stencil access while in scene.\n");
                return WINED3DERR_INVALIDCALL;
            }

            if (src_ds_flags != dst_ds_flags)
            {
                WARN("Rejecting depth / stencil blit between incompatible formats.\n");
                return WINED3DERR_INVALIDCALL;
            }

            if (src_rect_in && (src_rect_in->top || src_rect_in->left
                    || src_rect_in->bottom != src_surface->resource.height
                    || src_rect_in->right != src_surface->resource.width))
            {
                WARN("Rejecting depth / stencil blit with invalid source rect %s.\n",
                        wine_dbgstr_rect(src_rect_in));
                return WINED3DERR_INVALIDCALL;
            }

            if (dst_rect_in && (dst_rect_in->top || dst_rect_in->left
                    || dst_rect_in->bottom != dst_surface->resource.height
                    || dst_rect_in->right != dst_surface->resource.width))
            {
                WARN("Rejecting depth / stencil blit with invalid destination rect %s.\n",
                        wine_dbgstr_rect(src_rect_in));
                return WINED3DERR_INVALIDCALL;
            }

            if (src_surface->resource.height != dst_surface->resource.height
                    || src_surface->resource.width != dst_surface->resource.width)
            {
                WARN("Rejecting depth / stencil blit with mismatched surface sizes.\n");
                return WINED3DERR_INVALIDCALL;
            }

            surface_get_rect(src_surface, src_rect_in, &src_rect);
            surface_get_rect(dst_surface, dst_rect_in, &dst_rect);

            if (SUCCEEDED(wined3d_surface_depth_blt(src_surface, &src_rect, dst_surface, &dst_rect)))
                return WINED3D_OK;
        }
    }

    /* Special cases for render targets. */
    if ((dst_surface->resource.usage & WINED3DUSAGE_RENDERTARGET)
            || (src_surface && (src_surface->resource.usage & WINED3DUSAGE_RENDERTARGET)))
    {
        if (SUCCEEDED(IWineD3DSurfaceImpl_BltOverride(dst_surface, dst_rect_in,
                src_surface, src_rect_in, flags, fx, filter)))
            return WINED3D_OK;
    }

    /* For the rest call the X11 surface implementation. For render targets
     * this should be implemented OpenGL accelerated in BltOverride, other
     * blits are rather rare. */
    return surface_cpu_blt(dst_surface, dst_rect_in, src_surface, src_rect_in, flags, fx, filter);
}

/* Do not call while under the GL lock. */
static HRESULT surface_bltfast(struct wined3d_surface *dst_surface, DWORD dst_x, DWORD dst_y,
        struct wined3d_surface *src_surface, const RECT *src_rect_in, DWORD trans)
{
    struct wined3d_device *device = dst_surface->resource.device;

    TRACE("dst_surface %p, dst_x %u, dst_y %u, src_surface %p, src_rect %s, flags %#x.\n",
            dst_surface, dst_x, dst_y, src_surface, wine_dbgstr_rect(src_rect_in), trans);

    if ((dst_surface->flags & SFLAG_LOCKED) || (src_surface->flags & SFLAG_LOCKED))
    {
        WARN("Surface is busy, returning WINEDDERR_SURFACEBUSY.\n");
        return WINEDDERR_SURFACEBUSY;
    }

    if (device->inScene && (dst_surface == device->fb.depth_stencil || src_surface == device->fb.depth_stencil))
    {
        WARN("Attempt to access the depth / stencil surface while in a scene.\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* Special cases for RenderTargets */
    if ((dst_surface->resource.usage & WINED3DUSAGE_RENDERTARGET)
            || (src_surface->resource.usage & WINED3DUSAGE_RENDERTARGET))
    {

        RECT src_rect, dst_rect;
        DWORD flags = 0;

        surface_get_rect(src_surface, src_rect_in, &src_rect);

        dst_rect.left = dst_x;
        dst_rect.top = dst_y;
        dst_rect.right = dst_x + src_rect.right - src_rect.left;
        dst_rect.bottom = dst_y + src_rect.bottom - src_rect.top;

        /* Convert BltFast flags into Blt ones because BltOverride is called
         * from Blt as well. */
        if (trans & WINEDDBLTFAST_SRCCOLORKEY)
            flags |= WINEDDBLT_KEYSRC;
        if (trans & WINEDDBLTFAST_DESTCOLORKEY)
            flags |= WINEDDBLT_KEYDEST;
        if (trans & WINEDDBLTFAST_WAIT)
            flags |= WINEDDBLT_WAIT;
        if (trans & WINEDDBLTFAST_DONOTWAIT)
            flags |= WINEDDBLT_DONOTWAIT;

        if (SUCCEEDED(IWineD3DSurfaceImpl_BltOverride(dst_surface,
                &dst_rect, src_surface, &src_rect, flags, NULL, WINED3DTEXF_POINT)))
            return WINED3D_OK;
    }

    return surface_cpu_bltfast(dst_surface, dst_x, dst_y, src_surface, src_rect_in, trans);
}

static HRESULT surface_set_mem(struct wined3d_surface *surface, void *mem)
{
    TRACE("surface %p, mem %p.\n", surface, mem);

    if (mem && mem != surface->resource.allocatedMemory)
    {
        void *release = NULL;

        /* Do I have to copy the old surface content? */
        if (surface->flags & SFLAG_DIBSECTION)
        {
            SelectObject(surface->hDC, surface->dib.holdbitmap);
            DeleteDC(surface->hDC);
            /* Release the DIB section. */
            DeleteObject(surface->dib.DIBsection);
            surface->dib.bitmap_data = NULL;
            surface->resource.allocatedMemory = NULL;
            surface->hDC = NULL;
            surface->flags &= ~SFLAG_DIBSECTION;
        }
        else if (!(surface->flags & SFLAG_USERPTR))
        {
            release = surface->resource.heapMemory;
            surface->resource.heapMemory = NULL;
        }
        surface->resource.allocatedMemory = mem;
        surface->flags |= SFLAG_USERPTR;

        /* Now the surface memory is most up do date. Invalidate drawable and texture. */
        surface_modify_location(surface, SFLAG_INSYSMEM, TRUE);

        /* For client textures OpenGL has to be notified. */
        if (surface->flags & SFLAG_CLIENT)
            surface_release_client_storage(surface);

        /* Now free the old memory if any. */
        HeapFree(GetProcessHeap(), 0, release);
    }
    else if (surface->flags & SFLAG_USERPTR)
    {
        /* Map and GetDC will re-create the dib section and allocated memory. */
        surface->resource.allocatedMemory = NULL;
        /* HeapMemory should be NULL already. */
        if (surface->resource.heapMemory)
            ERR("User pointer surface has heap memory allocated.\n");
        surface->flags &= ~(SFLAG_USERPTR | SFLAG_INSYSMEM);

        if (surface->flags & SFLAG_CLIENT)
            surface_release_client_storage(surface);

        surface_prepare_system_memory(surface);
        surface_modify_location(surface, SFLAG_INSYSMEM, TRUE);
    }

    return WINED3D_OK;
}

/* Context activation is done by the caller. */
static void surface_remove_pbo(struct wined3d_surface *surface, const struct wined3d_gl_info *gl_info)
{
    if (!surface->resource.heapMemory)
    {
        surface->resource.heapMemory = HeapAlloc(GetProcessHeap(), 0, surface->resource.size + RESOURCE_ALIGNMENT);
        surface->resource.allocatedMemory = (BYTE *)(((ULONG_PTR)surface->resource.heapMemory
                + (RESOURCE_ALIGNMENT - 1)) & ~(RESOURCE_ALIGNMENT - 1));
    }

    ENTER_GL();
    GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, surface->pbo));
    checkGLcall("glBindBufferARB(GL_PIXEL_UNPACK_BUFFER, surface->pbo)");
    GL_EXTCALL(glGetBufferSubDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0,
            surface->resource.size, surface->resource.allocatedMemory));
    checkGLcall("glGetBufferSubDataARB");
    GL_EXTCALL(glDeleteBuffersARB(1, &surface->pbo));
    checkGLcall("glDeleteBuffersARB");
    LEAVE_GL();

    surface->pbo = 0;
    surface->flags &= ~SFLAG_PBO;
}

/* Do not call while under the GL lock. */
static void surface_unload(struct wined3d_resource *resource)
{
    struct wined3d_surface *surface = surface_from_resource(resource);
    struct wined3d_renderbuffer_entry *entry, *entry2;
    struct wined3d_device *device = resource->device;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;

    TRACE("surface %p.\n", surface);

    if (resource->pool == WINED3DPOOL_DEFAULT)
    {
        /* Default pool resources are supposed to be destroyed before Reset is called.
         * Implicit resources stay however. So this means we have an implicit render target
         * or depth stencil. The content may be destroyed, but we still have to tear down
         * opengl resources, so we cannot leave early.
         *
         * Put the surfaces into sysmem, and reset the content. The D3D content is undefined,
         * but we can't set the sysmem INDRAWABLE because when we're rendering the swapchain
         * or the depth stencil into an FBO the texture or render buffer will be removed
         * and all flags get lost
         */
        surface_init_sysmem(surface);
    }
    else
    {
        /* Load the surface into system memory */
        surface_load_location(surface, SFLAG_INSYSMEM, NULL);
        surface_modify_location(surface, SFLAG_INDRAWABLE, FALSE);
    }
    surface_modify_location(surface, SFLAG_INTEXTURE, FALSE);
    surface_modify_location(surface, SFLAG_INSRGBTEX, FALSE);
    surface->flags &= ~(SFLAG_ALLOCATED | SFLAG_SRGBALLOCATED);

    context = context_acquire(device, NULL);
    gl_info = context->gl_info;

    /* Destroy PBOs, but load them into real sysmem before */
    if (surface->flags & SFLAG_PBO)
        surface_remove_pbo(surface, gl_info);

    /* Destroy fbo render buffers. This is needed for implicit render targets, for
     * all application-created targets the application has to release the surface
     * before calling _Reset
     */
    LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &surface->renderbuffers, struct wined3d_renderbuffer_entry, entry)
    {
        ENTER_GL();
        gl_info->fbo_ops.glDeleteRenderbuffers(1, &entry->id);
        LEAVE_GL();
        list_remove(&entry->entry);
        HeapFree(GetProcessHeap(), 0, entry);
    }
    list_init(&surface->renderbuffers);
    surface->current_renderbuffer = NULL;

    /* If we're in a texture, the texture name belongs to the texture.
     * Otherwise, destroy it. */
    if (surface->container.type != WINED3D_CONTAINER_TEXTURE)
    {
        ENTER_GL();
        glDeleteTextures(1, &surface->texture_name);
        surface->texture_name = 0;
        glDeleteTextures(1, &surface->texture_name_srgb);
        surface->texture_name_srgb = 0;
        LEAVE_GL();
    }

    context_release(context);

    resource_unload(resource);
}

static const struct wined3d_resource_ops surface_resource_ops =
{
    surface_unload,
};

static const struct wined3d_surface_ops surface_ops =
{
    surface_private_setup,
    surface_cleanup,
    surface_realize_palette,
    surface_draw_overlay,
    surface_preload,
    surface_map,
    surface_unmap,
    surface_getdc,
    surface_flip,
    surface_blt,
    surface_bltfast,
    surface_set_mem,
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
    if (SUCCEEDED(hr))
    {
        HeapFree(GetProcessHeap(), 0, surface->resource.heapMemory);
        surface->resource.heapMemory = NULL;
        surface->resource.allocatedMemory = surface->dib.bitmap_data;
    }

    /* We don't mind the nonpow2 stuff in GDI. */
    surface->pow2Width = surface->resource.width;
    surface->pow2Height = surface->resource.height;

    return WINED3D_OK;
}

static void surface_gdi_cleanup(struct wined3d_surface *surface)
{
    TRACE("surface %p.\n", surface);

    if (surface->flags & SFLAG_DIBSECTION)
    {
        /* Release the DC. */
        SelectObject(surface->hDC, surface->dib.holdbitmap);
        DeleteDC(surface->hDC);
        /* Release the DIB section. */
        DeleteObject(surface->dib.DIBsection);
        surface->dib.bitmap_data = NULL;
        surface->resource.allocatedMemory = NULL;
    }

    if (surface->flags & SFLAG_USERPTR)
        wined3d_surface_set_mem(surface, NULL);
    if (surface->overlay_dest)
        list_remove(&surface->overlay_entry);

    HeapFree(GetProcessHeap(), 0, surface->palette9);

    resource_cleanup(&surface->resource);
}

static void gdi_surface_realize_palette(struct wined3d_surface *surface)
{
    struct wined3d_palette *palette = surface->palette;

    TRACE("surface %p.\n", surface);

    if (!palette) return;

    if (surface->flags & SFLAG_DIBSECTION)
    {
        RGBQUAD col[256];
        unsigned int i;

        TRACE("Updating the DC's palette.\n");

        for (i = 0; i < 256; ++i)
        {
            col[i].rgbRed = palette->palents[i].peRed;
            col[i].rgbGreen = palette->palents[i].peGreen;
            col[i].rgbBlue = palette->palents[i].peBlue;
            col[i].rgbReserved = 0;
        }
        SetDIBColorTable(surface->hDC, 0, 256, col);
    }

    /* Update the image because of the palette change. Some games like e.g.
     * Red Alert call SetEntries a lot to implement fading. */
    /* Tell the swapchain to update the screen. */
    if (surface->container.type == WINED3D_CONTAINER_SWAPCHAIN)
    {
        struct wined3d_swapchain *swapchain = surface->container.u.swapchain;
        if (surface == swapchain->front_buffer)
        {
            x11_copy_to_screen(swapchain, NULL);
        }
    }
}

static HRESULT gdi_surface_draw_overlay(struct wined3d_surface *surface)
{
    FIXME("GDI surfaces can't draw overlays yet.\n");
    return E_FAIL;
}

static void gdi_surface_preload(struct wined3d_surface *surface)
{
    TRACE("surface %p.\n", surface);

    ERR("Preloading GDI surfaces is not supported.\n");
}

static void gdi_surface_map(struct wined3d_surface *surface, const RECT *rect, DWORD flags)
{
    TRACE("surface %p, rect %s, flags %#x.\n",
            surface, wine_dbgstr_rect(rect), flags);

    if (!surface->resource.allocatedMemory)
    {
        /* This happens on gdi surfaces if the application set a user pointer
         * and resets it. Recreate the DIB section. */
        surface_create_dib_section(surface);
        surface->resource.allocatedMemory = surface->dib.bitmap_data;
    }
}

static void gdi_surface_unmap(struct wined3d_surface *surface)
{
    TRACE("surface %p.\n", surface);

    /* Tell the swapchain to update the screen. */
    if (surface->container.type == WINED3D_CONTAINER_SWAPCHAIN)
    {
        struct wined3d_swapchain *swapchain = surface->container.u.swapchain;
        if (surface == swapchain->front_buffer)
        {
            x11_copy_to_screen(swapchain, &surface->lockedRect);
        }
    }

    memset(&surface->lockedRect, 0, sizeof(RECT));
}

static HRESULT gdi_surface_getdc(struct wined3d_surface *surface)
{
    WINED3DLOCKED_RECT lock;
    HRESULT hr;

    TRACE("surface %p.\n", surface);

    /* Should have a DIB section already. */
    if (!(surface->flags & SFLAG_DIBSECTION))
    {
        WARN("DC not supported on this surface\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* Map the surface. */
    hr = wined3d_surface_map(surface, &lock, NULL, 0);
    if (FAILED(hr))
        ERR("Map failed, hr %#x.\n", hr);

    return hr;
}

static HRESULT gdi_surface_flip(struct wined3d_surface *surface, struct wined3d_surface *override)
{
    TRACE("surface %p, override %p.\n", surface, override);

    return WINED3D_OK;
}

static HRESULT gdi_surface_blt(struct wined3d_surface *dst_surface, const RECT *dst_rect,
        struct wined3d_surface *src_surface, const RECT *src_rect, DWORD flags,
        const WINEDDBLTFX *fx, WINED3DTEXTUREFILTERTYPE filter)
{
    TRACE("dst_surface %p, dst_rect %s, src_surface %p, src_rect %s, flags %#x, fx %p, filter %s.\n",
            dst_surface, wine_dbgstr_rect(dst_rect), src_surface, wine_dbgstr_rect(src_rect),
            flags, fx, debug_d3dtexturefiltertype(filter));

    return surface_cpu_blt(dst_surface, dst_rect, src_surface, src_rect, flags, fx, filter);
}

static HRESULT gdi_surface_bltfast(struct wined3d_surface *dst_surface, DWORD dst_x, DWORD dst_y,
        struct wined3d_surface *src_surface, const RECT *src_rect, DWORD trans)
{
    TRACE("dst_surface %p, dst_x %u, dst_y %u, src_surface %p, src_rect %s, flags %#x.\n",
            dst_surface, dst_x, dst_y, src_surface, wine_dbgstr_rect(src_rect), trans);

    return surface_cpu_bltfast(dst_surface, dst_x, dst_y, src_surface, src_rect, trans);
}

static HRESULT gdi_surface_set_mem(struct wined3d_surface *surface, void *mem)
{
    TRACE("surface %p, mem %p.\n", surface, mem);

    /* Render targets depend on their hdc, and we can't create an hdc on a user pointer. */
    if (surface->resource.usage & WINED3DUSAGE_RENDERTARGET)
    {
        ERR("Not supported on render targets.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (mem && mem != surface->resource.allocatedMemory)
    {
        void *release = NULL;

        /* Do I have to copy the old surface content? */
        if (surface->flags & SFLAG_DIBSECTION)
        {
            SelectObject(surface->hDC, surface->dib.holdbitmap);
            DeleteDC(surface->hDC);
            /* Release the DIB section. */
            DeleteObject(surface->dib.DIBsection);
            surface->dib.bitmap_data = NULL;
            surface->resource.allocatedMemory = NULL;
            surface->hDC = NULL;
            surface->flags &= ~SFLAG_DIBSECTION;
        }
        else if (!(surface->flags & SFLAG_USERPTR))
        {
            release = surface->resource.allocatedMemory;
        }
        surface->resource.allocatedMemory = mem;
        surface->flags |= SFLAG_USERPTR | SFLAG_INSYSMEM;

        /* Now free the old memory, if any. */
        HeapFree(GetProcessHeap(), 0, release);
    }
    else if (surface->flags & SFLAG_USERPTR)
    {
        /* Map() and GetDC() will re-create the dib section and allocated memory. */
        surface->resource.allocatedMemory = NULL;
        surface->flags &= ~SFLAG_USERPTR;
    }

    return WINED3D_OK;
}

static const struct wined3d_surface_ops gdi_surface_ops =
{
    gdi_surface_private_setup,
    surface_gdi_cleanup,
    gdi_surface_realize_palette,
    gdi_surface_draw_overlay,
    gdi_surface_preload,
    gdi_surface_map,
    gdi_surface_unmap,
    gdi_surface_getdc,
    gdi_surface_flip,
    gdi_surface_blt,
    gdi_surface_bltfast,
    gdi_surface_set_mem,
};

void surface_set_texture_name(struct wined3d_surface *surface, GLuint new_name, BOOL srgb)
{
    GLuint *name;
    DWORD flag;

    TRACE("surface %p, new_name %u, srgb %#x.\n", surface, new_name, srgb);

    if(srgb)
    {
        name = &surface->texture_name_srgb;
        flag = SFLAG_INSRGBTEX;
    }
    else
    {
        name = &surface->texture_name;
        flag = SFLAG_INTEXTURE;
    }

    if (!*name && new_name)
    {
        /* FIXME: We shouldn't need to remove SFLAG_INTEXTURE if the
         * surface has no texture name yet. See if we can get rid of this. */
        if (surface->flags & flag)
            ERR("Surface has %s set, but no texture name.\n", debug_surflocation(flag));
        surface_modify_location(surface, flag, FALSE);
    }

    *name = new_name;
    surface_force_reload(surface);
}

void surface_set_texture_target(struct wined3d_surface *surface, GLenum target)
{
    TRACE("surface %p, target %#x.\n", surface, target);

    if (surface->texture_target != target)
    {
        if (target == GL_TEXTURE_RECTANGLE_ARB)
        {
            surface->flags &= ~SFLAG_NORMCOORD;
        }
        else if (surface->texture_target == GL_TEXTURE_RECTANGLE_ARB)
        {
            surface->flags |= SFLAG_NORMCOORD;
        }
    }
    surface->texture_target = target;
    surface_force_reload(surface);
}

/* Context activation is done by the caller. */
void surface_bind(struct wined3d_surface *surface, const struct wined3d_gl_info *gl_info, BOOL srgb)
{
    TRACE("surface %p, gl_info %p, srgb %#x.\n", surface, gl_info, srgb);

    if (surface->container.type == WINED3D_CONTAINER_TEXTURE)
    {
        struct wined3d_texture *texture = surface->container.u.texture;

        TRACE("Passing to container (%p).\n", texture);
        texture->texture_ops->texture_bind(texture, gl_info, srgb);
    }
    else
    {
        if (surface->texture_level)
        {
            ERR("Standalone surface %p is non-zero texture level %u.\n",
                    surface, surface->texture_level);
        }

        if (srgb)
            ERR("Trying to bind standalone surface %p as sRGB.\n", surface);

        ENTER_GL();

        if (!surface->texture_name)
        {
            glGenTextures(1, &surface->texture_name);
            checkGLcall("glGenTextures");

            TRACE("Surface %p given name %u.\n", surface, surface->texture_name);

            glBindTexture(surface->texture_target, surface->texture_name);
            checkGLcall("glBindTexture");
            glTexParameteri(surface->texture_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(surface->texture_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(surface->texture_target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glTexParameteri(surface->texture_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(surface->texture_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            checkGLcall("glTexParameteri");
        }
        else
        {
            glBindTexture(surface->texture_target, surface->texture_name);
            checkGLcall("glBindTexture");
        }

        LEAVE_GL();
    }
}

/* This function checks if the primary render target uses the 8bit paletted format. */
static BOOL primary_render_target_is_p8(struct wined3d_device *device)
{
    if (device->fb.render_targets && device->fb.render_targets[0])
    {
        struct wined3d_surface *render_target = device->fb.render_targets[0];
        if ((render_target->resource.usage & WINED3DUSAGE_RENDERTARGET)
                && (render_target->resource.format->id == WINED3DFMT_P8_UINT))
            return TRUE;
    }
    return FALSE;
}

/* This call just downloads data, the caller is responsible for binding the
 * correct texture. */
/* Context activation is done by the caller. */
static void surface_download_data(struct wined3d_surface *surface, const struct wined3d_gl_info *gl_info)
{
    const struct wined3d_format *format = surface->resource.format;

    /* Only support read back of converted P8 surfaces. */
    if (surface->flags & SFLAG_CONVERTED && format->id != WINED3DFMT_P8_UINT)
    {
        FIXME("Readback conversion not supported for format %s.\n", debug_d3dformat(format->id));
        return;
    }

    ENTER_GL();

    if (format->flags & WINED3DFMT_FLAG_COMPRESSED)
    {
        TRACE("(%p) : Calling glGetCompressedTexImageARB level %d, format %#x, type %#x, data %p.\n",
                surface, surface->texture_level, format->glFormat, format->glType,
                surface->resource.allocatedMemory);

        if (surface->flags & SFLAG_PBO)
        {
            GL_EXTCALL(glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, surface->pbo));
            checkGLcall("glBindBufferARB");
            GL_EXTCALL(glGetCompressedTexImageARB(surface->texture_target, surface->texture_level, NULL));
            checkGLcall("glGetCompressedTexImageARB");
            GL_EXTCALL(glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0));
            checkGLcall("glBindBufferARB");
        }
        else
        {
            GL_EXTCALL(glGetCompressedTexImageARB(surface->texture_target,
                    surface->texture_level, surface->resource.allocatedMemory));
            checkGLcall("glGetCompressedTexImageARB");
        }

        LEAVE_GL();
    }
    else
    {
        void *mem;
        GLenum gl_format = format->glFormat;
        GLenum gl_type = format->glType;
        int src_pitch = 0;
        int dst_pitch = 0;

        /* In case of P8 the index is stored in the alpha component if the primary render target uses P8. */
        if (format->id == WINED3DFMT_P8_UINT && primary_render_target_is_p8(surface->resource.device))
        {
            gl_format = GL_ALPHA;
            gl_type = GL_UNSIGNED_BYTE;
        }

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
            mem = surface->resource.allocatedMemory;
        }

        TRACE("(%p) : Calling glGetTexImage level %d, format %#x, type %#x, data %p\n",
                surface, surface->texture_level, gl_format, gl_type, mem);

        if (surface->flags & SFLAG_PBO)
        {
            GL_EXTCALL(glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, surface->pbo));
            checkGLcall("glBindBufferARB");

            glGetTexImage(surface->texture_target, surface->texture_level, gl_format, gl_type, NULL);
            checkGLcall("glGetTexImage");

            GL_EXTCALL(glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0));
            checkGLcall("glBindBufferARB");
        }
        else
        {
            glGetTexImage(surface->texture_target, surface->texture_level, gl_format, gl_type, mem);
            checkGLcall("glGetTexImage");
        }
        LEAVE_GL();

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
             * this also means that any references to allocatedMemory should work with the data as if were a
             * standard texture with a non-power2 width instead of texture boxed up to be a power2 texture.
             *
             * internally the texture is still stored in a boxed format so any references to textureName will
             * get a boxed texture with width pow2width and not a texture of width resource.width.
             *
             * Performance should not be an issue, because applications normally do not lock the surfaces when
             * rendering. If an app does, the SFLAG_DYNLOCK flag will kick in and the memory copy won't be released,
             * and doesn't have to be re-read. */
            src_data = mem;
            dst_data = surface->resource.allocatedMemory;
            TRACE("(%p) : Repacking the surface data from pitch %d to pitch %d\n", surface, src_pitch, dst_pitch);
            for (y = 1; y < surface->resource.height; ++y)
            {
                /* skip the first row */
                src_data += src_pitch;
                dst_data += dst_pitch;
                memcpy(dst_data, src_data, dst_pitch);
            }

            HeapFree(GetProcessHeap(), 0, mem);
        }
    }

    /* Surface has now been downloaded */
    surface->flags |= SFLAG_INSYSMEM;
}

/* This call just uploads data, the caller is responsible for binding the
 * correct texture. */
/* Context activation is done by the caller. */
static void surface_upload_data(struct wined3d_surface *surface, const struct wined3d_gl_info *gl_info,
        const struct wined3d_format *format, BOOL srgb, const GLvoid *data)
{
    GLsizei width = surface->resource.width;
    GLsizei height = surface->resource.height;
    GLenum internal;

    if (srgb)
    {
        internal = format->glGammaInternal;
    }
    else if (surface->resource.usage & WINED3DUSAGE_RENDERTARGET && surface_is_offscreen(surface))
    {
        internal = format->rtInternal;
    }
    else
    {
        internal = format->glInternal;
    }

    TRACE("surface %p, internal %#x, width %d, height %d, format %#x, type %#x, data %p.\n",
            surface, internal, width, height, format->glFormat, format->glType, data);
    TRACE("target %#x, level %u, resource size %u.\n",
            surface->texture_target, surface->texture_level, surface->resource.size);

    if (format->heightscale != 1.0f && format->heightscale != 0.0f) height *= format->heightscale;

    ENTER_GL();

    if (surface->flags & SFLAG_PBO)
    {
        GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, surface->pbo));
        checkGLcall("glBindBufferARB");

        TRACE("(%p) pbo: %#x, data: %p.\n", surface, surface->pbo, data);
        data = NULL;
    }

    if (format->flags & WINED3DFMT_FLAG_COMPRESSED)
    {
        TRACE("Calling glCompressedTexSubImage2DARB.\n");

        GL_EXTCALL(glCompressedTexSubImage2DARB(surface->texture_target, surface->texture_level,
                0, 0, width, height, internal, surface->resource.size, data));
        checkGLcall("glCompressedTexSubImage2DARB");
    }
    else
    {
        TRACE("Calling glTexSubImage2D.\n");

        glTexSubImage2D(surface->texture_target, surface->texture_level,
                0, 0, width, height, format->glFormat, format->glType, data);
        checkGLcall("glTexSubImage2D");
    }

    if (surface->flags & SFLAG_PBO)
    {
        GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0));
        checkGLcall("glBindBufferARB");
    }

    LEAVE_GL();

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

/* This call just allocates the texture, the caller is responsible for binding
 * the correct texture. */
/* Context activation is done by the caller. */
static void surface_allocate_surface(struct wined3d_surface *surface, const struct wined3d_gl_info *gl_info,
        const struct wined3d_format *format, BOOL srgb)
{
    BOOL enable_client_storage = FALSE;
    GLsizei width = surface->pow2Width;
    GLsizei height = surface->pow2Height;
    const BYTE *mem = NULL;
    GLenum internal;

    if (srgb)
    {
        internal = format->glGammaInternal;
    }
    else if (surface->resource.usage & WINED3DUSAGE_RENDERTARGET && surface_is_offscreen(surface))
    {
        internal = format->rtInternal;
    }
    else
    {
        internal = format->glInternal;
    }

    if (format->heightscale != 1.0f && format->heightscale != 0.0f) height *= format->heightscale;

    TRACE("(%p) : Creating surface (target %#x)  level %d, d3d format %s, internal format %#x, width %d, height %d, gl format %#x, gl type=%#x\n",
            surface, surface->texture_target, surface->texture_level, debug_d3dformat(format->id),
            internal, width, height, format->glFormat, format->glType);

    ENTER_GL();

    if (gl_info->supported[APPLE_CLIENT_STORAGE])
    {
        if (surface->flags & (SFLAG_NONPOW2 | SFLAG_DIBSECTION | SFLAG_CONVERTED)
                || !surface->resource.allocatedMemory)
        {
            /* In some cases we want to disable client storage.
             * SFLAG_NONPOW2 has a bigger opengl texture than the client memory, and different pitches
             * SFLAG_DIBSECTION: Dibsections may have read / write protections on the memory. Avoid issues...
             * SFLAG_CONVERTED: The conversion destination memory is freed after loading the surface
             * allocatedMemory == NULL: Not defined in the extension. Seems to disable client storage effectively
             */
            glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE);
            checkGLcall("glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE)");
            surface->flags &= ~SFLAG_CLIENT;
            enable_client_storage = TRUE;
        }
        else
        {
            surface->flags |= SFLAG_CLIENT;

            /* Point OpenGL to our allocated texture memory. Do not use
             * resource.allocatedMemory here because it might point into a
             * PBO. Instead use heapMemory, but get the alignment right. */
            mem = (BYTE *)(((ULONG_PTR)surface->resource.heapMemory
                    + (RESOURCE_ALIGNMENT - 1)) & ~(RESOURCE_ALIGNMENT - 1));
        }
    }

    if (format->flags & WINED3DFMT_FLAG_COMPRESSED && mem)
    {
        GL_EXTCALL(glCompressedTexImage2DARB(surface->texture_target, surface->texture_level,
                internal, width, height, 0, surface->resource.size, mem));
        checkGLcall("glCompressedTexImage2DARB");
    }
    else
    {
        glTexImage2D(surface->texture_target, surface->texture_level,
                internal, width, height, 0, format->glFormat, format->glType, mem);
        checkGLcall("glTexImage2D");
    }

    if(enable_client_storage) {
        glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
        checkGLcall("glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE)");
    }
    LEAVE_GL();
}

/* In D3D the depth stencil dimensions have to be greater than or equal to the
 * render target dimensions. With FBOs, the dimensions have to be an exact match. */
/* TODO: We should synchronize the renderbuffer's content with the texture's content. */
/* GL locking is done by the caller */
void surface_set_compatible_renderbuffer(struct wined3d_surface *surface, struct wined3d_surface *rt)
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

GLenum surface_get_gl_buffer(struct wined3d_surface *surface)
{
    struct wined3d_swapchain *swapchain = surface->container.u.swapchain;

    TRACE("surface %p.\n", surface);

    if (surface->container.type != WINED3D_CONTAINER_SWAPCHAIN)
    {
        ERR("Surface %p is not on a swapchain.\n", surface);
        return GL_NONE;
    }

    if (swapchain->back_buffers && swapchain->back_buffers[0] == surface)
    {
        if (swapchain->render_to_fbo)
        {
            TRACE("Returning GL_COLOR_ATTACHMENT0\n");
            return GL_COLOR_ATTACHMENT0;
        }
        TRACE("Returning GL_BACK\n");
        return GL_BACK;
    }
    else if (surface == swapchain->front_buffer)
    {
        TRACE("Returning GL_FRONT\n");
        return GL_FRONT;
    }

    FIXME("Higher back buffer, returning GL_BACK\n");
    return GL_BACK;
}

/* Slightly inefficient way to handle multiple dirty rects but it works :) */
void surface_add_dirty_rect(struct wined3d_surface *surface, const WINED3DBOX *dirty_rect)
{
    TRACE("surface %p, dirty_rect %p.\n", surface, dirty_rect);

    if (!(surface->flags & SFLAG_INSYSMEM) && (surface->flags & SFLAG_INTEXTURE))
        /* No partial locking for textures yet. */
        surface_load_location(surface, SFLAG_INSYSMEM, NULL);

    surface_modify_location(surface, SFLAG_INSYSMEM, TRUE);
    if (dirty_rect)
    {
        surface->dirtyRect.left = min(surface->dirtyRect.left, dirty_rect->Left);
        surface->dirtyRect.top = min(surface->dirtyRect.top, dirty_rect->Top);
        surface->dirtyRect.right = max(surface->dirtyRect.right, dirty_rect->Right);
        surface->dirtyRect.bottom = max(surface->dirtyRect.bottom, dirty_rect->Bottom);
    }
    else
    {
        surface->dirtyRect.left = 0;
        surface->dirtyRect.top = 0;
        surface->dirtyRect.right = surface->resource.width;
        surface->dirtyRect.bottom = surface->resource.height;
    }

    /* if the container is a texture then mark it dirty. */
    if (surface->container.type == WINED3D_CONTAINER_TEXTURE)
    {
        TRACE("Passing to container.\n");
        wined3d_texture_set_dirty(surface->container.u.texture, TRUE);
    }
}

static BOOL surface_convert_color_to_float(struct wined3d_surface *surface,
        DWORD color, WINED3DCOLORVALUE *float_color)
{
    const struct wined3d_format *format = surface->resource.format;
    struct wined3d_device *device = surface->resource.device;

    switch (format->id)
    {
        case WINED3DFMT_P8_UINT:
            if (surface->palette)
            {
                float_color->r = surface->palette->palents[color].peRed / 255.0f;
                float_color->g = surface->palette->palents[color].peGreen / 255.0f;
                float_color->b = surface->palette->palents[color].peBlue / 255.0f;
            }
            else
            {
                float_color->r = 0.0f;
                float_color->g = 0.0f;
                float_color->b = 0.0f;
            }
            float_color->a = primary_render_target_is_p8(device) ? color / 255.0f : 1.0f;
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

HRESULT surface_load(struct wined3d_surface *surface, BOOL srgb)
{
    DWORD flag = srgb ? SFLAG_INSRGBTEX : SFLAG_INTEXTURE;

    TRACE("surface %p, srgb %#x.\n", surface, srgb);

    if (surface->resource.pool == WINED3DPOOL_SCRATCH)
    {
        ERR("Not supported on scratch surfaces.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (!(surface->flags & flag))
    {
        TRACE("Reloading because surface is dirty\n");
    }
    /* Reload if either the texture and sysmem have different ideas about the
     * color key, or the actual key values changed. */
    else if (!(surface->flags & SFLAG_GLCKEY) != !(surface->CKeyFlags & WINEDDSD_CKSRCBLT)
            || ((surface->CKeyFlags & WINEDDSD_CKSRCBLT)
            && (surface->glCKey.dwColorSpaceLowValue != surface->SrcBltCKey.dwColorSpaceLowValue
            || surface->glCKey.dwColorSpaceHighValue != surface->SrcBltCKey.dwColorSpaceHighValue)))
    {
        TRACE("Reloading because of color keying\n");
        /* To perform the color key conversion we need a sysmem copy of
         * the surface. Make sure we have it. */

        surface_load_location(surface, SFLAG_INSYSMEM, NULL);
        /* Make sure the texture is reloaded because of the color key change,
         * this kills performance though :( */
        /* TODO: This is not necessarily needed with hw palettized texture support. */
        surface_modify_location(surface, SFLAG_INSYSMEM, TRUE);
    }
    else
    {
        TRACE("surface is already in texture\n");
        return WINED3D_OK;
    }

    /* No partial locking for textures yet. */
    surface_load_location(surface, flag, NULL);
    surface_evict_sysmem(surface);

    return WINED3D_OK;
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
    ULONG refcount;

    TRACE("Surface %p, container %p of type %#x.\n",
            surface, surface->container.u.base, surface->container.type);

    switch (surface->container.type)
    {
        case WINED3D_CONTAINER_TEXTURE:
            return wined3d_texture_incref(surface->container.u.texture);

        case WINED3D_CONTAINER_SWAPCHAIN:
            return wined3d_swapchain_incref(surface->container.u.swapchain);

        default:
            ERR("Unhandled container type %#x.\n", surface->container.type);
        case WINED3D_CONTAINER_NONE:
            break;
    }

    refcount = InterlockedIncrement(&surface->resource.ref);
    TRACE("%p increasing refcount to %u.\n", surface, refcount);

    return refcount;
}

/* Do not call while under the GL lock. */
ULONG CDECL wined3d_surface_decref(struct wined3d_surface *surface)
{
    ULONG refcount;

    TRACE("Surface %p, container %p of type %#x.\n",
            surface, surface->container.u.base, surface->container.type);

    switch (surface->container.type)
    {
        case WINED3D_CONTAINER_TEXTURE:
            return wined3d_texture_decref(surface->container.u.texture);

        case WINED3D_CONTAINER_SWAPCHAIN:
            return wined3d_swapchain_decref(surface->container.u.swapchain);

        default:
            ERR("Unhandled container type %#x.\n", surface->container.type);
        case WINED3D_CONTAINER_NONE:
            break;
    }

    refcount = InterlockedDecrement(&surface->resource.ref);
    TRACE("%p decreasing refcount to %u.\n", surface, refcount);

    if (!refcount)
    {
        surface->surface_ops->surface_cleanup(surface);
        surface->resource.parent_ops->wined3d_object_destroyed(surface->resource.parent);

        TRACE("Destroyed surface %p.\n", surface);
        HeapFree(GetProcessHeap(), 0, surface);
    }

    return refcount;
}

HRESULT CDECL wined3d_surface_set_private_data(struct wined3d_surface *surface,
        REFGUID riid, const void *data, DWORD data_size, DWORD flags)
{
    return resource_set_private_data(&surface->resource, riid, data, data_size, flags);
}

HRESULT CDECL wined3d_surface_get_private_data(const struct wined3d_surface *surface,
        REFGUID guid, void *data, DWORD *data_size)
{
    return resource_get_private_data(&surface->resource, guid, data, data_size);
}

HRESULT CDECL wined3d_surface_free_private_data(struct wined3d_surface *surface, REFGUID refguid)
{
    return resource_free_private_data(&surface->resource, refguid);
}

DWORD CDECL wined3d_surface_set_priority(struct wined3d_surface *surface, DWORD priority)
{
    return resource_set_priority(&surface->resource, priority);
}

DWORD CDECL wined3d_surface_get_priority(const struct wined3d_surface *surface)
{
    return resource_get_priority(&surface->resource);
}

void CDECL wined3d_surface_preload(struct wined3d_surface *surface)
{
    TRACE("surface %p.\n", surface);

    surface->surface_ops->surface_preload(surface);
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

    /* So far we don't lose anything :) */
    surface->flags &= ~SFLAG_LOST;
    return WINED3D_OK;
}

HRESULT CDECL wined3d_surface_set_palette(struct wined3d_surface *surface, struct wined3d_palette *palette)
{
    TRACE("surface %p, palette %p.\n", surface, palette);

    if (surface->palette == palette)
    {
        TRACE("Nop palette change.\n");
        return WINED3D_OK;
    }

    if (surface->palette && (surface->resource.usage & WINED3DUSAGE_RENDERTARGET))
        surface->palette->flags &= ~WINEDDPCAPS_PRIMARYSURFACE;

    surface->palette = palette;

    if (palette)
    {
        if (surface->resource.usage & WINED3DUSAGE_RENDERTARGET)
            palette->flags |= WINEDDPCAPS_PRIMARYSURFACE;

        surface->surface_ops->surface_realize_palette(surface);
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_surface_set_color_key(struct wined3d_surface *surface,
        DWORD flags, const WINEDDCOLORKEY *color_key)
{
    TRACE("surface %p, flags %#x, color_key %p.\n", surface, flags, color_key);

    if (flags & WINEDDCKEY_COLORSPACE)
    {
        FIXME(" colorkey value not supported (%08x) !\n", flags);
        return WINED3DERR_INVALIDCALL;
    }

    /* Dirtify the surface, but only if a key was changed. */
    if (color_key)
    {
        switch (flags & ~WINEDDCKEY_COLORSPACE)
        {
            case WINEDDCKEY_DESTBLT:
                surface->DestBltCKey = *color_key;
                surface->CKeyFlags |= WINEDDSD_CKDESTBLT;
                break;

            case WINEDDCKEY_DESTOVERLAY:
                surface->DestOverlayCKey = *color_key;
                surface->CKeyFlags |= WINEDDSD_CKDESTOVERLAY;
                break;

            case WINEDDCKEY_SRCOVERLAY:
                surface->SrcOverlayCKey = *color_key;
                surface->CKeyFlags |= WINEDDSD_CKSRCOVERLAY;
                break;

            case WINEDDCKEY_SRCBLT:
                surface->SrcBltCKey = *color_key;
                surface->CKeyFlags |= WINEDDSD_CKSRCBLT;
                break;
        }
    }
    else
    {
        switch (flags & ~WINEDDCKEY_COLORSPACE)
        {
            case WINEDDCKEY_DESTBLT:
                surface->CKeyFlags &= ~WINEDDSD_CKDESTBLT;
                break;

            case WINEDDCKEY_DESTOVERLAY:
                surface->CKeyFlags &= ~WINEDDSD_CKDESTOVERLAY;
                break;

            case WINEDDCKEY_SRCOVERLAY:
                surface->CKeyFlags &= ~WINEDDSD_CKSRCOVERLAY;
                break;

            case WINEDDCKEY_SRCBLT:
                surface->CKeyFlags &= ~WINEDDSD_CKSRCBLT;
                break;
        }
    }

    return WINED3D_OK;
}

struct wined3d_palette * CDECL wined3d_surface_get_palette(const struct wined3d_surface *surface)
{
    TRACE("surface %p.\n", surface);

    return surface->palette;
}

DWORD CDECL wined3d_surface_get_pitch(const struct wined3d_surface *surface)
{
    const struct wined3d_format *format = surface->resource.format;
    DWORD pitch;

    TRACE("surface %p.\n", surface);

    if ((format->flags & (WINED3DFMT_FLAG_COMPRESSED | WINED3DFMT_FLAG_BROKEN_PITCH)) == WINED3DFMT_FLAG_COMPRESSED)
    {
        /* Since compressed formats are block based, pitch means the amount of
         * bytes to the next row of block rather than the next row of pixels. */
        UINT row_block_count = (surface->resource.width + format->block_width - 1) / format->block_width;
        pitch = row_block_count * format->block_byte_count;
    }
    else
    {
        unsigned char alignment = surface->resource.device->surface_alignment;
        pitch = surface->resource.format->byte_count * surface->resource.width;  /* Bytes / row */
        pitch = (pitch + alignment - 1) & ~(alignment - 1);
    }

    TRACE("Returning %u.\n", pitch);

    return pitch;
}

HRESULT CDECL wined3d_surface_set_mem(struct wined3d_surface *surface, void *mem)
{
    TRACE("surface %p, mem %p.\n", surface, mem);

    if (surface->flags & (SFLAG_LOCKED | SFLAG_DCINUSE))
    {
        WARN("Surface is locked or the DC is in use.\n");
        return WINED3DERR_INVALIDCALL;
    }

    return surface->surface_ops->surface_set_mem(surface, mem);
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

    surface->surface_ops->surface_draw_overlay(surface);

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

    surface->surface_ops->surface_draw_overlay(surface);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_surface_set_clipper(struct wined3d_surface *surface, struct wined3d_clipper *clipper)
{
    TRACE("surface %p, clipper %p.\n", surface, clipper);

    surface->clipper = clipper;

    return WINED3D_OK;
}

struct wined3d_clipper * CDECL wined3d_surface_get_clipper(const struct wined3d_surface *surface)
{
    TRACE("surface %p.\n", surface);

    return surface->clipper;
}

HRESULT CDECL wined3d_surface_set_format(struct wined3d_surface *surface, enum wined3d_format_id format_id)
{
    const struct wined3d_format *format = wined3d_get_format(&surface->resource.device->adapter->gl_info, format_id);

    TRACE("surface %p, format %s.\n", surface, debug_d3dformat(format_id));

    if (surface->resource.format->id != WINED3DFMT_UNKNOWN)
    {
        FIXME("The format of the surface must be WINED3DFORMAT_UNKNOWN.\n");
        return WINED3DERR_INVALIDCALL;
    }

    surface->resource.size = wined3d_format_calculate_size(format, surface->resource.device->surface_alignment,
            surface->pow2Width, surface->pow2Height);
    surface->flags |= (WINED3DFMT_D16_LOCKABLE == format_id) ? SFLAG_LOCKABLE : 0;
    surface->flags &= ~(SFLAG_ALLOCATED | SFLAG_SRGBALLOCATED);
    surface->resource.format = format;

    TRACE("size %u, byte_count %u\n", surface->resource.size, format->byte_count);
    TRACE("glFormat %#x, glInternal %#x, glType %#x.\n",
            format->glFormat, format->glInternal, format->glType);

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

struct d3dfmt_convertor_desc
{
    enum wined3d_format_id from, to;
    void (*convert)(const BYTE *src, BYTE *dst, DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h);
};

static const struct d3dfmt_convertor_desc convertors[] =
{
    {WINED3DFMT_R32_FLOAT,      WINED3DFMT_R16_FLOAT,       convert_r32_float_r16_float},
    {WINED3DFMT_B5G6R5_UNORM,   WINED3DFMT_B8G8R8X8_UNORM,  convert_r5g6b5_x8r8g8b8},
    {WINED3DFMT_B8G8R8A8_UNORM, WINED3DFMT_B8G8R8X8_UNORM,  convert_a8r8g8b8_x8r8g8b8},
    {WINED3DFMT_YUY2,           WINED3DFMT_B8G8R8X8_UNORM,  convert_yuy2_x8r8g8b8},
    {WINED3DFMT_YUY2,           WINED3DFMT_B5G6R5_UNORM,    convert_yuy2_r5g6b5},
};

static inline const struct d3dfmt_convertor_desc *find_convertor(enum wined3d_format_id from,
        enum wined3d_format_id to)
{
    unsigned int i;

    for (i = 0; i < (sizeof(convertors) / sizeof(*convertors)); ++i)
    {
        if (convertors[i].from == from && convertors[i].to == to)
            return &convertors[i];
    }

    return NULL;
}

/*****************************************************************************
 * surface_convert_format
 *
 * Creates a duplicate of a surface in a different format. Is used by Blt to
 * blit between surfaces with different formats.
 *
 * Parameters
 *  source: Source surface
 *  fmt: Requested destination format
 *
 *****************************************************************************/
static struct wined3d_surface *surface_convert_format(struct wined3d_surface *source, enum wined3d_format_id to_fmt)
{
    const struct d3dfmt_convertor_desc *conv;
    WINED3DLOCKED_RECT lock_src, lock_dst;
    struct wined3d_surface *ret = NULL;
    HRESULT hr;

    conv = find_convertor(source->resource.format->id, to_fmt);
    if (!conv)
    {
        FIXME("Cannot find a conversion function from format %s to %s.\n",
                debug_d3dformat(source->resource.format->id), debug_d3dformat(to_fmt));
        return NULL;
    }

    wined3d_surface_create(source->resource.device, source->resource.width,
            source->resource.height, to_fmt, TRUE /* lockable */, TRUE /* discard  */, 0 /* level */,
            0 /* usage */, WINED3DPOOL_SCRATCH, WINED3DMULTISAMPLE_NONE /* TODO: Multisampled conversion */,
            0 /* MultiSampleQuality */, source->surface_type, NULL /* parent */, &wined3d_null_parent_ops, &ret);
    if (!ret)
    {
        ERR("Failed to create a destination surface for conversion.\n");
        return NULL;
    }

    memset(&lock_src, 0, sizeof(lock_src));
    memset(&lock_dst, 0, sizeof(lock_dst));

    hr = wined3d_surface_map(source, &lock_src, NULL, WINED3DLOCK_READONLY);
    if (FAILED(hr))
    {
        ERR("Failed to lock the source surface.\n");
        wined3d_surface_decref(ret);
        return NULL;
    }
    hr = wined3d_surface_map(ret, &lock_dst, NULL, WINED3DLOCK_READONLY);
    if (FAILED(hr))
    {
        ERR("Failed to lock the destination surface.\n");
        wined3d_surface_unmap(source);
        wined3d_surface_decref(ret);
        return NULL;
    }

    conv->convert(lock_src.pBits, lock_dst.pBits, lock_src.Pitch, lock_dst.Pitch,
            source->resource.width, source->resource.height);

    wined3d_surface_unmap(ret);
    wined3d_surface_unmap(source);

    return ret;
}

static HRESULT _Blt_ColorFill(BYTE *buf, unsigned int width, unsigned int height,
        unsigned int bpp, UINT pitch, DWORD color)
{
    BYTE *first;
    int x, y;

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
                d[0] = (color      ) & 0xFF;
                d[1] = (color >>  8) & 0xFF;
                d[2] = (color >> 16) & 0xFF;
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

/* Do not call while under the GL lock. */
HRESULT CDECL wined3d_surface_blt(struct wined3d_surface *dst_surface, const RECT *dst_rect,
        struct wined3d_surface *src_surface, const RECT *src_rect, DWORD flags,
        const WINEDDBLTFX *fx, WINED3DTEXTUREFILTERTYPE filter)
{
    TRACE("dst_surface %p, dst_rect %s, src_surface %p, src_rect %s, flags %#x, fx %p, filter %s.\n",
            dst_surface, wine_dbgstr_rect(dst_rect), src_surface, wine_dbgstr_rect(src_rect),
            flags, fx, debug_d3dtexturefiltertype(filter));

    return dst_surface->surface_ops->surface_blt(dst_surface,
            dst_rect, src_surface, src_rect, flags, fx, filter);
}

/* Do not call while under the GL lock. */
HRESULT CDECL wined3d_surface_bltfast(struct wined3d_surface *dst_surface, DWORD dst_x, DWORD dst_y,
        struct wined3d_surface *src_surface, const RECT *src_rect, DWORD trans)
{
    TRACE("dst_surface %p, dst_x %u, dst_y %u, src_surface %p, src_rect %s, trans %#x.\n",
            dst_surface, dst_x, dst_y, src_surface, wine_dbgstr_rect(src_rect), trans);

    return dst_surface->surface_ops->surface_bltfast(dst_surface,
            dst_x, dst_y, src_surface, src_rect, trans);
}

HRESULT CDECL wined3d_surface_unmap(struct wined3d_surface *surface)
{
    TRACE("surface %p.\n", surface);

    if (!(surface->flags & SFLAG_LOCKED))
    {
        WARN("Trying to unmap unmapped surface.\n");
        return WINEDDERR_NOTLOCKED;
    }
    surface->flags &= ~SFLAG_LOCKED;

    surface->surface_ops->surface_unmap(surface);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_surface_map(struct wined3d_surface *surface,
        WINED3DLOCKED_RECT *locked_rect, const RECT *rect, DWORD flags)
{
    TRACE("surface %p, locked_rect %p, rect %s, flags %#x.\n",
            surface, locked_rect, wine_dbgstr_rect(rect), flags);

    if (surface->flags & SFLAG_LOCKED)
    {
        WARN("Surface is already mapped.\n");
        return WINED3DERR_INVALIDCALL;
    }
    surface->flags |= SFLAG_LOCKED;

    if (!(surface->flags & SFLAG_LOCKABLE))
        WARN("Trying to lock unlockable surface.\n");

    surface->surface_ops->surface_map(surface, rect, flags);

    locked_rect->Pitch = wined3d_surface_get_pitch(surface);

    if (!rect)
    {
        locked_rect->pBits = surface->resource.allocatedMemory;
        surface->lockedRect.left = 0;
        surface->lockedRect.top = 0;
        surface->lockedRect.right = surface->resource.width;
        surface->lockedRect.bottom = surface->resource.height;
    }
    else
    {
        const struct wined3d_format *format = surface->resource.format;

        if ((format->flags & (WINED3DFMT_FLAG_COMPRESSED | WINED3DFMT_FLAG_BROKEN_PITCH)) == WINED3DFMT_FLAG_COMPRESSED)
        {
            /* Compressed textures are block based, so calculate the offset of
             * the block that contains the top-left pixel of the locked rectangle. */
            locked_rect->pBits = surface->resource.allocatedMemory
                    + ((rect->top / format->block_height) * locked_rect->Pitch)
                    + ((rect->left / format->block_width) * format->block_byte_count);
        }
        else
        {
            locked_rect->pBits = surface->resource.allocatedMemory
                    + (locked_rect->Pitch * rect->top)
                    + (rect->left * format->byte_count);
        }
        surface->lockedRect.left = rect->left;
        surface->lockedRect.top = rect->top;
        surface->lockedRect.right = rect->right;
        surface->lockedRect.bottom = rect->bottom;
    }

    TRACE("Locked rect %s.\n", wine_dbgstr_rect(&surface->lockedRect));
    TRACE("Returning memory %p, pitch %u.\n", locked_rect->pBits, locked_rect->Pitch);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_surface_getdc(struct wined3d_surface *surface, HDC *dc)
{
    HRESULT hr;

    TRACE("surface %p, dc %p.\n", surface, dc);

    if (surface->flags & SFLAG_USERPTR)
    {
        ERR("Not supported on surfaces with application-provided memory.\n");
        return WINEDDERR_NODC;
    }

    /* Give more detailed info for ddraw. */
    if (surface->flags & SFLAG_DCINUSE)
        return WINEDDERR_DCALREADYCREATED;

    /* Can't GetDC if the surface is locked. */
    if (surface->flags & SFLAG_LOCKED)
        return WINED3DERR_INVALIDCALL;

    hr = surface->surface_ops->surface_getdc(surface);
    if (FAILED(hr))
        return hr;

    if (surface->resource.format->id == WINED3DFMT_P8_UINT
            || surface->resource.format->id == WINED3DFMT_P8_UINT_A8_UNORM)
    {
        /* GetDC on palettized formats is unsupported in D3D9, and the method
         * is missing in D3D8, so this should only be used for DX <=7
         * surfaces (with non-device palettes). */
        const PALETTEENTRY *pal = NULL;

        if (surface->palette)
        {
            pal = surface->palette->palents;
        }
        else
        {
            struct wined3d_swapchain *swapchain = surface->resource.device->swapchains[0];
            struct wined3d_surface *dds_primary = swapchain->front_buffer;

            if (dds_primary && dds_primary->palette)
                pal = dds_primary->palette->palents;
        }

        if (pal)
        {
            RGBQUAD col[256];
            unsigned int i;

            for (i = 0; i < 256; ++i)
            {
                col[i].rgbRed = pal[i].peRed;
                col[i].rgbGreen = pal[i].peGreen;
                col[i].rgbBlue = pal[i].peBlue;
                col[i].rgbReserved = 0;
            }
            SetDIBColorTable(surface->hDC, 0, 256, col);
        }
    }

    surface->flags |= SFLAG_DCINUSE;

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

    if ((surface->flags & SFLAG_PBO) && surface->resource.allocatedMemory)
    {
        /* Copy the contents of the DIB over to the PBO. */
        memcpy(surface->resource.allocatedMemory, surface->dib.bitmap_data, surface->dib.bitmap_size);
    }

    /* We locked first, so unlock now. */
    wined3d_surface_unmap(surface);

    surface->flags &= ~SFLAG_DCINUSE;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_surface_flip(struct wined3d_surface *surface, struct wined3d_surface *override, DWORD flags)
{
    struct wined3d_swapchain *swapchain;
    HRESULT hr;

    TRACE("surface %p, override %p, flags %#x.\n", surface, override, flags);

    if (surface->container.type != WINED3D_CONTAINER_SWAPCHAIN)
    {
        ERR("Flipped surface is not on a swapchain.\n");
        return WINEDDERR_NOTFLIPPABLE;
    }
    swapchain = surface->container.u.swapchain;

    hr = surface->surface_ops->surface_flip(surface, override);
    if (FAILED(hr))
        return hr;

    /* Just overwrite the swapchain presentation interval. This is ok because
     * only ddraw apps can call Flip, and only d3d8 and d3d9 applications
     * specify the presentation interval. */
    if (!(flags & (WINEDDFLIP_NOVSYNC | WINEDDFLIP_INTERVAL2 | WINEDDFLIP_INTERVAL3 | WINEDDFLIP_INTERVAL4)))
        swapchain->presentParms.PresentationInterval = WINED3DPRESENT_INTERVAL_ONE;
    else if (flags & WINEDDFLIP_NOVSYNC)
        swapchain->presentParms.PresentationInterval = WINED3DPRESENT_INTERVAL_IMMEDIATE;
    else if (flags & WINEDDFLIP_INTERVAL2)
        swapchain->presentParms.PresentationInterval = WINED3DPRESENT_INTERVAL_TWO;
    else if (flags & WINEDDFLIP_INTERVAL3)
        swapchain->presentParms.PresentationInterval = WINED3DPRESENT_INTERVAL_THREE;
    else
        swapchain->presentParms.PresentationInterval = WINED3DPRESENT_INTERVAL_FOUR;

    return wined3d_swapchain_present(swapchain, NULL, NULL, swapchain->win_handle, NULL, 0);
}

/* Do not call while under the GL lock. */
void surface_internal_preload(struct wined3d_surface *surface, enum WINED3DSRGB srgb)
{
    struct wined3d_device *device = surface->resource.device;

    TRACE("iface %p, srgb %#x.\n", surface, srgb);

    if (surface->container.type == WINED3D_CONTAINER_TEXTURE)
    {
        struct wined3d_texture *texture = surface->container.u.texture;

        TRACE("Passing to container (%p).\n", texture);
        texture->texture_ops->texture_preload(texture, srgb);
    }
    else
    {
        struct wined3d_context *context = NULL;

        TRACE("(%p) : About to load surface\n", surface);

        if (!device->isInDraw) context = context_acquire(device, NULL);

        if (surface->resource.format->id == WINED3DFMT_P8_UINT
                || surface->resource.format->id == WINED3DFMT_P8_UINT_A8_UNORM)
        {
            if (palette9_changed(surface))
            {
                TRACE("Reloading surface because the d3d8/9 palette was changed\n");
                /* TODO: This is not necessarily needed with hw palettized texture support */
                surface_load_location(surface, SFLAG_INSYSMEM, NULL);
                /* Make sure the texture is reloaded because of the palette change, this kills performance though :( */
                surface_modify_location(surface, SFLAG_INTEXTURE, FALSE);
            }
        }

        surface_load(surface, srgb == SRGB_SRGB ? TRUE : FALSE);

        if (surface->resource.pool == WINED3DPOOL_DEFAULT)
        {
            /* Tell opengl to try and keep this texture in video ram (well mostly) */
            GLclampf tmp;
            tmp = 0.9f;
            ENTER_GL();
            glPrioritizeTextures(1, &surface->texture_name, &tmp);
            LEAVE_GL();
        }

        if (context) context_release(context);
    }
}

BOOL surface_init_sysmem(struct wined3d_surface *surface)
{
    if (!surface->resource.allocatedMemory)
    {
        surface->resource.heapMemory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                surface->resource.size + RESOURCE_ALIGNMENT);
        if (!surface->resource.heapMemory)
        {
            ERR("Out of memory\n");
            return FALSE;
        }
        surface->resource.allocatedMemory =
            (BYTE *)(((ULONG_PTR)surface->resource.heapMemory + (RESOURCE_ALIGNMENT - 1)) & ~(RESOURCE_ALIGNMENT - 1));
    }
    else
    {
        memset(surface->resource.allocatedMemory, 0, surface->resource.size);
    }

    surface_modify_location(surface, SFLAG_INSYSMEM, TRUE);

    return TRUE;
}

/* Read the framebuffer back into the surface */
static void read_from_framebuffer(struct wined3d_surface *surface, const RECT *rect, void *dest, UINT pitch)
{
    struct wined3d_device *device = surface->resource.device;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;
    BYTE *mem;
    GLint fmt;
    GLint type;
    BYTE *row, *top, *bottom;
    int i;
    BOOL bpp;
    RECT local_rect;
    BOOL srcIsUpsideDown;
    GLint rowLen = 0;
    GLint skipPix = 0;
    GLint skipRow = 0;

    if(wined3d_settings.rendertargetlock_mode == RTL_DISABLE) {
        static BOOL warned = FALSE;
        if(!warned) {
            ERR("The application tries to lock the render target, but render target locking is disabled\n");
            warned = TRUE;
        }
        return;
    }

    context = context_acquire(device, surface);
    context_apply_blit_state(context, device);
    gl_info = context->gl_info;

    ENTER_GL();

    /* Select the correct read buffer, and give some debug output.
     * There is no need to keep track of the current read buffer or reset it, every part of the code
     * that reads sets the read buffer as desired.
     */
    if (surface_is_offscreen(surface))
    {
        /* Mapping the primary render target which is not on a swapchain.
         * Read from the back buffer. */
        TRACE("Mapping offscreen render target.\n");
        glReadBuffer(device->offscreenBuffer);
        srcIsUpsideDown = TRUE;
    }
    else
    {
        /* Onscreen surfaces are always part of a swapchain */
        GLenum buffer = surface_get_gl_buffer(surface);
        TRACE("Mapping %#x buffer.\n", buffer);
        glReadBuffer(buffer);
        checkGLcall("glReadBuffer");
        srcIsUpsideDown = FALSE;
    }

    /* TODO: Get rid of the extra rectangle comparison and construction of a full surface rectangle */
    if (!rect)
    {
        local_rect.left = 0;
        local_rect.top = 0;
        local_rect.right = surface->resource.width;
        local_rect.bottom = surface->resource.height;
    }
    else
    {
        local_rect = *rect;
    }
    /* TODO: Get rid of the extra GetPitch call, LockRect does that too. Cache the pitch */

    switch (surface->resource.format->id)
    {
        case WINED3DFMT_P8_UINT:
        {
            if (primary_render_target_is_p8(device))
            {
                /* In case of P8 render targets the index is stored in the alpha component */
                fmt = GL_ALPHA;
                type = GL_UNSIGNED_BYTE;
                mem = dest;
                bpp = surface->resource.format->byte_count;
            }
            else
            {
                /* GL can't return palettized data, so read ARGB pixels into a
                 * separate block of memory and convert them into palettized format
                 * in software. Slow, but if the app means to use palettized render
                 * targets and locks it...
                 *
                 * Use GL_RGB, GL_UNSIGNED_BYTE to read the surface for performance reasons
                 * Don't use GL_BGR as in the WINED3DFMT_R8G8B8 case, instead watch out
                 * for the color channels when palettizing the colors.
                 */
                fmt = GL_RGB;
                type = GL_UNSIGNED_BYTE;
                pitch *= 3;
                mem = HeapAlloc(GetProcessHeap(), 0, surface->resource.size * 3);
                if (!mem)
                {
                    ERR("Out of memory\n");
                    LEAVE_GL();
                    return;
                }
                bpp = surface->resource.format->byte_count * 3;
            }
        }
        break;

        default:
            mem = dest;
            fmt = surface->resource.format->glFormat;
            type = surface->resource.format->glType;
            bpp = surface->resource.format->byte_count;
    }

    if (surface->flags & SFLAG_PBO)
    {
        GL_EXTCALL(glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, surface->pbo));
        checkGLcall("glBindBufferARB");
        if (mem)
        {
            ERR("mem not null for pbo -- unexpected\n");
            mem = NULL;
        }
    }

    /* Save old pixel store pack state */
    glGetIntegerv(GL_PACK_ROW_LENGTH, &rowLen);
    checkGLcall("glGetIntegerv");
    glGetIntegerv(GL_PACK_SKIP_PIXELS, &skipPix);
    checkGLcall("glGetIntegerv");
    glGetIntegerv(GL_PACK_SKIP_ROWS, &skipRow);
    checkGLcall("glGetIntegerv");

    /* Setup pixel store pack state -- to glReadPixels into the correct place */
    glPixelStorei(GL_PACK_ROW_LENGTH, surface->resource.width);
    checkGLcall("glPixelStorei");
    glPixelStorei(GL_PACK_SKIP_PIXELS, local_rect.left);
    checkGLcall("glPixelStorei");
    glPixelStorei(GL_PACK_SKIP_ROWS, local_rect.top);
    checkGLcall("glPixelStorei");

    glReadPixels(local_rect.left, !srcIsUpsideDown ? (surface->resource.height - local_rect.bottom) : local_rect.top,
            local_rect.right - local_rect.left,
            local_rect.bottom - local_rect.top,
            fmt, type, mem);
    checkGLcall("glReadPixels");

    /* Reset previous pixel store pack state */
    glPixelStorei(GL_PACK_ROW_LENGTH, rowLen);
    checkGLcall("glPixelStorei");
    glPixelStorei(GL_PACK_SKIP_PIXELS, skipPix);
    checkGLcall("glPixelStorei");
    glPixelStorei(GL_PACK_SKIP_ROWS, skipRow);
    checkGLcall("glPixelStorei");

    if (surface->flags & SFLAG_PBO)
    {
        GL_EXTCALL(glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0));
        checkGLcall("glBindBufferARB");

        /* Check if we need to flip the image. If we need to flip use glMapBufferARB
         * to get a pointer to it and perform the flipping in software. This is a lot
         * faster than calling glReadPixels for each line. In case we want more speed
         * we should rerender it flipped in a FBO and read the data back from the FBO. */
        if (!srcIsUpsideDown)
        {
            GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, surface->pbo));
            checkGLcall("glBindBufferARB");

            mem = GL_EXTCALL(glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_READ_WRITE_ARB));
            checkGLcall("glMapBufferARB");
        }
    }

    /* TODO: Merge this with the palettization loop below for P8 targets */
    if(!srcIsUpsideDown) {
        UINT len, off;
        /* glReadPixels returns the image upside down, and there is no way to prevent this.
            Flip the lines in software */
        len = (local_rect.right - local_rect.left) * bpp;
        off = local_rect.left * bpp;

        row = HeapAlloc(GetProcessHeap(), 0, len);
        if(!row) {
            ERR("Out of memory\n");
            if (surface->resource.format->id == WINED3DFMT_P8_UINT)
                HeapFree(GetProcessHeap(), 0, mem);
            LEAVE_GL();
            return;
        }

        top = mem + pitch * local_rect.top;
        bottom = mem + pitch * (local_rect.bottom - 1);
        for(i = 0; i < (local_rect.bottom - local_rect.top) / 2; i++) {
            memcpy(row, top + off, len);
            memcpy(top + off, bottom + off, len);
            memcpy(bottom + off, row, len);
            top += pitch;
            bottom -= pitch;
        }
        HeapFree(GetProcessHeap(), 0, row);

        /* Unmap the temp PBO buffer */
        if (surface->flags & SFLAG_PBO)
        {
            GL_EXTCALL(glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB));
            GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0));
        }
    }

    LEAVE_GL();
    context_release(context);

    /* For P8 textures we need to perform an inverse palette lookup. This is
     * done by searching for a palette index which matches the RGB value.
     * Note this isn't guaranteed to work when there are multiple entries for
     * the same color but we have no choice. In case of P8 render targets,
     * the index is stored in the alpha component so no conversion is needed. */
    if (surface->resource.format->id == WINED3DFMT_P8_UINT && !primary_render_target_is_p8(device))
    {
        const PALETTEENTRY *pal = NULL;
        DWORD width = pitch / 3;
        int x, y, c;

        if (surface->palette)
        {
            pal = surface->palette->palents;
        }
        else
        {
            ERR("Palette is missing, cannot perform inverse palette lookup\n");
            HeapFree(GetProcessHeap(), 0, mem);
            return;
        }

        for(y = local_rect.top; y < local_rect.bottom; y++) {
            for(x = local_rect.left; x < local_rect.right; x++) {
                /*                      start              lines            pixels      */
                const BYTE *blue = mem + y * pitch + x * (sizeof(BYTE) * 3);
                const BYTE *green = blue  + 1;
                const BYTE *red = green + 1;

                for(c = 0; c < 256; c++) {
                    if(*red   == pal[c].peRed   &&
                       *green == pal[c].peGreen &&
                       *blue  == pal[c].peBlue)
                    {
                        *((BYTE *) dest + y * width + x) = c;
                        break;
                    }
                }
            }
        }
        HeapFree(GetProcessHeap(), 0, mem);
    }
}

/* Read the framebuffer contents into a texture */
static void read_from_framebuffer_texture(struct wined3d_surface *surface, BOOL srgb)
{
    struct wined3d_device *device = surface->resource.device;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;

    if (!surface_is_offscreen(surface))
    {
        /* We would need to flip onscreen surfaces, but there's no efficient
         * way to do that here. It makes more sense for the caller to
         * explicitly go through sysmem. */
        ERR("Not supported for onscreen targets.\n");
        return;
    }

    /* Activate the surface to read from. In some situations it isn't the currently active target(e.g. backbuffer
     * locking during offscreen rendering). RESOURCELOAD is ok because glCopyTexSubImage2D isn't affected by any
     * states in the stateblock, and no driver was found yet that had bugs in that regard.
     */
    context = context_acquire(device, surface);
    gl_info = context->gl_info;

    surface_prepare_texture(surface, gl_info, srgb);
    surface_bind_and_dirtify(surface, gl_info, srgb);

    TRACE("Reading back offscreen render target %p.\n", surface);

    ENTER_GL();

    glReadBuffer(device->offscreenBuffer);
    checkGLcall("glReadBuffer");

    glCopyTexSubImage2D(surface->texture_target, surface->texture_level,
            0, 0, 0, 0, surface->resource.width, surface->resource.height);
    checkGLcall("glCopyTexSubImage2D");

    LEAVE_GL();

    context_release(context);
}

/* Context activation is done by the caller. */
static void surface_prepare_texture_internal(struct wined3d_surface *surface,
        const struct wined3d_gl_info *gl_info, BOOL srgb)
{
    DWORD alloc_flag = srgb ? SFLAG_SRGBALLOCATED : SFLAG_ALLOCATED;
    CONVERT_TYPES convert;
    struct wined3d_format format;

    if (surface->flags & alloc_flag) return;

    d3dfmt_get_conv(surface, TRUE, TRUE, &format, &convert);
    if (convert != NO_CONVERSION || format.convert) surface->flags |= SFLAG_CONVERTED;
    else surface->flags &= ~SFLAG_CONVERTED;

    surface_bind_and_dirtify(surface, gl_info, srgb);
    surface_allocate_surface(surface, gl_info, &format, srgb);
    surface->flags |= alloc_flag;
}

/* Context activation is done by the caller. */
void surface_prepare_texture(struct wined3d_surface *surface, const struct wined3d_gl_info *gl_info, BOOL srgb)
{
    if (surface->container.type == WINED3D_CONTAINER_TEXTURE)
    {
        struct wined3d_texture *texture = surface->container.u.texture;
        UINT sub_count = texture->level_count * texture->layer_count;
        UINT i;

        TRACE("surface %p is a subresource of texture %p.\n", surface, texture);

        for (i = 0; i < sub_count; ++i)
        {
            struct wined3d_surface *s = surface_from_resource(texture->sub_resources[i]);
            surface_prepare_texture_internal(s, gl_info, srgb);
        }

        return;
    }

    surface_prepare_texture_internal(surface, gl_info, srgb);
}

static void flush_to_framebuffer_drawpixels(struct wined3d_surface *surface,
        const RECT *rect, GLenum fmt, GLenum type, UINT bpp, const BYTE *mem)
{
    struct wined3d_device *device = surface->resource.device;
    UINT pitch = wined3d_surface_get_pitch(surface);
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;
    RECT local_rect;
    UINT w, h;

    surface_get_rect(surface, rect, &local_rect);

    mem += local_rect.top * pitch + local_rect.left * bpp;
    w = local_rect.right - local_rect.left;
    h = local_rect.bottom - local_rect.top;

    /* Activate the correct context for the render target */
    context = context_acquire(device, surface);
    context_apply_blit_state(context, device);
    gl_info = context->gl_info;

    ENTER_GL();

    if (!surface_is_offscreen(surface))
    {
        GLenum buffer = surface_get_gl_buffer(surface);
        TRACE("Unlocking %#x buffer.\n", buffer);
        context_set_draw_buffer(context, buffer);

        surface_translate_drawable_coords(surface, context->win_handle, &local_rect);
        glPixelZoom(1.0f, -1.0f);
    }
    else
    {
        /* Primary offscreen render target */
        TRACE("Offscreen render target.\n");
        context_set_draw_buffer(context, device->offscreenBuffer);

        glPixelZoom(1.0f, 1.0f);
    }

    glRasterPos3i(local_rect.left, local_rect.top, 1);
    checkGLcall("glRasterPos3i");

    /* If not fullscreen, we need to skip a number of bytes to find the next row of data */
    glPixelStorei(GL_UNPACK_ROW_LENGTH, surface->resource.width);

    if (surface->flags & SFLAG_PBO)
    {
        GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, surface->pbo));
        checkGLcall("glBindBufferARB");
    }

    glDrawPixels(w, h, fmt, type, mem);
    checkGLcall("glDrawPixels");

    if (surface->flags & SFLAG_PBO)
    {
        GL_EXTCALL(glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0));
        checkGLcall("glBindBufferARB");
    }

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    checkGLcall("glPixelStorei(GL_UNPACK_ROW_LENGTH, 0)");

    LEAVE_GL();

    if (wined3d_settings.strict_draw_ordering
            || (surface->container.type == WINED3D_CONTAINER_SWAPCHAIN
            && surface->container.u.swapchain->front_buffer == surface))
        wglFlush();

    context_release(context);
}

HRESULT d3dfmt_get_conv(struct wined3d_surface *surface, BOOL need_alpha_ck,
        BOOL use_texturing, struct wined3d_format *format, CONVERT_TYPES *convert)
{
    BOOL colorkey_active = need_alpha_ck && (surface->CKeyFlags & WINEDDSD_CKSRCBLT);
    struct wined3d_device *device = surface->resource.device;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    BOOL blit_supported = FALSE;

    /* Copy the default values from the surface. Below we might perform fixups */
    /* TODO: get rid of color keying desc fixups by using e.g. a table. */
    *format = *surface->resource.format;
    *convert = NO_CONVERSION;

    /* Ok, now look if we have to do any conversion */
    switch (surface->resource.format->id)
    {
        case WINED3DFMT_P8_UINT:
            /* Below the call to blit_supported is disabled for Wine 1.2
             * because the function isn't operating correctly yet. At the
             * moment 8-bit blits are handled in software and if certain GL
             * extensions are around, surface conversion is performed at
             * upload time. The blit_supported call recognizes it as a
             * destination fixup. This type of upload 'fixup' and 8-bit to
             * 8-bit blits need to be handled by the blit_shader.
             * TODO: get rid of this #if 0. */
#if 0
            blit_supported = device->blitter->blit_supported(&device->adapter->gl_info, WINED3D_BLIT_OP_COLOR_BLIT,
                    &rect, surface->resource.usage, surface->resource.pool, surface->resource.format,
                    &rect, surface->resource.usage, surface->resource.pool, surface->resource.format);
#endif
            blit_supported = gl_info->supported[EXT_PALETTED_TEXTURE] || gl_info->supported[ARB_FRAGMENT_PROGRAM];

            /* Use conversion when the blit_shader backend supports it. It only supports this in case of
             * texturing. Further also use conversion in case of color keying.
             * Paletted textures can be emulated using shaders but only do that for 2D purposes e.g. situations
             * in which the main render target uses p8. Some games like GTA Vice City use P8 for texturing which
             * conflicts with this.
             */
            if (!((blit_supported && device->fb.render_targets && surface == device->fb.render_targets[0]))
                    || colorkey_active || !use_texturing)
            {
                format->glFormat = GL_RGBA;
                format->glInternal = GL_RGBA;
                format->glType = GL_UNSIGNED_BYTE;
                format->conv_byte_count = 4;
                if (colorkey_active)
                    *convert = CONVERT_PALETTED_CK;
                else
                    *convert = CONVERT_PALETTED;
            }
            break;

        case WINED3DFMT_B2G3R3_UNORM:
            /* **********************
                GL_UNSIGNED_BYTE_3_3_2
                ********************** */
            if (colorkey_active) {
                /* This texture format will never be used.. So do not care about color keying
                    up until the point in time it will be needed :-) */
                FIXME(" ColorKeying not supported in the RGB 332 format !\n");
            }
            break;

        case WINED3DFMT_B5G6R5_UNORM:
            if (colorkey_active)
            {
                *convert = CONVERT_CK_565;
                format->glFormat = GL_RGBA;
                format->glInternal = GL_RGB5_A1;
                format->glType = GL_UNSIGNED_SHORT_5_5_5_1;
                format->conv_byte_count = 2;
            }
            break;

        case WINED3DFMT_B5G5R5X1_UNORM:
            if (colorkey_active)
            {
                *convert = CONVERT_CK_5551;
                format->glFormat = GL_BGRA;
                format->glInternal = GL_RGB5_A1;
                format->glType = GL_UNSIGNED_SHORT_1_5_5_5_REV;
                format->conv_byte_count = 2;
            }
            break;

        case WINED3DFMT_B8G8R8_UNORM:
            if (colorkey_active)
            {
                *convert = CONVERT_CK_RGB24;
                format->glFormat = GL_RGBA;
                format->glInternal = GL_RGBA8;
                format->glType = GL_UNSIGNED_INT_8_8_8_8;
                format->conv_byte_count = 4;
            }
            break;

        case WINED3DFMT_B8G8R8X8_UNORM:
            if (colorkey_active)
            {
                *convert = CONVERT_RGB32_888;
                format->glFormat = GL_RGBA;
                format->glInternal = GL_RGBA8;
                format->glType = GL_UNSIGNED_INT_8_8_8_8;
                format->conv_byte_count = 4;
            }
            break;

        default:
            break;
    }

    return WINED3D_OK;
}

void d3dfmt_p8_init_palette(struct wined3d_surface *surface, BYTE table[256][4], BOOL colorkey)
{
    struct wined3d_device *device = surface->resource.device;
    struct wined3d_palette *pal = surface->palette;
    BOOL index_in_alpha = FALSE;
    unsigned int i;

    /* Old games like StarCraft, C&C, Red Alert and others use P8 render targets.
     * Reading back the RGB output each lockrect (each frame as they lock the whole screen)
     * is slow. Further RGB->P8 conversion is not possible because palettes can have
     * duplicate entries. Store the color key in the unused alpha component to speed the
     * download up and to make conversion unneeded. */
    index_in_alpha = primary_render_target_is_p8(device);

    if (!pal)
    {
        UINT dxVersion = device->wined3d->dxVersion;

        /* In DirectDraw the palette is a property of the surface, there are no such things as device palettes. */
        if (dxVersion <= 7)
        {
            ERR("This code should never get entered for DirectDraw!, expect problems\n");
            if (index_in_alpha)
            {
                /* Guarantees that memory representation remains correct after sysmem<->texture transfers even if
                 * there's no palette at this time. */
                for (i = 0; i < 256; i++) table[i][3] = i;
            }
        }
        else
        {
            /* Direct3D >= 8 palette usage style: P8 textures use device palettes, palette entry format is A8R8G8B8,
             * alpha is stored in peFlags and may be used by the app if D3DPTEXTURECAPS_ALPHAPALETTE device
             * capability flag is present (wine does advertise this capability) */
            for (i = 0; i < 256; ++i)
            {
                table[i][0] = device->palettes[device->currentPalette][i].peRed;
                table[i][1] = device->palettes[device->currentPalette][i].peGreen;
                table[i][2] = device->palettes[device->currentPalette][i].peBlue;
                table[i][3] = device->palettes[device->currentPalette][i].peFlags;
            }
        }
    }
    else
    {
        TRACE("Using surface palette %p\n", pal);
        /* Get the surface's palette */
        for (i = 0; i < 256; ++i)
        {
            table[i][0] = pal->palents[i].peRed;
            table[i][1] = pal->palents[i].peGreen;
            table[i][2] = pal->palents[i].peBlue;

            /* When index_in_alpha is set the palette index is stored in the
             * alpha component. In case of a readback we can then read
             * GL_ALPHA. Color keying is handled in BltOverride using a
             * GL_ALPHA_TEST using GL_NOT_EQUAL. In case of index_in_alpha the
             * color key itself is passed to glAlphaFunc in other cases the
             * alpha component of pixels that should be masked away is set to 0. */
            if (index_in_alpha)
            {
                table[i][3] = i;
            }
            else if (colorkey && (i >= surface->SrcBltCKey.dwColorSpaceLowValue)
                    && (i <= surface->SrcBltCKey.dwColorSpaceHighValue))
            {
                table[i][3] = 0x00;
            }
            else if (pal->flags & WINEDDPCAPS_ALPHA)
            {
                table[i][3] = pal->palents[i].peFlags;
            }
            else
            {
                table[i][3] = 0xFF;
            }
        }
    }
}

static HRESULT d3dfmt_convert_surface(const BYTE *src, BYTE *dst, UINT pitch, UINT width,
        UINT height, UINT outpitch, CONVERT_TYPES convert, struct wined3d_surface *surface)
{
    const BYTE *source;
    BYTE *dest;
    TRACE("(%p)->(%p),(%d,%d,%d,%d,%p)\n", src, dst, pitch, height, outpitch, convert, surface);

    switch (convert) {
        case NO_CONVERSION:
        {
            memcpy(dst, src, pitch * height);
            break;
        }
        case CONVERT_PALETTED:
        case CONVERT_PALETTED_CK:
        {
            BYTE table[256][4];
            unsigned int x, y;

            d3dfmt_p8_init_palette(surface, table, (convert == CONVERT_PALETTED_CK));

            for (y = 0; y < height; y++)
            {
                source = src + pitch * y;
                dest = dst + outpitch * y;
                /* This is an 1 bpp format, using the width here is fine */
                for (x = 0; x < width; x++) {
                    BYTE color = *source++;
                    *dest++ = table[color][0];
                    *dest++ = table[color][1];
                    *dest++ = table[color][2];
                    *dest++ = table[color][3];
                }
            }
        }
        break;

        case CONVERT_CK_565:
        {
            /* Converting the 565 format in 5551 packed to emulate color-keying.

              Note : in all these conversion, it would be best to average the averaging
                      pixels to get the color of the pixel that will be color-keyed to
                      prevent 'color bleeding'. This will be done later on if ever it is
                      too visible.

              Note2: Nvidia documents say that their driver does not support alpha + color keying
                     on the same surface and disables color keying in such a case
            */
            unsigned int x, y;
            const WORD *Source;
            WORD *Dest;

            TRACE("Color keyed 565\n");

            for (y = 0; y < height; y++) {
                Source = (const WORD *)(src + y * pitch);
                Dest = (WORD *) (dst + y * outpitch);
                for (x = 0; x < width; x++ ) {
                    WORD color = *Source++;
                    *Dest = ((color & 0xFFC0) | ((color & 0x1F) << 1));
                    if ((color < surface->SrcBltCKey.dwColorSpaceLowValue)
                            || (color > surface->SrcBltCKey.dwColorSpaceHighValue))
                        *Dest |= 0x0001;
                    Dest++;
                }
            }
        }
        break;

        case CONVERT_CK_5551:
        {
            /* Converting X1R5G5B5 format to R5G5B5A1 to emulate color-keying. */
            unsigned int x, y;
            const WORD *Source;
            WORD *Dest;
            TRACE("Color keyed 5551\n");
            for (y = 0; y < height; y++) {
                Source = (const WORD *)(src + y * pitch);
                Dest = (WORD *) (dst + y * outpitch);
                for (x = 0; x < width; x++ ) {
                    WORD color = *Source++;
                    *Dest = color;
                    if ((color < surface->SrcBltCKey.dwColorSpaceLowValue)
                            || (color > surface->SrcBltCKey.dwColorSpaceHighValue))
                        *Dest |= (1 << 15);
                    else
                        *Dest &= ~(1 << 15);
                    Dest++;
                }
            }
        }
        break;

        case CONVERT_CK_RGB24:
        {
            /* Converting R8G8B8 format to R8G8B8A8 with color-keying. */
            unsigned int x, y;
            for (y = 0; y < height; y++)
            {
                source = src + pitch * y;
                dest = dst + outpitch * y;
                for (x = 0; x < width; x++) {
                    DWORD color = ((DWORD)source[0] << 16) + ((DWORD)source[1] << 8) + (DWORD)source[2] ;
                    DWORD dstcolor = color << 8;
                    if ((color < surface->SrcBltCKey.dwColorSpaceLowValue)
                            || (color > surface->SrcBltCKey.dwColorSpaceHighValue))
                        dstcolor |= 0xff;
                    *(DWORD*)dest = dstcolor;
                    source += 3;
                    dest += 4;
                }
            }
        }
        break;

        case CONVERT_RGB32_888:
        {
            /* Converting X8R8G8B8 format to R8G8B8A8 with color-keying. */
            unsigned int x, y;
            for (y = 0; y < height; y++)
            {
                source = src + pitch * y;
                dest = dst + outpitch * y;
                for (x = 0; x < width; x++) {
                    DWORD color = 0xffffff & *(const DWORD*)source;
                    DWORD dstcolor = color << 8;
                    if ((color < surface->SrcBltCKey.dwColorSpaceLowValue)
                            || (color > surface->SrcBltCKey.dwColorSpaceHighValue))
                        dstcolor |= 0xff;
                    *(DWORD*)dest = dstcolor;
                    source += 4;
                    dest += 4;
                }
            }
        }
        break;

        default:
            ERR("Unsupported conversion type %#x.\n", convert);
    }
    return WINED3D_OK;
}

BOOL palette9_changed(struct wined3d_surface *surface)
{
    struct wined3d_device *device = surface->resource.device;

    if (surface->palette || (surface->resource.format->id != WINED3DFMT_P8_UINT
            && surface->resource.format->id != WINED3DFMT_P8_UINT_A8_UNORM))
    {
        /* If a ddraw-style palette is attached assume no d3d9 palette change.
         * Also the palette isn't interesting if the surface format isn't P8 or A8P8
         */
        return FALSE;
    }

    if (surface->palette9)
    {
        if (!memcmp(surface->palette9, device->palettes[device->currentPalette], sizeof(PALETTEENTRY) * 256))
        {
            return FALSE;
        }
    }
    else
    {
        surface->palette9 = HeapAlloc(GetProcessHeap(), 0, sizeof(PALETTEENTRY) * 256);
    }
    memcpy(surface->palette9, device->palettes[device->currentPalette], sizeof(PALETTEENTRY) * 256);

    return TRUE;
}

void flip_surface(struct wined3d_surface *front, struct wined3d_surface *back)
{
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
        HBITMAP tmp;
        BOOL hasDib = front->flags & SFLAG_DIBSECTION;
        tmp = front->dib.DIBsection;
        front->dib.DIBsection = back->dib.DIBsection;
        back->dib.DIBsection = tmp;

        if (back->flags & SFLAG_DIBSECTION) front->flags |= SFLAG_DIBSECTION;
        else front->flags &= ~SFLAG_DIBSECTION;
        if (hasDib) back->flags |= SFLAG_DIBSECTION;
        else back->flags &= ~SFLAG_DIBSECTION;
    }

    /* Flip the surface data */
    {
        void* tmp;

        tmp = front->dib.bitmap_data;
        front->dib.bitmap_data = back->dib.bitmap_data;
        back->dib.bitmap_data = tmp;

        tmp = front->resource.allocatedMemory;
        front->resource.allocatedMemory = back->resource.allocatedMemory;
        back->resource.allocatedMemory = tmp;

        tmp = front->resource.heapMemory;
        front->resource.heapMemory = back->resource.heapMemory;
        back->resource.heapMemory = tmp;
    }

    /* Flip the PBO */
    {
        GLuint tmp_pbo = front->pbo;
        front->pbo = back->pbo;
        back->pbo = tmp_pbo;
    }

    /* client_memory should not be different, but just in case */
    {
        BOOL tmp;
        tmp = front->dib.client_memory;
        front->dib.client_memory = back->dib.client_memory;
        back->dib.client_memory = tmp;
    }

    /* Flip the opengl texture */
    {
        GLuint tmp;

        tmp = back->texture_name;
        back->texture_name = front->texture_name;
        front->texture_name = tmp;

        tmp = back->texture_name_srgb;
        back->texture_name_srgb = front->texture_name_srgb;
        front->texture_name_srgb = tmp;

        resource_unload(&back->resource);
        resource_unload(&front->resource);
    }

    {
        DWORD tmp_flags = back->flags;
        back->flags = front->flags;
        front->flags = tmp_flags;
    }
}

/* Does a direct frame buffer -> texture copy. Stretching is done with single
 * pixel copy calls. */
static void fb_copy_to_texture_direct(struct wined3d_surface *dst_surface, struct wined3d_surface *src_surface,
        const RECT *src_rect, const RECT *dst_rect_in, WINED3DTEXTUREFILTERTYPE Filter)
{
    struct wined3d_device *device = dst_surface->resource.device;
    float xrel, yrel;
    UINT row;
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
    context_apply_blit_state(context, device);
    surface_internal_preload(dst_surface, SRGB_RGB);
    ENTER_GL();

    /* Bind the target texture */
    glBindTexture(dst_surface->texture_target, dst_surface->texture_name);
    checkGLcall("glBindTexture");
    if (surface_is_offscreen(src_surface))
    {
        TRACE("Reading from an offscreen target\n");
        upsidedown = !upsidedown;
        glReadBuffer(device->offscreenBuffer);
    }
    else
    {
        glReadBuffer(surface_get_gl_buffer(src_surface));
    }
    checkGLcall("glReadBuffer");

    xrel = (float) (src_rect->right - src_rect->left) / (float) (dst_rect.right - dst_rect.left);
    yrel = (float) (src_rect->bottom - src_rect->top) / (float) (dst_rect.bottom - dst_rect.top);

    if ((xrel - 1.0f < -eps) || (xrel - 1.0f > eps))
    {
        FIXME("Doing a pixel by pixel copy from the framebuffer to a texture, expect major performance issues\n");

        if(Filter != WINED3DTEXF_NONE && Filter != WINED3DTEXF_POINT) {
            ERR("Texture filtering not supported in direct blit\n");
        }
    }
    else if ((Filter != WINED3DTEXF_NONE && Filter != WINED3DTEXF_POINT)
            && ((yrel - 1.0f < -eps) || (yrel - 1.0f > eps)))
    {
        ERR("Texture filtering not supported in direct blit\n");
    }

    if (upsidedown
            && !((xrel - 1.0f < -eps) || (xrel - 1.0f > eps))
            && !((yrel - 1.0f < -eps) || (yrel - 1.0f > eps)))
    {
        /* Upside down copy without stretching is nice, one glCopyTexSubImage call will do */

        glCopyTexSubImage2D(dst_surface->texture_target, dst_surface->texture_level,
                dst_rect.left /*xoffset */, dst_rect.top /* y offset */,
                src_rect->left, src_surface->resource.height - src_rect->bottom,
                dst_rect.right - dst_rect.left, dst_rect.bottom - dst_rect.top);
    }
    else
    {
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
                UINT col;

                for (col = dst_rect.left; col < dst_rect.right; ++col)
                {
                    glCopyTexSubImage2D(dst_surface->texture_target, dst_surface->texture_level,
                            dst_rect.left + col /* x offset */, row /* y offset */,
                            src_rect->left + col * xrel, yoffset - (int) (row * yrel), 1, 1);
                }
            }
            else
            {
                glCopyTexSubImage2D(dst_surface->texture_target, dst_surface->texture_level,
                        dst_rect.left /* x offset */, row /* y offset */,
                        src_rect->left, yoffset - (int) (row * yrel), dst_rect.right - dst_rect.left, 1);
            }
        }
    }
    checkGLcall("glCopyTexSubImage2D");

    LEAVE_GL();
    context_release(context);

    /* The texture is now most up to date - If the surface is a render target and has a drawable, this
     * path is never entered
     */
    surface_modify_location(dst_surface, SFLAG_INTEXTURE, TRUE);
}

/* Uses the hardware to stretch and flip the image */
static void fb_copy_to_texture_hwstretch(struct wined3d_surface *dst_surface, struct wined3d_surface *src_surface,
        const RECT *src_rect, const RECT *dst_rect_in, WINED3DTEXTUREFILTERTYPE Filter)
{
    struct wined3d_device *device = dst_surface->resource.device;
    struct wined3d_swapchain *src_swapchain = NULL;
    GLuint src, backup = 0;
    float left, right, top, bottom; /* Texture coordinates */
    UINT fbwidth = src_surface->resource.width;
    UINT fbheight = src_surface->resource.height;
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
    context_apply_blit_state(context, device);
    surface_internal_preload(dst_surface, SRGB_RGB);

    src_offscreen = surface_is_offscreen(src_surface);
    noBackBufferBackup = src_offscreen && wined3d_settings.offscreen_rendering_mode == ORM_FBO;
    if (!noBackBufferBackup && !src_surface->texture_name)
    {
        /* Get it a description */
        surface_internal_preload(src_surface, SRGB_RGB);
    }
    ENTER_GL();

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

    if(noBackBufferBackup) {
        glGenTextures(1, &backup);
        checkGLcall("glGenTextures");
        glBindTexture(GL_TEXTURE_2D, backup);
        checkGLcall("glBindTexture(GL_TEXTURE_2D, backup)");
        texture_target = GL_TEXTURE_2D;
    } else {
        /* Backup the back buffer and copy the source buffer into a texture to draw an upside down stretched quad. If
         * we are reading from the back buffer, the backup can be used as source texture
         */
        texture_target = src_surface->texture_target;
        glBindTexture(texture_target, src_surface->texture_name);
        checkGLcall("glBindTexture(texture_target, src_surface->texture_name)");
        glEnable(texture_target);
        checkGLcall("glEnable(texture_target)");

        /* For now invalidate the texture copy of the back buffer. Drawable and sysmem copy are untouched */
        src_surface->flags &= ~SFLAG_INTEXTURE;
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
        glReadBuffer(device->offscreenBuffer);
    }
    else
    {
        glReadBuffer(surface_get_gl_buffer(src_surface));
    }

    /* TODO: Only back up the part that will be overwritten */
    glCopyTexSubImage2D(texture_target, 0,
                        0, 0 /* read offsets */,
                        0, 0,
                        fbwidth,
                        fbheight);

    checkGLcall("glCopyTexSubImage2D");

    /* No issue with overriding these - the sampler is dirty due to blit usage */
    glTexParameteri(texture_target, GL_TEXTURE_MAG_FILTER,
            wined3d_gl_mag_filter(magLookup, Filter));
    checkGLcall("glTexParameteri");
    glTexParameteri(texture_target, GL_TEXTURE_MIN_FILTER,
            wined3d_gl_min_mip_filter(minMipLookup, Filter, WINED3DTEXF_NONE));
    checkGLcall("glTexParameteri");

    if (src_surface->container.type == WINED3D_CONTAINER_SWAPCHAIN)
        src_swapchain = src_surface->container.u.swapchain;
    if (!src_swapchain || src_surface == src_swapchain->back_buffers[0])
    {
        src = backup ? backup : src_surface->texture_name;
    }
    else
    {
        glReadBuffer(GL_FRONT);
        checkGLcall("glReadBuffer(GL_FRONT)");

        glGenTextures(1, &src);
        checkGLcall("glGenTextures(1, &src)");
        glBindTexture(GL_TEXTURE_2D, src);
        checkGLcall("glBindTexture(GL_TEXTURE_2D, src)");

        /* TODO: Only copy the part that will be read. Use src_rect->left, src_rect->bottom as origin, but with the width watch
         * out for power of 2 sizes
         */
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, src_surface->pow2Width,
                src_surface->pow2Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        checkGLcall("glTexImage2D");
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0,
                            0, 0 /* read offsets */,
                            0, 0,
                            fbwidth,
                            fbheight);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        checkGLcall("glTexParameteri");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        checkGLcall("glTexParameteri");

        glReadBuffer(GL_BACK);
        checkGLcall("glReadBuffer(GL_BACK)");

        if(texture_target != GL_TEXTURE_2D) {
            glDisable(texture_target);
            glEnable(GL_TEXTURE_2D);
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

    if (src_surface->flags & SFLAG_NORMCOORD)
    {
        left /= src_surface->pow2Width;
        right /= src_surface->pow2Width;
        top /= src_surface->pow2Height;
        bottom /= src_surface->pow2Height;
    }

    /* draw the source texture stretched and upside down. The correct surface is bound already */
    glTexParameteri(texture_target, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(texture_target, GL_TEXTURE_WRAP_T, GL_CLAMP);

    context_set_draw_buffer(context, drawBuffer);
    glReadBuffer(drawBuffer);

    glBegin(GL_QUADS);
        /* bottom left */
        glTexCoord2f(left, bottom);
        glVertex2i(0, 0);

        /* top left */
        glTexCoord2f(left, top);
        glVertex2i(0, dst_rect.bottom - dst_rect.top);

        /* top right */
        glTexCoord2f(right, top);
        glVertex2i(dst_rect.right - dst_rect.left, dst_rect.bottom - dst_rect.top);

        /* bottom right */
        glTexCoord2f(right, bottom);
        glVertex2i(dst_rect.right - dst_rect.left, 0);
    glEnd();
    checkGLcall("glEnd and previous");

    if (texture_target != dst_surface->texture_target)
    {
        glDisable(texture_target);
        glEnable(dst_surface->texture_target);
        texture_target = dst_surface->texture_target;
    }

    /* Now read the stretched and upside down image into the destination texture */
    glBindTexture(texture_target, dst_surface->texture_name);
    checkGLcall("glBindTexture");
    glCopyTexSubImage2D(texture_target,
                        0,
                        dst_rect.left, dst_rect.top, /* xoffset, yoffset */
                        0, 0, /* We blitted the image to the origin */
                        dst_rect.right - dst_rect.left, dst_rect.bottom - dst_rect.top);
    checkGLcall("glCopyTexSubImage2D");

    if(drawBuffer == GL_BACK) {
        /* Write the back buffer backup back */
        if(backup) {
            if(texture_target != GL_TEXTURE_2D) {
                glDisable(texture_target);
                glEnable(GL_TEXTURE_2D);
                texture_target = GL_TEXTURE_2D;
            }
            glBindTexture(GL_TEXTURE_2D, backup);
            checkGLcall("glBindTexture(GL_TEXTURE_2D, backup)");
        }
        else
        {
            if (texture_target != src_surface->texture_target)
            {
                glDisable(texture_target);
                glEnable(src_surface->texture_target);
                texture_target = src_surface->texture_target;
            }
            glBindTexture(src_surface->texture_target, src_surface->texture_name);
            checkGLcall("glBindTexture(src_surface->texture_target, src_surface->texture_name)");
        }

        glBegin(GL_QUADS);
            /* top left */
            glTexCoord2f(0.0f, 0.0f);
            glVertex2i(0, fbheight);

            /* bottom left */
            glTexCoord2f(0.0f, (float)fbheight / (float)src_surface->pow2Height);
            glVertex2i(0, 0);

            /* bottom right */
            glTexCoord2f((float)fbwidth / (float)src_surface->pow2Width,
                    (float)fbheight / (float)src_surface->pow2Height);
            glVertex2i(fbwidth, 0);

            /* top right */
            glTexCoord2f((float)fbwidth / (float)src_surface->pow2Width, 0.0f);
            glVertex2i(fbwidth, fbheight);
        glEnd();
    }
    glDisable(texture_target);
    checkGLcall("glDisable(texture_target)");

    /* Cleanup */
    if (src != src_surface->texture_name && src != backup)
    {
        glDeleteTextures(1, &src);
        checkGLcall("glDeleteTextures(1, &src)");
    }
    if(backup) {
        glDeleteTextures(1, &backup);
        checkGLcall("glDeleteTextures(1, &backup)");
    }

    LEAVE_GL();

    if (wined3d_settings.strict_draw_ordering) wglFlush(); /* Flush to ensure ordering across contexts. */

    context_release(context);

    /* The texture is now most up to date - If the surface is a render target and has a drawable, this
     * path is never entered
     */
    surface_modify_location(dst_surface, SFLAG_INTEXTURE, TRUE);
}

/* Front buffer coordinates are always full screen coordinates, but our GL
 * drawable is limited to the window's client area. The sysmem and texture
 * copies do have the full screen size. Note that GL has a bottom-left
 * origin, while D3D has a top-left origin. */
void surface_translate_drawable_coords(struct wined3d_surface *surface, HWND window, RECT *rect)
{
    UINT drawable_height;

    if (surface->container.type == WINED3D_CONTAINER_SWAPCHAIN
            && surface == surface->container.u.swapchain->front_buffer)
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

/* blit between surface locations. onscreen on different swapchains is not supported.
 * depth / stencil is not supported. */
static void surface_blt_fbo(struct wined3d_device *device, const WINED3DTEXTUREFILTERTYPE filter,
        struct wined3d_surface *src_surface, DWORD src_location, const RECT *src_rect_in,
        struct wined3d_surface *dst_surface, DWORD dst_location, const RECT *dst_rect_in)
{
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;
    RECT src_rect, dst_rect;
    GLenum gl_filter;

    TRACE("device %p, filter %s,\n", device, debug_d3dtexturefiltertype(filter));
    TRACE("src_surface %p, src_location %s, src_rect %s,\n",
            src_surface, debug_surflocation(src_location), wine_dbgstr_rect(src_rect_in));
    TRACE("dst_surface %p, dst_location %s, dst_rect %s.\n",
            dst_surface, debug_surflocation(dst_location), wine_dbgstr_rect(dst_rect_in));

    src_rect = *src_rect_in;
    dst_rect = *dst_rect_in;

    switch (filter)
    {
        case WINED3DTEXF_LINEAR:
            gl_filter = GL_LINEAR;
            break;

        default:
            FIXME("Unsupported filter mode %s (%#x).\n", debug_d3dtexturefiltertype(filter), filter);
        case WINED3DTEXF_NONE:
        case WINED3DTEXF_POINT:
            gl_filter = GL_NEAREST;
            break;
    }

    if (src_location == SFLAG_INDRAWABLE && surface_is_offscreen(src_surface))
        src_location = SFLAG_INTEXTURE;
    if (dst_location == SFLAG_INDRAWABLE && surface_is_offscreen(dst_surface))
        dst_location = SFLAG_INTEXTURE;

    /* Make sure the locations are up-to-date. Loading the destination
     * surface isn't required if the entire surface is overwritten. (And is
     * in fact harmful if we're being called by surface_load_location() with
     * the purpose of loading the destination surface.) */
    surface_load_location(src_surface, src_location, NULL);
    if (!surface_is_full_rect(dst_surface, &dst_rect))
        surface_load_location(dst_surface, dst_location, NULL);

    if (src_location == SFLAG_INDRAWABLE) context = context_acquire(device, src_surface);
    else if (dst_location == SFLAG_INDRAWABLE) context = context_acquire(device, dst_surface);
    else context = context_acquire(device, NULL);

    if (!context->valid)
    {
        context_release(context);
        WARN("Invalid context, skipping blit.\n");
        return;
    }

    gl_info = context->gl_info;

    if (src_location == SFLAG_INDRAWABLE)
    {
        GLenum buffer = surface_get_gl_buffer(src_surface);

        TRACE("Source surface %p is onscreen.\n", src_surface);

        surface_translate_drawable_coords(src_surface, context->win_handle, &src_rect);

        ENTER_GL();
        context_bind_fbo(context, GL_READ_FRAMEBUFFER, NULL);
        glReadBuffer(buffer);
        checkGLcall("glReadBuffer()");
    }
    else
    {
        TRACE("Source surface %p is offscreen.\n", src_surface);
        ENTER_GL();
        context_apply_fbo_state_blit(context, GL_READ_FRAMEBUFFER, src_surface, NULL, src_location);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        checkGLcall("glReadBuffer()");
    }
    context_check_fbo_status(context, GL_READ_FRAMEBUFFER);
    LEAVE_GL();

    if (dst_location == SFLAG_INDRAWABLE)
    {
        GLenum buffer = surface_get_gl_buffer(dst_surface);

        TRACE("Destination surface %p is onscreen.\n", dst_surface);

        surface_translate_drawable_coords(dst_surface, context->win_handle, &dst_rect);

        ENTER_GL();
        context_bind_fbo(context, GL_DRAW_FRAMEBUFFER, NULL);
        context_set_draw_buffer(context, buffer);
    }
    else
    {
        TRACE("Destination surface %p is offscreen.\n", dst_surface);

        ENTER_GL();
        context_apply_fbo_state_blit(context, GL_DRAW_FRAMEBUFFER, dst_surface, NULL, dst_location);
        context_set_draw_buffer(context, GL_COLOR_ATTACHMENT0);
    }
    context_check_fbo_status(context, GL_DRAW_FRAMEBUFFER);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    IWineD3DDeviceImpl_MarkStateDirty(device, STATE_RENDER(WINED3DRS_COLORWRITEENABLE));
    IWineD3DDeviceImpl_MarkStateDirty(device, STATE_RENDER(WINED3DRS_COLORWRITEENABLE1));
    IWineD3DDeviceImpl_MarkStateDirty(device, STATE_RENDER(WINED3DRS_COLORWRITEENABLE2));
    IWineD3DDeviceImpl_MarkStateDirty(device, STATE_RENDER(WINED3DRS_COLORWRITEENABLE3));

    glDisable(GL_SCISSOR_TEST);
    IWineD3DDeviceImpl_MarkStateDirty(device, STATE_RENDER(WINED3DRS_SCISSORTESTENABLE));

    gl_info->fbo_ops.glBlitFramebuffer(src_rect.left, src_rect.top, src_rect.right, src_rect.bottom,
            dst_rect.left, dst_rect.top, dst_rect.right, dst_rect.bottom, GL_COLOR_BUFFER_BIT, gl_filter);
    checkGLcall("glBlitFramebuffer()");

    LEAVE_GL();

    if (wined3d_settings.strict_draw_ordering
            || (dst_location == SFLAG_INDRAWABLE
            && dst_surface->container.u.swapchain->front_buffer == dst_surface))
        wglFlush();

    context_release(context);
}

static void surface_blt_to_drawable(struct wined3d_device *device,
        WINED3DTEXTUREFILTERTYPE filter, BOOL color_key,
        struct wined3d_surface *src_surface, const RECT *src_rect_in,
        struct wined3d_surface *dst_surface, const RECT *dst_rect_in)
{
    struct wined3d_context *context;
    RECT src_rect, dst_rect;

    src_rect = *src_rect_in;
    dst_rect = *dst_rect_in;

    /* Make sure the surface is up-to-date. This should probably use
     * surface_load_location() and worry about the destination surface too,
     * unless we're overwriting it completely. */
    surface_internal_preload(src_surface, SRGB_RGB);

    /* Activate the destination context, set it up for blitting */
    context = context_acquire(device, dst_surface);
    context_apply_blit_state(context, device);

    if (!surface_is_offscreen(dst_surface))
        surface_translate_drawable_coords(dst_surface, context->win_handle, &dst_rect);

    device->blitter->set_shader(device->blit_priv, context->gl_info, src_surface);

    ENTER_GL();

    if (color_key)
    {
        glEnable(GL_ALPHA_TEST);
        checkGLcall("glEnable(GL_ALPHA_TEST)");

        /* When the primary render target uses P8, the alpha component
         * contains the palette index. Which means that the colorkey is one of
         * the palette entries. In other cases pixels that should be masked
         * away have alpha set to 0. */
        if (primary_render_target_is_p8(device))
            glAlphaFunc(GL_NOTEQUAL, (float)src_surface->SrcBltCKey.dwColorSpaceLowValue / 256.0f);
        else
            glAlphaFunc(GL_NOTEQUAL, 0.0f);
        checkGLcall("glAlphaFunc");
    }
    else
    {
        glDisable(GL_ALPHA_TEST);
        checkGLcall("glDisable(GL_ALPHA_TEST)");
    }

    draw_textured_quad(src_surface, &src_rect, &dst_rect, filter);

    if (color_key)
    {
        glDisable(GL_ALPHA_TEST);
        checkGLcall("glDisable(GL_ALPHA_TEST)");
    }

    LEAVE_GL();

    /* Leave the opengl state valid for blitting */
    device->blitter->unset_shader(context->gl_info);

    if (wined3d_settings.strict_draw_ordering
            || (dst_surface->container.type == WINED3D_CONTAINER_SWAPCHAIN
            && (dst_surface->container.u.swapchain->front_buffer == dst_surface)))
        wglFlush(); /* Flush to ensure ordering across contexts. */

    context_release(context);
}

/* Do not call while under the GL lock. */
HRESULT surface_color_fill(struct wined3d_surface *s, const RECT *rect, const WINED3DCOLORVALUE *color)
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

/* Do not call while under the GL lock. */
static HRESULT IWineD3DSurfaceImpl_BltOverride(struct wined3d_surface *dst_surface, const RECT *DestRect,
        struct wined3d_surface *src_surface, const RECT *SrcRect, DWORD flags, const WINEDDBLTFX *DDBltFx,
        WINED3DTEXTUREFILTERTYPE Filter)
{
    struct wined3d_device *device = dst_surface->resource.device;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_swapchain *srcSwapchain = NULL, *dstSwapchain = NULL;
    RECT dst_rect, src_rect;

    TRACE("dst_surface %p, dst_rect %s, src_surface %p, src_rect %s, flags %#x, blt_fx %p, filter %s.\n",
            dst_surface, wine_dbgstr_rect(DestRect), src_surface, wine_dbgstr_rect(SrcRect),
            flags, DDBltFx, debug_d3dtexturefiltertype(Filter));

    /* Get the swapchain. One of the surfaces has to be a primary surface */
    if (dst_surface->resource.pool == WINED3DPOOL_SYSTEMMEM)
    {
        WARN("Destination is in sysmem, rejecting gl blt\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (dst_surface->container.type == WINED3D_CONTAINER_SWAPCHAIN)
        dstSwapchain = dst_surface->container.u.swapchain;

    if (src_surface)
    {
        if (src_surface->resource.pool == WINED3DPOOL_SYSTEMMEM)
        {
            WARN("Src is in sysmem, rejecting gl blt\n");
            return WINED3DERR_INVALIDCALL;
        }

        if (src_surface->container.type == WINED3D_CONTAINER_SWAPCHAIN)
            srcSwapchain = src_surface->container.u.swapchain;
    }

    /* Early sort out of cases where no render target is used */
    if (!dstSwapchain && !srcSwapchain
            && src_surface != device->fb.render_targets[0]
            && dst_surface != device->fb.render_targets[0])
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

    surface_get_rect(dst_surface, DestRect, &dst_rect);
    if (src_surface) surface_get_rect(src_surface, SrcRect, &src_rect);

    /* The only case where both surfaces on a swapchain are supported is a back buffer -> front buffer blit on the same swapchain */
    if (dstSwapchain && dstSwapchain == srcSwapchain && dstSwapchain->back_buffers
            && dst_surface == dstSwapchain->front_buffer
            && src_surface == dstSwapchain->back_buffers[0])
    {
        /* Half-Life does a Blt from the back buffer to the front buffer,
         * Full surface size, no flags... Use present instead
         *
         * This path will only be entered for d3d7 and ddraw apps, because d3d8/9 offer no way to blit TO the front buffer
         */

        /* Check rects - IWineD3DDevice_Present doesn't handle them */
        while(1)
        {
            TRACE("Looking if a Present can be done...\n");
            /* Source Rectangle must be full surface */
            if (src_rect.left || src_rect.top
                    || src_rect.right != src_surface->resource.width
                    || src_rect.bottom != src_surface->resource.height)
            {
                TRACE("No, Source rectangle doesn't match\n");
                break;
            }

            /* No stretching may occur */
            if(src_rect.right != dst_rect.right - dst_rect.left ||
               src_rect.bottom != dst_rect.bottom - dst_rect.top) {
                TRACE("No, stretching is done\n");
                break;
            }

            /* Destination must be full surface or match the clipping rectangle */
            if (dst_surface->clipper && dst_surface->clipper->hWnd)
            {
                RECT cliprect;
                POINT pos[2];
                GetClientRect(dst_surface->clipper->hWnd, &cliprect);
                pos[0].x = dst_rect.left;
                pos[0].y = dst_rect.top;
                pos[1].x = dst_rect.right;
                pos[1].y = dst_rect.bottom;
                MapWindowPoints(GetDesktopWindow(), dst_surface->clipper->hWnd, pos, 2);

                if(pos[0].x != cliprect.left  || pos[0].y != cliprect.top   ||
                   pos[1].x != cliprect.right || pos[1].y != cliprect.bottom)
                {
                    TRACE("No, dest rectangle doesn't match(clipper)\n");
                    TRACE("Clip rect at %s\n", wine_dbgstr_rect(&cliprect));
                    TRACE("Blt dest: %s\n", wine_dbgstr_rect(&dst_rect));
                    break;
                }
            }
            else if (dst_rect.left || dst_rect.top
                    || dst_rect.right != dst_surface->resource.width
                    || dst_rect.bottom != dst_surface->resource.height)
            {
                TRACE("No, dest rectangle doesn't match(surface size)\n");
                break;
            }

            TRACE("Yes\n");

            /* These flags are unimportant for the flag check, remove them */
            if (!(flags & ~(WINEDDBLT_DONOTWAIT | WINEDDBLT_WAIT)))
            {
                WINED3DSWAPEFFECT orig_swap = dstSwapchain->presentParms.SwapEffect;

                /* The idea behind this is that a glReadPixels and a glDrawPixels call
                    * take very long, while a flip is fast.
                    * This applies to Half-Life, which does such Blts every time it finished
                    * a frame, and to Prince of Persia 3D, which uses this to draw at least the main
                    * menu. This is also used by all apps when they do windowed rendering
                    *
                    * The problem is that flipping is not really the same as copying. After a
                    * Blt the front buffer is a copy of the back buffer, and the back buffer is
                    * untouched. Therefore it's necessary to override the swap effect
                    * and to set it back after the flip.
                    *
                    * Windowed Direct3D < 7 apps do the same. The D3D7 sdk demos are nice
                    * testcases.
                    */

                dstSwapchain->presentParms.SwapEffect = WINED3DSWAPEFFECT_COPY;
                dstSwapchain->presentParms.PresentationInterval = WINED3DPRESENT_INTERVAL_IMMEDIATE;

                TRACE("Full screen back buffer -> front buffer blt, performing a flip instead.\n");
                wined3d_swapchain_present(dstSwapchain, NULL, NULL, dstSwapchain->win_handle, NULL, 0);

                dstSwapchain->presentParms.SwapEffect = orig_swap;

                return WINED3D_OK;
            }
            break;
        }

        TRACE("Unsupported blit between buffers on the same swapchain\n");
        return WINED3DERR_INVALIDCALL;
    } else if(dstSwapchain && dstSwapchain == srcSwapchain) {
        FIXME("Implement hardware blit between two surfaces on the same swapchain\n");
        return WINED3DERR_INVALIDCALL;
    } else if(dstSwapchain && srcSwapchain) {
        FIXME("Implement hardware blit between two different swapchains\n");
        return WINED3DERR_INVALIDCALL;
    }
    else if (dstSwapchain)
    {
        /* Handled with regular texture -> swapchain blit */
        if (src_surface == device->fb.render_targets[0])
            TRACE("Blit from active render target to a swapchain\n");
    }
    else if (srcSwapchain && dst_surface == device->fb.render_targets[0])
    {
        FIXME("Implement blit from a swapchain to the active render target\n");
        return WINED3DERR_INVALIDCALL;
    }

    if ((srcSwapchain || src_surface == device->fb.render_targets[0]) && !dstSwapchain)
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

        if(dst_rect.right - dst_rect.left != src_rect.right - src_rect.left) {
            stretchx = TRUE;
        } else {
            stretchx = FALSE;
        }

        /* Blt is a pretty powerful call, while glCopyTexSubImage2D is not. glCopyTexSubImage cannot
         * flip the image nor scale it.
         *
         * -> If the app asks for a unscaled, upside down copy, just perform one glCopyTexSubImage2D call
         * -> If the app wants a image width an unscaled width, copy it line per line
         * -> If the app wants a image that is scaled on the x axis, and the destination rectangle is smaller
         *    than the frame buffer, draw an upside down scaled image onto the fb, read it back and restore the
         *    back buffer. This is slower than reading line per line, thus not used for flipping
         * -> If the app wants a scaled image with a dest rect that is bigger than the fb, it has to be copied
         *    pixel by pixel
         *
         * If EXT_framebuffer_blit is supported that can be used instead. Note that EXT_framebuffer_blit implies
         * FBO support, so it doesn't really make sense to try and make it work with different offscreen rendering
         * backends. */
        if (fbo_blit_supported(gl_info, WINED3D_BLIT_OP_COLOR_BLIT,
                &src_rect, src_surface->resource.usage, src_surface->resource.pool, src_surface->resource.format,
                &dst_rect, dst_surface->resource.usage, dst_surface->resource.pool, dst_surface->resource.format))
        {
            surface_blt_fbo(device, Filter,
                    src_surface, SFLAG_INDRAWABLE, &src_rect,
                    dst_surface, SFLAG_INDRAWABLE, &dst_rect);
            surface_modify_location(dst_surface, SFLAG_INDRAWABLE, TRUE);
        }
        else if (!stretchx || dst_rect.right - dst_rect.left > src_surface->resource.width
                || dst_rect.bottom - dst_rect.top > src_surface->resource.height)
        {
            TRACE("No stretching in x direction, using direct framebuffer -> texture copy\n");
            fb_copy_to_texture_direct(dst_surface, src_surface, &src_rect, &dst_rect, Filter);
        } else {
            TRACE("Using hardware stretching to flip / stretch the texture\n");
            fb_copy_to_texture_hwstretch(dst_surface, src_surface, &src_rect, &dst_rect, Filter);
        }

        if (!(dst_surface->flags & SFLAG_DONOTFREE))
        {
            HeapFree(GetProcessHeap(), 0, dst_surface->resource.heapMemory);
            dst_surface->resource.allocatedMemory = NULL;
            dst_surface->resource.heapMemory = NULL;
        }
        else
        {
            dst_surface->flags &= ~SFLAG_INSYSMEM;
        }

        return WINED3D_OK;
    }
    else if (src_surface)
    {
        /* Blit from offscreen surface to render target */
        DWORD oldCKeyFlags = src_surface->CKeyFlags;
        WINEDDCOLORKEY oldBltCKey = src_surface->SrcBltCKey;

        TRACE("Blt from surface %p to rendertarget %p\n", src_surface, dst_surface);

        if (!(flags & (WINEDDBLT_KEYSRC | WINEDDBLT_KEYSRCOVERRIDE))
                && fbo_blit_supported(gl_info, WINED3D_BLIT_OP_COLOR_BLIT,
                        &src_rect, src_surface->resource.usage, src_surface->resource.pool,
                        src_surface->resource.format,
                        &dst_rect, dst_surface->resource.usage, dst_surface->resource.pool,
                        dst_surface->resource.format))
        {
            TRACE("Using surface_blt_fbo.\n");
            /* The source is always a texture, but never the currently active render target, and the texture
             * contents are never upside down. */
            surface_blt_fbo(device, Filter,
                    src_surface, SFLAG_INDRAWABLE, &src_rect,
                    dst_surface, SFLAG_INDRAWABLE, &dst_rect);
            surface_modify_location(dst_surface, SFLAG_INDRAWABLE, TRUE);
            return WINED3D_OK;
        }

        if (!(flags & (WINEDDBLT_KEYSRC | WINEDDBLT_KEYSRCOVERRIDE))
                && arbfp_blit.blit_supported(gl_info, WINED3D_BLIT_OP_COLOR_BLIT,
                        &src_rect, src_surface->resource.usage, src_surface->resource.pool,
                        src_surface->resource.format,
                        &dst_rect, dst_surface->resource.usage, dst_surface->resource.pool,
                        dst_surface->resource.format))
        {
            return arbfp_blit_surface(device, src_surface, &src_rect, dst_surface, &dst_rect,
                    WINED3D_BLIT_OP_COLOR_BLIT, Filter);
        }

        if (!device->blitter->blit_supported(gl_info, WINED3D_BLIT_OP_COLOR_BLIT,
                &src_rect, src_surface->resource.usage, src_surface->resource.pool, src_surface->resource.format,
                &dst_rect, dst_surface->resource.usage, dst_surface->resource.pool, dst_surface->resource.format))
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
            src_surface->CKeyFlags |= WINEDDSD_CKSRCBLT;
            src_surface->SrcBltCKey = DDBltFx->ddckSrcColorkey;
        }
        else
        {
            /* Do not use color key */
            src_surface->CKeyFlags &= ~WINEDDSD_CKSRCBLT;
        }

        surface_blt_to_drawable(device, Filter, flags & (WINEDDBLT_KEYSRC | WINEDDBLT_KEYSRCOVERRIDE),
                src_surface, &src_rect, dst_surface, &dst_rect);

        /* Restore the color key parameters */
        src_surface->CKeyFlags = oldCKeyFlags;
        src_surface->SrcBltCKey = oldBltCKey;

        surface_modify_location(dst_surface, SFLAG_INDRAWABLE, TRUE);

        return WINED3D_OK;
    }
    else
    {
        /* Source-Less Blit to render target */
        if (flags & WINEDDBLT_COLORFILL)
        {
            WINED3DCOLORVALUE color;

            TRACE("Colorfill\n");

            /* The color as given in the Blt function is in the surface format. */
            if (!surface_convert_color_to_float(dst_surface, DDBltFx->u5.dwFillColor, &color))
                return WINED3DERR_INVALIDCALL;

            return surface_color_fill(dst_surface, &dst_rect, &color);
        }
    }

    /* Default: Fall back to the generic blt. Not an error, a TRACE is enough */
    TRACE("Didn't find any usable render target setup for hw blit, falling back to software\n");
    return WINED3DERR_INVALIDCALL;
}

/* GL locking is done by the caller */
static void surface_depth_blt(struct wined3d_surface *surface, const struct wined3d_gl_info *gl_info,
        GLuint texture, GLsizei w, GLsizei h, GLenum target)
{
    struct wined3d_device *device = surface->resource.device;
    GLint compare_mode = GL_NONE;
    struct blt_info info;
    GLint old_binding = 0;
    RECT rect;

    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT);

    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_TRUE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glViewport(0, surface->pow2Height - h, w, h);

    SetRect(&rect, 0, h, w, 0);
    surface_get_blt_info(target, &rect, surface->pow2Width, surface->pow2Height, &info);
    GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB));
    glGetIntegerv(info.binding, &old_binding);
    glBindTexture(info.bind_target, texture);
    if (gl_info->supported[ARB_SHADOW])
    {
        glGetTexParameteriv(info.bind_target, GL_TEXTURE_COMPARE_MODE_ARB, &compare_mode);
        if (compare_mode != GL_NONE) glTexParameteri(info.bind_target, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
    }

    device->shader_backend->shader_select_depth_blt(device->shader_priv,
            gl_info, info.tex_type, &surface->ds_current_size);

    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord3fv(info.coords[0]);
    glVertex2f(-1.0f, -1.0f);
    glTexCoord3fv(info.coords[1]);
    glVertex2f(1.0f, -1.0f);
    glTexCoord3fv(info.coords[2]);
    glVertex2f(-1.0f, 1.0f);
    glTexCoord3fv(info.coords[3]);
    glVertex2f(1.0f, 1.0f);
    glEnd();

    if (compare_mode != GL_NONE) glTexParameteri(info.bind_target, GL_TEXTURE_COMPARE_MODE_ARB, compare_mode);
    glBindTexture(info.bind_target, old_binding);

    glPopAttrib();

    device->shader_backend->shader_deselect_depth_blt(device->shader_priv, gl_info);
}

void surface_modify_ds_location(struct wined3d_surface *surface,
        DWORD location, UINT w, UINT h)
{
    TRACE("surface %p, new location %#x, w %u, h %u.\n", surface, location, w, h);

    if (location & ~SFLAG_DS_LOCATIONS)
        FIXME("Invalid location (%#x) specified.\n", location);

    surface->ds_current_size.cx = w;
    surface->ds_current_size.cy = h;
    surface->flags &= ~SFLAG_DS_LOCATIONS;
    surface->flags |= location;
}

/* Context activation is done by the caller. */
void surface_load_ds_location(struct wined3d_surface *surface, struct wined3d_context *context, DWORD location)
{
    struct wined3d_device *device = surface->resource.device;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    GLsizei w, h;

    TRACE("surface %p, new location %#x.\n", surface, location);

    /* TODO: Make this work for modes other than FBO */
    if (wined3d_settings.offscreen_rendering_mode != ORM_FBO) return;

    if (!(surface->flags & location))
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

    if (!(surface->flags & SFLAG_DS_LOCATIONS))
    {
        /* This mostly happens when a depth / stencil is used without being
         * cleared first. In principle we could upload from sysmem, or
         * explicitly clear before first usage. For the moment there don't
         * appear to be a lot of applications depending on this, so a FIXME
         * should do. */
        FIXME("No up to date depth stencil location.\n");
        surface->flags |= location;
        surface->ds_current_size.cx = surface->resource.width;
        surface->ds_current_size.cy = surface->resource.height;
        return;
    }

    if (location == SFLAG_DS_OFFSCREEN)
    {
        GLint old_binding = 0;
        GLenum bind_target;

        /* The render target is allowed to be smaller than the depth/stencil
         * buffer, so the onscreen depth/stencil buffer is potentially smaller
         * than the offscreen surface. Don't overwrite the offscreen surface
         * with undefined data. */
        w = min(w, context->swapchain->presentParms.BackBufferWidth);
        h = min(h, context->swapchain->presentParms.BackBufferHeight);

        TRACE("Copying onscreen depth buffer to depth texture.\n");

        ENTER_GL();

        if (!device->depth_blt_texture)
        {
            glGenTextures(1, &device->depth_blt_texture);
        }

        /* Note that we use depth_blt here as well, rather than glCopyTexImage2D
         * directly on the FBO texture. That's because we need to flip. */
        context_bind_fbo(context, GL_FRAMEBUFFER, NULL);
        if (surface->texture_target == GL_TEXTURE_RECTANGLE_ARB)
        {
            glGetIntegerv(GL_TEXTURE_BINDING_RECTANGLE_ARB, &old_binding);
            bind_target = GL_TEXTURE_RECTANGLE_ARB;
        }
        else
        {
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &old_binding);
            bind_target = GL_TEXTURE_2D;
        }
        glBindTexture(bind_target, device->depth_blt_texture);
        glCopyTexImage2D(bind_target, surface->texture_level, surface->resource.format->glInternal, 0, 0, w, h, 0);
        glTexParameteri(bind_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(bind_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(bind_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(bind_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(bind_target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(bind_target, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
        glBindTexture(bind_target, old_binding);

        /* Setup the destination */
        if (!device->depth_blt_rb)
        {
            gl_info->fbo_ops.glGenRenderbuffers(1, &device->depth_blt_rb);
            checkGLcall("glGenRenderbuffersEXT");
        }
        if (device->depth_blt_rb_w != w || device->depth_blt_rb_h != h)
        {
            gl_info->fbo_ops.glBindRenderbuffer(GL_RENDERBUFFER, device->depth_blt_rb);
            checkGLcall("glBindRenderbufferEXT");
            gl_info->fbo_ops.glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, w, h);
            checkGLcall("glRenderbufferStorageEXT");
            device->depth_blt_rb_w = w;
            device->depth_blt_rb_h = h;
        }

        context_bind_fbo(context, GL_FRAMEBUFFER, &context->dst_fbo);
        gl_info->fbo_ops.glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, device->depth_blt_rb);
        checkGLcall("glFramebufferRenderbufferEXT");
        context_attach_depth_stencil_fbo(context, GL_FRAMEBUFFER, surface, FALSE);

        /* Do the actual blit */
        surface_depth_blt(surface, gl_info, device->depth_blt_texture, w, h, bind_target);
        checkGLcall("depth_blt");

        if (context->current_fbo) context_bind_fbo(context, GL_FRAMEBUFFER, &context->current_fbo->id);
        else context_bind_fbo(context, GL_FRAMEBUFFER, NULL);

        LEAVE_GL();

        if (wined3d_settings.strict_draw_ordering) wglFlush(); /* Flush to ensure ordering across contexts. */
    }
    else if (location == SFLAG_DS_ONSCREEN)
    {
        TRACE("Copying depth texture to onscreen depth buffer.\n");

        ENTER_GL();

        context_bind_fbo(context, GL_FRAMEBUFFER, NULL);
        surface_depth_blt(surface, gl_info, surface->texture_name,
                w, h, surface->texture_target);
        checkGLcall("depth_blt");

        if (context->current_fbo) context_bind_fbo(context, GL_FRAMEBUFFER, &context->current_fbo->id);

        LEAVE_GL();

        if (wined3d_settings.strict_draw_ordering) wglFlush(); /* Flush to ensure ordering across contexts. */
    }
    else
    {
        ERR("Invalid location (%#x) specified.\n", location);
    }

    surface->flags |= location;
    surface->ds_current_size.cx = surface->resource.width;
    surface->ds_current_size.cy = surface->resource.height;
}

void surface_modify_location(struct wined3d_surface *surface, DWORD flag, BOOL persistent)
{
    const struct wined3d_gl_info *gl_info = &surface->resource.device->adapter->gl_info;
    struct wined3d_surface *overlay;

    TRACE("surface %p, location %s, persistent %#x.\n",
            surface, debug_surflocation(flag), persistent);

    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
    {
        if (surface_is_offscreen(surface))
        {
            /* With ORM_FBO, SFLAG_INTEXTURE and SFLAG_INDRAWABLE are the same for offscreen targets. */
            if (flag & (SFLAG_INTEXTURE | SFLAG_INDRAWABLE)) flag |= (SFLAG_INTEXTURE | SFLAG_INDRAWABLE);
        }
        else
        {
            TRACE("Surface %p is an onscreen surface.\n", surface);
        }
    }

    if (flag & (SFLAG_INTEXTURE | SFLAG_INSRGBTEX)
            && gl_info->supported[EXT_TEXTURE_SRGB_DECODE])
    {
        flag |= (SFLAG_INTEXTURE | SFLAG_INSRGBTEX);
    }

    if (persistent)
    {
        if (((surface->flags & SFLAG_INTEXTURE) && !(flag & SFLAG_INTEXTURE))
                || ((surface->flags & SFLAG_INSRGBTEX) && !(flag & SFLAG_INSRGBTEX)))
        {
            if (surface->container.type == WINED3D_CONTAINER_TEXTURE)
            {
                TRACE("Passing to container.\n");
                wined3d_texture_set_dirty(surface->container.u.texture, TRUE);
            }
        }
        surface->flags &= ~SFLAG_LOCATIONS;
        surface->flags |= flag;

        /* Redraw emulated overlays, if any */
        if (flag & SFLAG_INDRAWABLE && !list_empty(&surface->overlays))
        {
            LIST_FOR_EACH_ENTRY(overlay, &surface->overlays, struct wined3d_surface, overlay_entry)
            {
                overlay->surface_ops->surface_draw_overlay(overlay);
            }
        }
    }
    else
    {
        if ((surface->flags & (SFLAG_INTEXTURE | SFLAG_INSRGBTEX)) && (flag & (SFLAG_INTEXTURE | SFLAG_INSRGBTEX)))
        {
            if (surface->container.type == WINED3D_CONTAINER_TEXTURE)
            {
                TRACE("Passing to container\n");
                wined3d_texture_set_dirty(surface->container.u.texture, TRUE);
            }
        }
        surface->flags &= ~flag;
    }

    if (!(surface->flags & SFLAG_LOCATIONS))
    {
        ERR("Surface %p does not have any up to date location.\n", surface);
    }
}

static DWORD resource_access_from_location(DWORD location)
{
    switch (location)
    {
        case SFLAG_INSYSMEM:
            return WINED3D_RESOURCE_ACCESS_CPU;

        case SFLAG_INDRAWABLE:
        case SFLAG_INSRGBTEX:
        case SFLAG_INTEXTURE:
            return WINED3D_RESOURCE_ACCESS_GPU;

        default:
            FIXME("Unhandled location %#x.\n", location);
            return 0;
    }
}

HRESULT surface_load_location(struct wined3d_surface *surface, DWORD flag, const RECT *rect)
{
    struct wined3d_device *device = surface->resource.device;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    BOOL drawable_read_ok = surface_is_offscreen(surface);
    struct wined3d_format format;
    CONVERT_TYPES convert;
    int width, pitch, outpitch;
    BYTE *mem;
    BOOL in_fbo = FALSE;

    TRACE("surface %p, location %s, rect %s.\n", surface, debug_surflocation(flag), wine_dbgstr_rect(rect));

    if (surface->resource.usage & WINED3DUSAGE_DEPTHSTENCIL)
    {
        if (flag == SFLAG_INTEXTURE)
        {
            struct wined3d_context *context = context_acquire(device, NULL);
            surface_load_ds_location(surface, context, SFLAG_DS_OFFSCREEN);
            context_release(context);
            return WINED3D_OK;
        }
        else
        {
            FIXME("Unimplemented location %s for depth/stencil buffers.\n", debug_surflocation(flag));
            return WINED3DERR_INVALIDCALL;
        }
    }

    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
    {
        if (surface_is_offscreen(surface))
        {
            /* With ORM_FBO, SFLAG_INTEXTURE and SFLAG_INDRAWABLE are the same for offscreen targets.
             * Prefer SFLAG_INTEXTURE. */
            if (flag == SFLAG_INDRAWABLE) flag = SFLAG_INTEXTURE;
            drawable_read_ok = FALSE;
            in_fbo = TRUE;
        }
        else
        {
            TRACE("Surface %p is an onscreen surface.\n", surface);
        }
    }

    if (flag == SFLAG_INSRGBTEX && gl_info->supported[EXT_TEXTURE_SRGB_DECODE])
    {
        flag = SFLAG_INTEXTURE;
    }

    if (surface->flags & flag)
    {
        TRACE("Location already up to date\n");
        return WINED3D_OK;
    }

    if (WARN_ON(d3d_surface))
    {
        DWORD required_access = resource_access_from_location(flag);
        if ((surface->resource.access_flags & required_access) != required_access)
            WARN("Operation requires %#x access, but surface only has %#x.\n",
                    required_access, surface->resource.access_flags);
    }

    if (!(surface->flags & SFLAG_LOCATIONS))
    {
        ERR("Surface %p does not have any up to date location.\n", surface);
        surface->flags |= SFLAG_LOST;
        return WINED3DERR_DEVICELOST;
    }

    if (flag == SFLAG_INSYSMEM)
    {
        surface_prepare_system_memory(surface);

        /* Download the surface to system memory */
        if (surface->flags & (SFLAG_INTEXTURE | SFLAG_INSRGBTEX))
        {
            struct wined3d_context *context = NULL;

            if (!device->isInDraw) context = context_acquire(device, NULL);

            surface_bind_and_dirtify(surface, gl_info, !(surface->flags & SFLAG_INTEXTURE));
            surface_download_data(surface, gl_info);

            if (context) context_release(context);
        }
        else
        {
            /* Note: It might be faster to download into a texture first. */
            read_from_framebuffer(surface, rect, surface->resource.allocatedMemory,
                    wined3d_surface_get_pitch(surface));
        }
    }
    else if (flag == SFLAG_INDRAWABLE)
    {
        if (wined3d_settings.rendertargetlock_mode == RTL_READTEX)
            surface_load_location(surface, SFLAG_INTEXTURE, NULL);

        if (surface->flags & SFLAG_INTEXTURE)
        {
            RECT r;

            surface_get_rect(surface, rect, &r);
            surface_blt_to_drawable(device, WINED3DTEXF_POINT, FALSE, surface, &r, surface, &r);
        }
        else
        {
            int byte_count;
            if ((surface->flags & SFLAG_LOCATIONS) == SFLAG_INSRGBTEX)
            {
                /* This needs a shader to convert the srgb data sampled from the GL texture into RGB
                 * values, otherwise we get incorrect values in the target. For now go the slow way
                 * via a system memory copy
                 */
                surface_load_location(surface, SFLAG_INSYSMEM, rect);
            }

            d3dfmt_get_conv(surface, FALSE /* We need color keying */,
                    FALSE /* We won't use textures */, &format, &convert);

            /* The width is in 'length' not in bytes */
            width = surface->resource.width;
            pitch = wined3d_surface_get_pitch(surface);

            /* Don't use PBOs for converted surfaces. During PBO conversion we look at SFLAG_CONVERTED
             * but it isn't set (yet) in all cases it is getting called. */
            if ((convert != NO_CONVERSION) && (surface->flags & SFLAG_PBO))
            {
                struct wined3d_context *context = NULL;

                TRACE("Removing the pbo attached to surface %p.\n", surface);

                if (!device->isInDraw) context = context_acquire(device, NULL);
                surface_remove_pbo(surface, gl_info);
                if (context) context_release(context);
            }

            if ((convert != NO_CONVERSION) && surface->resource.allocatedMemory)
            {
                int height = surface->resource.height;
                byte_count = format.conv_byte_count;

                /* Stick to the alignment for the converted surface too, makes it easier to load the surface */
                outpitch = width * byte_count;
                outpitch = (outpitch + device->surface_alignment - 1) & ~(device->surface_alignment - 1);

                mem = HeapAlloc(GetProcessHeap(), 0, outpitch * height);
                if(!mem) {
                    ERR("Out of memory %d, %d!\n", outpitch, height);
                    return WINED3DERR_OUTOFVIDEOMEMORY;
                }
                d3dfmt_convert_surface(surface->resource.allocatedMemory, mem, pitch,
                        width, height, outpitch, convert, surface);

                surface->flags |= SFLAG_CONVERTED;
            }
            else
            {
                surface->flags &= ~SFLAG_CONVERTED;
                mem = surface->resource.allocatedMemory;
                byte_count = format.byte_count;
            }

            flush_to_framebuffer_drawpixels(surface, rect, format.glFormat, format.glType, byte_count, mem);

            /* Don't delete PBO memory */
            if ((mem != surface->resource.allocatedMemory) && !(surface->flags & SFLAG_PBO))
                HeapFree(GetProcessHeap(), 0, mem);
        }
    }
    else /* if(flag & (SFLAG_INTEXTURE | SFLAG_INSRGBTEX)) */
    {
        const DWORD attach_flags = WINED3DFMT_FLAG_FBO_ATTACHABLE | WINED3DFMT_FLAG_FBO_ATTACHABLE_SRGB;

        if (drawable_read_ok && (surface->flags & SFLAG_INDRAWABLE))
        {
            read_from_framebuffer_texture(surface, flag == SFLAG_INSRGBTEX);
        }
        else if (surface->flags & (SFLAG_INSRGBTEX | SFLAG_INTEXTURE)
                && (surface->resource.format->flags & attach_flags) == attach_flags
                && fbo_blit_supported(gl_info, WINED3D_BLIT_OP_COLOR_BLIT,
                        NULL, surface->resource.usage, surface->resource.pool, surface->resource.format,
                        NULL, surface->resource.usage, surface->resource.pool, surface->resource.format))
        {
            DWORD src_location = flag == SFLAG_INSRGBTEX ? SFLAG_INTEXTURE : SFLAG_INSRGBTEX;
            RECT rect = {0, 0, surface->resource.width, surface->resource.height};

            surface_blt_fbo(surface->resource.device, WINED3DTEXF_POINT,
                    surface, src_location, &rect, surface, flag, &rect);
        }
        else
        {
            /* Upload from system memory */
            BOOL srgb = flag == SFLAG_INSRGBTEX;
            struct wined3d_context *context = NULL;

            d3dfmt_get_conv(surface, TRUE /* We need color keying */,
                    TRUE /* We will use textures */, &format, &convert);

            if (srgb)
            {
                if ((surface->flags & (SFLAG_INTEXTURE | SFLAG_INSYSMEM)) == SFLAG_INTEXTURE)
                {
                    /* Performance warning... */
                    FIXME("Downloading RGB surface %p to reload it as sRGB.\n", surface);
                    surface_load_location(surface, SFLAG_INSYSMEM, rect);
                }
            }
            else
            {
                if ((surface->flags & (SFLAG_INSRGBTEX | SFLAG_INSYSMEM)) == SFLAG_INSRGBTEX)
                {
                    /* Performance warning... */
                    FIXME("Downloading sRGB surface %p to reload it as RGB.\n", surface);
                    surface_load_location(surface, SFLAG_INSYSMEM, rect);
                }
            }
            if (!(surface->flags & SFLAG_INSYSMEM))
            {
                WARN("Trying to load a texture from sysmem, but SFLAG_INSYSMEM is not set.\n");
                /* Lets hope we get it from somewhere... */
                surface_load_location(surface, SFLAG_INSYSMEM, rect);
            }

            if (!device->isInDraw) context = context_acquire(device, NULL);

            surface_prepare_texture(surface, gl_info, srgb);
            surface_bind_and_dirtify(surface, gl_info, srgb);

            if (surface->CKeyFlags & WINEDDSD_CKSRCBLT)
            {
                surface->flags |= SFLAG_GLCKEY;
                surface->glCKey = surface->SrcBltCKey;
            }
            else surface->flags &= ~SFLAG_GLCKEY;

            /* The width is in 'length' not in bytes */
            width = surface->resource.width;
            pitch = wined3d_surface_get_pitch(surface);

            /* Don't use PBOs for converted surfaces. During PBO conversion we look at SFLAG_CONVERTED
             * but it isn't set (yet) in all cases it is getting called. */
            if ((convert != NO_CONVERSION || format.convert) && (surface->flags & SFLAG_PBO))
            {
                TRACE("Removing the pbo attached to surface %p.\n", surface);
                surface_remove_pbo(surface, gl_info);
            }

            if (format.convert)
            {
                /* This code is entered for texture formats which need a fixup. */
                UINT height = surface->resource.height;

                /* Stick to the alignment for the converted surface too, makes it easier to load the surface */
                outpitch = width * format.conv_byte_count;
                outpitch = (outpitch + device->surface_alignment - 1) & ~(device->surface_alignment - 1);

                mem = HeapAlloc(GetProcessHeap(), 0, outpitch * height);
                if(!mem) {
                    ERR("Out of memory %d, %d!\n", outpitch, height);
                    if (context) context_release(context);
                    return WINED3DERR_OUTOFVIDEOMEMORY;
                }
                format.convert(surface->resource.allocatedMemory, mem, pitch, width, height);
            }
            else if (convert != NO_CONVERSION && surface->resource.allocatedMemory)
            {
                /* This code is only entered for color keying fixups */
                UINT height = surface->resource.height;

                /* Stick to the alignment for the converted surface too, makes it easier to load the surface */
                outpitch = width * format.conv_byte_count;
                outpitch = (outpitch + device->surface_alignment - 1) & ~(device->surface_alignment - 1);

                mem = HeapAlloc(GetProcessHeap(), 0, outpitch * height);
                if(!mem) {
                    ERR("Out of memory %d, %d!\n", outpitch, height);
                    if (context) context_release(context);
                    return WINED3DERR_OUTOFVIDEOMEMORY;
                }
                d3dfmt_convert_surface(surface->resource.allocatedMemory, mem, pitch,
                        width, height, outpitch, convert, surface);
            }
            else
            {
                mem = surface->resource.allocatedMemory;
            }

            /* Make sure the correct pitch is used */
            ENTER_GL();
            glPixelStorei(GL_UNPACK_ROW_LENGTH, width);
            LEAVE_GL();

            if (mem || (surface->flags & SFLAG_PBO))
                surface_upload_data(surface, gl_info, &format, srgb, mem);

            /* Restore the default pitch */
            ENTER_GL();
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            LEAVE_GL();

            if (context) context_release(context);

            /* Don't delete PBO memory */
            if ((mem != surface->resource.allocatedMemory) && !(surface->flags & SFLAG_PBO))
                HeapFree(GetProcessHeap(), 0, mem);
        }
    }

    if (!rect)
    {
        surface->flags |= flag;

        if (flag != SFLAG_INSYSMEM && (surface->flags & SFLAG_INSYSMEM))
            surface_evict_sysmem(surface);
    }

    if (in_fbo && (surface->flags & (SFLAG_INTEXTURE | SFLAG_INDRAWABLE)))
    {
        /* With ORM_FBO, SFLAG_INTEXTURE and SFLAG_INDRAWABLE are the same for offscreen targets. */
        surface->flags |= (SFLAG_INTEXTURE | SFLAG_INDRAWABLE);
    }

    if (surface->flags & (SFLAG_INTEXTURE | SFLAG_INSRGBTEX)
            && gl_info->supported[EXT_TEXTURE_SRGB_DECODE])
    {
        surface->flags |= (SFLAG_INTEXTURE | SFLAG_INSRGBTEX);
    }

    return WINED3D_OK;
}

BOOL surface_is_offscreen(struct wined3d_surface *surface)
{
    struct wined3d_swapchain *swapchain = surface->container.u.swapchain;

    /* Not on a swapchain - must be offscreen */
    if (surface->container.type != WINED3D_CONTAINER_SWAPCHAIN) return TRUE;

    /* The front buffer is always onscreen */
    if (surface == swapchain->front_buffer) return FALSE;

    /* If the swapchain is rendered to an FBO, the backbuffer is
     * offscreen, otherwise onscreen */
    return swapchain->render_to_fbo;
}

static HRESULT ffp_blit_alloc(struct wined3d_device *device) { return WINED3D_OK; }
/* Context activation is done by the caller. */
static void ffp_blit_free(struct wined3d_device *device) { }

/* This function is used in case of 8bit paletted textures using GL_EXT_paletted_texture */
/* Context activation is done by the caller. */
static void ffp_blit_p8_upload_palette(struct wined3d_surface *surface, const struct wined3d_gl_info *gl_info)
{
    BYTE table[256][4];
    BOOL colorkey_active = (surface->CKeyFlags & WINEDDSD_CKSRCBLT) ? TRUE : FALSE;

    d3dfmt_p8_init_palette(surface, table, colorkey_active);

    TRACE("Using GL_EXT_PALETTED_TEXTURE for 8-bit paletted texture support\n");
    ENTER_GL();
    GL_EXTCALL(glColorTableEXT(surface->texture_target, GL_RGBA, 256, GL_RGBA, GL_UNSIGNED_BYTE, table));
    LEAVE_GL();
}

/* Context activation is done by the caller. */
static HRESULT ffp_blit_set(void *blit_priv, const struct wined3d_gl_info *gl_info, struct wined3d_surface *surface)
{
    enum complex_fixup fixup = get_complex_fixup(surface->resource.format->color_fixup);

    /* When EXT_PALETTED_TEXTURE is around, palette conversion is done by the GPU
     * else the surface is converted in software at upload time in LoadLocation.
     */
    if(fixup == COMPLEX_FIXUP_P8 && gl_info->supported[EXT_PALETTED_TEXTURE])
        ffp_blit_p8_upload_palette(surface, gl_info);

    ENTER_GL();
    glEnable(surface->texture_target);
    checkGLcall("glEnable(surface->texture_target)");
    LEAVE_GL();
    return WINED3D_OK;
}

/* Context activation is done by the caller. */
static void ffp_blit_unset(const struct wined3d_gl_info *gl_info)
{
    ENTER_GL();
    glDisable(GL_TEXTURE_2D);
    checkGLcall("glDisable(GL_TEXTURE_2D)");
    if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
    {
        glDisable(GL_TEXTURE_CUBE_MAP_ARB);
        checkGLcall("glDisable(GL_TEXTURE_CUBE_MAP_ARB)");
    }
    if (gl_info->supported[ARB_TEXTURE_RECTANGLE])
    {
        glDisable(GL_TEXTURE_RECTANGLE_ARB);
        checkGLcall("glDisable(GL_TEXTURE_RECTANGLE_ARB)");
    }
    LEAVE_GL();
}

static BOOL ffp_blit_supported(const struct wined3d_gl_info *gl_info, enum wined3d_blit_op blit_op,
        const RECT *src_rect, DWORD src_usage, WINED3DPOOL src_pool, const struct wined3d_format *src_format,
        const RECT *dst_rect, DWORD dst_usage, WINED3DPOOL dst_pool, const struct wined3d_format *dst_format)
{
    enum complex_fixup src_fixup;

    switch (blit_op)
    {
        case WINED3D_BLIT_OP_COLOR_BLIT:
            src_fixup = get_complex_fixup(src_format->color_fixup);
            if (TRACE_ON(d3d_surface) && TRACE_ON(d3d))
            {
                TRACE("Checking support for fixup:\n");
                dump_color_fixup_desc(src_format->color_fixup);
            }

            if (!is_identity_fixup(dst_format->color_fixup))
            {
                TRACE("Destination fixups are not supported\n");
                return FALSE;
            }

            if (src_fixup == COMPLEX_FIXUP_P8 && gl_info->supported[EXT_PALETTED_TEXTURE])
            {
                TRACE("P8 fixup supported\n");
                return TRUE;
            }

            /* We only support identity conversions. */
            if (is_identity_fixup(src_format->color_fixup))
            {
                TRACE("[OK]\n");
                return TRUE;
            }

            TRACE("[FAILED]\n");
            return FALSE;

        case WINED3D_BLIT_OP_COLOR_FILL:
            if (!(dst_usage & WINED3DUSAGE_RENDERTARGET))
            {
                TRACE("Color fill not supported\n");
                return FALSE;
            }

            return TRUE;

        case WINED3D_BLIT_OP_DEPTH_FILL:
            return TRUE;

        default:
            TRACE("Unsupported blit_op=%d\n", blit_op);
            return FALSE;
    }
}

/* Do not call while under the GL lock. */
static HRESULT ffp_blit_color_fill(struct wined3d_device *device, struct wined3d_surface *dst_surface,
        const RECT *dst_rect, const WINED3DCOLORVALUE *color)
{
    const RECT draw_rect = {0, 0, dst_surface->resource.width, dst_surface->resource.height};

    return device_clear_render_targets(device, 1, &dst_surface, NULL,
            1, dst_rect, &draw_rect, WINED3DCLEAR_TARGET, color, 0.0f, 0);
}

/* Do not call while under the GL lock. */
static HRESULT ffp_blit_depth_fill(struct wined3d_device *device,
        struct wined3d_surface *surface, const RECT *rect, float depth)
{
    const RECT draw_rect = {0, 0, surface->resource.width, surface->resource.height};

    return device_clear_render_targets(device, 0, NULL, surface,
            1, rect, &draw_rect, WINED3DCLEAR_ZBUFFER, 0, depth, 0);
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
static HRESULT cpu_blit_set(void *blit_priv, const struct wined3d_gl_info *gl_info, struct wined3d_surface *surface)
{
    return WINED3D_OK;
}

/* Context activation is done by the caller. */
static void cpu_blit_unset(const struct wined3d_gl_info *gl_info)
{
}

static BOOL cpu_blit_supported(const struct wined3d_gl_info *gl_info, enum wined3d_blit_op blit_op,
        const RECT *src_rect, DWORD src_usage, WINED3DPOOL src_pool, const struct wined3d_format *src_format,
        const RECT *dst_rect, DWORD dst_usage, WINED3DPOOL dst_pool, const struct wined3d_format *dst_format)
{
    if (blit_op == WINED3D_BLIT_OP_COLOR_FILL)
    {
        return TRUE;
    }

    return FALSE;
}

static HRESULT surface_cpu_blt(struct wined3d_surface *dst_surface, const RECT *dst_rect,
        struct wined3d_surface *src_surface, const RECT *src_rect, DWORD flags,
        const WINEDDBLTFX *fx, WINED3DTEXTUREFILTERTYPE filter)
{
    int bpp, srcheight, srcwidth, dstheight, dstwidth, width;
    const struct wined3d_format *src_format, *dst_format;
    struct wined3d_surface *orig_src = src_surface;
    WINED3DLOCKED_RECT dlock, slock;
    HRESULT hr = WINED3D_OK;
    const BYTE *sbuf;
    RECT xdst,xsrc;
    BYTE *dbuf;
    int x, y;

    TRACE("dst_surface %p, dst_rect %s, src_surface %p, src_rect %s, flags %#x, fx %p, filter %s.\n",
            dst_surface, wine_dbgstr_rect(dst_rect), src_surface, wine_dbgstr_rect(src_rect),
            flags, fx, debug_d3dtexturefiltertype(filter));

    if ((dst_surface->flags & SFLAG_LOCKED) || (src_surface && (src_surface->flags & SFLAG_LOCKED)))
    {
        WARN("Surface is busy, returning WINEDDERR_SURFACEBUSY\n");
        return WINEDDERR_SURFACEBUSY;
    }

    /* First check for the validity of source / destination rectangles.
     * This was verified using a test application and by MSDN. */
    if (src_rect)
    {
        if (src_surface)
        {
            if (src_rect->right < src_rect->left || src_rect->bottom < src_rect->top
                    || src_rect->left > src_surface->resource.width || src_rect->left < 0
                    || src_rect->top > src_surface->resource.height || src_rect->top < 0
                    || src_rect->right > src_surface->resource.width || src_rect->right < 0
                    || src_rect->bottom > src_surface->resource.height || src_rect->bottom < 0)
            {
                WARN("Application gave us bad source rectangle for Blt.\n");
                return WINEDDERR_INVALIDRECT;
            }

            if (!src_rect->right || !src_rect->bottom
                    || src_rect->left == (int)src_surface->resource.width
                    || src_rect->top == (int)src_surface->resource.height)
            {
                TRACE("Nothing to be done.\n");
                return WINED3D_OK;
            }
        }

        xsrc = *src_rect;
    }
    else if (src_surface)
    {
        xsrc.left = 0;
        xsrc.top = 0;
        xsrc.right = src_surface->resource.width;
        xsrc.bottom = src_surface->resource.height;
    }
    else
    {
        memset(&xsrc, 0, sizeof(xsrc));
    }

    if (dst_rect)
    {
        /* For the Destination rect, it can be out of bounds on the condition
         * that a clipper is set for the given surface. */
        if (!dst_surface->clipper && (dst_rect->right < dst_rect->left || dst_rect->bottom < dst_rect->top
                || dst_rect->left > dst_surface->resource.width || dst_rect->left < 0
                || dst_rect->top > dst_surface->resource.height || dst_rect->top < 0
                || dst_rect->right > dst_surface->resource.width || dst_rect->right < 0
                || dst_rect->bottom > dst_surface->resource.height || dst_rect->bottom < 0))
        {
            WARN("Application gave us bad destination rectangle for Blt without a clipper set.\n");
            return WINEDDERR_INVALIDRECT;
        }

        if (dst_rect->right <= 0 || dst_rect->bottom <= 0
                || dst_rect->left >= (int)dst_surface->resource.width
                || dst_rect->top >= (int)dst_surface->resource.height)
        {
            TRACE("Nothing to be done.\n");
            return WINED3D_OK;
        }

        if (!src_surface)
        {
            RECT full_rect;

            full_rect.left = 0;
            full_rect.top = 0;
            full_rect.right = dst_surface->resource.width;
            full_rect.bottom = dst_surface->resource.height;
            IntersectRect(&xdst, &full_rect, dst_rect);
        }
        else
        {
            BOOL clip_horiz, clip_vert;

            xdst = *dst_rect;
            clip_horiz = xdst.left < 0 || xdst.right > (int)dst_surface->resource.width;
            clip_vert = xdst.top < 0 || xdst.bottom > (int)dst_surface->resource.height;

            if (clip_vert || clip_horiz)
            {
                /* Now check if this is a special case or not... */
                if ((flags & WINEDDBLT_DDFX)
                        || (clip_horiz && xdst.right - xdst.left != xsrc.right - xsrc.left)
                        || (clip_vert && xdst.bottom - xdst.top != xsrc.bottom - xsrc.top))
                {
                    WARN("Out of screen rectangle in special case. Not handled right now.\n");
                    return WINED3D_OK;
                }

                if (clip_horiz)
                {
                    if (xdst.left < 0)
                    {
                        xsrc.left -= xdst.left;
                        xdst.left = 0;
                    }
                    if (xdst.right > dst_surface->resource.width)
                    {
                        xsrc.right -= (xdst.right - (int)dst_surface->resource.width);
                        xdst.right = (int)dst_surface->resource.width;
                    }
                }

                if (clip_vert)
                {
                    if (xdst.top < 0)
                    {
                        xsrc.top -= xdst.top;
                        xdst.top = 0;
                    }
                    if (xdst.bottom > dst_surface->resource.height)
                    {
                        xsrc.bottom -= (xdst.bottom - (int)dst_surface->resource.height);
                        xdst.bottom = (int)dst_surface->resource.height;
                    }
                }

                /* And check if after clipping something is still to be done... */
                if ((xdst.right <= 0) || (xdst.bottom <= 0)
                        || (xdst.left >= (int)dst_surface->resource.width)
                        || (xdst.top >= (int)dst_surface->resource.height)
                        || (xsrc.right <= 0) || (xsrc.bottom <= 0)
                        || (xsrc.left >= (int)src_surface->resource.width)
                        || (xsrc.top >= (int)src_surface->resource.height))
                {
                    TRACE("Nothing to be done after clipping.\n");
                    return WINED3D_OK;
                }
            }
        }
    }
    else
    {
        xdst.left = 0;
        xdst.top = 0;
        xdst.right = dst_surface->resource.width;
        xdst.bottom = dst_surface->resource.height;
    }

    if (src_surface == dst_surface)
    {
        wined3d_surface_map(dst_surface, &dlock, NULL, 0);
        slock = dlock;
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
                src_surface = surface_convert_format(src_surface, dst_format->id);
                if (!src_surface)
                {
                    /* The conv function writes a FIXME */
                    WARN("Cannot convert source surface format to dest format.\n");
                    goto release;
                }
            }
            wined3d_surface_map(src_surface, &slock, NULL, WINED3DLOCK_READONLY);
            src_format = src_surface->resource.format;
        }
        else
        {
            src_format = dst_format;
        }
        if (dst_rect)
            wined3d_surface_map(dst_surface, &dlock, &xdst, 0);
        else
            wined3d_surface_map(dst_surface, &dlock, NULL, 0);
    }

    if (!fx || !(fx->dwDDFX)) flags &= ~WINEDDBLT_DDFX;

    if (src_format->flags & dst_format->flags & WINED3DFMT_FLAG_FOURCC)
    {
        if (!dst_rect || src_surface == dst_surface)
        {
            memcpy(dlock.pBits, slock.pBits, dst_surface->resource.size);
            goto release;
        }
    }

    bpp = dst_surface->resource.format->byte_count;
    srcheight = xsrc.bottom - xsrc.top;
    srcwidth = xsrc.right - xsrc.left;
    dstheight = xdst.bottom - xdst.top;
    dstwidth = xdst.right - xdst.left;
    width = (xdst.right - xdst.left) * bpp;

    if (dst_rect && src_surface != dst_surface)
        dbuf = dlock.pBits;
    else
        dbuf = (BYTE*)dlock.pBits+(xdst.top*dlock.Pitch)+(xdst.left*bpp);

    if (flags & WINEDDBLT_WAIT)
    {
        flags &= ~WINEDDBLT_WAIT;
    }
    if (flags & WINEDDBLT_ASYNC)
    {
        static BOOL displayed = FALSE;
        if (!displayed)
            FIXME("Can't handle WINEDDBLT_ASYNC flag right now.\n");
        displayed = TRUE;
        flags &= ~WINEDDBLT_ASYNC;
    }
    if (flags & WINEDDBLT_DONOTWAIT)
    {
        /* WINEDDBLT_DONOTWAIT appeared in DX7 */
        static BOOL displayed = FALSE;
        if (!displayed)
            FIXME("Can't handle WINEDDBLT_DONOTWAIT flag right now.\n");
        displayed = TRUE;
        flags &= ~WINEDDBLT_DONOTWAIT;
    }

    /* First, all the 'source-less' blits */
    if (flags & WINEDDBLT_COLORFILL)
    {
        hr = _Blt_ColorFill(dbuf, dstwidth, dstheight, bpp, dlock.Pitch, fx->u5.dwFillColor);
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
                hr = _Blt_ColorFill(dbuf,dstwidth,dstheight,bpp,dlock.Pitch,0);
                break;
            case 0xAA0029: /* No-op */
                break;
            case WHITENESS:
                hr = _Blt_ColorFill(dbuf,dstwidth,dstheight,bpp,dlock.Pitch,~0);
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
        const BYTE *sbase;
        int sx, xinc, sy, yinc;

        if (!dstwidth || !dstheight) /* Hmm... stupid program? */
            goto release;

        if (filter != WINED3DTEXF_NONE && filter != WINED3DTEXF_POINT
                && (srcwidth != dstwidth || srcheight != dstheight))
        {
            /* Can happen when d3d9 apps do a StretchRect() call which isn't handled in GL. */
            FIXME("Filter %s not supported in software blit.\n", debug_d3dtexturefiltertype(filter));
        }

        sbase = (BYTE*)slock.pBits+(xsrc.top*slock.Pitch)+xsrc.left*bpp;
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
                    if (src_surface != dst_surface || xdst.top < xsrc.top
                            || xdst.right <= xsrc.left || xsrc.right <= xdst.left)
                    {
                        /* No overlap, or dst above src, so copy from top downwards. */
                        for (y = 0; y < dstheight; ++y)
                        {
                            memcpy(dbuf, sbuf, width);
                            sbuf += slock.Pitch;
                            dbuf += dlock.Pitch;
                        }
                    }
                    else if (xdst.top > xsrc.top)
                    {
                        /* Copy from bottom upwards. */
                        sbuf += (slock.Pitch*dstheight);
                        dbuf += (dlock.Pitch*dstheight);
                        for (y = 0; y < dstheight; ++y)
                        {
                            sbuf -= slock.Pitch;
                            dbuf -= dlock.Pitch;
                            memcpy(dbuf, sbuf, width);
                        }
                    }
                    else
                    {
                        /* Src and dst overlapping on the same line, use memmove. */
                        for (y = 0; y < dstheight; ++y)
                        {
                            memmove(dbuf, sbuf, width);
                            sbuf += slock.Pitch;
                            dbuf += dlock.Pitch;
                        }
                    }
                }
                else
                {
                    /* Stretching in y direction only. */
                    for (y = sy = 0; y < dstheight; ++y, sy += yinc)
                    {
                        sbuf = sbase + (sy >> 16) * slock.Pitch;
                        memcpy(dbuf, sbuf, width);
                        dbuf += dlock.Pitch;
                    }
                }
            }
            else
            {
                /* Stretching in X direction. */
                int last_sy = -1;
                for (y = sy = 0; y < dstheight; ++y, sy += yinc)
                {
                    sbuf = sbase + (sy >> 16) * slock.Pitch;

                    if ((sy >> 16) == (last_sy >> 16))
                    {
                        /* This source row is the same as last source row -
                         * Copy the already stretched row. */
                        memcpy(dbuf, dbuf - dlock.Pitch, width);
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
                    dbuf += dlock.Pitch;
                    last_sy = sy;
                }
            }
        }
        else
        {
            LONG dstyinc = dlock.Pitch, dstxinc = bpp;
            DWORD keylow = 0xFFFFFFFF, keyhigh = 0, keymask = 0xFFFFFFFF;
            DWORD destkeylow = 0x0, destkeyhigh = 0xFFFFFFFF, destkeymask = 0xFFFFFFFF;
            if (flags & (WINEDDBLT_KEYSRC | WINEDDBLT_KEYDEST | WINEDDBLT_KEYSRCOVERRIDE | WINEDDBLT_KEYDESTOVERRIDE))
            {
                /* The color keying flags are checked for correctness in ddraw */
                if (flags & WINEDDBLT_KEYSRC)
                {
                    keylow  = src_surface->SrcBltCKey.dwColorSpaceLowValue;
                    keyhigh = src_surface->SrcBltCKey.dwColorSpaceHighValue;
                }
                else if (flags & WINEDDBLT_KEYSRCOVERRIDE)
                {
                    keylow = fx->ddckSrcColorkey.dwColorSpaceLowValue;
                    keyhigh = fx->ddckSrcColorkey.dwColorSpaceHighValue;
                }

                if (flags & WINEDDBLT_KEYDEST)
                {
                    /* Destination color keys are taken from the source surface! */
                    destkeylow = src_surface->DestBltCKey.dwColorSpaceLowValue;
                    destkeyhigh = src_surface->DestBltCKey.dwColorSpaceHighValue;
                }
                else if (flags & WINEDDBLT_KEYDESTOVERRIDE)
                {
                    destkeylow = fx->ddckDestColorkey.dwColorSpaceLowValue;
                    destkeyhigh = fx->ddckDestColorkey.dwColorSpaceHighValue;
                }

                if (bpp == 1)
                {
                    keymask = 0xff;
                }
                else
                {
                    keymask = src_format->red_mask
                            | src_format->green_mask
                            | src_format->blue_mask;
                }
                flags &= ~(WINEDDBLT_KEYSRC | WINEDDBLT_KEYDEST | WINEDDBLT_KEYSRCOVERRIDE | WINEDDBLT_KEYDESTOVERRIDE);
            }

            if (flags & WINEDDBLT_DDFX)
            {
                BYTE *dTopLeft, *dTopRight, *dBottomLeft, *dBottomRight, *tmp;
                LONG tmpxy;
                dTopLeft     = dbuf;
                dTopRight    = dbuf + ((dstwidth - 1) * bpp);
                dBottomLeft  = dTopLeft + ((dstheight - 1) * dlock.Pitch);
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
        s = (const type *)(sbase + (sy >> 16) * slock.Pitch); \
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
                        sbuf = sbase + (sy >> 16) * slock.Pitch;
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
    if (src_surface && src_surface != orig_src)
        wined3d_surface_decref(src_surface);

    return hr;
}

static HRESULT surface_cpu_bltfast(struct wined3d_surface *dst_surface, DWORD dst_x, DWORD dst_y,
        struct wined3d_surface *src_surface, const RECT *src_rect, DWORD trans)
{
    const struct wined3d_format *src_format, *dst_format;
    RECT lock_src, lock_dst, lock_union;
    WINED3DLOCKED_RECT dlock, slock;
    HRESULT hr = WINED3D_OK;
    int bpp, w, h, x, y;
    const BYTE *sbuf;
    BYTE *dbuf;
    RECT rsrc2;

    TRACE("dst_surface %p, dst_x %u, dst_y %u, src_surface %p, src_rect %s, flags %#x.\n",
            dst_surface, dst_x, dst_y, src_surface, wine_dbgstr_rect(src_rect), trans);

    if ((dst_surface->flags & SFLAG_LOCKED) || (src_surface->flags & SFLAG_LOCKED))
    {
        WARN(" Surface is busy, returning DDERR_SURFACEBUSY\n");
        return WINEDDERR_SURFACEBUSY;
    }

    if (!src_rect)
    {
        WARN("src_rect is NULL!\n");
        rsrc2.left = 0;
        rsrc2.top = 0;
        rsrc2.right = src_surface->resource.width;
        rsrc2.bottom = src_surface->resource.height;
        src_rect = &rsrc2;
    }

    /* Check source rect for validity. Copied from normal Blt. Fixes Baldur's Gate. */
    if ((src_rect->bottom > src_surface->resource.height) || (src_rect->bottom < 0)
            || (src_rect->top > src_surface->resource.height) || (src_rect->top < 0)
            || (src_rect->left > src_surface->resource.width) || (src_rect->left < 0)
            || (src_rect->right > src_surface->resource.width) || (src_rect->right < 0)
            || (src_rect->right < src_rect->left) || (src_rect->bottom < src_rect->top))
    {
        WARN("Application gave us bad source rectangle for BltFast.\n");
        return WINEDDERR_INVALIDRECT;
    }

    h = src_rect->bottom - src_rect->top;
    if (h > dst_surface->resource.height - dst_y)
        h = dst_surface->resource.height - dst_y;
    if (h > src_surface->resource.height - src_rect->top)
        h = src_surface->resource.height - src_rect->top;
    if (h <= 0)
        return WINEDDERR_INVALIDRECT;

    w = src_rect->right - src_rect->left;
    if (w > dst_surface->resource.width - dst_x)
        w = dst_surface->resource.width - dst_x;
    if (w > src_surface->resource.width - src_rect->left)
        w = src_surface->resource.width - src_rect->left;
    if (w <= 0)
        return WINEDDERR_INVALIDRECT;

    /* Now compute the locking rectangle... */
    lock_src.left = src_rect->left;
    lock_src.top = src_rect->top;
    lock_src.right = lock_src.left + w;
    lock_src.bottom = lock_src.top + h;

    lock_dst.left = dst_x;
    lock_dst.top = dst_y;
    lock_dst.right = dst_x + w;
    lock_dst.bottom = dst_y + h;

    bpp = dst_surface->resource.format->byte_count;

    /* We need to lock the surfaces, or we won't get refreshes when done. */
    if (src_surface == dst_surface)
    {
        int pitch;

        UnionRect(&lock_union, &lock_src, &lock_dst);

        /* Lock the union of the two rectangles. */
        hr = wined3d_surface_map(dst_surface, &dlock, &lock_union, 0);
        if (FAILED(hr))
            goto error;

        pitch = dlock.Pitch;
        slock.Pitch = dlock.Pitch;

        /* Since slock was originally copied from this surface's description, we can just reuse it. */
        sbuf = dst_surface->resource.allocatedMemory + lock_src.top * pitch + lock_src.left * bpp;
        dbuf = dst_surface->resource.allocatedMemory + lock_dst.top * pitch + lock_dst.left * bpp;
        src_format = src_surface->resource.format;
        dst_format = src_format;
    }
    else
    {
        hr = wined3d_surface_map(src_surface, &slock, &lock_src, WINED3DLOCK_READONLY);
        if (FAILED(hr))
            goto error;
        hr = wined3d_surface_map(dst_surface, &dlock, &lock_dst, 0);
        if (FAILED(hr))
            goto error;

        sbuf = slock.pBits;
        dbuf = dlock.pBits;
        TRACE("Dst is at %p, Src is at %p.\n", dbuf, sbuf);

        src_format = src_surface->resource.format;
        dst_format = dst_surface->resource.format;
    }

    /* Handle compressed surfaces first... */
    if (src_format->flags & dst_format->flags & WINED3DFMT_FLAG_COMPRESSED)
    {
        UINT row_block_count;

        TRACE("compressed -> compressed copy\n");
        if (trans)
            FIXME("trans arg not supported when a compressed surface is involved\n");
        if (dst_x || dst_y)
            FIXME("offset for destination surface is not supported\n");
        if (src_surface->resource.format->id != dst_surface->resource.format->id)
        {
            FIXME("compressed -> compressed copy only supported for the same type of surface\n");
            hr = WINED3DERR_WRONGTEXTUREFORMAT;
            goto error;
        }

        row_block_count = (w + dst_format->block_width - 1) / dst_format->block_width;
        for (y = 0; y < h; y += dst_format->block_height)
        {
            memcpy(dbuf, sbuf, row_block_count * dst_format->block_byte_count);
            dbuf += dlock.Pitch;
            sbuf += slock.Pitch;
        }

        goto error;
    }
    if ((src_format->flags & WINED3DFMT_FLAG_COMPRESSED) && !(dst_format->flags & WINED3DFMT_FLAG_COMPRESSED))
    {
        /* TODO: Use the libtxc_dxtn.so shared library to do software
         * decompression. */
        ERR("Software decompression not supported.\n");
        goto error;
    }

    if (trans & (WINEDDBLTFAST_SRCCOLORKEY | WINEDDBLTFAST_DESTCOLORKEY))
    {
        DWORD keylow, keyhigh;
        DWORD mask = src_surface->resource.format->red_mask
                | src_surface->resource.format->green_mask
                | src_surface->resource.format->blue_mask;

        /* For some 8-bit formats like L8 and P8 color masks don't make sense */
        if (!mask && bpp == 1)
            mask = 0xff;

        TRACE("Color keyed copy.\n");
        if (trans & WINEDDBLTFAST_SRCCOLORKEY)
        {
            keylow = src_surface->SrcBltCKey.dwColorSpaceLowValue;
            keyhigh = src_surface->SrcBltCKey.dwColorSpaceHighValue;
        }
        else
        {
            /* I'm not sure if this is correct. */
            FIXME("WINEDDBLTFAST_DESTCOLORKEY not fully supported yet.\n");
            keylow = dst_surface->DestBltCKey.dwColorSpaceLowValue;
            keyhigh = dst_surface->DestBltCKey.dwColorSpaceHighValue;
        }

#define COPYBOX_COLORKEY(type) \
do { \
    const type *s = (const type *)sbuf; \
    type *d = (type *)dbuf; \
    type tmp; \
    for (y = 0; y < h; y++) \
    { \
        for (x = 0; x < w; x++) \
        { \
            tmp = s[x]; \
            if ((tmp & mask) < keylow || (tmp & mask) > keyhigh) d[x] = tmp; \
        } \
        s = (const type *)((const BYTE *)s + slock.Pitch); \
        d = (type *)((BYTE *)d + dlock.Pitch); \
    } \
} while(0)

        switch (bpp)
        {
            case 1:
                COPYBOX_COLORKEY(BYTE);
                break;
            case 2:
                COPYBOX_COLORKEY(WORD);
                break;
            case 4:
                COPYBOX_COLORKEY(DWORD);
                break;
            case 3:
            {
                const BYTE *s;
                DWORD tmp;
                BYTE *d;
                s = sbuf;
                d = dbuf;
                for (y = 0; y < h; ++y)
                {
                    for (x = 0; x < w * 3; x += 3)
                    {
                        tmp = (DWORD)s[x] + ((DWORD)s[x + 1] << 8) + ((DWORD)s[x + 2] << 16);
                        if (tmp < keylow || tmp > keyhigh)
                        {
                            d[x + 0] = s[x + 0];
                            d[x + 1] = s[x + 1];
                            d[x + 2] = s[x + 2];
                        }
                    }
                    s += slock.Pitch;
                    d += dlock.Pitch;
                }
                break;
            }
            default:
                FIXME("Source color key blitting not supported for bpp %u.\n", bpp * 8);
                hr = WINED3DERR_NOTAVAILABLE;
                goto error;
        }
#undef COPYBOX_COLORKEY
        TRACE("Copy done.\n");
    }
    else
    {
        int width = w * bpp;
        INT sbufpitch, dbufpitch;

        TRACE("No color key copy.\n");
        /* Handle overlapping surfaces. */
        if (sbuf < dbuf)
        {
            sbuf += (h - 1) * slock.Pitch;
            dbuf += (h - 1) * dlock.Pitch;
            sbufpitch = -slock.Pitch;
            dbufpitch = -dlock.Pitch;
        }
        else
        {
            sbufpitch = slock.Pitch;
            dbufpitch = dlock.Pitch;
        }
        for (y = 0; y < h; ++y)
        {
            /* This is pretty easy, a line for line memcpy. */
            memmove(dbuf, sbuf, width);
            sbuf += sbufpitch;
            dbuf += dbufpitch;
        }
        TRACE("Copy done.\n");
    }

error:
    if (src_surface == dst_surface)
    {
        wined3d_surface_unmap(dst_surface);
    }
    else
    {
        wined3d_surface_unmap(dst_surface);
        wined3d_surface_unmap(src_surface);
    }

    return hr;
}

/* Do not call while under the GL lock. */
static HRESULT cpu_blit_color_fill(struct wined3d_device *device, struct wined3d_surface *dst_surface,
        const RECT *dst_rect, const WINED3DCOLORVALUE *color)
{
    WINEDDBLTFX BltFx;

    memset(&BltFx, 0, sizeof(BltFx));
    BltFx.dwSize = sizeof(BltFx);
    BltFx.u5.dwFillColor = wined3d_format_convert_from_float(dst_surface->resource.format, color);
    return wined3d_surface_blt(dst_surface, dst_rect, NULL, NULL,
            WINEDDBLT_COLORFILL, &BltFx, WINED3DTEXF_POINT);
}

/* Do not call while under the GL lock. */
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

static HRESULT surface_init(struct wined3d_surface *surface, WINED3DSURFTYPE surface_type, UINT alignment,
        UINT width, UINT height, UINT level, BOOL lockable, BOOL discard, WINED3DMULTISAMPLE_TYPE multisample_type,
        UINT multisample_quality, struct wined3d_device *device, DWORD usage, enum wined3d_format_id format_id,
        WINED3DPOOL pool, void *parent, const struct wined3d_parent_ops *parent_ops)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    const struct wined3d_format *format = wined3d_get_format(gl_info, format_id);
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
    switch (pool)
    {
        case WINED3DPOOL_SCRATCH:
            if (!lockable)
            {
                FIXME("Called with a pool of SCRATCH and a lockable of FALSE "
                        "which are mutually exclusive, setting lockable to TRUE.\n");
                lockable = TRUE;
            }
            break;

        case WINED3DPOOL_SYSTEMMEM:
            if (!lockable)
                FIXME("Called with a pool of SYSTEMMEM and a lockable of FALSE, this is acceptable but unexpected.\n");
            break;

        case WINED3DPOOL_MANAGED:
            if (usage & WINED3DUSAGE_DYNAMIC)
                FIXME("Called with a pool of MANAGED and a usage of DYNAMIC which are mutually exclusive.\n");
            break;

        case WINED3DPOOL_DEFAULT:
            if (lockable && !(usage & (WINED3DUSAGE_DYNAMIC | WINED3DUSAGE_RENDERTARGET | WINED3DUSAGE_DEPTHSTENCIL)))
                WARN("Creating a lockable surface with a POOL of DEFAULT, that doesn't specify DYNAMIC usage.\n");
            break;

        default:
            FIXME("Unknown pool %#x.\n", pool);
            break;
    };

    if (usage & WINED3DUSAGE_RENDERTARGET && pool != WINED3DPOOL_DEFAULT)
        FIXME("Trying to create a render target that isn't in the default pool.\n");

    /* FIXME: Check that the format is supported by the device. */

    resource_size = wined3d_format_calculate_size(format, alignment, width, height);
    if (!resource_size)
        return WINED3DERR_INVALIDCALL;

    surface->surface_type = surface_type;

    switch (surface_type)
    {
        case SURFACE_OPENGL:
            surface->surface_ops = &surface_ops;
            break;

        case SURFACE_GDI:
            surface->surface_ops = &gdi_surface_ops;
            break;

        default:
            ERR("Requested unknown surface implementation %#x.\n", surface_type);
            return WINED3DERR_INVALIDCALL;
    }

    hr = resource_init(&surface->resource, device, WINED3DRTYPE_SURFACE, format,
            multisample_type, multisample_quality, usage, pool, width, height, 1,
            resource_size, parent, parent_ops, &surface_resource_ops);
    if (FAILED(hr))
    {
        WARN("Failed to initialize resource, returning %#x.\n", hr);
        return hr;
    }

    /* "Standalone" surface. */
    surface_set_container(surface, WINED3D_CONTAINER_NONE, NULL);

    surface->texture_level = level;
    list_init(&surface->overlays);

    /* Flags */
    surface->flags = SFLAG_NORMCOORD; /* Default to normalized coords. */
    if (discard)
        surface->flags |= SFLAG_DISCARD;
    if (lockable || format_id == WINED3DFMT_D16_LOCKABLE)
        surface->flags |= SFLAG_LOCKABLE;
    /* I'm not sure if this qualifies as a hack or as an optimization. It
     * seems reasonable to assume that lockable render targets will get
     * locked, so we might as well set SFLAG_DYNLOCK right at surface
     * creation. However, the other reason we want to do this is that several
     * ddraw applications access surface memory while the surface isn't
     * mapped. The SFLAG_DYNLOCK behaviour of keeping SYSMEM around for
     * future locks prevents these from crashing. */
    if (lockable && (usage & WINED3DUSAGE_RENDERTARGET))
        surface->flags |= SFLAG_DYNLOCK;

    /* Mark the texture as dirty so that it gets loaded first time around. */
    surface_add_dirty_rect(surface, NULL);
    list_init(&surface->renderbuffers);

    TRACE("surface %p, memory %p, size %u\n",
            surface, surface->resource.allocatedMemory, surface->resource.size);

    /* Call the private setup routine */
    hr = surface->surface_ops->surface_private_setup(surface);
    if (FAILED(hr))
    {
        ERR("Private setup failed, returning %#x\n", hr);
        surface->surface_ops->surface_cleanup(surface);
        return hr;
    }

    return hr;
}

HRESULT CDECL wined3d_surface_create(struct wined3d_device *device, UINT width, UINT height,
        enum wined3d_format_id format_id, BOOL lockable, BOOL discard, UINT level, DWORD usage, WINED3DPOOL pool,
        WINED3DMULTISAMPLE_TYPE multisample_type, DWORD multisample_quality, WINED3DSURFTYPE surface_type,
        void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_surface **surface)
{
    struct wined3d_surface *object;
    HRESULT hr;

    TRACE("device %p, width %u, height %u, format %s, lockable %#x, discard %#x, level %u\n",
            device, width, height, debug_d3dformat(format_id), lockable, discard, level);
    TRACE("surface %p, usage %s (%#x), pool %s, multisample_type %#x, multisample_quality %u\n",
            surface, debug_d3dusage(usage), usage, debug_d3dpool(pool), multisample_type, multisample_quality);
    TRACE("surface_type %#x, parent %p, parent_ops %p.\n", surface_type, parent, parent_ops);

    if (surface_type == SURFACE_OPENGL && !device->adapter)
    {
        ERR("OpenGL surfaces are not available without OpenGL.\n");
        return WINED3DERR_NOTAVAILABLE;
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate surface memory.\n");
        return WINED3DERR_OUTOFVIDEOMEMORY;
    }

    hr = surface_init(object, surface_type, device->surface_alignment, width, height, level, lockable,
            discard, multisample_type, multisample_quality, device, usage, format_id, pool, parent, parent_ops);
    if (FAILED(hr))
    {
        WARN("Failed to initialize surface, returning %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created surface %p.\n", object);
    *surface = object;

    return hr;
}
