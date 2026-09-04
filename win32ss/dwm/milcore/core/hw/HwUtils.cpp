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

#include "precomp.hpp"


//+----------------------------------------------------------------------------
//
//  Member:
//      CacheHwTextureOnBitmap
//
//  Synopsis:
//      Caches a hardware texture on the specified bitmap.
//
//  Notes:
//      The texture is assumed to have fully valid contents, and to be the same
//      size as the bitmap.
//

HRESULT
CacheHwTextureOnBitmap(
    __inout_ecount(1) IDirect3DTexture9 *pTexture,
    __inout_ecount(1) IWGXBitmap *pBitmap,
    __inout_ecount(1) CD3DDeviceLevel1 *pDevice
    )
{
    HRESULT hr = S_OK;

    CD3DVidMemOnlyTexture *pVidMemTexture  = NULL;
    CHwBitmapCache *pBitmapCache = NULL;
    CHwDeviceBitmapColorSource *pDeviceBitmapColorSource = NULL;
    UINT textureWidth;
    UINT textureHeight;

    //
    // Create a video memory texture wrapper for the texture.
    //
     
    IFC(CD3DVidMemOnlyTexture::Create(
        pTexture,
        false,              // fIsEvictable
        pDevice,
        &pVidMemTexture
        ));

    pVidMemTexture->GetTextureSize(
        &textureWidth,
        &textureHeight
        );


#if DBG
    {
        UINT dbgBitmapWidth;
        UINT dbgBitmapHeight;
        IGNORE_HR(pBitmap->GetSize(&dbgBitmapWidth, &dbgBitmapHeight));

        Assert(dbgBitmapWidth == textureWidth);
        Assert(dbgBitmapHeight == textureHeight);
    }
#endif

    //
    // Check for a bitmap cache.  Create and store one if it doesn't exist.
    //
    // Normally caching is optional, but in this case it is required, since
    // failure caching means we won't have access to the device bitmap
    // surface later when it is used as source.
    //

    IFC(CHwBitmapCache::GetCache(
        pDevice,
        pBitmap,
        NULL,
        true, 			// fSetResourceRequired
        &pBitmapCache
        ));

    {
        CMilRectU rcSurfBounds(0, 0, textureWidth, textureHeight, LTRB_Parameters);
        
        // Create the color source and put it in the cache
        IFC(pBitmapCache->CreateColorSourceForTexture(
            D3DFormatToPixelFormat(
                pVidMemTexture->D3DSurface0Desc().Format,
                FALSE),
            rcSurfBounds,           // rcBoundsRequired
            pVidMemTexture,
            OUT &pDeviceBitmapColorSource
            ));
        
        // Let the bitmap color source know that it contains fully valid bits.
        // A precondition for this function is that the texture contains valid
        // bits.
        pDeviceBitmapColorSource->UpdateValidBounds(
            rcSurfBounds
        );
    }

Cleanup:
    ReleaseInterfaceNoNULL(pVidMemTexture);
    ReleaseInterfaceNoNULL(pBitmapCache);
    ReleaseInterfaceNoNULL(pDeviceBitmapColorSource);
    
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Function:  ReadRenderTargetIntoSysMemBuffer
//
//  Synopsis:  Reads the surface's contents in the specified source rectangle
//             and writes it into the provided buffer.
//
//             Copied from CD3DSurface::ReadIntoSysMemBuffer because 
//             pSourceSurface is not from one of our devices. The original 
//             could have been refactored to work on IDirect3DDevice9, but then 
//             we would have lost CD3DDeviceLevel1 specific functionality 
//             (entry check, vid mem management...)
//
//             Differences:
//              1) No clip rects
//              2) pbBufferOut is the upper left corner, so we need to offset 
//                 it by rcCopy if manually updating
//              3) Only works if pSourceSurface is D3DPOOL_DEFAULT
//              4) Only works if system memory and video memory are same fmt
//
//+----------------------------------------------------------------------------

HRESULT
ReadRenderTargetIntoSysMemBuffer(
    __in IDirect3DSurface9 *pSourceSurface,
    __in const CMilRectU &rcCopy,
    MilPixelFormat::Enum fmtOut,
    UINT uStrideOut,
    DBG_ANALYSIS_PARAM_COMMA(UINT cbBufferOut)
    __out_bcount_full(cbBufferOut) BYTE *pbBufferOut
    )
{
    HRESULT hr = S_OK;

    const UINT uBitsPerPixel = GetPixelFormatSize(fmtOut);
    UINT uStrideCopy;
    const UINT uCopyWidth = rcCopy.Width();
    const UINT uCopyHeight = rcCopy.Height();
    const RECT rcDest = { 0, 0, uCopyWidth, uCopyHeight };

    IDirect3DDevice9 *pIDevice = NULL;
    IDirect3DDevice9Ex *pIDeviceEx = NULL;

    IDirect3DTexture9 *pD3DLockableTexture = NULL;
    IDirect3DSurface9 *pD3DLockableSurface = NULL;
    IDirect3DSurface9 *pD3DVidMemCopySurface = NULL;

    if (   rcCopy.left > SURFACE_RECT_MAX 
        || rcCopy.right > SURFACE_RECT_MAX 
        || rcCopy.top > SURFACE_RECT_MAX 
        || rcCopy.bottom > SURFACE_RECT_MAX
        || !rcCopy.IsWellOrdered())
    {
        IFC(E_INVALIDARG);
    }

    if (uBitsPerPixel % BITS_PER_BYTE)
    {
        IFC(E_INVALIDARG);
    }

    D3DSURFACE_DESC d3dsd;
    IFC(pSourceSurface->GetDesc(&d3dsd));

    if (d3dsd.Pool != D3DPOOL_DEFAULT)
    {
        IFC(E_INVALIDARG);
    }

    const D3DFORMAT d3dfmtOut = PixelFormatToD3DFormat(fmtOut);
    if (d3dsd.Format != d3dfmtOut)
    {
        IFC(E_INVALIDARG);
    }

    IFC(HrCalcByteAlignedScanlineStride(uCopyWidth, uBitsPerPixel, OUT uStrideCopy));

#if DBG_ANALYSIS
    Assert(uStrideCopy <= uStrideOut);
    Assert(SUCCEEDED(HrCheckBufferSize(fmtOut, uStrideOut, uCopyWidth, uCopyHeight, cbBufferOut)));
#endif

    IFC(pSourceSurface->GetDevice(&pIDevice));
    IGNORE_HR(pIDevice->QueryInterface(
        __uuidof(IDirect3DDevice9Ex),
        reinterpret_cast<void **>(&pIDeviceEx)
        ));

    bool fNeedToManuallyCopyBits = true;
    void *pvSysMemPixels = NULL;

    // If we're on WDDM, D3D can make a surface with our system memory assuming the layout
    // is the same.
    if (pIDeviceEx && uStrideCopy == uStrideOut)
    {
        fNeedToManuallyCopyBits = false;
        pvSysMemPixels = pbBufferOut;
    }

    // Create system memory surface to GetRenderTargetData() into. If the user's device
    // is a WDDM device capable of sharing, we'll create a system memory texture from already
    // existing bits to avoid allocating more system memory. We aren't using an offscreen plain 
    // surface because it has LockRect synchronization issues on XP.
    IFC(pIDevice->CreateTexture(
        uCopyWidth,
        uCopyHeight,
        1,          // Level
        0,          // Usage
        d3dfmtOut,
        D3DPOOL_SYSTEMMEM,
        &pD3DLockableTexture,
        pIDeviceEx && pvSysMemPixels ? reinterpret_cast<HANDLE *>(&pvSysMemPixels) : NULL // pSharedHandle
        ));

    IFC(pD3DLockableTexture->GetSurfaceLevel(
        0,
        &pD3DLockableSurface
        ));

    // GetRenderTargetData can only read the entire surface, so if we aren't trying to copy the
    // entire surface, StretchRect to a new surface equal to what we're copying and GRTD it. We 
    // also need to do this for multisampled surfaces as GetRenderTargetData doesn't work on them.
    if (   uCopyWidth != d3dsd.Width 
        || uCopyHeight != d3dsd.Height 
        || d3dsd.MultiSampleType != D3DMULTISAMPLE_NONE)
    {
        IFC(pIDevice->CreateRenderTarget(
            uCopyWidth,
            uCopyHeight,
            d3dsd.Format,
            D3DMULTISAMPLE_NONE,
            0,          // MS quality
            FALSE,      // Lockable
            &pD3DVidMemCopySurface,
            NULL        // pSharedHandle
            ));

        IFC(pIDevice->StretchRect(
            pSourceSurface,
            reinterpret_cast<const CMilRectL *>(&rcCopy),    // cast is safe, checked for int above
            pD3DVidMemCopySurface,
            &rcDest,
            D3DTEXF_NONE
            ));
    }
    else
    {
        SetInterface(pD3DVidMemCopySurface, pSourceSurface);
    }

    IFC(pIDevice->GetRenderTargetData(
        pD3DVidMemCopySurface,
        pD3DLockableSurface
        ));

    if (fNeedToManuallyCopyBits)
    {
        D3DLOCKED_RECT rcLock;
        IFC(pD3DLockableSurface->LockRect(&rcLock, &rcDest, D3DLOCK_READONLY));

        const UINT uBytesPerPixel = uBitsPerPixel / BITS_PER_BYTE;
        const BYTE *pbBufferIn = static_cast<const BYTE *>(rcLock.pBits);
#if DBG_ANALYSIS
        const BYTE *pbAnalysisBufferOutOrig = pbBufferOut;
#endif

        // If we're copying full rows, then we can do one memcpy. D3D surface pitch does not
        // necessarily equal width * bpp
        if (   uStrideCopy == static_cast<UINT>(rcLock.Pitch) 
            && rcCopy.right - rcCopy.left == d3dsd.Width)
        {     
            UINT cbCopy;
            IFC(UIntMult(uStrideCopy, rcCopy.bottom - rcCopy.top, &cbCopy));

            Assert(cbCopy <= cbBufferOut);
            
            memcpy(pbBufferOut, pbBufferIn, cbCopy);
        }
        else
        {
            // pbBufferOut is the upper left pixel so adjust for rcCopy
            UINT cbLeftInset, cbBufferOutInset;
            IFC(UIntMult(uBytesPerPixel, rcCopy.left, &cbLeftInset));
            IFC(UIntMult(uStrideOut, rcCopy.top, &cbBufferOutInset));
            IFC(UIntAdd(cbLeftInset, cbBufferOutInset, &cbBufferOutInset));
            pbBufferOut += cbBufferOutInset;
            
            for (UINT i = rcCopy.top; i < rcCopy.bottom; ++i)
            {
                Assert(pbBufferOut + uStrideCopy <= pbAnalysisBufferOutOrig + cbBufferOut);
                
                memcpy(pbBufferOut, pbBufferIn, uStrideCopy);
                pbBufferOut += uStrideOut;
                pbBufferIn += rcLock.Pitch;
            }
        }

        IGNORE_HR(pD3DLockableSurface->UnlockRect());
    }

Cleanup:
    ReleaseInterface(pD3DLockableTexture);
    ReleaseInterface(pD3DLockableSurface);
    ReleaseInterface(pD3DVidMemCopySurface);
    ReleaseInterface(pIDevice);
    ReleaseInterface(pIDeviceEx);

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:  IsD3DFailure
//
//  Synopsis:  Returns true if "hr" is a bad D3D HRESULT
//
//+----------------------------------------------------------------------------

bool IsD3DFailure(HRESULT hr)
{
    static const HRESULT hrD3DMask = MAKE_D3DHRESULT(0);
    return static_cast<HRESULT>(hr & hrD3DMask) == hrD3DMask;   
}




