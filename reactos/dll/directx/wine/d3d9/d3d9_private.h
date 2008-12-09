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
extern HRESULT vdecl_convert_fvf(
    DWORD FVF,
    D3DVERTEXELEMENT9** ppVertexElements);
extern CRITICAL_SECTION d3d9_cs;

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
extern const IDirect3D9ExVtbl Direct3D9_Vtbl;

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

void filter_caps(D3DCAPS9* pCaps);

/* ---------------- */
/* IDirect3DDevice9 */
/* ---------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DDevice9ExVtbl Direct3DDevice9_Vtbl;

/*****************************************************************************
 * IDirect3DDevice9 implementation structure
 */
typedef struct IDirect3DDevice9Impl
{
    /* IUnknown fields */
    const IDirect3DDevice9ExVtbl   *lpVtbl;
    LONG                          ref;

    /* IDirect3DDevice9 fields */
    IWineD3DDevice               *WineD3DDevice;

    /* Avoids recursion with nested ReleaseRef to 0 */
    BOOL                          inDestruction;

    IDirect3DVertexDeclaration9  **convertedDecls;
    unsigned int                 numConvertedDecls, declArraySize;

    BOOL                          notreset;
} IDirect3DDevice9Impl;


/* IDirect3DDevice9: */
extern HRESULT  WINAPI  IDirect3DDevice9Impl_CreateAdditionalSwapChain(LPDIRECT3DDEVICE9EX iface, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain9** pSwapChain);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetSwapChain(LPDIRECT3DDEVICE9EX iface, UINT iSwapChain, IDirect3DSwapChain9** pSwapChain);
extern UINT     WINAPI  IDirect3DDevice9Impl_GetNumberOfSwapChains(LPDIRECT3DDEVICE9EX iface);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_CreateTexture(LPDIRECT3DDEVICE9EX iface, UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_CreateVolumeTexture(LPDIRECT3DDEVICE9EX iface, UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9** ppVolumeTexture, HANDLE* pSharedHandle);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_CreateCubeTexture(LPDIRECT3DDEVICE9EX iface, UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture9** ppCubeTexture, HANDLE* pSharedHandle);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_CreateVertexBuffer(LPDIRECT3DDEVICE9EX iface, UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9** ppVertexBuffer, HANDLE* pSharedHandle);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_CreateIndexBuffer(LPDIRECT3DDEVICE9EX iface, UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer9** ppIndexBuffer, HANDLE* pSharedHandle);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_CreateStateBlock(LPDIRECT3DDEVICE9EX iface, D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9** ppSB);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_BeginStateBlock(LPDIRECT3DDEVICE9EX iface);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_EndStateBlock(LPDIRECT3DDEVICE9EX iface, IDirect3DStateBlock9** ppSB);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_CreateVertexDeclaration(LPDIRECT3DDEVICE9EX iface, CONST D3DVERTEXELEMENT9* pVertexElements, IDirect3DVertexDeclaration9** ppDecl);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetVertexDeclaration(LPDIRECT3DDEVICE9EX iface, IDirect3DVertexDeclaration9* pDecl);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetVertexDeclaration(LPDIRECT3DDEVICE9EX iface, IDirect3DVertexDeclaration9** ppDecl);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_CreateVertexShader(LPDIRECT3DDEVICE9EX iface, CONST DWORD* pFunction, IDirect3DVertexShader9** ppShader);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetVertexShader(LPDIRECT3DDEVICE9EX iface, IDirect3DVertexShader9* pShader);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetVertexShader(LPDIRECT3DDEVICE9EX iface, IDirect3DVertexShader9** ppShader);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetVertexShaderConstantF(LPDIRECT3DDEVICE9EX iface, UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetVertexShaderConstantF(LPDIRECT3DDEVICE9EX iface, UINT StartRegister, float* pConstantData, UINT Vector4fCount);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetVertexShaderConstantI(LPDIRECT3DDEVICE9EX iface, UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetVertexShaderConstantI(LPDIRECT3DDEVICE9EX iface, UINT StartRegister, int* pConstantData, UINT Vector4iCount);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetVertexShaderConstantB(LPDIRECT3DDEVICE9EX iface, UINT StartRegister, CONST BOOL* pConstantData, UINT BoolCount);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetVertexShaderConstantB(LPDIRECT3DDEVICE9EX iface, UINT StartRegister, BOOL* pConstantData, UINT BoolCount);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_CreatePixelShader(LPDIRECT3DDEVICE9EX iface, CONST DWORD* pFunction, IDirect3DPixelShader9** ppShader);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetPixelShader(LPDIRECT3DDEVICE9EX iface, IDirect3DPixelShader9* pShader);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetPixelShader(LPDIRECT3DDEVICE9EX iface, IDirect3DPixelShader9** ppShader);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetPixelShaderConstantF(LPDIRECT3DDEVICE9EX iface, UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetPixelShaderConstantF(LPDIRECT3DDEVICE9EX iface, UINT StartRegister, float* pConstantData, UINT Vector4fCount);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetPixelShaderConstantI(LPDIRECT3DDEVICE9EX iface, UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetPixelShaderConstantI(LPDIRECT3DDEVICE9EX iface, UINT StartRegister, int* pConstantData, UINT Vector4iCount);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_SetPixelShaderConstantB(LPDIRECT3DDEVICE9EX iface, UINT StartRegister, CONST BOOL* pConstantData, UINT BoolCount);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_GetPixelShaderConstantB(LPDIRECT3DDEVICE9EX iface, UINT StartRegister, BOOL* pConstantData, UINT BoolCount);
extern HRESULT  WINAPI  IDirect3DDevice9Impl_CreateQuery(LPDIRECT3DDEVICE9EX iface, D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery);


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

/* ------------------ */
/* IDirect3DResource9 */
/* ------------------ */

/*****************************************************************************
 * IDirect3DResource9 implementation structure
 */
typedef struct IDirect3DResource9Impl
{
    /* IUnknown fields */
    const IDirect3DResource9Vtbl *lpVtbl;
    LONG                    ref;

    /* IDirect3DResource9 fields */
    IWineD3DResource       *wineD3DResource;
} IDirect3DResource9Impl;

extern HRESULT  WINAPI        IDirect3DResource9Impl_GetDevice(LPDIRECT3DRESOURCE9 iface, IDirect3DDevice9** ppDevice);


/* ----------------- */
/* IDirect3DSurface9 */
/* ----------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern const IDirect3DSurface9Vtbl Direct3DSurface9_Vtbl;

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

    /* Flags an implicit surface */
    BOOL                        isImplicit;
} IDirect3DSurface9Impl;

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
    IWineD3DVertexBuffer   *wineD3DVertexBuffer;

    /* Parent reference */
    LPDIRECT3DDEVICE9EX       parentDevice;
} IDirect3DVertexBuffer9Impl;

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
    IWineD3DIndexBuffer    *wineD3DIndexBuffer;

    /* Parent reference */
    LPDIRECT3DDEVICE9EX       parentDevice;
} IDirect3DIndexBuffer9Impl;

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

extern DWORD    WINAPI        IDirect3DBaseTexture9Impl_GetLOD(LPDIRECT3DBASETEXTURE9 iface);

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

void IDirect3DVertexDeclaration9Impl_Destroy(LPDIRECT3DVERTEXDECLARATION9 iface);

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


/* Callbacks */
extern HRESULT WINAPI D3D9CB_CreateSurface(IUnknown *device, IUnknown *pSuperior, UINT Width, UINT Height,
                                         WINED3DFORMAT Format, DWORD Usage, WINED3DPOOL Pool, UINT Level,
                                         WINED3DCUBEMAP_FACES Face, IWineD3DSurface** ppSurface,
                                         HANDLE* pSharedHandle);

extern HRESULT WINAPI D3D9CB_CreateVolume(IUnknown  *pDevice, IUnknown *pSuperior, UINT Width, UINT Height, UINT Depth,
                                          WINED3DFORMAT  Format, WINED3DPOOL Pool, DWORD Usage,
                                          IWineD3DVolume **ppVolume,
                                          HANDLE   * pSharedHandle);

extern HRESULT WINAPI D3D9CB_CreateDepthStencilSurface(IUnknown *device, IUnknown *pSuperior, UINT Width, UINT Height,
                                         WINED3DFORMAT Format, WINED3DMULTISAMPLE_TYPE MultiSample,
                                         DWORD MultisampleQuality, BOOL Discard,
                                         IWineD3DSurface** ppSurface, HANDLE* pSharedHandle);

extern HRESULT WINAPI D3D9CB_CreateRenderTarget(IUnknown *device, IUnknown *pSuperior, UINT Width, UINT Height,
                                         WINED3DFORMAT Format, WINED3DMULTISAMPLE_TYPE MultiSample,
                                         DWORD MultisampleQuality, BOOL Lockable,
                                         IWineD3DSurface** ppSurface, HANDLE* pSharedHandle);

extern ULONG WINAPI D3D9CB_DestroySwapChain (IWineD3DSwapChain *pSwapChain);

extern ULONG WINAPI D3D9CB_DestroyDepthStencilSurface (IWineD3DSurface *pSurface);

extern ULONG WINAPI D3D9CB_DestroyRenderTarget (IWineD3DSurface *pSurface);

extern ULONG WINAPI D3D9CB_DestroySurface(IWineD3DSurface *pSurface);

extern ULONG WINAPI D3D9CB_DestroyVolume(IWineD3DVolume *pVolume);

#endif /* __WINE_D3D9_PRIVATE_H */
