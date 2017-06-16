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

#include <config.h>

#include <assert.h>
#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#define COBJMACROS
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wingdi.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(d3d9);

#include <d3d9.h>
#include <wine/wined3d.h>

#define D3DPRESENTFLAGS_MASK 0x00000fffu

extern const struct wined3d_parent_ops d3d9_null_wined3d_parent_ops DECLSPEC_HIDDEN;

HRESULT vdecl_convert_fvf(DWORD FVF, D3DVERTEXELEMENT9 **ppVertexElements) DECLSPEC_HIDDEN;
D3DFORMAT d3dformat_from_wined3dformat(enum wined3d_format_id format) DECLSPEC_HIDDEN;
BOOL is_gdi_compat_wined3dformat(enum wined3d_format_id format) DECLSPEC_HIDDEN;
enum wined3d_format_id wined3dformat_from_d3dformat(D3DFORMAT format) DECLSPEC_HIDDEN;
void present_parameters_from_wined3d_swapchain_desc(D3DPRESENT_PARAMETERS *present_parameters,
        const struct wined3d_swapchain_desc *swapchain_desc) DECLSPEC_HIDDEN;
void d3dcaps_from_wined3dcaps(D3DCAPS9 *caps, const WINED3DCAPS *wined3d_caps) DECLSPEC_HIDDEN;

struct d3d9
{
    IDirect3D9Ex IDirect3D9Ex_iface;
    LONG refcount;
    struct wined3d *wined3d;
    BOOL extended;
};

BOOL d3d9_init(struct d3d9 *d3d9, BOOL extended) DECLSPEC_HIDDEN;

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
    struct wined3d_device_parent device_parent;
    LONG refcount;
    struct wined3d_device *wined3d_device;
    struct d3d9 *d3d_parent;

    struct fvf_declaration *fvf_decls;
    UINT fvf_decl_count, fvf_decl_size;

    struct wined3d_buffer *vertex_buffer;
    UINT vertex_buffer_size;
    UINT vertex_buffer_pos;
    struct wined3d_buffer *index_buffer;
    UINT index_buffer_size;
    UINT index_buffer_pos;

    LONG device_state;
    BOOL in_destruction;
    BOOL in_scene;
    BOOL has_vertex_declaration;

    UINT implicit_swapchain_count;
    struct d3d9_swapchain **implicit_swapchains;
};

HRESULT device_init(struct d3d9_device *device, struct d3d9 *parent, struct wined3d *wined3d,
        UINT adapter, D3DDEVTYPE device_type, HWND focus_window, DWORD flags,
        D3DPRESENT_PARAMETERS *parameters, D3DDISPLAYMODEEX *mode) DECLSPEC_HIDDEN;

struct d3d9_resource
{
    LONG refcount;
    struct wined3d_private_store private_store;
};

void d3d9_resource_cleanup(struct d3d9_resource *resource) DECLSPEC_HIDDEN;
HRESULT d3d9_resource_free_private_data(struct d3d9_resource *resource, const GUID *guid) DECLSPEC_HIDDEN;
HRESULT d3d9_resource_get_private_data(struct d3d9_resource *resource, const GUID *guid,
        void *data, DWORD *data_size) DECLSPEC_HIDDEN;
void d3d9_resource_init(struct d3d9_resource *resource) DECLSPEC_HIDDEN;
HRESULT d3d9_resource_set_private_data(struct d3d9_resource *resource, const GUID *guid,
        const void *data, DWORD data_size, DWORD flags) DECLSPEC_HIDDEN;

struct d3d9_volume
{
    IDirect3DVolume9 IDirect3DVolume9_iface;
    struct d3d9_resource resource;
    struct wined3d_texture *wined3d_texture;
    unsigned int sub_resource_idx;
    struct d3d9_texture *texture;
};

void volume_init(struct d3d9_volume *volume, struct wined3d_texture *wined3d_texture,
        unsigned int sub_resource_idx, const struct wined3d_parent_ops **parent_ops) DECLSPEC_HIDDEN;

struct d3d9_swapchain
{
    IDirect3DSwapChain9Ex IDirect3DSwapChain9Ex_iface;
    LONG refcount;
    struct wined3d_swapchain *wined3d_swapchain;
    IDirect3DDevice9Ex *parent_device;
};

HRESULT d3d9_swapchain_create(struct d3d9_device *device, struct wined3d_swapchain_desc *desc,
        struct d3d9_swapchain **swapchain) DECLSPEC_HIDDEN;

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
};

struct wined3d_rendertarget_view *d3d9_surface_acquire_rendertarget_view(struct d3d9_surface *surface) DECLSPEC_HIDDEN;
struct d3d9_device *d3d9_surface_get_device(const struct d3d9_surface *surface) DECLSPEC_HIDDEN;
void d3d9_surface_release_rendertarget_view(struct d3d9_surface *surface,
        struct wined3d_rendertarget_view *rtv) DECLSPEC_HIDDEN;
void surface_init(struct d3d9_surface *surface, struct wined3d_texture *wined3d_texture,
        unsigned int sub_resource_idx, const struct wined3d_parent_ops **parent_ops) DECLSPEC_HIDDEN;
struct d3d9_surface *unsafe_impl_from_IDirect3DSurface9(IDirect3DSurface9 *iface) DECLSPEC_HIDDEN;

struct d3d9_vertexbuffer
{
    IDirect3DVertexBuffer9 IDirect3DVertexBuffer9_iface;
    struct d3d9_resource resource;
    struct wined3d_buffer *wined3d_buffer;
    IDirect3DDevice9Ex *parent_device;
    DWORD fvf;
};

HRESULT vertexbuffer_init(struct d3d9_vertexbuffer *buffer, struct d3d9_device *device,
        UINT size, UINT usage, DWORD fvf, D3DPOOL pool) DECLSPEC_HIDDEN;
struct d3d9_vertexbuffer *unsafe_impl_from_IDirect3DVertexBuffer9(IDirect3DVertexBuffer9 *iface) DECLSPEC_HIDDEN;

struct d3d9_indexbuffer
{
    IDirect3DIndexBuffer9 IDirect3DIndexBuffer9_iface;
    struct d3d9_resource resource;
    struct wined3d_buffer *wined3d_buffer;
    IDirect3DDevice9Ex *parent_device;
    enum wined3d_format_id format;
};

HRESULT indexbuffer_init(struct d3d9_indexbuffer *buffer, struct d3d9_device *device,
        UINT size, DWORD usage, D3DFORMAT format, D3DPOOL pool) DECLSPEC_HIDDEN;
struct d3d9_indexbuffer *unsafe_impl_from_IDirect3DIndexBuffer9(IDirect3DIndexBuffer9 *iface) DECLSPEC_HIDDEN;

struct d3d9_texture
{
    IDirect3DBaseTexture9 IDirect3DBaseTexture9_iface;
    struct d3d9_resource resource;
    struct wined3d_texture *wined3d_texture;
    IDirect3DDevice9Ex *parent_device;
    struct list rtv_list;
};

HRESULT cubetexture_init(struct d3d9_texture *texture, struct d3d9_device *device,
        UINT edge_length, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool) DECLSPEC_HIDDEN;
HRESULT texture_init(struct d3d9_texture *texture, struct d3d9_device *device,
        UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool) DECLSPEC_HIDDEN;
HRESULT volumetexture_init(struct d3d9_texture *texture, struct d3d9_device *device,
        UINT width, UINT height, UINT depth, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool) DECLSPEC_HIDDEN;
struct d3d9_texture *unsafe_impl_from_IDirect3DBaseTexture9(IDirect3DBaseTexture9 *iface) DECLSPEC_HIDDEN;

struct d3d9_stateblock
{
    IDirect3DStateBlock9 IDirect3DStateBlock9_iface;
    LONG refcount;
    struct wined3d_stateblock *wined3d_stateblock;
    IDirect3DDevice9Ex *parent_device;
};

HRESULT stateblock_init(struct d3d9_stateblock *stateblock, struct d3d9_device *device,
        D3DSTATEBLOCKTYPE type, struct wined3d_stateblock *wined3d_stateblock) DECLSPEC_HIDDEN;

struct d3d9_vertex_declaration
{
    IDirect3DVertexDeclaration9 IDirect3DVertexDeclaration9_iface;
    LONG refcount;
    D3DVERTEXELEMENT9 *elements;
    UINT element_count;
    struct wined3d_vertex_declaration *wined3d_declaration;
    DWORD fvf;
    IDirect3DDevice9Ex *parent_device;
};

HRESULT d3d9_vertex_declaration_create(struct d3d9_device *device,
        const D3DVERTEXELEMENT9 *elements, struct d3d9_vertex_declaration **declaration) DECLSPEC_HIDDEN;
struct d3d9_vertex_declaration *unsafe_impl_from_IDirect3DVertexDeclaration9(
        IDirect3DVertexDeclaration9 *iface) DECLSPEC_HIDDEN;

struct d3d9_vertexshader
{
    IDirect3DVertexShader9 IDirect3DVertexShader9_iface;
    LONG refcount;
    struct wined3d_shader *wined3d_shader;
    IDirect3DDevice9Ex *parent_device;
};

HRESULT vertexshader_init(struct d3d9_vertexshader *shader,
        struct d3d9_device *device, const DWORD *byte_code) DECLSPEC_HIDDEN;
struct d3d9_vertexshader *unsafe_impl_from_IDirect3DVertexShader9(IDirect3DVertexShader9 *iface) DECLSPEC_HIDDEN;

#define D3D9_MAX_VERTEX_SHADER_CONSTANTF 256
#define D3D9_MAX_SIMULTANEOUS_RENDERTARGETS 4

struct d3d9_pixelshader
{
    IDirect3DPixelShader9 IDirect3DPixelShader9_iface;
    LONG refcount;
    struct wined3d_shader *wined3d_shader;
    IDirect3DDevice9Ex *parent_device;
};

HRESULT pixelshader_init(struct d3d9_pixelshader *shader,
        struct d3d9_device *device, const DWORD *byte_code) DECLSPEC_HIDDEN;
struct d3d9_pixelshader *unsafe_impl_from_IDirect3DPixelShader9(IDirect3DPixelShader9 *iface) DECLSPEC_HIDDEN;

struct d3d9_query
{
    IDirect3DQuery9 IDirect3DQuery9_iface;
    LONG refcount;
    struct wined3d_query *wined3d_query;
    IDirect3DDevice9Ex *parent_device;
    DWORD data_size;
};

HRESULT query_init(struct d3d9_query *query, struct d3d9_device *device, D3DQUERYTYPE type) DECLSPEC_HIDDEN;

static inline struct d3d9_device *impl_from_IDirect3DDevice9Ex(IDirect3DDevice9Ex *iface)
{
    return CONTAINING_RECORD(iface, struct d3d9_device, IDirect3DDevice9Ex_iface);
}

#endif /* __WINE_D3D9_PRIVATE_H */
