// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Description:
//      Implementation for CSwDoubleBufferedBitmap which provides a pair of
//      bitmaps with a synchronized CopyForward operation.
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CSwDoubleBufferedBitmap, MILRender, "CSwDoubleBufferedBitmap");

//+-------------------------------------------------------------------------
//
//  Constructor
//
//  Synopsis:
//      Creates a new CSwDoubleBufferedBitmap.  CSwDoubleBufferedBitmaps
//      should be created and initialized via the static Create() method
//      and HrInit(), so the constructor is private.
//
//--------------------------------------------------------------------------

CSwDoubleBufferedBitmap::CSwDoubleBufferedBitmap() :
    m_backBufferPixelFormat(MilPixelFormat::DontCare),
    m_width(0),
    m_height(0),
    m_pBackBuffer(NULL),
    m_pBackBufferAsWriteProtectedBitmap(NULL),
    m_pFrontBuffer(NULL),
    m_pFormatConverter(NULL),
    m_pDirtyRects(NULL),
    m_numDirtyRects(0)
{
}

//+-----------------------------------------------------------------------------
//  Destructor
//------------------------------------------------------------------------------

CSwDoubleBufferedBitmap::~CSwDoubleBufferedBitmap()
{
    ReleaseInterface(m_pBackBuffer);
    ReleaseInterface(m_pBackBufferAsWriteProtectedBitmap);
    ReleaseInterface(m_pFrontBuffer);
    ReleaseInterface(m_pFormatConverter);

    if (m_pDirtyRects)
    {
        WPFFree(ProcessHeap, m_pDirtyRects);
        m_pDirtyRects = NULL;
    }
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      HrInit
//
//  Synopsis:
//      Initializes a new instance of CSwDoubleBufferedBitmap.  This allocates
//      the front and back buffers, and a format converter if necessary to copy
//      pixels between the two.
//
//------------------------------------------------------------------------------

HRESULT
CSwDoubleBufferedBitmap::HrInit(
    UINT width,
    UINT height,
    double dpiX,
    double dpiY,
    MilPixelFormat::Enum pixelFormat,
    __in_opt IWICPalette *pPalette
    )
{
    HRESULT hr = S_OK;

    IWGXBitmap *            pBackBuffer           = NULL;
    CSystemMemoryBitmap *   pFrontBuffer          = NULL;
    CWriteProtectedBitmap * pWriteProtectedBitmap = NULL;
    IWICImagingFactory *    pImagingFactory       = NULL;
    IWICFormatConverter *   pFormatConverter      = NULL;
    IWICPalette *           pWicPalette           = NULL;

    // We restrict the dimensions of the bitmap to INT_MAX so that we
    // can treat our dirty rects as RECTs.
    if (width > INT_MAX || height > INT_MAX)
    {
        IFC(E_INVALIDARG);
    }

    // All parameter validation is done in CWriteProtectedBitmap::Create
    IFC(CWriteProtectedBitmap::Create(width, height, dpiX, dpiY, pixelFormat, pPalette, &pWriteProtectedBitmap));

    pWriteProtectedBitmap->AddRef();
    m_pBackBufferAsWriteProtectedBitmap = pWriteProtectedBitmap;
    
    m_width = width;
    m_height = height;
    m_backBufferPixelFormat = pixelFormat;
    m_backBufferSize = pWriteProtectedBitmap->GetBufferSize();

    if (pPalette != NULL)
    {
        // If we have a palette, we'll need the WIC flavor of it since we
        // will be creating a WIC format converter.
        IFC(pPalette->QueryInterface(
            IID_IWICPalette,
            reinterpret_cast<void**>(&pWicPalette)
            ));
    }

    // Allocate a list of dirty rects
    m_pDirtyRects = WPFAllocType(CMilRectU*, ProcessHeap, Mt(CSwDoubleBufferedBitmap), sizeof(CMilRectU) * c_maxBitmapDirtyListSize);

    // QI to a friendly interface that we'll pass out to the user
    // of this double buffered bitmap.  We ask for the WIC interface instead of
    // the MIL interface so that we can use the extensive set of format
    // converters provided by WIC but not MIL.
    IFC(pWriteProtectedBitmap->QueryInterface(
        IID_IWGXBitmap,
        reinterpret_cast<void**>(&pBackBuffer)
        ));

    IFC(CWGXWrapperBitmap::Create(
        pBackBuffer,
        &m_pBackBuffer
        ));

    MilPixelFormat::Enum frontBufferPixelFormat = HasAlphaChannel(m_backBufferPixelFormat) ?
                                                    MilPixelFormat::PBGRA32bpp :
                                                    MilPixelFormat::BGR32bpp;

    IFC(CSystemMemoryBitmap::Create(
        m_width,
        m_height,
        frontBufferPixelFormat, 
        TRUE,  // fClear
        TRUE,  // fIsDynamic
        &pFrontBuffer
        ));
    IFC(pFrontBuffer->SetResolution(dpiX, dpiY));

    pFrontBuffer->AddRef();
    m_pFrontBuffer = pFrontBuffer;

    // If the pixel formats of the front and back buffer do not match, we
    // cache a format converter to pull pixels through on copy-forward.
    if (m_backBufferPixelFormat != frontBufferPixelFormat)
    {   
        IFC(CoCreateInstance(
            CLSID_WICImagingFactoryWPF,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IWICImagingFactory,
            (LPVOID*)&pImagingFactory
            ));

        IFC(pImagingFactory->CreateFormatConverter(&pFormatConverter));

        IFC(pFormatConverter->Initialize(
            m_pBackBuffer,
            MilPfToWic(frontBufferPixelFormat),
            WICBitmapDitherTypeNone,
            pWicPalette,
            0.0f,
            WICBitmapPaletteTypeCustom
            ));

        IFC(CWICWrapperBitmapSource::Create(
            pFormatConverter,
            &m_pFormatConverter
            ));
    }

Cleanup:
    
    ReleaseInterfaceNoNULL(pWriteProtectedBitmap);
    ReleaseInterfaceNoNULL(pBackBuffer);
    ReleaseInterfaceNoNULL(pFrontBuffer);
    ReleaseInterfaceNoNULL(pImagingFactory);
    ReleaseInterfaceNoNULL(pFormatConverter);
    ReleaseInterfaceNoNULL(pWicPalette);
    
    if (FAILED(hr))
    {
        ReleaseInterface(m_pBackBuffer);
        ReleaseInterface(m_pBackBufferAsWriteProtectedBitmap);
        ReleaseInterface(m_pFrontBuffer);
        ReleaseInterface(m_pFormatConverter);
        
        if (m_pDirtyRects)
        {
            WPFFree(ProcessHeap, m_pDirtyRects);
            m_pDirtyRects = NULL;
        }
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CSwDoubleBufferedBitmap::Create
//
//  Synopsis:
//      Static method which creates a new CSwDoubleBufferedBitmap instance and
//      returns a pointer to its IMILSwDoubleBufferedBitmap interface.
//
//------------------------------------------------------------------------------

HRESULT
CSwDoubleBufferedBitmap::Create(
    UINT width,
    UINT height,
    double dpiX,
    double dpiY,
    __in MilPixelFormat::Enum pixelFormat,
    __in_opt IWICPalette *pIPalette,
    __deref_out CSwDoubleBufferedBitmap **ppSwDoubleBufferedBitmap
    )
{
    HRESULT hr = S_OK;
    CSwDoubleBufferedBitmap *pSwDoubleBufferedBitmap = NULL;

    pSwDoubleBufferedBitmap = new CSwDoubleBufferedBitmap();
    IFCOOM(pSwDoubleBufferedBitmap);
    pSwDoubleBufferedBitmap->AddRef();

    IFC(pSwDoubleBufferedBitmap->HrInit(
        width,
        height,
        dpiX,
        dpiY,
        pixelFormat,
        pIPalette
        ));

    // Hand back an AddRef'd pointer to the CSwDoubleBufferedBitmap instance.
    pSwDoubleBufferedBitmap->AddRef();
    *ppSwDoubleBufferedBitmap = pSwDoubleBufferedBitmap;

Cleanup:
    ReleaseInterfaceNoNULL(pSwDoubleBufferedBitmap);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      GetBackBuffer
//
//  Synopsis:
//      Return a pointer to the back buffer.
//
//------------------------------------------------------------------------------

void
CSwDoubleBufferedBitmap::GetBackBuffer(
    __deref_out IWICBitmap **ppBackBuffer,
    __out_opt UINT * pBackBufferSize
    ) const
{
    SetInterface(*ppBackBuffer, m_pBackBuffer);

    if (pBackBufferSize != NULL)
    {
        *pBackBufferSize = m_backBufferSize;
    }
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      GetPossiblyFormatConvertedBackBuffer
//
//  Synopsis:
//      Return a pointer to the back buffer, or the the format converter for
//      the back buffer if the back buffer needed one.
//
//------------------------------------------------------------------------------
void
CSwDoubleBufferedBitmap::GetPossiblyFormatConvertedBackBuffer(
    __deref_out IWGXBitmapSource **ppPossiblyFormatConvertedBackBuffer
    ) const
{
    if (m_pFormatConverter)
    {
        SetInterface(*ppPossiblyFormatConvertedBackBuffer, m_pFormatConverter);
    }
    else
    {
        SetInterface(*ppPossiblyFormatConvertedBackBuffer, m_pBackBufferAsWriteProtectedBitmap);
    }
}


//+-----------------------------------------------------------------------------
//
//  Method:
//      GetFrontBuffer
//
//  Synopsis:
//      Return a pointer to the front buffer.
//
//------------------------------------------------------------------------------

void
CSwDoubleBufferedBitmap::GetFrontBuffer(
    __deref_out IWGXBitmap **ppFrontBuffer
    ) const
{
    SetInterface(*ppFrontBuffer, m_pFrontBuffer);
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CSwDoubleBufferedBitmap::AddDirtyRect
//
//  Synopsis:
//      Adds a dirty rect to the back buffer.  CWGXBitmap does not track dirty
//      rects for bitmaps that are not cached.  Since the back buffer is not
//      actually ever seen by MIL, it will certainly never be cached.  Thus
//      we can't just call AddDirtyRect on the back buffer bitmap.  Instead
//      we track dirty rects manually.
//
//      This code was adapted from CWGXBitmap::AddDirtyRect
//
//------------------------------------------------------------------------------

HRESULT
CSwDoubleBufferedBitmap::AddDirtyRect(__in const CMilRectU *prcDirty)
{
    HRESULT hr = S_OK;
    CMilRectU rcBounds(0, 0, m_width, m_height, XYWH_Parameters);
    CMilRectU rcDirty = *prcDirty;

    if (!rcDirty.IsEmpty())
    {
        // Each dirty rect will eventually be treated as a RECT, so we must
        // ensure that the Left, Right, Top, and Bottom values never exceed
        // INT_MAX.  We already restrict our dimensions to INT_MAX, so as
        // long as the dirty rect is fully within the bounds of the bitmap,
        // we are safe.
        if (!rcBounds.DoesContain(rcDirty))
        {
            IFC(E_INVALIDARG);
        }

        // Adding a dirty rect that spans the entire bitmap will simply
        // replace all existing dirty rects.
        if (rcDirty.IsEquivalentTo(rcBounds))
        {
            m_pDirtyRects[0] = rcBounds;
            m_numDirtyRects = 1;
        }
        else
        {
            // Check to see if one of the existing dirty rects fully contains the
            // new dirty rect.  If so, there is no need to add it.
            for (UINT i = 0; i < m_numDirtyRects; i++)
            {
                if (m_pDirtyRects[i].DoesContain(rcDirty))
                {
                    // No dirty list change - new dirty rect is already included.
                    goto Cleanup;
                }
            }

            // Collapse existing dirty rects if we're about to exceed our maximum.
            if (m_numDirtyRects >= c_maxBitmapDirtyListSize)
            {
                // Collapse dirty list to a single large rect (including new rect)
                while (m_numDirtyRects > 1)
                {
                    m_pDirtyRects[0].Union(m_pDirtyRects[--m_numDirtyRects]);
                }
                m_pDirtyRects[0].Union(rcDirty);

                Assert(m_numDirtyRects == 1);
            }
            else
            {
                m_pDirtyRects[m_numDirtyRects++] = rcDirty;
            }
        }
    }

Cleanup:

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CSwDoubleBufferedBitmap::CopyForwardDirtyRects
//
//  Synopsis:
//      Copies dirty rects from the back buffer to the front buffer.
//
//------------------------------------------------------------------------------

HRESULT
CSwDoubleBufferedBitmap::CopyForwardDirtyRects()
{
    HRESULT hr = S_OK;

    IWGXBitmapSource *pIWGXBitmapSource = NULL;
    IWGXBitmapLock *pFrontBufferLock = NULL;
    UINT cbLockStride = 0;
    UINT cbBufferSize = 0;
    BYTE *pbSurface = NULL;

    Assert(m_pBackBuffer);

    // This locks only the rect specified as dirty for each copy. It would
    // be more efficient to just lock the entire rect once for all of the
    // copies, but then we need to manually compute offsets into the front
    // buffer specific to each pixel format.
    while (m_numDirtyRects > 0)
    {
        // We have to jump through a few RECT hoops here since
        // IWGXBitmapSource::Lock/CopyPixels take a WICRect and
        // IWGXBitmap::AddDirtyRect takes a GDI RECT, neither of which are
        // CMilRectU which we use in CSwDoubleBufferedBitmap for geometric operations.
        //
        // CMilRectU and RECT share the same memory alignment, but different
        // signs.  Since we restrict the size of our bitmap to MAX_INT, we can
        // safely cast.
        const RECT *rcDirty = reinterpret_cast<RECT const *>(&m_pDirtyRects[--m_numDirtyRects]);
        WICRect copyRegion = {
            static_cast<int>(rcDirty->left),
            static_cast<int>(rcDirty->top),
            static_cast<int>(rcDirty->right - rcDirty->left),
            static_cast<int>(rcDirty->bottom - rcDirty->top)
            };

        // This adds copyRegion as a dirty rect to m_pFrontBuffer automatically.
        IFC(m_pFrontBuffer->Lock(
            &copyRegion,
            MilBitmapLock::Write,
            &pFrontBufferLock
            ));

        IFC(pFrontBufferLock->GetStride(&cbLockStride));
        IFC(pFrontBufferLock->GetDataPointer(&cbBufferSize, &pbSurface));

        // If a format converter has been allocated, it is necessary that we call copy
        // pixels through it rather than directly from the back buffer since its very
        // existence implies that a conversion is needed.
        GetPossiblyFormatConvertedBackBuffer(&pIWGXBitmapSource);

        IFC(pIWGXBitmapSource->CopyPixels(
            &copyRegion,
            cbLockStride,
            cbBufferSize,
            pbSurface
            ));

        // We need to release the lock and format converter here because we are in a loop.
        ReleaseInterface(pIWGXBitmapSource);
        ReleaseInterface(pFrontBufferLock);
    }

Cleanup:
    ReleaseInterfaceNoNULL(pIWGXBitmapSource);
    ReleaseInterfaceNoNULL(pFrontBufferLock);

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Method:
//      CSwDoubleBufferedBitmap::ProtectBackBuffer
//
//  Synopsis:
//      Puts the back buffer in a state such that if someone happens to
//      still hold a pointer to it, they can't modify it.  This could be 
//      implemented as a copy to a new bitmap (that they dont have a pointer to), 
//      or because the back buffer is actually a CWriteProtectedBitmap, we can 
//      just write protect it.
//
//------------------------------------------------------------------------------
HRESULT 
CSwDoubleBufferedBitmap::ProtectBackBuffer()
{
    HRESULT hr = S_OK;

    IFC(m_pBackBufferAsWriteProtectedBitmap->ProtectBitmap());
    
Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Method:
//      CSwDoubleBufferedBitmap::HrFindInterface
//
//  Synopsis:
//      Not implemented.  We need to define this method because CMILCOMBase
//      makes it abstract.  But we don't actually support any interfaces, we
//      just use the AddRef/Release ref counting logic.
//
//------------------------------------------------------------------------------

STDMETHODIMP
CSwDoubleBufferedBitmap::HrFindInterface(
    __in REFIID riid,
    __deref_out void **ppvObject
    )
{
    RRETURN(E_NOINTERFACE);
}



