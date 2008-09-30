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
#include "wine/wined3d_interface.h"

/* Device caps */
#define INITIAL_SHADER_HANDLE_TABLE_SIZE        64

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

/** Private Interfaces: */
typedef struct IDirect3DStateBlockImpl IDirect3DStateBlockImpl;
typedef struct IDirect3DVertexShaderImpl IDirect3DVertexShaderImpl;
typedef struct IDirect3DPixelShaderImpl IDirect3DPixelShaderImpl;
typedef struct IDirect3DVertexShaderDeclarationImpl IDirect3DVertexShaderDeclarationImpl;

/* Advance declaration of structures to satisfy compiler */
typedef struct IDirect3DVertexShader8Impl IDirect3DVertexShader8Impl;

/* Global critical section */
extern CRITICAL_SECTION d3d8_cs;

/* ===========================================================================
    The interfaces themselves
   =========================================================================== */

/* ---------- */
/* IDirect3D8 */
/* ---------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3D8Vtbl Direct3D8_Vtbl;

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

/* ---------------- */
/* IDirect3DDevice8 */
/* ---------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DDevice8Vtbl Direct3DDevice8_Vtbl;

/*****************************************************************************
 * IDirect3DDevice8 implementation structure
 */

typedef void * shader_handle;

struct FvfToDecl
{
    DWORD fvf;
    IWineD3DVertexDeclaration *decl;
};

struct IDirect3DDevice8Impl
{
    /* IUnknown fields */
    const IDirect3DDevice8Vtbl   *lpVtbl;
    LONG                         ref;
/* But what about baseVertexIndex in state blocks? hmm... it may be a better idea to pass this to wined3d */
    IWineD3DDevice               *WineD3DDevice;
    DWORD                         shader_handle_table_size;
    DWORD                         allocated_shader_handles;
    shader_handle                *shader_handles;
    shader_handle                *free_shader_handles;

    /* FVF management */
    struct FvfToDecl       *decls;
    UINT                    numConvertedDecls, declArraySize;

    /* Avoids recursion with nested ReleaseRef to 0 */
    BOOL                          inDestruction;
};

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

/* ------------------- */
/* IDirect3DSwapChain8 */
/* ------------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DSwapChain8Vtbl Direct3DSwapChain8_Vtbl;

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

/* ----------------- */
/* IDirect3DSurface8 */
/* ----------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DSurface8Vtbl Direct3DSurface8_Vtbl;

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

    /* Flags an implicit surface */
    BOOL                        isImplicit;
};

/* ------------------ */
/* IDirect3DResource8 */
/* ------------------ */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DResource8Vtbl Direct3DResource8_Vtbl;

/*****************************************************************************
 * IDirect3DResource8 implementation structure
 */
struct IDirect3DResource8Impl
{
    /* IUnknown fields */
    const IDirect3DResource8Vtbl *lpVtbl;
    LONG                          ref;

    /* IDirect3DResource8 fields */
    IWineD3DResource             *wineD3DResource;
};
extern HRESULT WINAPI IDirect3DResource8Impl_GetDevice(LPDIRECT3DRESOURCE8 iface, IDirect3DDevice8** ppDevice);

/* ---------------------- */
/* IDirect3DVertexBuffer8 */
/* ---------------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DVertexBuffer8Vtbl Direct3DVertexBuffer8_Vtbl;

/*****************************************************************************
 * IDirect3DVertexBuffer8 implementation structure
 */
struct IDirect3DVertexBuffer8Impl
{
    /* IUnknown fields */
    const IDirect3DVertexBuffer8Vtbl *lpVtbl;
    LONG                              ref;

    /* IDirect3DResource8 fields */
    IWineD3DVertexBuffer             *wineD3DVertexBuffer;

    /* Parent reference */
    LPDIRECT3DDEVICE8                 parentDevice;
};

/* --------------------- */
/* IDirect3DIndexBuffer8 */
/* --------------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DIndexBuffer8Vtbl Direct3DIndexBuffer8_Vtbl;

/*****************************************************************************
 * IDirect3DIndexBuffer8 implementation structure
 */
struct IDirect3DIndexBuffer8Impl
{
    /* IUnknown fields */
    const IDirect3DIndexBuffer8Vtbl *lpVtbl;
    LONG                             ref;

    /* IDirect3DResource8 fields */
    IWineD3DIndexBuffer             *wineD3DIndexBuffer;

    /* Parent reference */
    LPDIRECT3DDEVICE8                parentDevice;
};

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
 * Predeclare the interface implementation structures
 */
extern const IDirect3DCubeTexture8Vtbl Direct3DCubeTexture8_Vtbl;

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

/* ----------------- */
/* IDirect3DTexture8 */
/* ----------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DTexture8Vtbl Direct3DTexture8_Vtbl;

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

/* ----------------------- */
/* IDirect3DVolumeTexture8 */
/* ----------------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DVolumeTexture8Vtbl Direct3DVolumeTexture8_Vtbl;

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

/* ----------------------- */
/* IDirect3DStateBlockImpl */
/* ----------------------- */

/* TODO: Generate a valid GUIDs */
/* {83B073CE-6F30-11d9-C687-00046142C14F} */
DEFINE_GUID(IID_IDirect3DStateBlock8, 
0x83b073ce, 0x6f30, 0x11d9, 0xc6, 0x87, 0x0, 0x4, 0x61, 0x42, 0xc1, 0x4f);

DEFINE_GUID(IID_IDirect3DVertexDeclaration8,
0x5dd7478d, 0xcbf3, 0x41a6, 0x8c, 0xfd, 0xfd, 0x19, 0x2b, 0x11, 0xc7, 0x90);

DEFINE_GUID(IID_IDirect3DVertexShader8,
0xefc5557e, 0x6265, 0x4613, 0x8a, 0x94, 0x43, 0x85, 0x78, 0x89, 0xeb, 0x36);

DEFINE_GUID(IID_IDirect3DPixelShader8,
0x6d3bdbdc, 0x5b02, 0x4415, 0xb8, 0x52, 0xce, 0x5e, 0x8b, 0xcc, 0xb2, 0x89);


/*****************************************************************************
 * IDirect3DStateBlock8 interface
 */
#define INTERFACE IDirect3DStateBlock8
DECLARE_INTERFACE_(IDirect3DStateBlock8, IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IDirect3DStateBlock9 methods ***/
    STDMETHOD(GetDevice)(THIS_ struct IDirect3DDevice8** ppDevice) PURE;
    STDMETHOD(Capture)(THIS) PURE;
    STDMETHOD(Apply)(THIS) PURE;
};
#undef INTERFACE

/*** IUnknown methods ***/
#define IDirect3DStateBlock8_QueryInterface(p,a,b)  (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirect3DStateBlock8_AddRef(p)              (p)->lpVtbl->AddRef(p)
#define IDirect3DStateBlock8_Release(p)             (p)->lpVtbl->Release(p)
/*** IDirect3DStateBlock9 methods ***/
#define IDirect3DStateBlock8_GetDevice(p,a)         (p)->lpVtbl->GetDevice(p,a)
#define IDirect3DStateBlock8_Capture(p)             (p)->lpVtbl->Capture(p)
#define IDirect3DStateBlock8_Apply(p)               (p)->lpVtbl->Apply(p)

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DStateBlock8Vtbl Direct3DStateBlock8_Vtbl;

/*****************************************************************************
 * IDirect3DStateBlock implementation structure
 */
typedef struct  IDirect3DStateBlock8Impl {
    /* IUnknown fields */
    const IDirect3DStateBlock8Vtbl *lpVtbl;
    LONG                   ref;

    /* IDirect3DResource8 fields */
    IWineD3DStateBlock             *wineD3DStateBlock;
} IDirect3DStateBlock8Impl;

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

/*** Implementation ***/
extern const IDirect3DVertexDeclaration8Vtbl Direct3DVertexDeclaration8_Vtbl;

typedef struct {
    const IDirect3DVertexDeclaration8Vtbl *lpVtbl;
    LONG ref_count;

    DWORD *elements;
    DWORD elements_size; /* Size of elements, in bytes */

    IWineD3DVertexDeclaration *wined3d_vertex_declaration;
} IDirect3DVertexDeclaration8Impl;


/*****************************************************************************
 * IDirect3DVertexShader9 interface
 */
#define INTERFACE IDirect3DVertexShader8
DECLARE_INTERFACE_(IDirect3DVertexShader8, IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IDirect3DVertexShader9 methods ***/
    STDMETHOD(GetDevice)(THIS_ struct IDirect3DDevice8** ppDevice) PURE;
    STDMETHOD(GetFunction)(THIS_ void*, UINT* pSizeOfData) PURE;
};
#undef INTERFACE

/*** IUnknown methods ***/
#define IDirect3DVertexShader8_QueryInterface(p,a,b)  (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirect3DVertexShader8_AddRef(p)              (p)->lpVtbl->AddRef(p)
#define IDirect3DVertexShader8_Release(p)             (p)->lpVtbl->Release(p)
/*** IDirect3DVertexShader8 methods ***/
#define IDirect3DVertexShader8_GetDevice(p,a)         (p)->lpVtbl->GetDevice(p,a)
#define IDirect3DVertexShader8_GetFunction(p,a,b)     (p)->lpVtbl->GetFunction(p,a,b)

/* ------------------------- */
/* IDirect3DVertexShader8Impl */
/* ------------------------- */

/*****************************************************************************
 * IDirect3DPixelShader9 interface
 */
#define INTERFACE IDirect3DPixelShader8
DECLARE_INTERFACE_(IDirect3DPixelShader8,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IDirect3DPixelShader8 methods ***/
    STDMETHOD(GetDevice)(THIS_ struct IDirect3DDevice8** ppDevice) PURE;
    STDMETHOD(GetFunction)(THIS_ void*, UINT* pSizeOfData) PURE;
};
#undef INTERFACE

/*** IUnknown methods ***/
#define IDirect3DPixelShader8_QueryInterface(p,a,b)  (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirect3DPixelShader8_AddRef(p)              (p)->lpVtbl->AddRef(p)
#define IDirect3DPixelShader8_Release(p)             (p)->lpVtbl->Release(p)
/*** IDirect3DPixelShader8 methods ***/
#define IDirect3DPixelShader8_GetDevice(p,a)         (p)->lpVtbl->GetDevice(p,a)
#define IDirect3DPixelShader8_GetFunction(p,a,b)     (p)->lpVtbl->GetFunction(p,a,b)


/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DVertexShader8Vtbl Direct3DVertexShader8_Vtbl;

/*****************************************************************************
 * IDirect3DVertexShader implementation structure
 */

struct IDirect3DVertexShader8Impl {
  const IDirect3DVertexShader8Vtbl *lpVtbl;
  LONG ref;

  DWORD                             handle;
  IDirect3DVertexDeclaration8      *vertex_declaration;
  IWineD3DVertexShader             *wineD3DVertexShader;
};


/* ------------------------ */
/* IDirect3DPixelShaderImpl */
/* ------------------------ */


/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DPixelShader8Vtbl Direct3DPixelShader8_Vtbl;

/*****************************************************************************
 * IDirect3DPixelShader implementation structure
 */
typedef struct IDirect3DPixelShader8Impl {
    const IDirect3DPixelShader8Vtbl *lpVtbl;
    LONG                             ref;

    DWORD                            handle;
    /* The device, to be replaced by an IDirect3DDeviceImpl */
    IWineD3DPixelShader             *wineD3DPixelShader;
} IDirect3DPixelShader8Impl;

/**
 * Internals functions
 *
 * to see how not defined it here
 */
void load_local_constants(const DWORD *d3d8_elements, IWineD3DVertexShader *wined3d_vertex_shader);
UINT convert_to_wined3d_declaration(const DWORD *d3d8_elements, DWORD *d3d8_elements_size, WINED3DVERTEXELEMENT **wined3d_elements);
size_t parse_token(const DWORD* pToken);

/* Callbacks */
extern HRESULT WINAPI D3D8CB_CreateSurface(IUnknown *device, IUnknown *pSuperior, UINT Width, UINT Height,
                                         WINED3DFORMAT Format, DWORD Usage, WINED3DPOOL Pool, UINT Level,
                                         WINED3DCUBEMAP_FACES Face, IWineD3DSurface** ppSurface,
                                         HANDLE* pSharedHandle);

extern HRESULT WINAPI D3D8CB_CreateVolume(IUnknown  *pDevice, IUnknown *pSuperior, UINT Width, UINT Height, UINT Depth,
                                          WINED3DFORMAT  Format, WINED3DPOOL Pool, DWORD Usage,
                                          IWineD3DVolume **ppVolume,
                                          HANDLE   * pSharedHandle);

extern HRESULT WINAPI D3D8CB_CreateDepthStencilSurface(IUnknown *device, IUnknown *pSuperior, UINT Width, UINT Height,
                                         WINED3DFORMAT Format, WINED3DMULTISAMPLE_TYPE MultiSample,
                                         DWORD MultisampleQuality, BOOL Discard,
                                         IWineD3DSurface** ppSurface, HANDLE* pSharedHandle);

extern HRESULT WINAPI D3D8CB_CreateRenderTarget(IUnknown *device, IUnknown *pSuperior, UINT Width, UINT Height,
                                         WINED3DFORMAT Format, WINED3DMULTISAMPLE_TYPE MultiSample,
                                         DWORD MultisampleQuality, BOOL Lockable,
                                         IWineD3DSurface** ppSurface, HANDLE* pSharedHandle);

extern ULONG WINAPI D3D8CB_DestroySwapChain (IWineD3DSwapChain *pSwapChain);

extern ULONG WINAPI D3D8CB_DestroyDepthStencilSurface (IWineD3DSurface *pSurface);

extern ULONG WINAPI D3D8CB_DestroyRenderTarget (IWineD3DSurface *pSurface);

extern ULONG WINAPI D3D8CB_DestroySurface(IWineD3DSurface *pSurface);

extern ULONG WINAPI D3D8CB_DestroyVolume(IWineD3DVolume *pVolume);

#endif /* __WINE_D3DX8_PRIVATE_H */
