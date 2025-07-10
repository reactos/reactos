/*
 * Direct3D 9 private include file
 *
 * Copyright 2002-2003 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2005 Oliver Stieber
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

#ifndef __WINE_D3D9_PRIVATE_H
#define __WINE_D3D9_PRIVATE_H

#include <assert.h>
#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/debug.h"

#include "d3d9.h"
#include "d3d9on12.h"
#include "wine/wined3d.h"

#define D3D9_MAX_VERTEX_SHADER_CONSTANTF 256
#define D3D9_MAX_TEXTURE_UNITS 20
#define D3D9_MAX_STREAMS 16

#define D3DPRESENTFLAGS_MASK 0x00000fffu

#define D3D9_TEXTURE_MIPMAP_DIRTY 0x1

#define D3DFMT_RESZ MAKEFOURCC('R','E','S','Z')
#define D3D9_RESZ_CODE 0x7fa05000

extern const struct wined3d_parent_ops d3d9_null_wined3d_parent_ops;

HRESULT vdecl_convert_fvf(DWORD FVF, D3DVERTEXELEMENT9 **ppVertexElements);
D3DFORMAT d3dformat_from_wined3dformat(enum wined3d_format_id format);
BOOL is_gdi_compat_wined3dformat(enum wined3d_format_id format);
enum wined3d_format_id wined3dformat_from_d3dformat(D3DFORMAT format);
unsigned int wined3dmapflags_from_d3dmapflags(unsigned int flags, unsigned int usage);
void present_parameters_from_wined3d_swapchain_desc(D3DPRESENT_PARAMETERS *present_parameters,
        const struct wined3d_swapchain_desc *swapchain_desc, DWORD presentation_interval);

struct d3d9
{
    IDirect3D9Ex IDirect3D9Ex_iface;
    LONG refcount;
    struct wined3d *wined3d;
    struct wined3d_output **wined3d_outputs;
    unsigned int wined3d_output_count;
    BOOL extended;
    BOOL d3d9on12;
};

void d3d9_caps_from_wined3dcaps(const struct d3d9 *d3d9, unsigned int adapter_ordinal,
        D3DCAPS9 *caps, const struct wined3d_caps *wined3d_caps);
BOOL d3d9_init(struct d3d9 *d3d9, BOOL extended, BOOL d3d9on12);

struct fvf_declaration
{
    struct wined3d_vertex_declaration *decl;
    DWORD fvf;
};

enum d3d9_device_state
{
    D3D9_DEVICE_STATE_OK,
    D3D9_DEVICE_STATE_LOST,
    D3D9_DEVICE_STATE_NOT_RESET,
};

struct d3d9_device
{
    IDirect3DDevice9Ex IDirect3DDevice9Ex_iface;
    IDirect3DDevice9On12 IDirect3DDevice9On12_iface;
    struct wined3d_device_parent device_parent;
    LONG refcount;
    struct wined3d_device *wined3d_device;
    struct wined3d_device_context *immediate_context;
    unsigned int adapter_ordinal;
    struct d3d9 *d3d_parent;

    struct fvf_declaration *fvf_decls;
    UINT fvf_decl_count, fvf_decl_size;

    struct wined3d_streaming_buffer vertex_buffer, index_buffer;

    struct d3d9_surface *render_targets[D3D_MAX_SIMULTANEOUS_RENDERTARGETS];

    LONG device_state;
    DWORD sysmem_vb : 16; /* D3D9_MAX_STREAMS */
    DWORD sysmem_ib : 1;
    DWORD in_destruction : 1;
    DWORD in_scene : 1;
    DWORD padding : 13;

    DWORD auto_mipmaps; /* D3D9_MAX_TEXTURE_UNITS */

    unsigned int max_user_clip_planes, vs_uniform_count;

    UINT implicit_swapchain_count;
    struct wined3d_swapchain **implicit_swapchains;

    struct wined3d_stateblock *recording, *state, *update_state;
    const struct wined3d_stateblock_state *stateblock_state;
};

HRESULT device_init(struct d3d9_device *device, struct d3d9 *parent, struct wined3d *wined3d,
        UINT adapter, D3DDEVTYPE device_type, HWND focus_window, DWORD flags,
        D3DPRESENT_PARAMETERS *parameters, D3DDISPLAYMODEEX *mode);

struct d3d9_resource
{
    LONG refcount;
    struct wined3d_private_store private_store;
};

void d3d9_resource_cleanup(struct d3d9_resource *resource);
HRESULT d3d9_resource_free_private_data(struct d3d9_resource *resource, const GUID *guid);
HRESULT d3d9_resource_get_private_data(struct d3d9_resource *resource, const GUID *guid,
        void *data, DWORD *data_size);
void d3d9_resource_init(struct d3d9_resource *resource);
HRESULT d3d9_resource_set_private_data(struct d3d9_resource *resource, const GUID *guid,
        const void *data, DWORD data_size, DWORD flags);

struct d3d9_volume
{
    IDirect3DVolume9 IDirect3DVolume9_iface;
    struct d3d9_resource resource;
    struct wined3d_texture *wined3d_texture;
    unsigned int sub_resource_idx;
    struct d3d9_texture *texture;
};

void volume_init(struct d3d9_volume *volume, struct wined3d_texture *wined3d_texture,
        unsigned int sub_resource_idx, const struct wined3d_parent_ops **parent_ops);

struct d3d9_swapchain
{
    IDirect3DSwapChain9Ex IDirect3DSwapChain9Ex_iface;
    LONG refcount;
    struct wined3d_swapchain *wined3d_swapchain;
    struct wined3d_swapchain_state_parent state_parent;
    IDirect3DDevice9Ex *parent_device;
    unsigned int swap_interval;
};

HRESULT d3d9_swapchain_create(struct d3d9_device *device, struct wined3d_swapchain_desc *desc,
        unsigned int swap_interval, struct d3d9_swapchain **swapchain);

struct d3d9_surface
{
    IDirect3DSurface9 IDirect3DSurface9_iface;
    struct d3d9_resource resource;
    struct wined3d_texture *wined3d_texture;
    unsigned int sub_resource_idx;
    struct list rtv_entry;
    struct wined3d_rendertarget_view *wined3d_rtv;
    IDirect3DDevice9Ex *parent_device;
    IUnknown *container;
    struct d3d9_texture *texture;
    struct wined3d_swapchain *swapchain;
};

struct wined3d_rendertarget_view *d3d9_surface_acquire_rendertarget_view(struct d3d9_surface *surface);
struct d3d9_surface *d3d9_surface_create(struct wined3d_texture *wined3d_texture,
        unsigned int sub_resource_idx, IUnknown *container);
struct d3d9_device *d3d9_surface_get_device(const struct d3d9_surface *surface);
void d3d9_surface_release_rendertarget_view(struct d3d9_surface *surface,
        struct wined3d_rendertarget_view *rtv);
struct d3d9_surface *unsafe_impl_from_IDirect3DSurface9(IDirect3DSurface9 *iface);

struct d3d9_vertexbuffer
{
    IDirect3DVertexBuffer9 IDirect3DVertexBuffer9_iface;
    struct d3d9_resource resource;
    struct wined3d_buffer *wined3d_buffer;
    IDirect3DDevice9Ex *parent_device;
    struct wined3d_buffer *draw_buffer;
    DWORD fvf, usage;
};

HRESULT vertexbuffer_init(struct d3d9_vertexbuffer *buffer, struct d3d9_device *device,
        UINT size, UINT usage, DWORD fvf, D3DPOOL pool);
struct d3d9_vertexbuffer *unsafe_impl_from_IDirect3DVertexBuffer9(IDirect3DVertexBuffer9 *iface);

struct d3d9_indexbuffer
{
    IDirect3DIndexBuffer9 IDirect3DIndexBuffer9_iface;
    struct d3d9_resource resource;
    struct wined3d_buffer *wined3d_buffer;
    IDirect3DDevice9Ex *parent_device;
    enum wined3d_format_id format;
    DWORD usage;
    bool sysmem;
};

HRESULT indexbuffer_init(struct d3d9_indexbuffer *buffer, struct d3d9_device *device,
        UINT size, DWORD usage, D3DFORMAT format, D3DPOOL pool);
struct d3d9_indexbuffer *unsafe_impl_from_IDirect3DIndexBuffer9(IDirect3DIndexBuffer9 *iface);

struct d3d9_texture
{
    IDirect3DBaseTexture9 IDirect3DBaseTexture9_iface;
    struct d3d9_resource resource;
    struct wined3d_texture *wined3d_texture, *draw_texture;
    struct d3d9_device *parent_device;
    struct list rtv_list;
    DWORD usage;
    BOOL flags;
    D3DTEXTUREFILTERTYPE autogen_filter_type;
};

HRESULT d3d9_texture_2d_init(struct d3d9_texture *texture, struct d3d9_device *device, unsigned int width,
        unsigned int height, unsigned int level_count, DWORD usage, D3DFORMAT format, D3DPOOL pool);
HRESULT d3d9_texture_3d_init(struct d3d9_texture *texture, struct d3d9_device *device, unsigned int width,
        unsigned int height, unsigned int depth, unsigned int level_count, DWORD usage, D3DFORMAT format, D3DPOOL pool);
HRESULT d3d9_texture_cube_init(struct d3d9_texture *texture, struct d3d9_device *device,
        unsigned int edge_length, unsigned int level_count, DWORD usage, D3DFORMAT format, D3DPOOL pool);

static inline struct wined3d_texture *d3d9_texture_get_draw_texture(struct d3d9_texture *texture)
{
    return texture->draw_texture ? texture->draw_texture : texture->wined3d_texture;
}

struct d3d9_texture *unsafe_impl_from_IDirect3DBaseTexture9(IDirect3DBaseTexture9 *iface);
void d3d9_texture_flag_auto_gen_mipmap(struct d3d9_texture *texture);
void d3d9_texture_gen_auto_mipmap(struct d3d9_texture *texture);

struct d3d9_stateblock
{
    IDirect3DStateBlock9 IDirect3DStateBlock9_iface;
    LONG refcount;
    struct wined3d_stateblock *wined3d_stateblock;
    IDirect3DDevice9Ex *parent_device;
};

HRESULT stateblock_init(struct d3d9_stateblock *stateblock, struct d3d9_device *device,
        D3DSTATEBLOCKTYPE type, struct wined3d_stateblock *wined3d_stateblock);

struct d3d9_vertex_declaration
{
    IDirect3DVertexDeclaration9 IDirect3DVertexDeclaration9_iface;
    LONG refcount;
    D3DVERTEXELEMENT9 *elements;
    UINT element_count;
    DWORD stream_map;
    struct wined3d_vertex_declaration *wined3d_declaration;
    DWORD fvf;
    IDirect3DDevice9Ex *parent_device;
};

HRESULT d3d9_vertex_declaration_create(struct d3d9_device *device,
        const D3DVERTEXELEMENT9 *elements, struct d3d9_vertex_declaration **declaration);
struct d3d9_vertex_declaration *unsafe_impl_from_IDirect3DVertexDeclaration9(
        IDirect3DVertexDeclaration9 *iface);

struct d3d9_vertexshader
{
    IDirect3DVertexShader9 IDirect3DVertexShader9_iface;
    LONG refcount;
    struct wined3d_shader *wined3d_shader;
    IDirect3DDevice9Ex *parent_device;
};

HRESULT vertexshader_init(struct d3d9_vertexshader *shader,
        struct d3d9_device *device, const DWORD *byte_code);
struct d3d9_vertexshader *unsafe_impl_from_IDirect3DVertexShader9(IDirect3DVertexShader9 *iface);

struct d3d9_pixelshader
{
    IDirect3DPixelShader9 IDirect3DPixelShader9_iface;
    LONG refcount;
    struct wined3d_shader *wined3d_shader;
    IDirect3DDevice9Ex *parent_device;
};

HRESULT pixelshader_init(struct d3d9_pixelshader *shader,
        struct d3d9_device *device, const DWORD *byte_code);
struct d3d9_pixelshader *unsafe_impl_from_IDirect3DPixelShader9(IDirect3DPixelShader9 *iface);

struct d3d9_query
{
    IDirect3DQuery9 IDirect3DQuery9_iface;
    LONG refcount;
    struct wined3d_query *wined3d_query;
    IDirect3DDevice9Ex *parent_device;
    DWORD data_size;
};

HRESULT query_init(struct d3d9_query *query, struct d3d9_device *device, D3DQUERYTYPE type);

static inline struct d3d9_device *impl_from_IDirect3DDevice9Ex(IDirect3DDevice9Ex *iface)
{
    return CONTAINING_RECORD(iface, struct d3d9_device, IDirect3DDevice9Ex_iface);
}

static inline DWORD d3dusage_from_wined3dusage(unsigned int wined3d_usage, unsigned int bind_flags)
{
    DWORD usage = wined3d_usage & WINED3DUSAGE_MASK;
    if (bind_flags & WINED3D_BIND_RENDER_TARGET)
        usage |= D3DUSAGE_RENDERTARGET;
    if (bind_flags & WINED3D_BIND_DEPTH_STENCIL)
        usage |= D3DUSAGE_DEPTHSTENCIL;
    return usage;
}

static inline D3DPOOL d3dpool_from_wined3daccess(unsigned int access, unsigned int usage)
{
    if (usage & WINED3DUSAGE_MANAGED)
        return D3DPOOL_MANAGED;

    switch (access & (WINED3D_RESOURCE_ACCESS_GPU | WINED3D_RESOURCE_ACCESS_CPU))
    {
        default:
        case WINED3D_RESOURCE_ACCESS_GPU:
            return D3DPOOL_DEFAULT;
        case WINED3D_RESOURCE_ACCESS_CPU:
            if (usage & WINED3DUSAGE_SCRATCH)
                return D3DPOOL_SCRATCH;
            return D3DPOOL_SYSTEMMEM;
        case WINED3D_RESOURCE_ACCESS_GPU | WINED3D_RESOURCE_ACCESS_CPU:
            return D3DPOOL_MANAGED;
    }
}

static inline D3DMULTISAMPLE_TYPE d3dmultisample_type_from_wined3d(enum wined3d_multisample_type type)
{
    return (D3DMULTISAMPLE_TYPE)type;
}

static inline D3DSCANLINEORDERING d3dscanlineordering_from_wined3d(enum wined3d_scanline_ordering ordering)
{
    return (D3DSCANLINEORDERING)ordering;
}

static inline unsigned int map_access_from_usage(unsigned int usage)
{
    if (usage & D3DUSAGE_WRITEONLY)
        return WINED3D_RESOURCE_ACCESS_MAP_W;
    return WINED3D_RESOURCE_ACCESS_MAP_R | WINED3D_RESOURCE_ACCESS_MAP_W;
}

static inline unsigned int wined3daccess_from_d3dpool(D3DPOOL pool, unsigned int usage)
{
    unsigned int access;

    switch (pool)
    {
        case D3DPOOL_DEFAULT:
            access = WINED3D_RESOURCE_ACCESS_GPU;
            break;
        case D3DPOOL_MANAGED:
            access = WINED3D_RESOURCE_ACCESS_GPU | WINED3D_RESOURCE_ACCESS_CPU;
            break;
        case D3DPOOL_SYSTEMMEM:
        case D3DPOOL_SCRATCH:
            access = WINED3D_RESOURCE_ACCESS_CPU;
            break;
        default:
            access = 0;
    }
    if (pool != D3DPOOL_DEFAULT || usage & D3DUSAGE_DYNAMIC)
        access |= map_access_from_usage(usage);
    return access;
}

static inline unsigned int wined3d_usage_from_d3d(D3DPOOL pool, DWORD usage)
{
    usage &= WINED3DUSAGE_MASK;
    if (pool == D3DPOOL_SCRATCH)
        usage |= WINED3DUSAGE_SCRATCH;
    else if (pool == D3DPOOL_MANAGED)
        usage |= WINED3DUSAGE_MANAGED;
    return usage;
}

static inline unsigned int wined3d_bind_flags_from_d3d9_usage(DWORD usage)
{
    unsigned int bind_flags = 0;

    if (usage & D3DUSAGE_RENDERTARGET)
        bind_flags |= WINED3D_BIND_RENDER_TARGET;
    if (usage & D3DUSAGE_DEPTHSTENCIL)
        bind_flags |= WINED3D_BIND_DEPTH_STENCIL;

    return bind_flags;
}

static inline enum wined3d_multisample_type wined3d_multisample_type_from_d3d(D3DMULTISAMPLE_TYPE type)
{
    return (enum wined3d_multisample_type)type;
}

static inline enum wined3d_device_type wined3d_device_type_from_d3d(D3DDEVTYPE type)
{
    return (enum wined3d_device_type)type;
}

static inline enum wined3d_scanline_ordering wined3d_scanline_ordering_from_d3d(D3DSCANLINEORDERING ordering)
{
    return (enum wined3d_scanline_ordering)ordering;
}

#endif /* __WINE_D3D9_PRIVATE_H */
