// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics
//      $Keywords:
//
//  $Description:
//      Provides utililites. Visibile outside hw directory.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------


HRESULT CacheHwTextureOnBitmap(
    __inout_ecount(1) IDirect3DTexture9 *pITexture,
    __inout_ecount(1) IWGXBitmap *pBitmap,
    __inout_ecount(1) CD3DDeviceLevel1 *pDevice
    );

HRESULT ReadRenderTargetIntoSysMemBuffer(
    __in IDirect3DSurface9 *pSourceSurface,
    __in const CMilRectU &rcCopy,
    MilPixelFormat::Enum fmtOut,
    UINT uStrideOut,
    DBG_ANALYSIS_PARAM_COMMA(UINT cbBufferOut)
    __out_bcount_full(cbBufferOut) BYTE *pbBufferOut
    );

bool IsD3DFailure(HRESULT hr);



