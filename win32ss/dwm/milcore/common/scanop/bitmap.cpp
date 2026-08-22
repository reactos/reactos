// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/**************************************************************************
*

*
* Abstract:
*
*   Contains bitmap base class implementation
*
**************************************************************************/

#include "precomp.hpp"

MtDefine(CBaseWGXBitmap, MILImaging, "CBaseWGXBitmap");
MtDefine(CWGXBitmapLock, MILImaging, "CWGXBitmapLock");
MtDefine(CWGXBitmapLockUnaligned, MILImaging, "CWGXBitmapLockUnaligned");

MtDefine(MUnalignedLockData, MILRawMemory, "MUnalignedLockData");

MtDefine(DirtyRectArray, CWGXBitmap, "CWGXBitmap::m_rgDirtyRects");

CBaseWGXBitmap::CBaseWGXBitmap()
{
    m_rgDirtyRects = NULL;
    m_cDirtyRects = 0;
    m_fClearDirtyListOnNextAdd = true;
    m_nUniquenessBeforeFirstDirtyRect = 0;
    m_nUniquenessAtLastDirtyRect = 0;
}

CBaseWGXBitmap::~CBaseWGXBitmap()
{
    WPFFree(ProcessHeap, m_rgDirtyRects);
}

STDMETHODIMP
CBaseWGXBitmap::HrFindInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void **ppvObject
    )
{
    HRESULT hr = E_INVALIDARG;

    if (ppvObject)
    {
        if (riid == IID_IMILResourceCache)
        {
            *ppvObject = static_cast<IMILResourceCache*>(this);

            hr = S_OK;
        }
        else if (riid == IID_IWGXBitmap)
        {
            *ppvObject = static_cast<IWGXBitmap*>(this);

            hr = S_OK;
        }
        else if (riid == IID_IWGXBitmapSource)
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

//+------------------------------------------------------------------------
//
//  Function:  CWGXBitmap::AddDirtyRect
//
//  Synopsis:  Adds a new rectangle to the dirty list. Pass NULL to mark
//             the entire bitmap as dirty. This function updates the
//             uniqueness count so that the bitmap will be re-realized
//             upon drawing.
//
//-------------------------------------------------------------------------
STDMETHODIMP
CBaseWGXBitmap::AddDirtyRect(
    __in_ecount(1) const RECT *prcDirtyRect
    )
{
    HRESULT hr = S_OK;

    AssertEntry(*this);

    UINT uWidth, uHeight;
    IFC(GetSize(&uWidth, &uHeight));

    if (   (prcDirtyRect != NULL)
        && (   prcDirtyRect->left < 0
            || prcDirtyRect->top < 0
            || prcDirtyRect->right <= prcDirtyRect->left
            || (static_cast<UINT>(prcDirtyRect->right) > uWidth)
            || prcDirtyRect->bottom <= prcDirtyRect->top
            || (static_cast<UINT>(prcDirtyRect->bottom) > uHeight)
       )   )
    {
        IFC(E_INVALIDARG);
    }

    CMilRectU const *prcDirty = reinterpret_cast<CMilRectU const *>(prcDirtyRect);

    // No need to remember dirty rects for a bitmap that is not
    // cached yet.
    if (CMILResourceCache::Count == 0)
    {
        Assert(m_cDirtyRects == 0);
        goto Cleanup;
    }

    if (m_fClearDirtyListOnNextAdd ||
        m_nUniquenessAtLastDirtyRect != GetUniqueCount())
    {
        m_cDirtyRects = 0;

        m_fClearDirtyListOnNextAdd = FALSE;
    }

    //
    // prcDirtyRect == NULL implies that the entire
    // surface is dirty
    //

    if ((prcDirty == NULL) ||
        ((prcDirty->Width<UINT>() == uWidth) &&
         (prcDirty->Height<UINT>() == uHeight)))
    {
        m_cDirtyRects = 0;
    }
    else
    {
        if (m_cDirtyRects == 0)
        {
            m_nUniquenessBeforeFirstDirtyRect = GetUniqueCount();
        }
        else
        {
            Assert(m_rgDirtyRects);
        }

        if (   (m_cDirtyRects > 0)
            && (m_rgDirtyRects[0].DoesContain(*prcDirty)))
        {
            // No dirty list change - new dirty rect is already included.
        }
        else if (m_cDirtyRects >= c_maxBitmapDirtyListSize)
        {
            // Collapse dirty list to a single large rect (including new rect)
            while (m_cDirtyRects > 1)
            {
                m_rgDirtyRects[0].Union(m_rgDirtyRects[--m_cDirtyRects]);
            }
            m_rgDirtyRects[0].Union(*prcDirty);

            Assert(m_cDirtyRects == 1);
        }
        else
        {
            if (!m_rgDirtyRects)
            {
                m_rgDirtyRects =
                    WPFAllocType(CMilRectU *,
                                 ProcessHeap, Mt(DirtyRectArray),
                                 c_maxBitmapDirtyListSize*sizeof(*m_rgDirtyRects));
                IFCOOM(m_rgDirtyRects);
            }

            m_rgDirtyRects[m_cDirtyRects++] = *prcDirty;
        }
    }

Cleanup:
    // Update the uniqueness count to ensure that
    // the bitmap knows there is a change
    UpdateUniqueCount();

    if (SUCCEEDED(hr))
    {
        m_nUniquenessAtLastDirtyRect = GetUniqueCount();
    }
    else
    {
        // If we fail then we should update the entire bitmap. Clearing
        // the dirty list will cause this to happen.
        m_cDirtyRects = 0;
    }
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CBaseWGXBitmap::GetDirtyRects
//
//  Synopsis:
//      Gets a reference to the dirty rect array.  Additionally any call will
//      set call to clear out the list upon next dirty addition.
//
//  Return:
//      True means caller's cache (whatever the caller retains and is
//      associated with uniqueness) is not completely invalid and dirty list is
//      useful.  Note caller's cache may be completely valid, which is
//      indicated by *pcDirtyRects be set to 0.
//
//      False means just the opposite.  The caller cache is at least somewhat
//      invalid.  But since no dirty rect list can be generated that indicates
//      what area are invalid, the caller's cache must be treated as completely
//      invalid.
//
//  Outputs:
//      Out parameters are always set, even if false it returned.
//
//      *pCachedUniqueness will always hold the bitmap's current uniqueness
//      value.
//
//      If false is returned or given cached uniqueness (*pCachedUniqueness)
//      matches current uniqueness, then:
//          1. *pcDirtyRects will be 0 and
//          2. *prgDirtyRects will be NULL.
//      Otherwise:
//          1. *pcDirtyRects will be a value from 1 to 5 and
//          2. *prgDirtyRects will be an array of *pcDirtyRects rects.  The
//             array may contain rectangles that overlap.
//
//-----------------------------------------------------------------------------
__success(true) STDMETHODIMP_(bool) 
CBaseWGXBitmap::GetDirtyRects(
    __deref_out_ecount(*pcDirtyRects) MilRectU const ** const prgDirtyRects,   // disallow assignment to prgDirtyRects directly
    __deref_out_range(0,5) UINT * const pcDirtyRects,
    __inout_ecount(1) UINT * const pCachedUniqueness
    )
{
    // Note: the caller should assert the entry for the length of their
    // use of this dirty list. This assert is just an extra protection
    AssertEntry(*this);

    Assert(prgDirtyRects);
    Assert(pcDirtyRects);

    bool fReturningValidDirtyList;

    // Set indicator that dirty list should be cleared on next dirty add.
    // Clearing is done so that the most recent caller of GetDirtyRects has the
    // best chance of getting a positive result the next time it calls.
    m_fClearDirtyListOnNextAdd = TRUE;


    UINT nCurrentUnique = GetUniqueCount();
    UINT nCachedUniqueness = *pCachedUniqueness;

    // Always update cached uniqueness - this is the uniqueness that will be
    // matched after applying dirty rects (or a full update if no dirty list is
    // available.)
    *pCachedUniqueness = nCurrentUnique;

    //
    // Check to see if the cache must be updated by this
    // dirty rect list. Checks to make sure that the list is
    // a. meaningful to the cache
    // b. valid
    // c. non-empty
    //

    if (nCachedUniqueness == m_nUniquenessBeforeFirstDirtyRect &&
        m_nUniquenessAtLastDirtyRect == nCurrentUnique &&
        m_cDirtyRects != 0)
    {
        Assert(m_rgDirtyRects);
        *prgDirtyRects = m_rgDirtyRects;
        *pcDirtyRects = m_cDirtyRects;
        fReturningValidDirtyList = true;
    }
    else
    {
        // In this case the cache should update the entire rect
        // if the uniqueness is different
        *prgDirtyRects = NULL;
        *pcDirtyRects = 0;

        // If the uniqueness is different, then there is no available dirty
        // list.  When the same, the list is empty.  Either way the same
        // pointer and count is returned.
        fReturningValidDirtyList = (nCachedUniqueness == nCurrentUnique);
    }

    return fReturningValidDirtyList;
}

STDMETHODIMP_(IWGXBitmap::SourceState::Enum) 
CBaseWGXBitmap::SourceState() const
{
    return IWGXBitmap::SourceState::FullSystemMemory;
}

STDMETHODIMP_(void)
CBaseWGXBitmap::GetUniquenessToken(
    __out_ecount(1) UINT *puToken
    ) const
{
    *puToken = GetUniqueCount();
}

CWGXBitmap::CWGXBitmap()
{
    m_pPalette = NULL;

    m_nWidth = 0;
    m_nHeight = 0;

    // Use a default value of 0 to show resolution hasn't been set. This is
    // safer than DpiScale::DefaultPixelsPerInch(), which could cause rendering bugs, since we
    // usually test at the same desktop DPI.

    m_fltDpiX = 0.0f;
    m_fltDpiY = 0.0f;

    m_PixelFormat = MilPixelFormat::DontCare;
}

CWGXBitmap::~CWGXBitmap()
{
    ReleaseInterfaceNoNULL(m_pPalette);

    Verify(SUCCEEDED(m_lockState.CheckNoLocks()));
}

HRESULT CWGXBitmap::Init(
    __in_ecount(1) IWGXBitmapSource *pISource
    )
{
    return E_FAIL;
}

HRESULT CWGXBitmap::CopyPixelsHelper(
    __in MilPixelFormat::Enum pixelFormat,
    __in UINT width,
    __in UINT height,
    __in UINT cbInputStride,
    __in UINT cbInputBufferSize,
    __in_bcount(cbInputBufferSize) BYTE *pbInputBuffer,
    __in UINT cbOutputStride,
    __in UINT cbOutputBufferSize,
    __out_bcount(cbOutputBufferSize) BYTE *pbOutputBuffer
    )
{
    HRESULT hr = S_OK;

    UINT cbCopyStride = 0;

    // Check for input overflow
    IFC(HrCheckBufferSize(
            pixelFormat,
            cbInputStride,
            width,
            height,
            cbInputBufferSize));

    // Check for output overflow
    IFC(HrCheckBufferSize(
            pixelFormat,
            cbOutputStride,
            width,
            height,
            cbOutputBufferSize));

    IFC(HrCalcByteAlignedScanlineStride(width, pixelFormat, cbCopyStride));

    if (cbCopyStride > cbOutputStride)
    {
        IFC(E_INVALIDARG);
    }

    // NOTE: we rely on the above Lock call to correctly align bit-packed
    // pixel formats, so that when we Copy, we are copying left-aligned
    // pixels.

    for (UINT i=0; i<height; i++)
    {
#pragma prefast(push)
#pragma prefast(disable:2015, "prefast can't correctly recognize the buffer size validation")
#pragma prefast(disable:12105, "prefast can't correctly recognize the buffer size validation")
        GpMemcpy(pbOutputBuffer, pbInputBuffer, cbCopyStride);
#pragma prefast(pop)
        pbOutputBuffer += cbOutputStride;
        pbInputBuffer += cbInputStride;
    }

Cleanup:

    RRETURN(hr);
}

/**************************************************************************
*
* Function Description:
*
*   Copy
*
*   Copies pixel blocks into and out of the bitmap.
*   This implementation provides Copy from Lock in the base class for bitmaps
*   which already have the pixels loaded into memory. Such bitmaps may simply
*   implement Lock and inherit this method.
*
* Arguments:
*
*   prc      - Rectangle in the surface space. NULL indicates the entire
*              surface. Rectangles which exceed the surface bounds cause
*              this routine to fail.
*   pvPixels - Caller provided pixel buffer.
*   cbStride - Stride for the caller pixel buffer.
*
* Created:
*
*   11/14/2001 asecchia
*      Created it.
*
**************************************************************************/

HRESULT CWGXBitmap::CopyPixels(
    __in_ecount_opt(1) const WICRect *prc,
    __in UINT cbStride,
    __in UINT cbBufferSize,
    __out_ecount(cbBufferSize) BYTE *pbPixels
    )
{
    if (!pbPixels)
    {
        RRETURN(E_INVALIDARG);
    }

    IWGXBitmapLock *pILock = NULL;
    WICRect rect;

    HRESULT hr = S_OK;


    if (prc == NULL)
    {
        rect.X = 0;
        rect.Y = 0;

        if (SUCCEEDED(hr))
        {
            hr = UIntToInt(m_nWidth, &rect.Width);
        }

        if (SUCCEEDED(hr))
        {
            hr = UIntToInt(m_nHeight, &rect.Height);
        }

        if (SUCCEEDED(hr))
        {
            prc = &rect;
        }
    }

    if (SUCCEEDED(hr))
    {
        RECT rcLock;

        hr = HrCheckPixelRect(prc, &rcLock);
    }

    if (SUCCEEDED(hr))
    {
        MIL_THR(Lock(prc, MilBitmapLock::Read, &pILock));
    }

    UINT nLockWidth = 0;
    UINT nLockHeight = 0;

    if (SUCCEEDED(hr))
    {
        MIL_THR(HrCheckBufferSize(m_PixelFormat, cbStride, prc, cbBufferSize));
    }

    if (SUCCEEDED(hr))
    {
        MIL_THR(pILock->GetSize(&nLockWidth, &nLockHeight));
    }

    UINT cbLockStride = 0;

    if (SUCCEEDED(hr))
    {
        MIL_THR(pILock->GetStride(&cbLockStride));
    }

    BYTE *pbSurface = NULL;
    UINT cbSurface = 0;

    if (SUCCEEDED(hr))
    {
        MIL_THR(pILock->GetDataPointer(&cbSurface, &pbSurface));
    }

    if (SUCCEEDED(hr))
    {
        // Copy the pixels from the surface into the caller-provided buffer.

        MIL_THR(CopyPixelsHelper(
                m_PixelFormat,
                prc->Width,
                nLockHeight,
                cbLockStride,
                cbSurface,
                pbSurface,
                cbStride,
                cbBufferSize,
                pbPixels));
    }

    ReleaseInterfaceNoNULL(pILock);

    RRETURN(hr);
}

STDMETHODIMP CWGXBitmap::GetSize(
    __out_ecount(1) UINT *pnWidth,
    __out_ecount(1) UINT *pnHeight
    )
{

    if (!pnWidth || !pnHeight)
    {
        return E_INVALIDARG;
    }

    *pnWidth = m_nWidth;
    *pnHeight = m_nHeight;

    return S_OK;
}

STDMETHODIMP CWGXBitmap::GetPixelFormat(
    __out_ecount(1) MilPixelFormat::Enum *pPixelFormat
    )
{
    *pPixelFormat = m_PixelFormat;

    return S_OK;
}

STDMETHODIMP CWGXBitmap::CopyPalette(
    __in_ecount(1) IWICPalette *pIPalette
    )
{
    HRESULT hr = S_OK;
    
    if (m_pPalette == NULL)
    {
        IFC(WGXERR_PALETTEUNAVAILABLE);
    }

    IFC(pIPalette->InitializeFromPalette(m_pPalette));

Cleanup:
    RRETURN(hr);
}

/**************************************************************************
*
* Function Description:
*
*   GetResolution
*
*   Often bitmaps have an implied resolution equal to the DPI of the screen
*   Define this functionality on the base class so that it can be inherited
*
* Created:
*
*   12/23/2001 asecchia
*      Created it.
*
**************************************************************************/

STDMETHODIMP CWGXBitmap::GetResolution(
    __out_ecount(1) double *pDpiX,
    __out_ecount(1) double *pDpiY
    )
{
    HRESULT hr = S_OK;

    if (!pDpiX || !pDpiY)
    {
        IFC(E_INVALIDARG);
    }

    // Assume a default resolution on bitmaps.
    *pDpiX = (double)m_fltDpiX;
    *pDpiY = (double)m_fltDpiY;

Cleanup:
    RRETURN(hr);
}

STDMETHODIMP CWGXBitmap::SetPalette(
    __in_ecount(1) IWICPalette *pIPalette
    )
{
    HRESULT hr = S_OK;
    IWICImagingFactory *pIWICFactory = NULL;

    if (m_pPalette == NULL)
    {
        IFC(WICCreateImagingFactory_Proxy(WINCODEC_SDK_VERSION_WPF, &pIWICFactory));
        IFC(pIWICFactory->CreatePalette(&m_pPalette));
    }

    IFC(m_pPalette->InitializeFromPalette(pIPalette));

    // Ideally we'd check the original palette to see if anything really changed, but that's
    // somewhat expensive
    UpdateUniqueCount();

Cleanup:
    if (FAILED(hr))
    {
        ReleaseInterface(m_pPalette);
    }
    
    ReleaseInterface(pIWICFactory);

    RRETURN(hr);
}

STDMETHODIMP CWGXBitmap::Unlock(
    __in_ecount(1) CWGXBitmapLock *pBitmapLock
    )
{
    DWORD dwFlags = pBitmapLock->GetFlags();

    if (dwFlags & MilBitmapLock::Write)
    {
        m_lockState.UnlockWrite();
    }
    else if (dwFlags & MilBitmapLock::Read)
    {
        m_lockState.UnlockRead();
    }

    return S_OK;
}

STDMETHODIMP CWGXBitmap::SetResolution(
    __in double dblDpiX,
    __in double dblDpiY)
{
    float fltDpiX = static_cast<float>(dblDpiX);
    float fltDpiY = static_cast<float>(dblDpiY);

    if (fltDpiX != m_fltDpiX || fltDpiY != m_fltDpiY)
    {
        m_fltDpiX = fltDpiX;
        m_fltDpiY = fltDpiY;

        UpdateUniqueCount();
    }

    return S_OK;
}

HRESULT CWGXBitmap::HrCheckPixelRect(
    __in_ecount_opt(1) const WICRect *prcInput,
    __out_ecount(1) RECT *prcOutput)
{
    HRESULT hr = S_OK;

    SetRect(prcOutput, 0, 0, m_nWidth, m_nHeight);

    if (prcInput)
    {
        RECT rc;

        rc.left = prcInput->X;
        rc.top = prcInput->Y;

        ULONG tmp1, tmp2, tmpResult;

        IFC(LongToULong(prcInput->X, &tmp1));
        IFC(LongToULong(prcInput->Width, &tmp2));
        IFC(ULongAdd(tmp1, tmp2, &tmpResult));
        IFC(ULongToLong(tmpResult, &(rc.right)));

        IFC(LongToULong(prcInput->Y, &tmp1));
        IFC(LongToULong(prcInput->Height, &tmp2));
        IFC(ULongAdd(tmp1, tmp2, &tmpResult));
        IFC(ULongToLong(tmpResult, &(rc.bottom)));

        // If prc is specified, but it's empty (no area or negative
        // width, height) then this is not a valid input.

        // The prc must be fully contained within the image rectangle
        // for it to be valid. We test this by intersecting the two
        // rectangles and making sure the result is the same as prc.
        // This works because we ensure prc is well formed and not empty.

        IntersectRect(prcOutput, &rc, prcOutput);

        if (IsRectEmpty(&rc) || !EqualRect(prcOutput, &rc))
        {
            RRETURN(E_INVALIDARG);
        }
    }

Cleanup:
    RRETURN(hr);
}

HRESULT CWGXBitmap::HrLock(
    __in_ecount(1) const RECT &rcLock,
    __in MilPixelFormat::Enum pxlFormat,
    __in UINT cbStride,
    __in UINT cbBufferSize,
    __in_bcount(cbBufferSize) VOID *pvPixels,
    __in DWORD dwFlags,
    __deref_out_ecount(1) IWGXBitmapLock **ppILock,
    __in BOOL fLockOwnsPixelData
    )
{
    HRESULT hr = S_OK;
    BOOL fUnlockNeeded = FALSE;

    CWGXBitmapLock *pLock = NULL;

    if (dwFlags & MilBitmapLock::Write)
    {
        IFC(m_lockState.LockWrite());
    }
    else if (dwFlags & MilBitmapLock::Read)
    {
        IFC(m_lockState.LockRead());
    }
    else
    {
        IFC(E_INVALIDARG);
    }
    fUnlockNeeded = TRUE;

    pLock = new CWGXBitmapLock();
    IFCOOM(pLock);
    pLock->AddRef();

    IFC(pLock->Init(
        this,
        rcLock.right - rcLock.left,
        rcLock.bottom - rcLock.top,
        pxlFormat,
        cbStride,
        cbBufferSize,
        pvPixels,
        dwFlags,
        fLockOwnsPixelData
        ));
    fUnlockNeeded = FALSE;

    if (dwFlags & MilBitmapLock::Write)
    {
        IFC(AddDirtyRect(&rcLock));
    }

    *ppILock = pLock;
    pLock = NULL;

Cleanup:
    if (fUnlockNeeded)
    {
        if (dwFlags & MilBitmapLock::Write)
        {
            m_lockState.UnlockWrite();
        }
        else if (dwFlags & MilBitmapLock::Read)
        {
            m_lockState.UnlockRead();
        }
    }

    // If we succeed in locking but fail afterwards, this will release the
    // outstanding bitmap lock.
    ReleaseInterface(pLock);

    RRETURN(hr);
}

HRESULT CWGXBitmap::HrLockUnaligned(
    __in_ecount(1) const RECT &rcLock,
    MilPixelFormat::Enum pxlFormat,
    __in_range(1, 7) UINT nBitsStart,
    UINT nBitsTotal,
    UINT cbStride,
    UINT cbBufferSize,
    __in_bcount(cbBufferSize) VOID *pvPixels,
    DWORD dwFlags,
    __deref_out_ecount(1) IWGXBitmapLock **ppILock
    )
{
    HRESULT hr = S_OK;
    BOOL fUnlockNeeded = FALSE;

    CWGXBitmapLockUnaligned *pLock = NULL;

    if (dwFlags & MilBitmapLock::Write)
    {
        IFC(m_lockState.LockWrite());
    }
    else if (dwFlags & MilBitmapLock::Read)
    {
        IFC(m_lockState.LockRead());
    }
    else
    {
        IFC(E_INVALIDARG);
    }
    fUnlockNeeded = TRUE;

    pLock = new CWGXBitmapLockUnaligned();
    IFCOOM(pLock);
    pLock->AddRef();

    IFC(pLock->HrInit(
            this,
            rcLock.right - rcLock.left,
            rcLock.bottom - rcLock.top,
            pxlFormat,
            cbStride,
            cbBufferSize,
            pvPixels,
            dwFlags,
            nBitsStart,
            nBitsTotal
            ));
    fUnlockNeeded = FALSE;

    if (dwFlags & MilBitmapLock::Write)
    {
        IFC(AddDirtyRect(&rcLock));
    }

    *ppILock = pLock;
    pLock = NULL;

Cleanup:
    if (fUnlockNeeded)
    {
        if (dwFlags & MilBitmapLock::Write)
        {
            m_lockState.UnlockWrite();
        }
        else if (dwFlags & MilBitmapLock::Read)
        {
            m_lockState.UnlockRead();
        }
    }

    // If we succeed in locking but fail afterwards, this will release the
    // outstanding bitmap lock.
    ReleaseInterface(pLock);

    RRETURN(hr);
}

HRESULT CWGXBitmap::HrInitDataFromBitmapSource(
    __in_ecount(1) IWGXBitmapSource *pISource,
    __in_ecount_opt(1) WICRect * srcRect)
{
    HRESULT hr = S_OK;
    IWICImagingFactory *pIWICFactory = NULL;

    if (SUCCEEDED(hr))
    {
        MIL_THR(pISource->GetSize(&m_nWidth, &m_nHeight));

        if (srcRect != NULL)
        {
            Assert ((srcRect->Width > 0) && (srcRect->Height > 0) &&
                    (srcRect->X >= 0) && (srcRect->Y >= 0) &&
                    (static_cast<UINT>(srcRect->X + srcRect->Width)  <= m_nWidth) &&
                    (static_cast<UINT>(srcRect->Y + srcRect->Height) <= m_nHeight));
            m_nWidth  = srcRect->Width;
            m_nHeight = srcRect->Height;
        }
    }

    if (SUCCEEDED(hr))
    {
        MIL_THR(pISource->GetPixelFormat(&m_PixelFormat));
    }

    if (SUCCEEDED(hr))
    {
        DWORD dwPixelFormatSize = GetPixelFormatSize(m_PixelFormat);
        if (dwPixelFormatSize == 0)
        {
            MIL_THR(WINCODEC_ERR_UNKNOWNIMAGEFORMAT);
        }
        else if (m_nWidth >= (UINT)(INT_MAX / dwPixelFormatSize))
        {
            MIL_THR(WINCODEC_ERR_VALUEOVERFLOW);
        }
    }

    // Check whether the bitmap size overflows.
    if (SUCCEEDED(hr))
    {
        UINT cbStride;

        // This won't overflow because we checked for that above.
        MIL_THR(HrCalcDWordAlignedScanlineStride(m_nWidth, m_PixelFormat, cbStride));

        if (SUCCEEDED(hr))
        {
            if (m_nHeight != 0 &&
                (cbStride >= (UINT)(INT_MAX / m_nHeight)))
            {
                MIL_THR(WINCODEC_ERR_VALUEOVERFLOW);
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        double dblDpiX = 0.0;
        double dblDpiY = 0.0;

        MIL_THR(pISource->GetResolution(&dblDpiX, &dblDpiY));

        m_fltDpiX = static_cast<FLOAT>(dblDpiX);
        m_fltDpiY = static_cast<FLOAT>(dblDpiY);
    }

    if (SUCCEEDED(hr))
    {
        if (IsIndexedPixelFormat(m_PixelFormat))
        {
            if (m_pPalette == NULL)
            {
                MIL_THR(WICCreateImagingFactory_Proxy(WINCODEC_SDK_VERSION_WPF, &pIWICFactory));
                if (SUCCEEDED(hr))
                {
                    MIL_THR(pIWICFactory->CreatePalette(&m_pPalette));
                }
            }

            if (SUCCEEDED(hr))
            {
                MIL_THR(pISource->CopyPalette(m_pPalette));
            }
        }
    }

    if (FAILED(hr))
        ReleaseInterface(m_pPalette);

    ReleaseInterface(pIWICFactory);

    RRETURN(hr);
}



HRESULT CreateBitmapFromSourceRect(
    __in_ecount(1) IWGXBitmapSource *pISource,
    __in UINT x,
    __in UINT y,
    __in UINT width,
    __in UINT height,
    __in BOOL fCopySource,
    __deref_out_ecount(1) IWGXBitmap **ppBitmap
    )
{
    HRESULT hr = S_OK;

    CSystemMemoryBitmap *pTempBitmap = new CSystemMemoryBitmap();

    if (NULL == pTempBitmap)
    {
        MIL_THR(E_OUTOFMEMORY);
    }
    else
    {
        pTempBitmap->AddRef();
    }

    if (SUCCEEDED(hr))
    {
        WICRect rc = {x, y, width, height };
        MIL_THR(pTempBitmap->Init(pISource, &rc, fCopySource));
    }

    if (SUCCEEDED(hr))
    {
        pTempBitmap->AddRef();
        *ppBitmap = pTempBitmap;
    }

    if (pTempBitmap != NULL)
    {
        pTempBitmap->Release();
    }

    RRETURN(hr);
}

/**************************************************************************
*
* Function Description:
*
*   CWGXBitmapLockState constructor
*
**************************************************************************/

CWGXBitmapLockState::CWGXBitmapLockState()
{
    m_lockState = lockNone;
}

/**************************************************************************
*
* Function Description:
*
*   CWGXBitmapLockState destructor
*
**************************************************************************/

CWGXBitmapLockState::~CWGXBitmapLockState()
{
    Assert(m_lockState == lockNone);
}

/**************************************************************************
*
* Function Description:
*
*   CheckNoLocks - Verify there are no current locks outstanding.
*
**************************************************************************/

HRESULT CWGXBitmapLockState::CheckNoLocks()
{
    RRETURN(m_lockState == lockNone ? S_OK : WINCODEC_ERR_WRONGSTATE);
}

/**************************************************************************
*
* Function Description:
*
*   LockRead - Updates the lock state for a reading.  If there is an outstanding write
*   lock this operation will return E_ACCESSDENIED.
*
**************************************************************************/

HRESULT CWGXBitmapLockState::LockRead()
{
    HRESULT hr = S_OK;

    LONG lockCount = 0;
    LONG original = 0;

    do
    {
        lockCount = m_lockState & (~lockWrite);

        LONG incCount = lockCount + 1;
        // Check for locked for write and also extreme case of too many simultaneous read requests.
        if (incCount & lockWrite)
        {
            TraceTag((tagMILWarning, "CWGXBitmapLockState::LockRead failed -- there is already an outstanding write lock or too many reads."));
            IFC(WINCODEC_ERR_ALREADYLOCKED);
        }

        original = InterlockedCompareExchange(&m_lockState, incCount, lockCount);

        // If we don't compare, then we didn't do the exchange, in which case there was contention with
        // a read operation and spin again.
    } while (original != lockCount);

Cleanup:
    RRETURN(hr);
}

/**************************************************************************
*
* Function Description:
*
*   LockWrite - Updates the lock state for a writing.  If there are any outstanding read
*   locks this operation will return E_ACCESSDENIED.
*
*
**************************************************************************/

HRESULT CWGXBitmapLockState::LockWrite()
{
    HRESULT hr = S_OK;

    LONG original = InterlockedCompareExchange(&m_lockState, lockWrite, lockNone);

    if (original != lockNone)
    {
        // Someone has either a read or write lock outstanding.  So fail.
        TraceTag((tagMILWarning, "CWGXBitmapLockState::LockWrite failed -- there is already an outstanding read or write lock."));
        IFC(WINCODEC_ERR_ALREADYLOCKED);
    }

Cleanup:
    RRETURN(hr);
}

/**************************************************************************
*
* Function Description:
*
*   UnlockRead - Unlocks the state for reading.  If there are multiple readers, the count is
*   decremented.
*
*
**************************************************************************/

VOID CWGXBitmapLockState::UnlockRead()
{
    Assert((m_lockState & lockWrite) == 0);
    Assert(m_lockState != lockNone);

    InterlockedDecrement(&m_lockState);
}

/**************************************************************************
*
* Function Description:
*
*   UnlockWrite - Unlocks the state for writing.  If there is an outstanding write lock it is
*   released and further read/write lock requests can be made.
*
*
**************************************************************************/

VOID CWGXBitmapLockState::UnlockWrite()
{
    Assert(m_lockState == lockWrite);
    m_lockState = lockNone;
}



/**************************************************************************
*
* Function Description:
*
*   CWGXBitmapLock constructor - Initializes the lock for read or write access.
*
*
**************************************************************************/

CWGXBitmapLock::CWGXBitmapLock()
{
    m_fValid = FALSE;
    m_fOwnsPixelData = FALSE;
    m_pIBitmap = NULL;
}

/**************************************************************************
*
* Function Description:
*
*   CWGXBitmapLock destructor - Unlocks the bitmap lock state.
*
*
**************************************************************************/

CWGXBitmapLock::~CWGXBitmapLock()
{
    Unlock();
}

/**************************************************************************
*
* Function Description:
*
*   CWGXBitmapLock::HrFindInterfaces returns interfaces implemented on this object.
*
*
**************************************************************************/

STDMETHODIMP CWGXBitmapLock::HrFindInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void **ppvObject)
{
    HRESULT hr = E_INVALIDARG;

    if (ppvObject)
    {
        if (riid == IID_IWGXBitmapLock)
        {
            *ppvObject = static_cast<IWGXBitmapLock *>(this);

            hr = S_OK;
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }

    return hr;
}

HRESULT CWGXBitmapLock::Init(
    __in_ecount(1) CWGXBitmap *pIBitmap,
    __in UINT nWidth,
    __in UINT nHeight,
    __in MilPixelFormat::Enum pxlFormat,
    __in UINT cbStride,
    __in UINT cbBufferSize,
    __in_bcount(cbBufferSize) VOID *pvPixels,
    __in DWORD dwFlags,
    __in BOOL fOwnsPixelData
    )
{
    HRESULT hr = S_OK;

    Assert(!m_fValid);

    IFC(HrCheckBufferSize(
            pxlFormat,
            cbStride,
            nWidth,
            nHeight,
            cbBufferSize));

    m_fValid = TRUE;
    m_fOwnsPixelData = fOwnsPixelData;

    m_BitmapData.Width = nWidth;
    m_BitmapData.Height = nHeight;
    m_BitmapData.PixelFormat = pxlFormat;
    m_BitmapData.Stride = cbStride;
    m_BitmapData.Pixels = pvPixels;
    m_BitmapData.BufferSize = cbBufferSize;

    m_pIBitmap = pIBitmap;
    m_pIBitmap->AddRef();

    m_dwFlags = dwFlags;

Cleanup:
    RRETURN(hr);
}

/**************************************************************************
*
* Function Description:
*
*   CWGXBitmapLock::CheckValid returns whether the bitmap lock is in a usable state.
*
*
**************************************************************************/

HRESULT CWGXBitmapLock::CheckValid()
{
    RRETURN(m_fValid ? S_OK : WINCODEC_ERR_WRONGSTATE);
}

/**************************************************************************
*
* Function Description:
*
*   CWGXBitmapLock::GetFlags returns the current flags last associated with this bitmap lock.  If
*   the lock is not initialized, it returns 0.
*
**************************************************************************/

DWORD CWGXBitmapLock::GetFlags()
{
    return m_dwFlags;
}

HRESULT CWGXBitmapLock::GetSize(
    __out_ecount(1) UINT *puiWidth,
    __out_ecount(1) UINT *puiHeight)
{
    HRESULT hr = S_OK;


    CHECKPTRARG(puiWidth);
    CHECKPTRARG(puiHeight);

    IFC(CheckValid());

    *puiWidth = m_BitmapData.Width;
    *puiHeight = m_BitmapData.Height;

Cleanup:
    RRETURN(hr);
}

HRESULT CWGXBitmapLock::GetStride(
    __out_ecount(1) UINT *puiStride)
{
    HRESULT hr = S_OK;


    CHECKPTRARG(puiStride);

    IFC(CheckValid());

    *puiStride = m_BitmapData.Stride;

Cleanup:
    RRETURN(hr);
}

HRESULT CWGXBitmapLock::GetDataPointer(
    __out_ecount(1) UINT *pcbBufferSize,
    __deref_out_bcount(*pcbBufferSize) BYTE **ppvData)
{
    HRESULT hr = S_OK;


    CHECKPTRARG(ppvData);
    CHECKPTRARG(pcbBufferSize);

    IFC(CheckValid());

    *pcbBufferSize = m_BitmapData.BufferSize;
    *ppvData = static_cast<BYTE*> (m_BitmapData.Pixels);

Cleanup:
    RRETURN(hr);
}

HRESULT CWGXBitmapLock::GetPixelFormat(
    __out_ecount(1) MilPixelFormat::Enum *pPixelFormat)
{
    HRESULT hr = S_OK;

    IFC(CheckValid());

    *pPixelFormat = m_BitmapData.PixelFormat;

Cleanup:
    RRETURN(hr);
}

void CWGXBitmapLock::Unlock()
{
    if (m_pIBitmap != NULL)
    {
        Verify(SUCCEEDED(CheckValid()));

        // Ask the bitmap to "unlock".  In fact, we can't propagate an error
        // code out so we swallow it here.
        IGNORE_HR(m_pIBitmap->Unlock(this));

        ReleaseInterface(m_pIBitmap);

        // In this case the lock owns the memory, so it is responsible for freeing it
        if (m_fOwnsPixelData)
        {
            GpFree(m_BitmapData.Pixels);
            m_BitmapData.Pixels = NULL;
        }
    }

    m_fValid = FALSE;
}

CWGXBitmapLockUnaligned::CWGXBitmapLockUnaligned()
{
    m_BitmapData.Pixels = NULL;
}

CWGXBitmapLockUnaligned::~CWGXBitmapLockUnaligned()
{
    UnlockUnaligned();
}

/**************************************************************************
*
* Function Description:
*
*   HrInit
*
*   This function sets up the Lock object. It creates a buffer to back the
*   pixels for the Lock and reads the source data into it, aligning the bits
*   as it does this.
*
**************************************************************************/

HRESULT CWGXBitmapLockUnaligned::HrInit(
    __in_ecount(1) CWGXBitmap *pIBitmap,
    UINT nWidth,
    UINT nHeight,
    MilPixelFormat::Enum pxlFormat,
    UINT cbStride,
    UINT cbBufferSize,
    __in_bcount(cbBufferSize) VOID *pvPixels,
    DWORD dwFlags,
    __in_range(1, 7) UINT nBitsStart,
    UINT nBitsTotal
    )
{
    HRESULT hr = S_OK;

    // Remember the original data. We will need this on unlock.

    m_cbOriginalStride = cbStride;
    m_pvOriginalPixels = pvPixels;
    m_cbOriginalBufferSize = cbBufferSize;

    m_dwFlags = dwFlags;
    m_nBitsStart = nBitsStart;
    m_nBitsTotal = nBitsTotal;

    // Stash the stride for our temporary buffer in the m_BitmapData - this
    // is the location we will be handing out to our caller to satisfy the
    // Lock

    UINT cbDstStride;
    MIL_THR(HrCalcDWordAlignedScanlineStride(nWidth, pxlFormat, cbDstStride));
    void *pvUnalignedBuffer = NULL;

    if (SUCCEEDED(hr))
    {
        // Allocate a buffer to receive the aligned pixels.

        MIL_THR(HrMalloc(
            Mt(MUnalignedLockData),
            cbDstStride,
            nHeight,
            &pvUnalignedBuffer
            ));
    }
    const UINT cbUnalignedBuffer = cbDstStride * nHeight;

    if (SUCCEEDED(hr))
    {
        // if the caller needs to read the pixels, fill the lock buffer.

        if (m_dwFlags & MilBitmapLock::Read)
        {
            // now copy the source into our temporary buffer, aligning each
            // scanline.

            BYTE *pbSrc = static_cast<BYTE*>(pvPixels);
            BYTE *pbDst = static_cast<BYTE*>(pvUnalignedBuffer);

            for (unsigned int i = 0; i < nHeight; i++)
            {
                // dstSize == cbDstStride // stride okay here because buffer has size=stride*height
                // srcSize == cbBufferSize - cbStride * i
                ReadUnalignedScanline(
                    pbDst,
                    pbSrc,
                    nBitsTotal,
                    nBitsStart
                    );

                pbSrc += cbStride;
                pbDst += cbDstStride;
            }
        }

        // Finally stash the pointers, stride, etc away in our ancestor class
        // as reference to answer IWGXBitmapLock questions. We answer
        // these out of our aligned buffer.

        MIL_THR(CWGXBitmapLock::Init(
            pIBitmap,
            nWidth,
            nHeight,
            pxlFormat,
            cbDstStride,
            cbUnalignedBuffer,
            pvUnalignedBuffer,
            dwFlags
            ));
    }

    if (FAILED(hr))
    {
        GpFree(pvUnalignedBuffer);
    }

    RRETURN(hr);
}

/**************************************************************************
*
* Function Description:
*
*   Unlock
*
*   Unlock the pixels. If they were locked for write, we propagate the potential
*   changes back to the original by re-aligning the lock-buffer back into the
*   source.
*
**************************************************************************/

void CWGXBitmapLockUnaligned::UnlockUnaligned()
{
    // If we locked for write, we have to re-unalign the data in the temporary
    // buffer and copy it back into the original.

    if (m_BitmapData.Pixels && (m_dwFlags & MilBitmapLock::Write))
    {
        // now copy from our temporary buffer, aligning each
        // scanline correctly back into the source pixels.

        BYTE *pbDst = static_cast<BYTE*>(m_pvOriginalPixels);
        BYTE *pbSrc = static_cast<BYTE*>(m_BitmapData.Pixels);

        for (unsigned int i = 0; i < m_BitmapData.Height; i++)
        {
            // dstSize == m_cbOriginalBufferSize - i * m_cbOriginalStride
            // srcSize == m_BitmapData.BufferSize - i * m_BitmapData.Stride
            WriteUnalignedScanline(
                pbDst,
                pbSrc,
                m_nBitsTotal,
                m_nBitsStart
                );

            pbSrc += m_BitmapData.Stride;
            pbDst += m_cbOriginalStride;
        }
    }

    GpFree(m_BitmapData.Pixels);
    m_BitmapData.Pixels = NULL;
}


