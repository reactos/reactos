// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/**************************************************************************
*

*
* Abstract:
*
*   Contains system memory based bitmap class implementations
*
*
**************************************************************************/

#include "precomp.hpp"

MtDefine(CSystemMemoryBitmap, MILImaging, "CSystemMemoryBitmap");
MtDefine(CClientMemoryBitmap, MILImaging, "CClientMemoryBitmap");
MtDefine(CDummySource, MILImaging, "CDummySource");

MtDefine(MSystemBitmapBits, MILRawMemory, "MSystemBitmapBits");
MtDefine(MSystemBitmapAux, MILRawMemory, "MSystemBitmapAux");


/**************************************************************************
*
* CSystemMemoryBitmap::Create
*
*
**************************************************************************/
HRESULT CSystemMemoryBitmap::Create(
    UINT width,
    UINT height,
    MilPixelFormat::Enum pixelFormat,
    BOOL fClear,
    BOOL fIsDynamic,
    __deref_out CSystemMemoryBitmap **ppSystemMemoryBitmap
    )
{
    HRESULT hr = S_OK;

    CSystemMemoryBitmap * pSystemMemoryBitmap = new CSystemMemoryBitmap();
    IFCOOM(pSystemMemoryBitmap);
    pSystemMemoryBitmap->AddRef();

    IFC(pSystemMemoryBitmap->Init(width, height, pixelFormat, fClear, fIsDynamic));

    *ppSystemMemoryBitmap = pSystemMemoryBitmap;  // take over reference
    pSystemMemoryBitmap = NULL;
    
Cleanup:

    ReleaseInterfaceNoNULL(pSystemMemoryBitmap);

    RRETURN(hr);  
}

/**************************************************************************
*
* CSystemMemoryBitmap::CSystemMemoryBitmap()
*
*
**************************************************************************/

CSystemMemoryBitmap::CSystemMemoryBitmap()
{
    m_nStride = 0;
    m_pPixels = NULL;
    m_fIsDynamic = false;
}

/**************************************************************************
*
* CSystemMemoryBitmap::~CSystemMemoryBitmap()
*
*
**************************************************************************/

CSystemMemoryBitmap::~CSystemMemoryBitmap()
{
    if (m_pPixels)
    {
        GpFree(m_pPixels);
    }
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CSystemMemoryBitmap::HrFindInterface
//
//  Synopsis:
//      Internal method providing support for QueryInterface.
//
//-----------------------------------------------------------------------------
STDMETHODIMP CSystemMemoryBitmap::HrFindInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void **ppvObject)
{
    HRESULT hr = E_INVALIDARG;

    if (ppvObject)
    {
        if (riid == IID_IMILDynamicResource)
        {
            *ppvObject = static_cast<IMILDynamicResource*>(this);

            hr = S_OK;
        }
        else
        {
            hr = CWGXBitmap::HrFindInterface(riid, ppvObject);
        }
    }

    RRETURN(hr);
}

/**************************************************************************
*
* Function Description:
*
*   Lock
*   Provides access to the pixels. The image is locked, but no processing
*   is performed - the pointer returned is a pointer to the actual bitmap
*   so that read and write semantics are supported.
*
* Created:
*
*   10/30/2001 asecchia
*      Created it.
*
**************************************************************************/

HRESULT CSystemMemoryBitmap::Lock(
    __in_ecount_opt(1) const WICRect *prcLock,
    __in DWORD dwFlags,
    __deref_out_ecount(1) IWGXBitmapLock **ppILock
    )
{
    RECT rcLock;
    HRESULT hr = S_OK;


    MIL_THR(HrCheckPixelRect(prcLock, &rcLock));

    // Bit position of the left-coordinate in the lock rectangle.

    UINT nBitLeft = rcLock.left * GetPixelFormatSize(m_PixelFormat);

    // Bit position of the left within the byte which contains the first
    // pixel in the rectangle.

    UINT nBitPosition = nBitLeft % 8;

    if (SUCCEEDED(hr))
    {
        const int c_nWidth = rcLock.right - rcLock.left;
        const int c_nHeight = rcLock.bottom - rcLock.top;

        // Pointer to the byte in which the left most pixel in the top row
        // begins. If this is not a byte-packed format, this will be exactly
        // equal to the start of the first pixel. If it is packed, we must use
        // the nBitPosition variable to align the output.

        void *pvPixels = static_cast<BYTE*>(m_pPixels) +
            (static_cast<int>(nBitLeft) / 8) +
            rcLock.top * static_cast<int>(m_nStride);

        if (nBitPosition == 0)
        {
            WICRect rc = {0, 0, c_nWidth, c_nHeight};

            MIL_THR(HrLock(
                rcLock,
                m_PixelFormat,
                m_nStride,
                GetRequiredBufferSize(m_PixelFormat, m_nStride, &rc),
                pvPixels,
                dwFlags,
                ppILock
                ));
        }
        else
        {
            WICRect rc = {0, 0, c_nWidth, c_nHeight};

            // Handle bit-packed formats (1, 2, 4bpp) for which the left-hand
            // side of the lock rectangle does not fall on a byte-aligned
            // address.

            UINT nBitRight = rcLock.right * GetPixelFormatSize(m_PixelFormat);

            MIL_THR(HrLockUnaligned(
                rcLock,
                m_PixelFormat,
                nBitPosition,
                nBitRight - nBitLeft,
                m_nStride,
                GetRequiredBufferSize(m_PixelFormat, m_nStride, &rc),
                pvPixels,
                dwFlags,
                ppILock
                ));
        }
    }

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CSystemMemoryBitmap::UnsafeUpdateFromSource
//
//  Synopsis:
//      Update a portion of bitmap from given source.  This method is unsafe in
//      that is doesn't verify specified area to update.  Nor does it handle
//      bit depths other than multiples of 8.  Those are left to the caller.
//

HRESULT CSystemMemoryBitmap::UnsafeUpdateFromSource(
    __in_ecount(1) IWGXBitmapSource *pISource,
    __in_ecount(1) MilRectU &rcSrc,
    UINT uDstLeft,
    UINT uDstTop
    )
{
    HRESULT hr = S_OK;

    Assert(rcSrc.left < rcSrc.right);
    Assert(rcSrc.top < rcSrc.bottom);
    Assert(uDstLeft < m_nWidth);
    Assert(uDstTop < m_nHeight);

    WICRect rcUpdate = {
        rcSrc.left, rcSrc.top,
        rcSrc.right - rcSrc.left, rcSrc.bottom - rcSrc.top
    };

    Assert(static_cast<UINT>(rcUpdate.Width) <= m_nWidth);
    Assert(static_cast<UINT>(rcUpdate.Height) <= m_nHeight);
#if DBG
    UINT uDbgDstRight = uDstLeft + rcUpdate.Width;
    Assert(uDbgDstRight > uDstLeft);    // Check overflow
    Assert(uDbgDstRight <= m_nWidth);
    UINT uDbgDstBottom = uDstTop + rcUpdate.Height;
    Assert(uDbgDstBottom > uDstTop);    // Check overflow
    Assert(uDbgDstBottom <= m_nHeight);
#endif

    UINT uPixelSize = GetPixelFormatSize(m_PixelFormat);

    Assert(uPixelSize % 8 == 0);
    uPixelSize /= 8;

    BYTE *pbDestPixels = reinterpret_cast<BYTE *>(m_pPixels)
                         + uPixelSize * uDstLeft
                         + m_nStride * uDstTop;

    MIL_THR(pISource->CopyPixels(
        &rcUpdate,
        m_nStride,
        m_nStride * (rcUpdate.Height - 1) + uPixelSize * rcUpdate.Width,
        pbDestPixels
        ));

    RRETURN(hr);
}


/**************************************************************************
*
* CSystemMemoryBitmap::IsDynamicResource
*
*
**************************************************************************/
HRESULT CSystemMemoryBitmap::IsDynamicResource(
    __out_ecount(1) bool *pfIsDynamic
    )
{
    *pfIsDynamic = m_fIsDynamic;
    return S_OK;
}

/**************************************************************************
*
* CSystemMemoryBitmap::Init
*
*
**************************************************************************/

HRESULT CSystemMemoryBitmap::Init(
    __in UINT nWidth,
    __in UINT nHeight,
    __in MilPixelFormat::Enum pxlFormat,
    __in BOOL fClear,
    __in BOOL fIsDynamic
    )
{
    HRESULT hr = S_OK;

    UINT nStride;

    m_fIsDynamic = !!fIsDynamic;

    IFC(HrCalcDWordAlignedScanlineStride(nWidth, pxlFormat, nStride));

    Assert(nStride > 0);

    IFC(HrMalloc(
        Mt(MSystemBitmapBits),
        nStride,
        nHeight,
        &m_pPixels
        ));

    m_nWidth = nWidth;
    m_nHeight = nHeight;
    m_PixelFormat = pxlFormat;
    m_nStride = nStride;

    if (fClear) // Clear to 0 which is usually black
    {
        GpMemset(m_pPixels, 0, nStride * nHeight);
    }

Cleanup:
    RRETURN(hr);
}

/**************************************************************************
*
* CSystemMemoryBitmap::Init
*
*
**************************************************************************/

HRESULT CSystemMemoryBitmap::Init(
    __in_ecount(1) IWGXBitmapSource *pISource
    )
{
    HRESULT hr = E_INVALIDARG;

    m_fIsDynamic = false;
    
    if (pISource)
    {
        MIL_THR(HrInitDataFromBitmapSource(pISource));

        if (SUCCEEDED(hr))
        {
            MIL_THR(HrCalcDWordAlignedScanlineStride(m_nWidth, m_PixelFormat, m_nStride));
            Assert(m_nStride > 0);
        }

        if (SUCCEEDED(hr))
        {
            MIL_THR(HrMalloc(
                Mt(MSystemBitmapBits),
                m_nStride,
                m_nHeight,
                &m_pPixels
                ));
        }

        if (SUCCEEDED(hr))
        {
            WICRect rc = {0, 0, m_nWidth, m_nHeight};
            MIL_THR(pISource->CopyPixels(&rc, m_nStride, m_nStride*m_nHeight, (BYTE*)m_pPixels));
        }

    }

    if (FAILED(hr))
    {
        if (m_pPixels != NULL)
        {
            GpFree(m_pPixels);
            m_pPixels = NULL;
        }
    }

    RRETURN(hr);
}

/**************************************************************************
*
* CSystemMemoryBitmap::Init
*
*
**************************************************************************/

HRESULT CSystemMemoryBitmap::Init(
    __in_ecount(1) IWGXBitmapSource *pISource,
    __in_ecount(1) WICRect * srcRect,
    BOOL fCopySource
    )
{
    HRESULT hr = E_INVALIDARG;
    WICRect rc;

    m_fIsDynamic = false;

    if (pISource)
    {
        hr = S_OK;

        if (srcRect != NULL)
        {
            UINT    width = 0;
            UINT    height = 0;

            MIL_THR(pISource->GetSize(&width, &height));

            if (SUCCEEDED(hr))
            {
                if ((srcRect->Width > 0) && (srcRect->Height > 0) && (srcRect->X < (int)width) && (srcRect->Y < (int)height))
                {
                    // Crop the srcRect, if necessary
                    rc = *srcRect;

                    if (rc.X < 0)
                    {
                        rc.Width += rc.X;
                        rc.X = 0;
                    }
                    if ((rc.X + rc.Width) > (int)width)
                    {
                        rc.Width = width - rc.X;
                    }

                    if (rc.Y < 0)
                    {
                        rc.Height += rc.Y;
                        rc.Y = 0;
                    }
                    if ((rc.Y + rc.Height) > (int)height)
                    {
                        rc.Height = height - rc.Y;
                    }

                    if ((rc.Width > 0) && (rc.Height > 0))
                    {
                        srcRect = &rc;
                    }
                    else
                    {
                        MIL_THR(E_INVALIDARG);    // invalid srcRect
                    }
                }
                else
                {
                    MIL_THR(E_INVALIDARG);        // invalid srcRect
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            MIL_THR(HrInitDataFromBitmapSource(pISource, srcRect));
        }

        if (SUCCEEDED(hr))
        {
            MIL_THR(HrCalcDWordAlignedScanlineStride(m_nWidth, m_PixelFormat, m_nStride));
            Assert(m_nStride > 0);
        }

        if (SUCCEEDED(hr))
        {
            MIL_THR(HrMalloc(
                Mt(MSystemBitmapBits),
                m_nStride,
                m_nHeight,
                &m_pPixels
                ));
        }

        if (SUCCEEDED(hr)
            && fCopySource
            )
        {
            if (srcRect == NULL)
            {
                WICRect rcDefault = {0, 0, m_nWidth, m_nHeight};
                MIL_THR(pISource->CopyPixels(&rcDefault, m_nStride, m_nStride*m_nHeight, (BYTE*)m_pPixels));
            }
            else
            {
                MIL_THR(pISource->CopyPixels(srcRect, m_nStride, m_nStride*m_nHeight, (BYTE*)m_pPixels));
            }
        }
    }

    if (FAILED(hr))
    {
        if (m_pPixels != NULL)
        {
            GpFree(m_pPixels);
            m_pPixels = NULL;
        }
    }

    RRETURN(hr);
}

/**************************************************************************
*
* CClientMemoryBitmap::~CClientMemoryBitmap
*
*
**************************************************************************/

CClientMemoryBitmap::~CClientMemoryBitmap()
{
    // Set m_pPixels to NULL so CSystemMemoryBitmap::~CSystemMemoryBitmap
    // doesn't free it
    m_pPixels = NULL;
}

/**************************************************************************
*
* CClientMemoryBitmap::HrInit
*
*
**************************************************************************/

HRESULT CClientMemoryBitmap::HrInit(
    __in UINT nWidth,
    __in UINT nHeight,
    __in MilPixelFormat::Enum pxlFormat,
    __in UINT cbBufferSize,
    __in_bcount(cbBufferSize) void *pvPixels,
    __in UINT cbStride
    )
{
    HRESULT hr = S_OK;

    //
    // Ensure that the caller is passing us a valid pixel format. It is invalid
    // to pass in bogus pixel formats. This Assert is to simplify tracking
    // down mistakes in the caller.
    //

    Assert(IsValidPixelFormat(pxlFormat));

    // validate parameters

    if (nWidth == 0 || nHeight == 0 || !pvPixels || cbStride == 0)
    {
        MIL_THR(E_INVALIDARG);
    }

    if (SUCCEEDED(hr) && (nHeight >= INT_MAX / cbStride))
    {
        MIL_THR(E_INVALIDARG);
    }

    if (SUCCEEDED(hr))
    {
        MIL_THR(HrCheckBufferSize(
                pxlFormat,
                cbStride,
                nWidth,
                nHeight,
                cbBufferSize));
    }

    if (SUCCEEDED(hr))
    {
        // Copy the caller's parameters
        m_nWidth = nWidth;
        m_nHeight = nHeight;
        m_pPixels = pvPixels;
        m_PixelFormat = pxlFormat;
        m_nStride = cbStride;
    }

    RRETURN(hr);
}

/**************************************************************************
*
* CDummySource::CDummySource
*
*
**************************************************************************/

CDummySource::CDummySource(
    __in UINT nWidth,
    __in UINT nHeight,
    __in MilPixelFormat::Enum pxlFormat)
{
    m_nWidth = nWidth;
    m_nHeight = nHeight;
    m_PixelFormat = pxlFormat;

    Assert (pxlFormat != MilPixelFormat::DontCare);
}

/**************************************************************************
*
* CDummySource::~CDummySource
*
*
**************************************************************************/

CDummySource::~CDummySource()
{
}

/**************************************************************************
*
* CDummySource::HrFindInterface
*
*
**************************************************************************/

STDMETHODIMP CDummySource::HrFindInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void **ppvObject)
{
    HRESULT hr = E_INVALIDARG;

    if (ppvObject)
    {
        if (riid == IID_IWGXBitmapSource)
        {
            *ppvObject = static_cast<IWGXBitmapSource*>(this);

            hr = S_OK;
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }

    return hr;
}

/**************************************************************************
*
* CDummySource::CopyPixels
*
*
**************************************************************************/

HRESULT CDummySource::CopyPixels(
    __in_ecount_opt(1) const MILRect *prc,
    __in UINT cbStride,
    __in UINT cbBufferSize,
    __out_ecount(cbBufferSize) BYTE *pvPixels
    )
{
    Assert (pvPixels != NULL);

    if (!pvPixels || !prc)
        RRETURN(E_INVALIDARG);

    HRESULT hr = S_OK;
    RECT rcLock;

    SetRect(&rcLock, 0, 0, m_nWidth, m_nHeight);
    if (prc)
    {
        RECT rc = {
            prc->X,
            prc->Y,
            prc->X+prc->Width,
            prc->Y+prc->Height
        };

        IntersectRect(&rcLock, &rc, &rcLock);

        if (IsRectEmpty(&rc) || !EqualRect(&rcLock, &rc))
        {
            RRETURN(E_INVALIDARG);
        }
    }

    WICRect mrc = {rcLock.left, rcLock.top, rcLock.right - rcLock.left, rcLock.bottom - rcLock.top};

    IFC(HrCheckBufferSize(m_PixelFormat, cbStride, &mrc, cbBufferSize));

    rcLock.right -= rcLock.left;
    rcLock.bottom -= rcLock.top;

    UINT nStride;
    IFC(HrCalcDWordAlignedScanlineStride(rcLock.right, m_PixelFormat, nStride));
    BYTE *pbPixels = pvPixels;
    for (INT n = 0; n < rcLock.bottom; n++)
    {
#pragma prefast(push)
#pragma prefast(disable:2015, "prefast can't correctly recognize the buffer size validation")
        GpMemset(pbPixels, 0, nStride);
#pragma prefast(pop)
        pbPixels += cbStride;
    }

Cleanup:
    RRETURN(hr);
}

/**************************************************************************
*
* CDummySource::GetSize
*
*
**************************************************************************/

STDMETHODIMP CDummySource::GetSize(
    __out_ecount(1) UINT *pnWidth,
    __out_ecount(1) UINT *pnHeight
    )
{
    Assert (pnWidth != NULL);
    Assert (pnHeight != NULL);

    if (!pnWidth || !pnHeight)
        RRETURN(E_INVALIDARG);

    *pnWidth = m_nWidth;
    *pnHeight = m_nHeight;

    RRETURN(S_OK);
}

/**************************************************************************
*
* CDummySource::GetPixelFormat
*
*
**************************************************************************/

STDMETHODIMP CDummySource::GetPixelFormat(
    __out_ecount(1) MilPixelFormat::Enum *pPixelFormat
    )
{
    *pPixelFormat = m_PixelFormat;
    
    return S_OK;
}

/**************************************************************************
*
* CDummySource::CopyPalette
*
*
**************************************************************************/

STDMETHODIMP CDummySource::CopyPalette(
    __in_ecount(1) IWICPalette *pIPalette
    )
{
    // Return OK without modifying the palette - this allows us to create
    // Indexed bitmaps using an uninitialized palette. The caller must
    // separately initialize the palette.

    RRETURN(S_OK);
}

/**************************************************************************
*
* CDummySource::GetResolution
*
*
**************************************************************************/

STDMETHODIMP CDummySource::GetResolution(
    __out_ecount(1) double *pDpiX,
    __out_ecount(1) double *pDpiY
    )
{
    Assert(pDpiX);
    Assert(pDpiY);

    if (!pDpiX || !pDpiY)
        RRETURN(E_INVALIDARG);

    // Use a default value of 0. This indicates "unknown".

    *pDpiX = 0.0;
    *pDpiY = 0.0;

    RRETURN(S_OK);
}






