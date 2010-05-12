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

#include <stdarg.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/debug.h"
#include "d3d8.h"
#include "wine/wined3d.h"

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

/* Direct3D8 Interfaces: */
typedef struct IDirect3DBaseTexture8Impl IDirect3DBaseTexture8Impl;
typedef struct IDirect3DVolumeTexture8Impl IDirect3DVolumeTexture8Impl;
typedef struct IDirect3D8Impl IDirect3D8Impl;
typedef struct IDirect3DDevice8Impl IDirect3DDevice8Impl;
typedef struct IDirect3DTexture8Impl IDirect3DTexture8Impl;
typedef struct IDirect3DCubeTexture8Impl IDirect3DCubeTexture8Impl;
typedef struct IDirect3DIndexBuffer8Impl IDirect3DIndexBuffer8Impl;
typedef struct IDirect3DSurface8Impl IDirect3DSurface8Impl;
typedef struct IDirect3DSwapChain8Impl IDirect3DSwapChain8Impl;
typedef struct IDirect3DResource8Impl IDirect3DResource8Impl;
typedef struct IDirect3DVolume8Impl IDirect3DVolume8Impl;
typedef struct IDirect3DVertexBuffer8Impl IDirect3DVertexBuffer8Impl;
typedef struct IDirect3DVertexShader8Impl IDirect3DVertexShader8Impl;

/* ===========================================================================
    The interfaces themselves
   =========================================================================== */

/* ---------- */
/* IDirect3D8 */
/* ---------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3D8Vtbl Direct3D8_Vtbl DECLSPEC_HIDDEN;

/*****************************************************************************
 * IDirect3D implementation structure
 */
struct IDirect3D8Impl
{
    /* IUnknown fields */
    const IDirect3D8Vtbl   *lpVtbl;
    LONG                    ref;

    /* The WineD3D device */
    IWineD3D               *WineD3D;
};

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
    struct IDirect3DVertexDeclaration8 *decl;
};

struct IDirect3DDevice8Impl
{
    /* IUnknown fields */
    const IDirect3DDevice8Vtbl   *lpVtbl;
    const IWineD3DDeviceParentVtbl *device_parent_vtbl;
    LONG                         ref;
/* But what about baseVertexIndex in state blocks? hmm... it may be a better idea to pass this to wined3d */
    IWineD3DDevice               *WineD3DDevice;
    struct d3d8_handle_table handle_table;

    /* FVF management */
    struct FvfToDecl       *decls;
    UINT                    numConvertedDecls, declArraySize;

    /* Avoids recursion with nested ReleaseRef to 0 */
    BOOL                          inDestruction;
};

HRESULT device_init(IDirect3DDevice8Impl *device, IWineD3D *wined3d, UINT adapter,
        D3DDEVTYPE device_type, HWND focus_window, DWORD flags, D3DPRESENT_PARAMETERS *parameters) DECLSPEC_HIDDEN;

/* ---------------- */
/* IDirect3DVolume8 */
/* ---------------- */

/*****************************************************************************
 * IDirect3DVolume8 implementation structure
 */
struct IDirect3DVolume8Impl
{
    /* IUnknown fields */
    const IDirect3DVolume8Vtbl *lpVtbl;
    LONG                        ref;

    /* IDirect3DVolume8 fields */
    IWineD3DVolume             *wineD3DVolume;

    /* The volume container */
    IUnknown                    *container;

    /* If set forward refcounting to this object */
    IUnknown                    *forwardReference;
};

HRESULT volume_init(IDirect3DVolume8Impl *volume, IDirect3DDevice8Impl *device, UINT width, UINT height,
        UINT depth, DWORD usage, WINED3DFORMAT format, WINED3DPOOL pool) DECLSPEC_HIDDEN;

/* ------------------- */
/* IDirect3DSwapChain8 */
/* ------------------- */

/*****************************************************************************
 * IDirect3DSwapChain8 implementation structure
 */
struct IDirect3DSwapChain8Impl
{
    /* IUnknown fields */
    const IDirect3DSwapChain8Vtbl *lpVtbl;
    LONG                           ref;

    /* IDirect3DSwapChain8 fields */
    IWineD3DSwapChain             *wineD3DSwapChain;

    /* Parent reference */
    LPDIRECT3DDEVICE8              parentDevice;
};

HRESULT swapchain_init(IDirect3DSwapChain8Impl *swapchain, IDirect3DDevice8Impl *device,
        D3DPRESENT_PARAMETERS *present_parameters) DECLSPEC_HIDDEN;

/* ----------------- */
/* IDirect3DSurface8 */
/* ----------------- */

/*****************************************************************************
 * IDirect3DSurface8 implementation structure
 */
struct IDirect3DSurface8Impl
{
    /* IUnknown fields */
    const IDirect3DSurface8Vtbl *lpVtbl;
    LONG                         ref;

    /* IDirect3DSurface8 fields */
    IWineD3DSurface             *wineD3DSurface;

    /* Parent reference */
    LPDIRECT3DDEVICE8                  parentDevice;

    /* The surface container */
    IUnknown                    *container;

    /* If set forward refcounting to this object */
    IUnknown                    *forwardReference;
};

HRESULT surface_init(IDirect3DSurface8Impl *surface, IDirect3DDevice8Impl *device,
        UINT width, UINT height, D3DFORMAT format, BOOL lockable, BOOL discard, UINT level,
        DWORD usage, D3DPOOL pool, D3DMULTISAMPLE_TYPE multisample_type, DWORD multisample_quality) DECLSPEC_HIDDEN;

struct IDirect3DResource8Impl
{
    /* IUnknown fields */
    const IDirect3DResource8Vtbl *lpVtbl;
    LONG                          ref;

    /* IDirect3DResource8 fields */
    IWineD3DResource             *wineD3DResource;
};

/* ---------------------- */
/* IDirect3DVertexBuffer8 */
/* ---------------------- */

/*****************************************************************************
 * IDirect3DVertexBuffer8 implementation structure
 */
struct IDirect3DVertexBuffer8Impl
{
    /* IUnknown fields */
    const IDirect3DVertexBuffer8Vtbl *lpVtbl;
    LONG                              ref;

    /* IDirect3DResource8 fields */
    IWineD3DBuffer *wineD3DVertexBuffer;

    /* Parent reference */
    LPDIRECT3DDEVICE8                 parentDevice;

    DWORD                             fvf;
};

HRESULT vertexbuffer_init(IDirect3DVertexBuffer8Impl *buffer, IDirect3DDevice8Impl *device,
        UINT size, DWORD usage, DWORD fvf, D3DPOOL pool) DECLSPEC_HIDDEN;

/* --------------------- */
/* IDirect3DIndexBuffer8 */
/* --------------------- */

/*****************************************************************************
 * IDirect3DIndexBuffer8 implementation structure
 */
struct IDirect3DIndexBuffer8Impl
{
    /* IUnknown fields */
    const IDirect3DIndexBuffer8Vtbl *lpVtbl;
    LONG                             ref;

    /* IDirect3DResource8 fields */
    IWineD3DBuffer                  *wineD3DIndexBuffer;

    /* Parent reference */
    LPDIRECT3DDEVICE8                parentDevice;

    WINED3DFORMAT                    format;
};

HRESULT indexbuffer_init(IDirect3DIndexBuffer8Impl *buffer, IDirect3DDevice8Impl *device,
        UINT size, DWORD usage, D3DFORMAT format, D3DPOOL pool) DECLSPEC_HIDDEN;

/* --------------------- */
/* IDirect3DBaseTexture8 */
/* --------------------- */

/*****************************************************************************
 * IDirect3DBaseTexture8 implementation structure
 */
struct IDirect3DBaseTexture8Impl
{
    /* IUnknown fields */
    const IDirect3DBaseTexture8Vtbl *lpVtbl;
    LONG                   ref;

    /* IDirect3DResource8 fields */
    IWineD3DBaseTexture             *wineD3DBaseTexture;
};

/* --------------------- */
/* IDirect3DCubeTexture8 */
/* --------------------- */

/*****************************************************************************
 * IDirect3DCubeTexture8 implementation structure
 */
struct IDirect3DCubeTexture8Impl
{
    /* IUnknown fields */
    const IDirect3DCubeTexture8Vtbl *lpVtbl;
    LONG                   ref;

    /* IDirect3DResource8 fields */
    IWineD3DCubeTexture             *wineD3DCubeTexture;

    /* Parent reference */
    LPDIRECT3DDEVICE8                parentDevice;
};

HRESULT cubetexture_init(IDirect3DCubeTexture8Impl *texture, IDirect3DDevice8Impl *device,
        UINT edge_length, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool) DECLSPEC_HIDDEN;

/* ----------------- */
/* IDirect3DTexture8 */
/* ----------------- */

/*****************************************************************************
 * IDirect3DTexture8 implementation structure
 */
struct IDirect3DTexture8Impl
{
    /* IUnknown fields */
    const IDirect3DTexture8Vtbl *lpVtbl;
    LONG                   ref;

    /* IDirect3DResourc8 fields */
    IWineD3DTexture             *wineD3DTexture;

    /* Parent reference */
    LPDIRECT3DDEVICE8            parentDevice;
};

HRESULT texture_init(IDirect3DTexture8Impl *texture, IDirect3DDevice8Impl *device,
        UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool) DECLSPEC_HIDDEN;

/* ----------------------- */
/* IDirect3DVolumeTexture8 */
/* ----------------------- */

/*****************************************************************************
 * IDirect3DVolumeTexture8 implementation structure
 */
struct IDirect3DVolumeTexture8Impl
{
    /* IUnknown fields */
    const IDirect3DVolumeTexture8Vtbl *lpVtbl;
    LONG                   ref;

    /* IDirect3DResource8 fields */
    IWineD3DVolumeTexture             *wineD3DVolumeTexture;

    /* Parent reference */
    LPDIRECT3DDEVICE8                  parentDevice;
};

HRESULT volumetexture_init(IDirect3DVolumeTexture8Impl *texture, IDirect3DDevice8Impl *device,
        UINT width, UINT height, UINT depth, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool) DECLSPEC_HIDDEN;

DEFINE_GUID(IID_IDirect3DVertexDeclaration8,
0x5dd7478d, 0xcbf3, 0x41a6, 0x8c, 0xfd, 0xfd, 0x19, 0x2b, 0x11, 0xc7, 0x90);

DEFINE_GUID(IID_IDirect3DVertexShader8,
0xefc5557e, 0x6265, 0x4613, 0x8a, 0x94, 0x43, 0x85, 0x78, 0x89, 0xeb, 0x36);

DEFINE_GUID(IID_IDirect3DPixelShader8,
0x6d3bdbdc, 0x5b02, 0x4415, 0xb8, 0x52, 0xce, 0x5e, 0x8b, 0xcc, 0xb2, 0x89);

/*****************************************************************************
 * IDirect3DVertexDeclaration8 interface
 */
#define INTERFACE IDirect3DVertexDeclaration8
DECLARE_INTERFACE_(IDirect3DVertexDeclaration8, IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** obj_ptr) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
};
#undef INTERFACE

/*** IUnknown methods ***/
#define IDirect3DVertexDeclaration8_QueryInterface(p,a,b)  (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirect3DVertexDeclaration8_AddRef(p)              (p)->lpVtbl->AddRef(p)
#define IDirect3DVertexDeclaration8_Release(p)             (p)->lpVtbl->Release(p)

typedef struct {
    const IDirect3DVertexDeclaration8Vtbl *lpVtbl;
    LONG ref_count;

    DWORD *elements;
    DWORD elements_size; /* Size of elements, in bytes */

    IWineD3DVertexDeclaration *wined3d_vertex_declaration;
    DWORD shader_handle;
} IDirect3DVertexDeclaration8Impl;

HRESULT vertexdeclaration_init(IDirect3DVertexDeclaration8Impl *declaration,
        IDirect3DDevice8Impl *device, const DWORD *elements, DWORD shader_handle) DECLSPEC_HIDDEN;
HRESULT vertexdeclaration_init_fvf(IDirect3DVertexDeclaration8Impl *declaration,
        IDirect3DDevice8Impl *device, DWORD fvf) DECLSPEC_HIDDEN;

/*****************************************************************************
 * IDirect3DVertexShader8 interface
 */
#define INTERFACE IDirect3DVertexShader8
DECLARE_INTERFACE_(IDirect3DVertexShader8, IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
};
#undef INTERFACE

/*** IUnknown methods ***/
#define IDirect3DVertexShader8_QueryInterface(p,a,b)  (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirect3DVertexShader8_AddRef(p)              (p)->lpVtbl->AddRef(p)
#define IDirect3DVertexShader8_Release(p)             (p)->lpVtbl->Release(p)

/* ------------------------- */
/* IDirect3DVertexShader8Impl */
/* ------------------------- */

/*****************************************************************************
 * IDirect3DPixelShader8 interface
 */
#define INTERFACE IDirect3DPixelShader8
DECLARE_INTERFACE_(IDirect3DPixelShader8,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
};
#undef INTERFACE

/*** IUnknown methods ***/
#define IDirect3DPixelShader8_QueryInterface(p,a,b)  (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirect3DPixelShader8_AddRef(p)              (p)->lpVtbl->AddRef(p)
#define IDirect3DPixelShader8_Release(p)             (p)->lpVtbl->Release(p)

/*****************************************************************************
 * IDirect3DVertexShader implementation structure
 */

struct IDirect3DVertexShader8Impl {
  const IDirect3DVertexShader8Vtbl *lpVtbl;
  LONG ref;

  IDirect3DVertexDeclaration8      *vertex_declaration;
  IWineD3DVertexShader             *wineD3DVertexShader;
};

HRESULT vertexshader_init(IDirect3DVertexShader8Impl *shader, IDirect3DDevice8Impl *device,
        const DWORD *declaration, const DWORD *byte_code, DWORD shader_handle, DWORD usage) DECLSPEC_HIDDEN;

#define D3D8_MAX_VERTEX_SHADER_CONSTANTF 256

/*****************************************************************************
 * IDirect3DPixelShader implementation structure
 */
typedef struct IDirect3DPixelShader8Impl {
    const IDirect3DPixelShader8Vtbl *lpVtbl;
    LONG                             ref;

    DWORD                            handle;
    IWineD3DPixelShader             *wineD3DPixelShader;
} IDirect3DPixelShader8Impl;

HRESULT pixelshader_init(IDirect3DPixelShader8Impl *shader, IDirect3DDevice8Impl *device,
        const DWORD *byte_code, DWORD shader_handle) DECLSPEC_HIDDEN;

/**
 * Internals functions
 *
 * to see how not defined it here
 */
D3DFORMAT d3dformat_from_wined3dformat(WINED3DFORMAT format) DECLSPEC_HIDDEN;
WINED3DFORMAT wined3dformat_from_d3dformat(D3DFORMAT format) DECLSPEC_HIDDEN;
void load_local_constants(const DWORD *d3d8_elements, IWineD3DVertexShader *wined3d_vertex_shader) DECLSPEC_HIDDEN;
size_t parse_token(const DWORD *pToken) DECLSPEC_HIDDEN;

#endif /* __WINE_D3DX8_PRIVATE_H */
