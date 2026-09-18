// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//  Description:
//      Contains declarations for generic render utility routines.
//
//------------------------------------------------------------------------------

#pragma once

//------------------------------------------------------------------------------
//
//  Defines: Flags for GetMinimalTextureDesc
//
//  Description:
//  Flags passed to GetMinimalTextureDesc that control what fields
//      of the surface description are checked.
//
//------------------------------------------------------------------------------

#define GMTD_IGNORE_WIDTH     0x1
#define GMTD_IGNORE_HEIGHT    0x2
#define GMTD_IGNORE_FORMAT    0x4
#define GMTD_NONPOW2CONDITIONAL_OK  0x10

#define GMTD_CHECK_WIDTH      (GMTD_IGNORE_HEIGHT | \
                                         GMTD_IGNORE_FORMAT)
#define GMTD_CHECK_HEIGHT     (GMTD_IGNORE_WIDTH | \
                                         GMTD_IGNORE_FORMAT)
#define GMTD_CHECK_FORMAT     (GMTD_IGNORE_WIDTH | \
                                         GMTD_IGNORE_HEIGHT)

#define GMTD_CHECK_ALL        (GMTD_CHECK_WIDTH & \
                                         GMTD_CHECK_HEIGHT & \
                                         GMTD_CHECK_FORMAT)

#define GMTD_DEFAULT          GMTD_CHECK_ALL

//
// The bitmap cache logic needs the mipmap levels to have a strict ordering 
// policy.
//
// Mipmaps realizations with greater levels must have a greater value.
//
// Example:
//
// TMML_All > TMML_One
//
enum TextureMipMapLevel
{
    TMML_All = 1,
    TMML_One = 0,
    TMML_Unknown
};


//+----------------------------------------------------------------------------
//
//  Function:
//      TextureAddressingAllowsConditionalNonPower2Usage
//
//  Synopsis:
//      Returns true if given texture addressing modes allow use of D3D's
//      conditional non-power of two support.
//

MIL_FORCEINLINE bool TextureAddressingAllowsConditionalNonPower2Usage(
    D3DTEXTUREADDRESS taU,
    D3DTEXTUREADDRESS taV
    )
{
    // Conditional non-power of two support only works when both texture
    // addressing modes are CLAMP (a.k.a. extend edge) despite presence of
    // D3DPTADDRESSCAPS_INDEPENDENTUV.
    return ((taU == D3DTADDRESS_CLAMP) && (taV == D3DTADDRESS_CLAMP));
}


void PopulateSurfaceDesc(
    D3DFORMAT fmtPixelFormat,
    D3DPOOL d3dPool,
    DWORD dwUsage, 
    UINT uTextureWidth,
    UINT uTextureHeight,
    __out_ecount(1) D3DSURFACE_DESC *pDesc 
    );

HRESULT GetMinimalTextureDesc(
    __in_ecount(1) IDirect3DDevice9 *pD3DDevice,
    D3DFORMAT AdapterFormat,
    __in_ecount(1) D3DCAPS9 const *pcaps,
    __inout_ecount(1) D3DSURFACE_DESC *pd3dsd,
    BOOL fPalUsesAlpha,
    DWORD dwFlags
    );

D3DFORMAT GetSuperiorSurfaceFormat(
    D3DFORMAT d3dFormat,
    BOOL fPalUsesAlpha
    );


