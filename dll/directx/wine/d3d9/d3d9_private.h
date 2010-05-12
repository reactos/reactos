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

#include <stdarg.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#include "d3d9.h"
#include "wine/wined3d.h"

/* ===========================================================================
   Internal use
   =========================================================================== */
extern HRESULT vdecl_convert_fvf(DWORD FVF, D3DVERTEXELEMENT9 **ppVertexElements) DECLSPEC_HIDDEN;
D3DFORMAT d3dformat_from_wined3dformat(WINED3DFORMAT format) DECLSPEC_HIDDEN;
WINED3DFORMAT wined3dformat_from_d3dformat(D3DFORMAT format) DECLSPEC_HIDDEN;

/* ===========================================================================
    Macros
   =========================================================================== */
/* Not nice, but it lets wined3d support different versions of directx */
#define WINECAPSTOD3D9CAPS(_pD3D9Caps, _pWineCaps) \
    _pD3D9Caps->DeviceType                        = (D3DDEVTYPE) _pWineCaps->DeviceType; \
    _pD3D9Caps->AdapterOrdinal                    = _pWineCaps->AdapterOrdinal; \
    _pD3D9Caps->Caps                              = _pWineCaps->Caps; \
    _pD3D9Caps->Caps2                             = _pWineCaps->Caps2; \
    _pD3D9Caps->Caps3                             = _pWineCaps->Caps3; \
    _pD3D9Caps->PresentationIntervals             = _pWineCaps->PresentationIntervals; \
    _pD3D9Caps->CursorCaps                        = _pWineCaps->CursorCaps; \
    _pD3D9Caps->DevCaps                           = _pWineCaps->DevCaps; \
    _pD3D9Caps->PrimitiveMiscCaps                 = _pWineCaps->PrimitiveMiscCaps; \
    _pD3D9Caps->RasterCaps                        = _pWineCaps->RasterCaps; \
    _pD3D9Caps->ZCmpCaps                          = _pWineCaps->ZCmpCaps; \
    _pD3D9Caps->SrcBlendCaps                      = _pWineCaps->SrcBlendCaps; \
    _pD3D9Caps->DestBlendCaps                     = _pWineCaps->DestBlendCaps; \
    _pD3D9Caps->AlphaCmpCaps                      = _pWineCaps->AlphaCmpCaps; \
    _pD3D9Caps->ShadeCaps                         = _pWineCaps->ShadeCaps; \
    _pD3D9Caps->TextureCaps                       = _pWineCaps->TextureCaps; \
    _pD3D9Caps->TextureFilterCaps                 = _pWineCaps->TextureFilterCaps; \
    _pD3D9Caps->CubeTextureFilterCaps             = _pWineCaps->CubeTextureFilterCaps; \
    _pD3D9Caps->VolumeTextureFilterCaps           = _pWineCaps->VolumeTextureFilterCaps; \
    _pD3D9Caps->TextureAddressCaps                = _pWineCaps->TextureAddressCaps; \
    _pD3D9Caps->VolumeTextureAddressCaps          = _pWineCaps->VolumeTextureAddressCaps; \
    _pD3D9Caps->LineCaps                          = _pWineCaps->LineCaps; \
    _pD3D9Caps->MaxTextureWidth                   = _pWineCaps->MaxTextureWidth; \
    _pD3D9Caps->MaxTextureHeight                  = _pWineCaps->MaxTextureHeight; \
    _pD3D9Caps->MaxVolumeExtent                   = _pWineCaps->MaxVolumeExtent; \
    _pD3D9Caps->MaxTextureRepeat                  = _pWineCaps->MaxTextureRepeat; \
    _pD3D9Caps->MaxTextureAspectRatio             = _pWineCaps->MaxTextureAspectRatio; \
    _pD3D9Caps->MaxAnisotropy                     = _pWineCaps->MaxAnisotropy; \
    _pD3D9Caps->MaxVertexW                        = _pWineCaps->MaxVertexW; \
    _pD3D9Caps->GuardBandLeft                     = _pWineCaps->GuardBandLeft; \
    _pD3D9Caps->GuardBandTop                      = _pWineCaps->GuardBandTop; \
    _pD3D9Caps->GuardBandRight                    = _pWineCaps->GuardBandRight; \
    _pD3D9Caps->GuardBandBottom                   = _pWineCaps->GuardBandBottom; \
    _pD3D9Caps->ExtentsAdjust                     = _pWineCaps->ExtentsAdjust; \
    _pD3D9Caps->StencilCaps                       = _pWineCaps->StencilCaps; \
    _pD3D9Caps->FVFCaps                           = _pWineCaps->FVFCaps; \
    _pD3D9Caps->TextureOpCaps                     = _pWineCaps->TextureOpCaps; \
    _pD3D9Caps->MaxTextureBlendStages             = _pWineCaps->MaxTextureBlendStages; \
    _pD3D9Caps->MaxSimultaneousTextures           = _pWineCaps->MaxSimultaneousTextures; \
    _pD3D9Caps->VertexProcessingCaps              = _pWineCaps->VertexProcessingCaps; \
    _pD3D9Caps->MaxActiveLights                   = _pWineCaps->MaxActiveLights; \
    _pD3D9Caps->MaxUserClipPlanes                 = _pWineCaps->MaxUserClipPlanes; \
    _pD3D9Caps->MaxVertexBlendMatrices            = _pWineCaps->MaxVertexBlendMatrices; \
    _pD3D9Caps->MaxVertexBlendMatrixIndex         = _pWineCaps->MaxVertexBlendMatrixIndex; \
    _pD3D9Caps->MaxPointSize                      = _pWineCaps->MaxPointSize; \
    _pD3D9Caps->MaxPrimitiveCount                 = _pWineCaps->MaxPrimitiveCount; \
    _pD3D9Caps->MaxVertexIndex                    = _pWineCaps->MaxVertexIndex; \
    _pD3D9Caps->MaxStreams                        = _pWineCaps->MaxStreams; \
    _pD3D9Caps->MaxStreamStride                   = _pWineCaps->MaxStreamStride; \
    _pD3D9Caps->VertexShaderVersion               = _pWineCaps->VertexShaderVersion; \
    _pD3D9Caps->MaxVertexShaderConst              = _pWineCaps->MaxVertexShaderConst; \
    _pD3D9Caps->PixelShaderVersion                = _pWineCaps->PixelShaderVersion; \
    _pD3D9Caps->PixelShader1xMaxValue             = _pWineCaps->PixelShader1xMaxValue; \
    _pD3D9Caps->DevCaps2                          = _pWineCaps->DevCaps2; \
    _pD3D9Caps->MaxNpatchTessellationLevel        = _pWineCaps->MaxNpatchTessellationLevel; \
    _pD3D9Caps->MasterAdapterOrdinal              = _pWineCaps->MasterAdapterOrdinal; \
    _pD3D9Caps->AdapterOrdinalInGroup             = _pWineCaps->AdapterOrdinalInGroup; \
    _pD3D9Caps->NumberOfAdaptersInGroup           = _pWineCaps->NumberOfAdaptersInGroup; \
    _pD3D9Caps->DeclTypes                         = _pWineCaps->DeclTypes; \
    _pD3D9Caps->NumSimultaneousRTs                = _pWineCaps->NumSimultaneousRTs; \
    _pD3D9Caps->StretchRectFilterCaps             = _pWineCaps->StretchRectFilterCaps; \
    _pD3D9Caps->VS20Caps.Caps                     = _pWineCaps->VS20Caps.Caps; \
    _pD3D9Caps->VS20Caps.DynamicFlowControlDepth  = _pWineCaps->VS20Caps.DynamicFlowControlDepth; \
    _pD3D9Caps->VS20Caps.NumTemps                 = _pWineCaps->VS20Caps.NumTemps; \
    _pD3D9Caps->VS20Caps.NumTemps                 = _pWineCaps->VS20Caps.NumTemps; \
    _pD3D9Caps->VS20Caps.StaticFlowControlDepth   = _pWineCaps->VS20Caps.StaticFlowControlDepth; \
    _pD3D9Caps->PS20Caps.Caps                     = _pWineCaps->PS20Caps.Caps; \
    _pD3D9Caps->PS20Caps.DynamicFlowControlDepth  = _pWineCaps->PS20Caps.DynamicFlowControlDepth; \
    _pD3D9Caps->PS20Caps.NumTemps                 = _pWineCaps->PS20Caps.NumTemps; \
    _pD3D9Caps->PS20Caps.StaticFlowControlDepth   = _pWineCaps->PS20Caps.StaticFlowControlDepth; \
    _pD3D9Caps->PS20Caps.NumInstructionSlots      = _pWineCaps->PS20Caps.NumInstructionSlots; \
    _pD3D9Caps->VertexTextureFilterCaps           = _pWineCaps->VertexTextureFilterCaps; \
    _pD3D9Caps->MaxVShaderInstructionsExecuted    = _pWineCaps->MaxVShaderInstructionsExecuted; \
    _pD3D9Caps->MaxPShaderInstructionsExecuted    = _pWineCaps->MaxPShaderInstructionsExecuted; \
    _pD3D9Caps->MaxVertexShader30InstructionSlots = _pWineCaps->MaxVertexShader30InstructionSlots; \
    _pD3D9Caps->MaxPixelShader30InstructionSlots  = _pWineCaps->MaxPixelShader30InstructionSlots;

/* ===========================================================================
    D3D9 interfaces
   =========================================================================== */

/* ---------- */
/* IDirect3D9 */
/* ---------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3D9ExVtbl Direct3D9_Vtbl DECLSPEC_HIDDEN;

/*****************************************************************************
 * IDirect3D implementation structure
 */
typedef struct IDirect3D9Impl
{
    /* IUnknown fields */
    const IDirect3D9ExVtbl   *lpVtbl;
    LONG                    ref;

    /* The WineD3D device */
    IWineD3D               *WineD3D;

    /* Created via Direct3DCreate9Ex? Can QI extended interfaces */
    BOOL                    extended;
} IDirect3D9Impl;

void filter_caps(D3DCAPS9* pCaps) DECLSPEC_HIDDEN;

/*****************************************************************************
 * IDirect3DDevice9 implementation structure
 */
typedef struct IDirect3DDevice9Impl
{
    /* IUnknown fields */
    const IDirect3DDevice9ExVtbl   *lpVtbl;
    const IWineD3DDeviceParentVtbl *device_parent_vtbl;
    LONG                          ref;

    /* IDirect3DDevice9 fields */
    IWineD3DDevice               *WineD3DDevice;

    /* Avoids recursion with nested ReleaseRef to 0 */
    BOOL                          inDestruction;

    IDirect3DVertexDeclaration9  **convertedDecls;
    unsigned int                 numConvertedDecls, declArraySize;

    BOOL                          notreset;
} IDirect3DDevice9Impl;

HRESULT device_init(IDirect3DDevice9Impl *device, IWineD3D *wined3d, UINT adapter, D3DDEVTYPE device_type,
        HWND focus_window, DWORD flags, D3DPRESENT_PARAMETERS *parameters) DECLSPEC_HIDDEN;

/* IDirect3DDevice9: */
extern HRESULT WINAPI IDirect3DDevice9Impl_GetSwapChain(IDirect3DDevice9Ex *iface,
        UINT iSwapChain, IDirect3DSwapChain9 **pSwapChain) DECLSPEC_HIDDEN;
extern UINT WINAPI IDirect3DDevice9Impl_GetNumberOfSwapChains(IDirect3DDevice9Ex *iface) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirect3DDevice9Impl_SetVertexDeclaration(IDirect3DDevice9Ex *iface,
        IDirect3DVertexDeclaration9 *pDecl) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirect3DDevice9Impl_GetVertexDeclaration(IDirect3DDevice9Ex *iface,
        IDirect3DVertexDeclaration9 **ppDecl) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirect3DDevice9Impl_SetVertexShader(IDirect3DDevice9Ex *iface,
        IDirect3DVertexShader9 *pShader) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirect3DDevice9Impl_GetVertexShader(IDirect3DDevice9Ex *iface,
        IDirect3DVertexShader9 **ppShader) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirect3DDevice9Impl_SetVertexShaderConstantF(IDirect3DDevice9Ex *iface,
        UINT StartRegister, const float *pConstantData, UINT Vector4fCount) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirect3DDevice9Impl_GetVertexShaderConstantF(IDirect3DDevice9Ex *iface,
        UINT StartRegister, float *pConstantData, UINT Vector4fCount) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirect3DDevice9Impl_SetVertexShaderConstantI(IDirect3DDevice9Ex *iface,
        UINT StartRegister, const int *pConstantData, UINT Vector4iCount) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirect3DDevice9Impl_GetVertexShaderConstantI(IDirect3DDevice9Ex *iface,
        UINT StartRegister, int *pConstantData, UINT Vector4iCount) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirect3DDevice9Impl_SetVertexShaderConstantB(IDirect3DDevice9Ex *iface,
        UINT StartRegister, const BOOL *pConstantData, UINT BoolCount) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirect3DDevice9Impl_GetVertexShaderConstantB(IDirect3DDevice9Ex *iface,
        UINT StartRegister, BOOL *pConstantData, UINT BoolCount) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirect3DDevice9Impl_SetPixelShader(IDirect3DDevice9Ex *iface,
        IDirect3DPixelShader9 *pShader) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirect3DDevice9Impl_GetPixelShader(IDirect3DDevice9Ex *iface,
        IDirect3DPixelShader9  **ppShader) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirect3DDevice9Impl_SetPixelShaderConstantF(IDirect3DDevice9Ex *iface,
        UINT StartRegister, const float *pConstantData, UINT Vector4fCount) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirect3DDevice9Impl_GetPixelShaderConstantF(IDirect3DDevice9Ex *iface,
        UINT StartRegister, float *pConstantData, UINT Vector4fCount) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirect3DDevice9Impl_SetPixelShaderConstantI(IDirect3DDevice9Ex *iface,
        UINT StartRegister, const int *pConstantData, UINT Vector4iCount) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirect3DDevice9Impl_GetPixelShaderConstantI(IDirect3DDevice9Ex *iface,
        UINT StartRegister, int *pConstantData, UINT Vector4iCount) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirect3DDevice9Impl_SetPixelShaderConstantB(IDirect3DDevice9Ex *iface,
        UINT StartRegister, const BOOL *pConstantData, UINT BoolCount) DECLSPEC_HIDDEN;
extern HRESULT WINAPI IDirect3DDevice9Impl_GetPixelShaderConstantB(IDirect3DDevice9Ex *iface,
        UINT StartRegister, BOOL *pConstantData, UINT BoolCount) DECLSPEC_HIDDEN;

/* ---------------- */
/* IDirect3DVolume9 */
/* ---------------- */

/*****************************************************************************
 * IDirect3DVolume9 implementation structure
 */
typedef struct IDirect3DVolume9Impl
{
    /* IUnknown fields */
    const IDirect3DVolume9Vtbl *lpVtbl;
    LONG                    ref;

    /* IDirect3DVolume9 fields */
    IWineD3DVolume         *wineD3DVolume;

    /* The volume container */
    IUnknown                    *container;

    /* If set forward refcounting to this object */
    IUnknown                    *forwardReference;
} IDirect3DVolume9Impl;

HRESULT volume_init(IDirect3DVolume9Impl *volume, IDirect3DDevice9Impl *device, UINT width, UINT height,
        UINT depth, DWORD usage, WINED3DFORMAT format, WINED3DPOOL pool) DECLSPEC_HIDDEN;

/* ------------------- */
/* IDirect3DSwapChain9 */
/* ------------------- */

/*****************************************************************************
 * IDirect3DSwapChain9 implementation structure
 */
typedef struct IDirect3DSwapChain9Impl
{
    /* IUnknown fields */
    const IDirect3DSwapChain9Vtbl *lpVtbl;
    LONG                    ref;

    /* IDirect3DSwapChain9 fields */
    IWineD3DSwapChain      *wineD3DSwapChain;

    /* Parent reference */
    LPDIRECT3DDEVICE9EX       parentDevice;

    /* Flags an implicit swap chain */
    BOOL                        isImplicit;
} IDirect3DSwapChain9Impl;

HRESULT swapchain_init(IDirect3DSwapChain9Impl *swapchain, IDirect3DDevice9Impl *device,
        D3DPRESENT_PARAMETERS *present_parameters) DECLSPEC_HIDDEN;

/* ----------------- */
/* IDirect3DSurface9 */
/* ----------------- */

/*****************************************************************************
 * IDirect3DSurface9 implementation structure
 */
typedef struct IDirect3DSurface9Impl
{
    /* IUnknown fields */
    const IDirect3DSurface9Vtbl *lpVtbl;
    LONG                    ref;

    /* IDirect3DResource9 fields */
    IWineD3DSurface        *wineD3DSurface;

    /* Parent reference */
    LPDIRECT3DDEVICE9EX       parentDevice;

    /* The surface container */
    IUnknown                    *container;

    /* If set forward refcounting to this object */
    IUnknown                    *forwardReference;

    BOOL                        getdc_supported;
} IDirect3DSurface9Impl;

HRESULT surface_init(IDirect3DSurface9Impl *surface, IDirect3DDevice9Impl *device,
        UINT width, UINT height, D3DFORMAT format, BOOL lockable, BOOL discard, UINT level,
        DWORD usage, D3DPOOL pool, D3DMULTISAMPLE_TYPE multisample_type, DWORD multisample_quality) DECLSPEC_HIDDEN;

/* ---------------------- */
/* IDirect3DVertexBuffer9 */
/* ---------------------- */

/*****************************************************************************
 * IDirect3DVertexBuffer9 implementation structure
 */
typedef struct IDirect3DVertexBuffer9Impl
{
    /* IUnknown fields */
    const IDirect3DVertexBuffer9Vtbl *lpVtbl;
    LONG                    ref;

    /* IDirect3DResource9 fields */
    IWineD3DBuffer *wineD3DVertexBuffer;

    /* Parent reference */
    LPDIRECT3DDEVICE9EX       parentDevice;

    DWORD fvf;
} IDirect3DVertexBuffer9Impl;

HRESULT vertexbuffer_init(IDirect3DVertexBuffer9Impl *buffer, IDirect3DDevice9Impl *device,
        UINT size, UINT usage, DWORD fvf, D3DPOOL pool) DECLSPEC_HIDDEN;

/* --------------------- */
/* IDirect3DIndexBuffer9 */
/* --------------------- */

/*****************************************************************************
 * IDirect3DIndexBuffer9 implementation structure
 */
typedef struct IDirect3DIndexBuffer9Impl
{
    /* IUnknown fields */
    const IDirect3DIndexBuffer9Vtbl *lpVtbl;
    LONG                    ref;

    /* IDirect3DResource9 fields */
    IWineD3DBuffer         *wineD3DIndexBuffer;

    /* Parent reference */
    LPDIRECT3DDEVICE9EX       parentDevice;
    WINED3DFORMAT             format;
} IDirect3DIndexBuffer9Impl;

HRESULT indexbuffer_init(IDirect3DIndexBuffer9Impl *buffer, IDirect3DDevice9Impl *device,
        UINT size, DWORD usage, D3DFORMAT format, D3DPOOL pool) DECLSPEC_HIDDEN;

/* --------------------- */
/* IDirect3DBaseTexture9 */
/* --------------------- */

/*****************************************************************************
 * IDirect3DBaseTexture9 implementation structure
 */
typedef struct IDirect3DBaseTexture9Impl
{
    /* IUnknown fields */
    const IDirect3DBaseTexture9Vtbl *lpVtbl;
    LONG                    ref;

    /* IDirect3DResource9 fields */
    IWineD3DBaseTexture    *wineD3DBaseTexture;
} IDirect3DBaseTexture9Impl;

/* --------------------- */
/* IDirect3DCubeTexture9 */
/* --------------------- */

/*****************************************************************************
 * IDirect3DCubeTexture9 implementation structure
 */
typedef struct IDirect3DCubeTexture9Impl
{
    /* IUnknown fields */
    const IDirect3DCubeTexture9Vtbl *lpVtbl;
    LONG                    ref;

    /* IDirect3DResource9 fields */
    IWineD3DCubeTexture    *wineD3DCubeTexture;

    /* Parent reference */
    LPDIRECT3DDEVICE9EX       parentDevice;
}  IDirect3DCubeTexture9Impl;

HRESULT cubetexture_init(IDirect3DCubeTexture9Impl *texture, IDirect3DDevice9Impl *device,
        UINT edge_length, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool) DECLSPEC_HIDDEN;

/* ----------------- */
/* IDirect3DTexture9 */
/* ----------------- */

/*****************************************************************************
 * IDirect3DTexture9 implementation structure
 */
typedef struct IDirect3DTexture9Impl
{
    /* IUnknown fields */
    const IDirect3DTexture9Vtbl *lpVtbl;
    LONG                    ref;

    /* IDirect3DResource9 fields */
    IWineD3DTexture        *wineD3DTexture;

    /* Parent reference */
    LPDIRECT3DDEVICE9EX       parentDevice;
} IDirect3DTexture9Impl;

HRESULT texture_init(IDirect3DTexture9Impl *texture, IDirect3DDevice9Impl *device,
        UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool) DECLSPEC_HIDDEN;

/* ----------------------- */
/* IDirect3DVolumeTexture9 */
/* ----------------------- */

/*****************************************************************************
 * IDirect3DVolumeTexture9 implementation structure
 */
typedef struct IDirect3DVolumeTexture9Impl
{
    /* IUnknown fields */
    const IDirect3DVolumeTexture9Vtbl *lpVtbl;
    LONG                    ref;

    /* IDirect3DResource9 fields */
    IWineD3DVolumeTexture  *wineD3DVolumeTexture;

    /* Parent reference */
    LPDIRECT3DDEVICE9EX       parentDevice;
} IDirect3DVolumeTexture9Impl;

HRESULT volumetexture_init(IDirect3DVolumeTexture9Impl *texture, IDirect3DDevice9Impl *device,
        UINT width, UINT height, UINT depth, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool) DECLSPEC_HIDDEN;

/* ----------------------- */
/* IDirect3DStateBlock9 */
/* ----------------------- */

/*****************************************************************************
 * IDirect3DStateBlock9 implementation structure
 */
typedef struct  IDirect3DStateBlock9Impl {
    /* IUnknown fields */
    const IDirect3DStateBlock9Vtbl *lpVtbl;
    LONG                    ref;

    /* IDirect3DStateBlock9 fields */
    IWineD3DStateBlock     *wineD3DStateBlock;

    /* Parent reference */
    LPDIRECT3DDEVICE9EX       parentDevice;
} IDirect3DStateBlock9Impl;

HRESULT stateblock_init(IDirect3DStateBlock9Impl *stateblock, IDirect3DDevice9Impl *device,
        D3DSTATEBLOCKTYPE type, IWineD3DStateBlock *wined3d_stateblock) DECLSPEC_HIDDEN;

/* --------------------------- */
/* IDirect3DVertexDeclaration9 */
/* --------------------------- */

/*****************************************************************************
 * IDirect3DVertexDeclaration implementation structure
 */
typedef struct IDirect3DVertexDeclaration9Impl {
  /* IUnknown fields */
  const IDirect3DVertexDeclaration9Vtbl *lpVtbl;
  LONG    ref;

  D3DVERTEXELEMENT9 *elements;
  UINT element_count;

  /* IDirect3DVertexDeclaration9 fields */
  IWineD3DVertexDeclaration *wineD3DVertexDeclaration;
  DWORD convFVF;

  /* Parent reference */
  LPDIRECT3DDEVICE9EX parentDevice;
} IDirect3DVertexDeclaration9Impl;

void IDirect3DVertexDeclaration9Impl_Destroy(LPDIRECT3DVERTEXDECLARATION9 iface) DECLSPEC_HIDDEN;
HRESULT vertexdeclaration_init(IDirect3DVertexDeclaration9Impl *declaration,
        IDirect3DDevice9Impl *device, const D3DVERTEXELEMENT9 *elements) DECLSPEC_HIDDEN;

/* ---------------------- */
/* IDirect3DVertexShader9 */
/* ---------------------- */

/*****************************************************************************
 * IDirect3DVertexShader implementation structure
 */
typedef struct IDirect3DVertexShader9Impl {
  /* IUnknown fields */
  const IDirect3DVertexShader9Vtbl *lpVtbl;
  LONG  ref;

  /* IDirect3DVertexShader9 fields */
  IWineD3DVertexShader *wineD3DVertexShader;

  /* Parent reference */
  LPDIRECT3DDEVICE9EX parentDevice;
} IDirect3DVertexShader9Impl;

HRESULT vertexshader_init(IDirect3DVertexShader9Impl *shader,
        IDirect3DDevice9Impl *device, const DWORD *byte_code) DECLSPEC_HIDDEN;

#define D3D9_MAX_VERTEX_SHADER_CONSTANTF 256
#define D3D9_MAX_SIMULTANEOUS_RENDERTARGETS 4

/* --------------------- */
/* IDirect3DPixelShader9 */
/* --------------------- */

/*****************************************************************************
 * IDirect3DPixelShader implementation structure
 */
typedef struct IDirect3DPixelShader9Impl {
  /* IUnknown fields */
    const IDirect3DPixelShader9Vtbl *lpVtbl;
    LONG                    ref;

    /* IDirect3DPixelShader9 fields */
    IWineD3DPixelShader    *wineD3DPixelShader;

    /* Parent reference */
    LPDIRECT3DDEVICE9EX       parentDevice;
} IDirect3DPixelShader9Impl;

HRESULT pixelshader_init(IDirect3DPixelShader9Impl *shader,
        IDirect3DDevice9Impl *device, const DWORD *byte_code) DECLSPEC_HIDDEN;

/* --------------- */
/* IDirect3DQuery9 */
/* --------------- */

/*****************************************************************************
 * IDirect3DPixelShader implementation structure
 */
typedef struct IDirect3DQuery9Impl {
    /* IUnknown fields */
    const IDirect3DQuery9Vtbl *lpVtbl;
    LONG                 ref;

    /* IDirect3DQuery9 fields */
    IWineD3DQuery       *wineD3DQuery;

    /* Parent reference */
    LPDIRECT3DDEVICE9EX    parentDevice;
} IDirect3DQuery9Impl;

HRESULT query_init(IDirect3DQuery9Impl *query, IDirect3DDevice9Impl *device,
        D3DQUERYTYPE type) DECLSPEC_HIDDEN;

#endif /* __WINE_D3D9_PRIVATE_H */
