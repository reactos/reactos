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
#include <wingdi.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(d3d8);

#include <d3d8.h>
#include <wine/wined3d.h>

/* CreateVertexShader can return > 0xFFFF */
#define VS_HIGHESTFIXEDFXF 0xF0000000

/* ===========================================================================
    Macros
   =========================================================================== */
/* Not nice, but it lets wined3d support different versions of directx */
#define WINECAPSTOD3D8CAPS(_pD3D8Caps, _pWineCaps) \
    _pD3D8Caps->DeviceType                        = (D3DDEVTYPE) _pWineCaps->DeviceType; \
    _pD3D8Caps->AdapterOrdinal                    = _pWineCaps->AdapterOrdinal; \
    _pD3D8Caps->Caps                              = _pWineCaps->Caps; \
    _pD3D8Caps->Caps2                             = _pWineCaps->Caps2; \
    _pD3D8Caps->Caps3                             = _pWineCaps->Caps3; \
    _pD3D8Caps->PresentationIntervals             = _pWineCaps->PresentationIntervals; \
    _pD3D8Caps->CursorCaps                        = _pWineCaps->CursorCaps; \
    _pD3D8Caps->DevCaps                           = _pWineCaps->DevCaps; \
    _pD3D8Caps->PrimitiveMiscCaps                 = _pWineCaps->PrimitiveMiscCaps; \
    _pD3D8Caps->RasterCaps                        = _pWineCaps->RasterCaps; \
    _pD3D8Caps->ZCmpCaps                          = _pWineCaps->ZCmpCaps; \
    _pD3D8Caps->SrcBlendCaps                      = _pWineCaps->SrcBlendCaps; \
    _pD3D8Caps->DestBlendCaps                     = _pWineCaps->DestBlendCaps; \
    _pD3D8Caps->AlphaCmpCaps                      = _pWineCaps->AlphaCmpCaps; \
    _pD3D8Caps->ShadeCaps                         = _pWineCaps->ShadeCaps; \
    _pD3D8Caps->TextureCaps                       = _pWineCaps->TextureCaps; \
    _pD3D8Caps->TextureFilterCaps                 = _pWineCaps->TextureFilterCaps; \
    _pD3D8Caps->CubeTextureFilterCaps             = _pWineCaps->CubeTextureFilterCaps; \
    _pD3D8Caps->VolumeTextureFilterCaps           = _pWineCaps->VolumeTextureFilterCaps; \
    _pD3D8Caps->TextureAddressCaps                = _pWineCaps->TextureAddressCaps; \
    _pD3D8Caps->VolumeTextureAddressCaps          = _pWineCaps->VolumeTextureAddressCaps; \
    _pD3D8Caps->LineCaps                          = _pWineCaps->LineCaps; \
    _pD3D8Caps->MaxTextureWidth                   = _pWineCaps->MaxTextureWidth; \
    _pD3D8Caps->MaxTextureHeight                  = _pWineCaps->MaxTextureHeight; \
    _pD3D8Caps->MaxVolumeExtent                   = _pWineCaps->MaxVolumeExtent; \
    _pD3D8Caps->MaxTextureRepeat                  = _pWineCaps->MaxTextureRepeat; \
    _pD3D8Caps->MaxTextureAspectRatio             = _pWineCaps->MaxTextureAspectRatio; \
    _pD3D8Caps->MaxAnisotropy                     = _pWineCaps->MaxAnisotropy; \
    _pD3D8Caps->MaxVertexW                        = _pWineCaps->MaxVertexW; \
    _pD3D8Caps->GuardBandLeft                     = _pWineCaps->GuardBandLeft; \
    _pD3D8Caps->GuardBandTop                      = _pWineCaps->GuardBandTop; \
    _pD3D8Caps->GuardBandRight                    = _pWineCaps->GuardBandRight; \
    _pD3D8Caps->GuardBandBottom                   = _pWineCaps->GuardBandBottom; \
    _pD3D8Caps->ExtentsAdjust                     = _pWineCaps->ExtentsAdjust; \
    _pD3D8Caps->StencilCaps                       = _pWineCaps->StencilCaps; \
    _pD3D8Caps->FVFCaps                           = _pWineCaps->FVFCaps; \
    _pD3D8Caps->TextureOpCaps                     = _pWineCaps->TextureOpCaps; \
    _pD3D8Caps->MaxTextureBlendStages             = _pWineCaps->MaxTextureBlendStages; \
    _pD3D8Caps->MaxSimultaneousTextures           = _pWineCaps->MaxSimultaneousTextures; \
    _pD3D8Caps->VertexProcessingCaps              = _pWineCaps->VertexProcessingCaps; \
    _pD3D8Caps->MaxActiveLights                   = _pWineCaps->MaxActiveLights; \
    _pD3D8Caps->MaxUserClipPlanes                 = _pWineCaps->MaxUserClipPlanes; \
    _pD3D8Caps->MaxVertexBlendMatrices            = _pWineCaps->MaxVertexBlendMatrices; \
    _pD3D8Caps->MaxVertexBlendMatrixIndex         = _pWineCaps->MaxVertexBlendMatrixIndex; \
    _pD3D8Caps->MaxPointSize                      = _pWineCaps->MaxPointSize; \
    _pD3D8Caps->MaxPrimitiveCount                 = _pWineCaps->MaxPrimitiveCount; \
    _pD3D8Caps->MaxVertexIndex                    = _pWineCaps->MaxVertexIndex; \
    _pD3D8Caps->MaxStreams                        = _pWineCaps->MaxStreams; \
    _pD3D8Caps->MaxStreamStride                   = _pWineCaps->MaxStreamStride; \
    _pD3D8Caps->VertexShaderVersion               = _pWineCaps->VertexShaderVersion; \
    _pD3D8Caps->MaxVertexShaderConst              = _pWineCaps->MaxVertexShaderConst; \
    _pD3D8Caps->PixelShaderVersion                = _pWineCaps->PixelShaderVersion; \
    _pD3D8Caps->MaxPixelShaderValue               = _pWineCaps->PixelShader1xMaxValue;

void fixup_caps(WINED3DCAPS *pWineCaps) DECLSPEC_HIDDEN;

struct d3d8
{
    IDirect3D8 IDirect3D8_iface;
    LONG refcount;
    struct wined3d *wined3d;
};

BOOL d3d8_init(struct d3d8 *d3d8) DECLSPEC_HIDDEN;

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
    IDirect3D8             *d3d_parent;
    struct                  d3d8_handle_table handle_table;

    /* FVF management */
    struct FvfToDecl       *decls;
    UINT                    numConvertedDecls, declArraySize;

    /* User data draws */
    struct wined3d_buffer *vertex_buffer;
    UINT                   vertex_buffer_size;
    UINT                   vertex_buffer_pos;
    struct wined3d_buffer *index_buffer;
    UINT                   index_buffer_size;
    UINT                   index_buffer_pos;

    LONG device_state;
    /* Avoids recursion with nested ReleaseRef to 0 */
    BOOL                    inDestruction;

    /* The d3d8 API supports only one implicit swapchain (no D3DCREATE_ADAPTERGROUP_DEVICE,
     * no GetSwapchain, GetBackBuffer doesn't accept a swapchain number). */
    struct d3d8_swapchain   *implicit_swapchain;
};

HRESULT device_init(struct d3d8_device *device, struct d3d8 *parent, struct wined3d *wined3d, UINT adapter,
        D3DDEVTYPE device_type, HWND focus_window, DWORD flags, D3DPRESENT_PARAMETERS *parameters) DECLSPEC_HIDDEN;

static inline struct d3d8_device *impl_from_IDirect3DDevice8(IDirect3DDevice8 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d8_device, IDirect3DDevice8_iface);
}

struct d3d8_resource
{
    LONG refcount;
    struct wined3d_private_store private_store;
};

void d3d8_resource_cleanup(struct d3d8_resource *resource) DECLSPEC_HIDDEN;
HRESULT d3d8_resource_free_private_data(struct d3d8_resource *resource, const GUID *guid) DECLSPEC_HIDDEN;
HRESULT d3d8_resource_get_private_data(struct d3d8_resource *resource, const GUID *guid,
        void *data, DWORD *data_size) DECLSPEC_HIDDEN;
void d3d8_resource_init(struct d3d8_resource *resource) DECLSPEC_HIDDEN;
HRESULT d3d8_resource_set_private_data(struct d3d8_resource *resource, const GUID *guid,
        const void *data, DWORD data_size, DWORD flags) DECLSPEC_HIDDEN;

struct d3d8_volume
{
    IDirect3DVolume8 IDirect3DVolume8_iface;
    struct d3d8_resource resource;
    struct wined3d_texture *wined3d_texture;
    unsigned int sub_resource_idx;
    struct d3d8_texture *texture;
};

void volume_init(struct d3d8_volume *volume, struct wined3d_texture *wined3d_texture,
        unsigned int sub_resource_idx, const struct wined3d_parent_ops **parent_ops) DECLSPEC_HIDDEN;

struct d3d8_swapchain
{
    IDirect3DSwapChain8 IDirect3DSwapChain8_iface;
    LONG refcount;
    struct wined3d_swapchain *wined3d_swapchain;
    IDirect3DDevice8 *parent_device;
};

HRESULT d3d8_swapchain_create(struct d3d8_device *device, struct wined3d_swapchain_desc *desc,
        struct d3d8_swapchain **swapchain) DECLSPEC_HIDDEN;

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
};

struct wined3d_rendertarget_view *d3d8_surface_get_rendertarget_view(struct d3d8_surface *surface) DECLSPEC_HIDDEN;
void surface_init(struct d3d8_surface *surface, struct wined3d_texture *wined3d_texture, unsigned int sub_resource_idx,
        const struct wined3d_parent_ops **parent_ops) DECLSPEC_HIDDEN;
struct d3d8_surface *unsafe_impl_from_IDirect3DSurface8(IDirect3DSurface8 *iface) DECLSPEC_HIDDEN;

struct d3d8_vertexbuffer
{
    IDirect3DVertexBuffer8 IDirect3DVertexBuffer8_iface;
    struct d3d8_resource resource;
    struct wined3d_buffer *wined3d_buffer;
    IDirect3DDevice8 *parent_device;
    DWORD fvf;
};

HRESULT vertexbuffer_init(struct d3d8_vertexbuffer *buffer, struct d3d8_device *device,
        UINT size, DWORD usage, DWORD fvf, D3DPOOL pool) DECLSPEC_HIDDEN;
struct d3d8_vertexbuffer *unsafe_impl_from_IDirect3DVertexBuffer8(IDirect3DVertexBuffer8 *iface) DECLSPEC_HIDDEN;

struct d3d8_indexbuffer
{
    IDirect3DIndexBuffer8 IDirect3DIndexBuffer8_iface;
    struct d3d8_resource resource;
    struct wined3d_buffer *wined3d_buffer;
    IDirect3DDevice8 *parent_device;
    enum wined3d_format_id format;
};

HRESULT indexbuffer_init(struct d3d8_indexbuffer *buffer, struct d3d8_device *device,
        UINT size, DWORD usage, D3DFORMAT format, D3DPOOL pool) DECLSPEC_HIDDEN;
struct d3d8_indexbuffer *unsafe_impl_from_IDirect3DIndexBuffer8(IDirect3DIndexBuffer8 *iface) DECLSPEC_HIDDEN;

struct d3d8_texture
{
    IDirect3DBaseTexture8 IDirect3DBaseTexture8_iface;
    struct d3d8_resource resource;
    struct wined3d_texture *wined3d_texture;
    IDirect3DDevice8 *parent_device;
    struct list rtv_list;
};

HRESULT cubetexture_init(struct d3d8_texture *texture, struct d3d8_device *device,
        UINT edge_length, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool) DECLSPEC_HIDDEN;
HRESULT texture_init(struct d3d8_texture *texture, struct d3d8_device *device,
        UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool) DECLSPEC_HIDDEN;
HRESULT volumetexture_init(struct d3d8_texture *texture, struct d3d8_device *device,
        UINT width, UINT height, UINT depth, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool) DECLSPEC_HIDDEN;
struct d3d8_texture *unsafe_impl_from_IDirect3DBaseTexture8(IDirect3DBaseTexture8 *iface) DECLSPEC_HIDDEN;

struct d3d8_vertex_declaration
{
    DWORD *elements;
    DWORD elements_size; /* Size of elements, in bytes */
    struct wined3d_vertex_declaration *wined3d_vertex_declaration;
    DWORD shader_handle;
};

void d3d8_vertex_declaration_destroy(struct d3d8_vertex_declaration *declaration) DECLSPEC_HIDDEN;
HRESULT d3d8_vertex_declaration_init(struct d3d8_vertex_declaration *declaration,
        struct d3d8_device *device, const DWORD *elements, DWORD shader_handle) DECLSPEC_HIDDEN;
HRESULT d3d8_vertex_declaration_init_fvf(struct d3d8_vertex_declaration *declaration,
        struct d3d8_device *device, DWORD fvf) DECLSPEC_HIDDEN;

struct d3d8_vertex_shader
{
  struct d3d8_vertex_declaration *vertex_declaration;
  struct wined3d_shader *wined3d_shader;
};

void d3d8_vertex_shader_destroy(struct d3d8_vertex_shader *shader) DECLSPEC_HIDDEN;
HRESULT d3d8_vertex_shader_init(struct d3d8_vertex_shader *shader, struct d3d8_device *device,
        const DWORD *declaration, const DWORD *byte_code, DWORD shader_handle, DWORD usage) DECLSPEC_HIDDEN;

#define D3D8_MAX_VERTEX_SHADER_CONSTANTF 256

struct d3d8_pixel_shader
{
    DWORD handle;
    struct wined3d_shader *wined3d_shader;
};

void d3d8_pixel_shader_destroy(struct d3d8_pixel_shader *shader) DECLSPEC_HIDDEN;
HRESULT d3d8_pixel_shader_init(struct d3d8_pixel_shader *shader, struct d3d8_device *device,
        const DWORD *byte_code, DWORD shader_handle) DECLSPEC_HIDDEN;

D3DFORMAT d3dformat_from_wined3dformat(enum wined3d_format_id format) DECLSPEC_HIDDEN;
enum wined3d_format_id wined3dformat_from_d3dformat(D3DFORMAT format) DECLSPEC_HIDDEN;
void load_local_constants(const DWORD *d3d8_elements, struct wined3d_shader *wined3d_vertex_shader) DECLSPEC_HIDDEN;
size_t parse_token(const DWORD *pToken) DECLSPEC_HIDDEN;

#endif /* __WINE_D3DX8_PRIVATE_H */
