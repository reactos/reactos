/*
 * Direct3D 8 private include file
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
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

#ifndef __WINE_D3D8_PRIVATE_H
#define __WINE_D3D8_PRIVATE_H

#include <assert.h>
#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/debug.h"
#include "d3d8.h"
#include "wine/wined3d.h"

#define D3DPRESENTFLAGS_MASK 0x00000fffu

#define D3D8_MAX_VERTEX_SHADER_CONSTANTF 256
#define D3D8_MAX_PIXEL_SHADER_CONSTANTF 8
#define D3D8_MAX_STREAMS 16

#define D3DFMT_RESZ MAKEFOURCC('R','E','S','Z')
#define D3D8_RESZ_CODE 0x7fa05000

/* CreateVertexShader can return > 0xFFFF */
#define VS_HIGHESTFIXEDFXF 0xF0000000

extern const struct wined3d_parent_ops d3d8_null_wined3d_parent_ops;

void d3dcaps_from_wined3dcaps(D3DCAPS8 *caps, const struct wined3d_caps *wined3d_caps,
        unsigned int adapter_ordinal);

struct d3d8
{
    IDirect3D8 IDirect3D8_iface;
    LONG refcount;
    struct wined3d *wined3d;
    struct wined3d_output **wined3d_outputs;
    unsigned int wined3d_output_count;
};

BOOL d3d8_init(struct d3d8 *d3d8);

/*****************************************************************************
 * IDirect3DDevice8 implementation structure
 */

#define D3D8_INITIAL_HANDLE_TABLE_SIZE 64
#define D3D8_INVALID_HANDLE ~0U

enum d3d8_handle_type
{
    D3D8_HANDLE_FREE,
    D3D8_HANDLE_VS,
    D3D8_HANDLE_PS,
    D3D8_HANDLE_SB,
};

struct d3d8_handle_entry
{
    void *object;
    enum d3d8_handle_type type;
};

struct d3d8_handle_table
{
    struct d3d8_handle_entry *entries;
    struct d3d8_handle_entry *free_entries;
    UINT table_size;
    UINT entry_count;
};

struct FvfToDecl
{
    DWORD fvf;
    struct d3d8_vertex_declaration *declaration;
};

enum d3d8_device_state
{
    D3D8_DEVICE_STATE_OK,
    D3D8_DEVICE_STATE_LOST,
    D3D8_DEVICE_STATE_NOT_RESET,
};

struct d3d8_device
{
    /* IUnknown fields */
    IDirect3DDevice8        IDirect3DDevice8_iface;
    struct wined3d_device_parent device_parent;
    LONG                    ref;
    struct wined3d_device  *wined3d_device;
    struct wined3d_device_context *immediate_context;
    unsigned int            adapter_ordinal;
    struct d3d8            *d3d_parent;
    struct                  d3d8_handle_table handle_table;

    /* FVF management */
    struct FvfToDecl       *decls;
    UINT                    numConvertedDecls, declArraySize;

    struct wined3d_streaming_buffer vertex_buffer, index_buffer;

    LONG device_state;
    DWORD sysmem_vb : 16; /* D3D8_MAX_STREAMS */
    DWORD sysmem_ib : 1;
    DWORD in_destruction : 1;
    DWORD padding : 14;

    unsigned int max_user_clip_planes, vs_uniform_count;

    /* The d3d8 API supports only one implicit swapchain (no D3DCREATE_ADAPTERGROUP_DEVICE,
     * no GetSwapchain, GetBackBuffer doesn't accept a swapchain number). */
    struct wined3d_swapchain *implicit_swapchain;

    struct wined3d_stateblock *recording, *state, *update_state;
    const struct wined3d_stateblock_state *stateblock_state;
};

HRESULT device_init(struct d3d8_device *device, struct d3d8 *parent, struct wined3d *wined3d, UINT adapter,
        D3DDEVTYPE device_type, HWND focus_window, DWORD flags, D3DPRESENT_PARAMETERS *parameters);

static inline struct d3d8_device *impl_from_IDirect3DDevice8(IDirect3DDevice8 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d8_device, IDirect3DDevice8_iface);
}

struct d3d8_resource
{
    LONG refcount;
    struct wined3d_private_store private_store;
};

void d3d8_resource_cleanup(struct d3d8_resource *resource);
HRESULT d3d8_resource_free_private_data(struct d3d8_resource *resource, const GUID *guid);
HRESULT d3d8_resource_get_private_data(struct d3d8_resource *resource, const GUID *guid,
        void *data, DWORD *data_size);
void d3d8_resource_init(struct d3d8_resource *resource);
HRESULT d3d8_resource_set_private_data(struct d3d8_resource *resource, const GUID *guid,
        const void *data, DWORD data_size, DWORD flags);

struct d3d8_volume
{
    IDirect3DVolume8 IDirect3DVolume8_iface;
    struct d3d8_resource resource;
    struct wined3d_texture *wined3d_texture;
    unsigned int sub_resource_idx;
    struct d3d8_texture *texture;
};

void volume_init(struct d3d8_volume *volume, struct wined3d_texture *wined3d_texture,
        unsigned int sub_resource_idx, const struct wined3d_parent_ops **parent_ops);

struct d3d8_swapchain
{
    IDirect3DSwapChain8 IDirect3DSwapChain8_iface;
    LONG refcount;
    struct wined3d_swapchain *wined3d_swapchain;
    struct wined3d_swapchain_state_parent state_parent;
    IDirect3DDevice8 *parent_device;
    unsigned int swap_interval;
};

HRESULT d3d8_swapchain_create(struct d3d8_device *device, struct wined3d_swapchain_desc *desc,
        unsigned int swap_interval, struct d3d8_swapchain **swapchain);

struct d3d8_surface
{
    IDirect3DSurface8 IDirect3DSurface8_iface;
    struct d3d8_resource resource;
    struct wined3d_texture *wined3d_texture;
    unsigned int sub_resource_idx;
    struct list rtv_entry;
    struct wined3d_rendertarget_view *wined3d_rtv;
    IDirect3DDevice8 *parent_device;
    IUnknown *container;
    struct d3d8_texture *texture;
    struct wined3d_swapchain *swapchain;
};

struct wined3d_rendertarget_view *d3d8_surface_acquire_rendertarget_view(struct d3d8_surface *surface);
struct d3d8_surface *d3d8_surface_create(struct wined3d_texture *wined3d_texture,
        unsigned int sub_resource_idx, IUnknown *container);
struct d3d8_device *d3d8_surface_get_device(const struct d3d8_surface *surface);
void d3d8_surface_release_rendertarget_view(struct d3d8_surface *surface,
        struct wined3d_rendertarget_view *rtv);
struct d3d8_surface *unsafe_impl_from_IDirect3DSurface8(IDirect3DSurface8 *iface);

struct d3d8_vertexbuffer
{
    IDirect3DVertexBuffer8 IDirect3DVertexBuffer8_iface;
    struct d3d8_resource resource;
    struct wined3d_buffer *wined3d_buffer;
    IDirect3DDevice8 *parent_device;
    struct wined3d_buffer *draw_buffer;
    DWORD fvf, usage;
    bool discarded;
};

HRESULT vertexbuffer_init(struct d3d8_vertexbuffer *buffer, struct d3d8_device *device,
        UINT size, DWORD usage, DWORD fvf, D3DPOOL pool);
struct d3d8_vertexbuffer *unsafe_impl_from_IDirect3DVertexBuffer8(IDirect3DVertexBuffer8 *iface);

struct d3d8_indexbuffer
{
    IDirect3DIndexBuffer8 IDirect3DIndexBuffer8_iface;
    struct d3d8_resource resource;
    struct wined3d_buffer *wined3d_buffer;
    IDirect3DDevice8 *parent_device;
    enum wined3d_format_id format;
    DWORD usage;
    bool sysmem;
    bool discarded;
};

HRESULT indexbuffer_init(struct d3d8_indexbuffer *buffer, struct d3d8_device *device,
        UINT size, DWORD usage, D3DFORMAT format, D3DPOOL pool);
struct d3d8_indexbuffer *unsafe_impl_from_IDirect3DIndexBuffer8(IDirect3DIndexBuffer8 *iface);

struct d3d8_texture
{
    IDirect3DBaseTexture8 IDirect3DBaseTexture8_iface;
    struct d3d8_resource resource;
    struct wined3d_texture *wined3d_texture, *draw_texture;
    struct d3d8_device *parent_device;
    struct list rtv_list;
};

HRESULT d3d8_texture_2d_init(struct d3d8_texture *texture, struct d3d8_device *device, unsigned int width,
        unsigned int height, unsigned int level_count, DWORD usage, D3DFORMAT format, D3DPOOL pool);
HRESULT d3d8_texture_3d_init(struct d3d8_texture *texture, struct d3d8_device *device, unsigned int width,
        unsigned int height, unsigned int depth, unsigned int levels, DWORD usage, D3DFORMAT format, D3DPOOL pool);
HRESULT d3d8_texture_cube_init(struct d3d8_texture *texture, struct d3d8_device *device,
        unsigned int edge_length, unsigned int level_count, DWORD usage, D3DFORMAT format, D3DPOOL pool);

static inline struct wined3d_texture *d3d8_texture_get_draw_texture(struct d3d8_texture *texture)
{
    return texture->draw_texture ? texture->draw_texture : texture->wined3d_texture;
}

struct d3d8_texture *unsafe_impl_from_IDirect3DBaseTexture8(IDirect3DBaseTexture8 *iface);

struct d3d8_vertex_declaration
{
    DWORD *elements;
    DWORD elements_size; /* Size of elements, in bytes */
    DWORD stream_map;
    struct wined3d_vertex_declaration *wined3d_vertex_declaration;
    DWORD shader_handle;
};

void d3d8_vertex_declaration_destroy(struct d3d8_vertex_declaration *declaration);
HRESULT d3d8_vertex_declaration_init(struct d3d8_vertex_declaration *declaration,
        struct d3d8_device *device, const DWORD *elements, DWORD shader_handle);
HRESULT d3d8_vertex_declaration_init_fvf(struct d3d8_vertex_declaration *declaration,
        struct d3d8_device *device, DWORD fvf);

struct d3d8_vertex_shader
{
  struct d3d8_vertex_declaration *vertex_declaration;
  struct wined3d_shader *wined3d_shader;
};

void d3d8_vertex_shader_destroy(struct d3d8_vertex_shader *shader);
HRESULT d3d8_vertex_shader_init(struct d3d8_vertex_shader *shader, struct d3d8_device *device,
        const DWORD *declaration, const DWORD *byte_code, DWORD shader_handle, DWORD usage);

struct d3d8_pixel_shader
{
    DWORD handle;
    struct wined3d_shader *wined3d_shader;
};

void d3d8_pixel_shader_destroy(struct d3d8_pixel_shader *shader);
HRESULT d3d8_pixel_shader_init(struct d3d8_pixel_shader *shader, struct d3d8_device *device,
        const DWORD *byte_code, DWORD shader_handle);

D3DFORMAT d3dformat_from_wined3dformat(enum wined3d_format_id format);
enum wined3d_format_id wined3dformat_from_d3dformat(D3DFORMAT format);
unsigned int wined3dmapflags_from_d3dmapflags(unsigned int flags, unsigned int usage);
void load_local_constants(const DWORD *d3d8_elements, struct wined3d_shader *wined3d_vertex_shader);
size_t parse_token(const DWORD *pToken);

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
    return usage | WINED3DUSAGE_VIDMEM_ACCOUNTING;
}

static inline unsigned int wined3d_bind_flags_from_d3d8_usage(DWORD usage)
{
    unsigned int bind_flags = 0;

    if (usage & D3DUSAGE_RENDERTARGET)
        bind_flags |= WINED3D_BIND_RENDER_TARGET;
    if (usage & D3DUSAGE_DEPTHSTENCIL)
        bind_flags |= WINED3D_BIND_DEPTH_STENCIL;

    return bind_flags;
}

static inline D3DMULTISAMPLE_TYPE d3dmultisample_type_from_wined3d(enum wined3d_multisample_type type)
{
    return (D3DMULTISAMPLE_TYPE)type;
}

static inline enum wined3d_device_type wined3d_device_type_from_d3d(D3DDEVTYPE type)
{
    return (enum wined3d_device_type)type;
}

#endif /* __WINE_D3DX8_PRIVATE_H */
