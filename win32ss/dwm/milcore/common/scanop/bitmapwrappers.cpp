// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/**************************************************************************
*

*
* Abstract:
*
*   Contains WIC and WGX wrapper classes
*
**************************************************************************/

#include "precomp.hpp"

MtDefine(CWICWrapperBitmap, MILImaging, "CWICWrapperBitmap");
MtDefine(CWICWrapperBitmapSource, MILImaging, "CWICWrapperBitmapSouce");
MtDefine(CWICWrapperBitmapLock, MILImaging, "CWICWrapperBitmapLock");
MtDefine(CWGXWrapperBitmap, MILImaging, "CWGXWrapperBitmap");
MtDefine(CWGXWrapperBitmapSource, MILImaging, "CWGXWrapperBitmapSouce");
MtDefine(CWGXWrapperBitmapLock, MILImaging, "CWGXWrapperBitmapLock");

HRESULT
WrapInClosestBitmapInterface(
    __in_ecount(1) IWICBitmapSource *pIWICBitmapSource,
    __deref_out_ecount(1) IWGXBitmapSource **ppIWGXBitmapSource
    )
{
    HRESULT hr = S_OK;
    IWICBitmap *pIWICBitmap = NULL;
    
    if (SUCCEEDED(pIWICBitmapSource->QueryInterface(
            IID_IWICBitmap, reinterpret_cast<void **>(&pIWICBitmap)
            ))
       )
    { 
        // Undo the QI() AddRef
        ReleaseInterface(pIWICBitmapSource);
        
        IWGXBitmap *pIWGXBitmap = NULL;
        
        IFC(CWICWrapperBitmap::Create(
            pIWICBitmap,
            &pIWGXBitmap
            ));
        
        *ppIWGXBitmapSource = pIWGXBitmap;
    }
    else
    {
        IFC(CWICWrapperBitmapSource::Create(
            pIWICBitmapSource,
            ppIWGXBitmapSource
            ));
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
WrapInClosestBitmapInterface(
    __in_ecount(1) IWGXBitmapSource *pIWGXBitmapSource,
    __deref_out_ecount(1) IWICBitmapSource **ppIWICBitmapSource
    )
{
    HRESULT hr = S_OK;
    IWGXBitmap *pIWGXBitmap = NULL;
    
    if (SUCCEEDED(pIWGXBitmapSource->QueryInterface(
            IID_IWGXBitmap, reinterpret_cast<void **>(&pIWGXBitmap)
            ))
       )
    { 
        // Undo the QI() AddRef
        ReleaseInterface(pIWGXBitmapSource);
        
        IWICBitmap *pIWICBitmap = NULL;
        
        IFC(CWGXWrapperBitmap::Create(
            pIWGXBitmap,
            &pIWICBitmap
            ));
        
        *ppIWICBitmapSource = pIWICBitmap;  // Steal ref
    }
    else
    {
        IFC(CWGXWrapperBitmapSource::Create(
            pIWGXBitmapSource,
            ppIWICBitmapSource
            ));
    }

Cleanup:
    RRETURN(hr);
}


CWGXWrapperBitmapSource::CWGXWrapperBitmapSource(__in_ecount(1) IWGXBitmapSource *pIWGXBitmapSource) 
    : m_pIWGXBitmapSource(pIWGXBitmapSource)
{
    m_pIWGXBitmapSource->AddRef();
}

CWGXWrapperBitmapSource::~CWGXWrapperBitmapSource()
{
    ReleaseInterface(m_pIWGXBitmapSource);
}


/* static */ HRESULT
CWGXWrapperBitmapSource::Create(
    __in_ecount(1) IWGXBitmapSource *pIBitmap,
    __deref_out_ecount(1) IWICBitmapSource **ppWrapper
    )
{
    HRESULT hr = S_OK;
    
    CWGXWrapperBitmapSource *pWrapper = new CWGXWrapperBitmapSource(pIBitmap);
    IFCOOM(pWrapper);
    pWrapper->AddRef();

    *ppWrapper = pWrapper;

Cleanup:
    RRETURN(hr);
}

STDMETHODIMP
CWGXWrapperBitmapSource::GetSize(
    __out_ecount(1) UINT *puWidth,
    __out_ecount(1) UINT *puHeight
    )
{
    RRETURN(m_pIWGXBitmapSource->GetSize(puWidth, puHeight));
}


STDMETHODIMP
CWGXWrapperBitmapSource::GetPixelFormat(
    __out_ecount(1) WICPixelFormatGUID *pPixelFormat
    )
{
    HRESULT hr = S_OK;
    MilPixelFormat::Enum fmtMIL;
    
    IFC(m_pIWGXBitmapSource->GetPixelFormat(&fmtMIL));

    *pPixelFormat = MilPfToWic(fmtMIL);

Cleanup:
    RRETURN(hr);
}

STDMETHODIMP
CWGXWrapperBitmapSource::GetResolution(
    __out_ecount(1) double *pDpiX,
    __out_ecount(1) double *pDpiY
    )
{
    RRETURN(m_pIWGXBitmapSource->GetResolution(pDpiX, pDpiY));
}

STDMETHODIMP
CWGXWrapperBitmapSource::CopyPalette(
    __in_ecount(1) IWICPalette *pIPalette
    )
{
    RRETURN(m_pIWGXBitmapSource->CopyPalette(pIPalette));
}

STDMETHODIMP
CWGXWrapperBitmapSource::CopyPixels(
    __in_ecount(1) const WICRect *prc,
    __in_ecount(1) UINT cbStride,
    __in_ecount(1) UINT cbBufferSize,
    __out_ecount(cbBufferSize) BYTE *pbPixels
    )
{
    RRETURN(m_pIWGXBitmapSource->CopyPixels(prc, cbStride, cbBufferSize, pbPixels));
}

STDMETHODIMP CWGXWrapperBitmapSource::HrFindInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void **ppvObject)
{
    HRESULT hr = E_INVALIDARG;

    if (ppvObject)
    {
        if (riid == IID_IWICBitmapSource)
        {
            *ppvObject = static_cast<IWICBitmapSource *>(this);

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
* Function Description:
*
*   CWGXWrapperBitmapLock constructor
*
**************************************************************************/

CWGXWrapperBitmapLock::CWGXWrapperBitmapLock(__in_ecount(1) IWGXBitmapLock *pIWGXLock) :
    m_pIWGXLock(pIWGXLock)
{
    m_pIWGXLock->AddRef();
}

/**************************************************************************
*
* Function Description:
*
*   CWGXWrapperBitmapLock destructor - Unlocks the bitmap lock state.
*
*
**************************************************************************/

CWGXWrapperBitmapLock::~CWGXWrapperBitmapLock()
{
    ReleaseInterface(m_pIWGXLock);
}

/**************************************************************************
*
* Function Description:
*
*   CWGXWrapperBitmapLock::HrFindInterfaces returns interfaces implemented on this object.
*
*
**************************************************************************/

STDMETHODIMP CWGXWrapperBitmapLock::HrFindInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void **ppvObject)
{
    HRESULT hr = E_INVALIDARG;

    if (ppvObject)
    {
        if (riid == IID_IWICBitmapLock)
        {
            *ppvObject = static_cast<IWICBitmapLock *>(this);

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
* Function Description:
*
*   CWGXWrapperBitmapLock::Create
*
**************************************************************************/

/* static */ HRESULT CWGXWrapperBitmapLock::Create(
    __in_ecount(1) IWGXBitmapLock *pIWGXLock,
    __deref_out_ecount(1) IWICBitmapLock **ppWrapperLock
    )
{
    HRESULT hr = S_OK;

    CWGXWrapperBitmapLock *pLock = new CWGXWrapperBitmapLock(pIWGXLock);
    IFCOOM(pLock);
    pLock->AddRef();

    *ppWrapperLock = pLock;

Cleanup:
    RRETURN(hr);
}

HRESULT CWGXWrapperBitmapLock::GetSize(
    __out_ecount(1) UINT *puWidth,
    __out_ecount(1) UINT *puHeight
    )
{
    RRETURN(m_pIWGXLock->GetSize(puWidth, puHeight));
}

HRESULT CWGXWrapperBitmapLock::GetStride(
    __out_ecount(1) UINT *puStride
    )
{
    RRETURN(m_pIWGXLock->GetStride(puStride));
}

HRESULT CWGXWrapperBitmapLock::GetDataPointer(
    __out_ecount(1) UINT *pcbBufferSize,
    __deref_out_bcount(*pcbBufferSize) BYTE **ppbData
    )
{
    RRETURN(m_pIWGXLock->GetDataPointer(pcbBufferSize, ppbData));
}

HRESULT CWGXWrapperBitmapLock::GetPixelFormat(
    __out_ecount(1) WICPixelFormatGUID *pPixelFormat
    )
{
    HRESULT hr = S_OK;
    MilPixelFormat::Enum fmtMIL;

    IFC(m_pIWGXLock->GetPixelFormat(&fmtMIL));
    *pPixelFormat = MilPfToWic(fmtMIL);

Cleanup:
    RRETURN(hr);
}

CWGXWrapperBitmap::CWGXWrapperBitmap(__in_ecount(1) IWGXBitmap *pIWGXBitmap) 
    : m_pIWGXBitmap(pIWGXBitmap)
{
    m_pIWGXBitmap->AddRef();
}

CWGXWrapperBitmap::~CWGXWrapperBitmap()
{
    ReleaseInterface(m_pIWGXBitmap);
}

/* static */ HRESULT
CWGXWrapperBitmap::Create(
    __in_ecount(1) IWGXBitmap *pIBitmap,
    __deref_out_ecount(1) IWICBitmap **ppWrapper
    )
{
    HRESULT hr = S_OK;
    
    CWGXWrapperBitmap *pWrapper = new CWGXWrapperBitmap(pIBitmap);
    IFCOOM(pWrapper);
    pWrapper->AddRef();

    *ppWrapper = pWrapper;

Cleanup:
    RRETURN(hr);
}

STDMETHODIMP
CWGXWrapperBitmap::GetSize(
    __out_ecount(1) UINT *puWidth,
    __out_ecount(1) UINT *puHeight
    )
{
    RRETURN(m_pIWGXBitmap->GetSize(puWidth, puHeight));
}

STDMETHODIMP
CWGXWrapperBitmap::GetPixelFormat(
    __out_ecount(1) WICPixelFormatGUID *pPixelFormat
    )
{
    HRESULT hr = S_OK;
    MilPixelFormat::Enum fmtMIL;
    
    IFC(m_pIWGXBitmap->GetPixelFormat(&fmtMIL));

    *pPixelFormat = MilPfToWic(fmtMIL);

Cleanup:
    RRETURN(hr);
}

STDMETHODIMP
CWGXWrapperBitmap::GetResolution(
    __out_ecount(1) double *pDpiX,
    __out_ecount(1) double *pDpiY
    )
{
    RRETURN(m_pIWGXBitmap->GetResolution(pDpiX, pDpiY));
}

STDMETHODIMP
CWGXWrapperBitmap::CopyPalette(
    __in_ecount(1) IWICPalette *pIPalette
    )
{
    RRETURN(m_pIWGXBitmap->CopyPalette(pIPalette));
}

STDMETHODIMP
CWGXWrapperBitmap::CopyPixels(
    __in_ecount(1) const WICRect *prc,
    __in_ecount(1) UINT cbStride,
    __in_ecount(1) UINT cbBufferSize,
    __out_ecount(cbBufferSize) BYTE *pbPixels
    )
{
    RRETURN(m_pIWGXBitmap->CopyPixels(prc, cbStride, cbBufferSize, pbPixels));
}


STDMETHODIMP
CWGXWrapperBitmap::Lock(
    __in_ecount_opt(1) const WICRect *prcLock,
    DWORD flags,
    __deref_out_ecount(1) IWICBitmapLock **ppILock
    )
{
    HRESULT hr = S_OK;
    IWICBitmapLock *pLockWrapper = NULL;
    IWGXBitmapLock *pIWGXLock = NULL;

    // Ensure that flag mapping is the same
    C_ASSERT(MilBitmapLock::Write == WICBitmapLockWrite);
    C_ASSERT(MilBitmapLock::Read == WICBitmapLockRead);

    IFC(m_pIWGXBitmap->Lock(prcLock, flags, &pIWGXLock));

    IFC(CWGXWrapperBitmapLock::Create(
        pIWGXLock,
        &pLockWrapper
        ));

    *ppILock = pLockWrapper;

Cleanup:
    // Creating the wrapper addref'd it again
    ReleaseInterface(pIWGXLock);
    
    RRETURN(hr);
}

STDMETHODIMP
CWGXWrapperBitmap::SetPalette(
    __in_ecount(1) IWICPalette *pIPalette
    )
{
    RRETURN(m_pIWGXBitmap->SetPalette(pIPalette));
}

STDMETHODIMP
CWGXWrapperBitmap::SetResolution(
    double dpiX,
    double dpiY
    )
{
    RRETURN(m_pIWGXBitmap->SetResolution(dpiX, dpiY));
}


STDMETHODIMP CWGXWrapperBitmap::HrFindInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void **ppvObject)
{
    HRESULT hr = E_INVALIDARG;

    if (ppvObject)
    {
        if (riid == IID_IWICBitmap)
        {
            *ppvObject = static_cast<IWICBitmap *>(this);

            hr = S_OK;
        }
        else if (riid == IID_IWICBitmapSource)
        {
            *ppvObject = static_cast<IWICBitmapSource *>(this);

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
* Function Description:
*
*   CWICWrapperBitmapLock constructor
*
*
**************************************************************************/

CWICWrapperBitmapLock::CWICWrapperBitmapLock(__in_ecount(1) IWICBitmapLock *pIWICLock) : 
    m_pIWICLock(pIWICLock)
{
    m_pIWICLock->AddRef();
}

/**************************************************************************
*
* Function Description:
*
*   CWICWrapperBitmapLock destructor - Unlocks the bitmap lock state.
*
*
**************************************************************************/

CWICWrapperBitmapLock::~CWICWrapperBitmapLock()
{
    ReleaseInterface(m_pIWICLock);
}

/**************************************************************************
*
* Function Description:
*
*   CWICWrapperBitmapLock::HrFindInterfaces returns interfaces implemented on this object.
*
*
**************************************************************************/

STDMETHODIMP CWICWrapperBitmapLock::HrFindInterface(
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

/**************************************************************************
*
* Function Description:
*
*   CWICWrapperBitmapLock::Create
*
*
**************************************************************************/

/* static */ HRESULT CWICWrapperBitmapLock::Create(
    __in_ecount(1) IWICBitmapLock *pIWICLock,
    __deref_out_ecount(1) IWGXBitmapLock **ppWrapperLock
    )
{
    HRESULT hr = S_OK;

    CWICWrapperBitmapLock *pLock = new CWICWrapperBitmapLock(pIWICLock);
    IFCOOM(pLock);
    pLock->AddRef();

    *ppWrapperLock = pLock;

Cleanup:
    RRETURN(hr);
}

HRESULT CWICWrapperBitmapLock::GetSize(
    __out_ecount(1) UINT *puWidth,
    __out_ecount(1) UINT *puHeight
    )
{
    RRETURN(m_pIWICLock->GetSize(puWidth, puHeight));
}

HRESULT CWICWrapperBitmapLock::GetStride(
    __out_ecount(1) UINT *puStride
    )
{
    RRETURN(m_pIWICLock->GetStride(puStride));
}

HRESULT CWICWrapperBitmapLock::GetDataPointer(
    __out_ecount(1) UINT *pcbBufferSize,
    __deref_out_bcount(*pcbBufferSize) BYTE **ppbData
    )
{
    RRETURN(m_pIWICLock->GetDataPointer(pcbBufferSize, ppbData));
}

HRESULT CWICWrapperBitmapLock::GetPixelFormat(
    __out_ecount(1) MilPixelFormat::Enum *pPixelFormat
    )
{
    HRESULT hr = S_OK;
    WICPixelFormatGUID fmtWIC;

    IFC(m_pIWICLock->GetPixelFormat(&fmtWIC));
    IFC(WicPfToMil(fmtWIC, pPixelFormat));

Cleanup:
    RRETURN(hr);
}

CWICWrapperBitmapSource::CWICWrapperBitmapSource(
    __in_ecount(1) IWICBitmapSource *pIWICBitmapSource
    ) 
    : m_pIWICBitmapSource(pIWICBitmapSource)
{
    m_pIWICBitmapSource->AddRef();
}

CWICWrapperBitmapSource::~CWICWrapperBitmapSource()
{
    ReleaseInterface(m_pIWICBitmapSource);
}

/* static */ HRESULT
CWICWrapperBitmapSource::Create(
    __in_ecount(1) IWICBitmapSource *pIWICBitmapSource,
    __deref_out_ecount(1) IWGXBitmapSource **ppWrapper
    )
{
    HRESULT hr = S_OK;
    
    CWICWrapperBitmapSource *pWrapper = new CWICWrapperBitmapSource(pIWICBitmapSource);
    IFCOOM(pWrapper);
    pWrapper->AddRef();

    *ppWrapper = pWrapper;

Cleanup:
    RRETURN(hr);
}

STDMETHODIMP
CWICWrapperBitmapSource::GetSize(
    __out_ecount(1) UINT *puWidth,
    __out_ecount(1) UINT *puHeight
    )
{
    RRETURN(m_pIWICBitmapSource->GetSize(puWidth, puHeight));
}

STDMETHODIMP
CWICWrapperBitmapSource::GetPixelFormat(
    __out_ecount(1) MilPixelFormat::Enum *pPixelFormat
    )
{
    HRESULT hr = S_OK;
    
    WICPixelFormatGUID fmtWIC;
    IFC(m_pIWICBitmapSource->GetPixelFormat(&fmtWIC));
    IFC(WicPfToMil(fmtWIC, pPixelFormat));

Cleanup:
    RRETURN(hr);
}

STDMETHODIMP
CWICWrapperBitmapSource::GetResolution(
    __out_ecount(1) double *pDpiX,
    __out_ecount(1) double *pDpiY
    )
{
    RRETURN(m_pIWICBitmapSource->GetResolution(pDpiX, pDpiY));
}

STDMETHODIMP
CWICWrapperBitmapSource::CopyPalette(
    __in_ecount(1) IWICPalette *pIPalette
    )
{
    RRETURN(m_pIWICBitmapSource->CopyPalette(pIPalette));
}

STDMETHODIMP
CWICWrapperBitmapSource::CopyPixels(
    __in_ecount(1) const MILRect *prc,
    __in_ecount(1) UINT cbStride,
    __in_ecount(1) UINT cbBufferSize,
    __out_ecount(cbBufferSize) BYTE *pvPixels
    )
{
    RRETURN(m_pIWICBitmapSource->CopyPixels(prc, cbStride, cbBufferSize, pvPixels));
}

STDMETHODIMP CWICWrapperBitmapSource::HrFindInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void **ppvObject)
{
    HRESULT hr = E_INVALIDARG;

    if (ppvObject)
    {
        if (riid == IID_IWGXBitmapSource)
        {
            *ppvObject = static_cast<IWGXBitmapSource *>(this);

            hr = S_OK;
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }

    return hr;
}


CWICWrapperBitmap::CWICWrapperBitmap(__in_ecount(1) IWICBitmap *pIBitmap) 
    : m_pIBitmap(pIBitmap)
{
    m_pIBitmap->AddRef();
}

CWICWrapperBitmap::~CWICWrapperBitmap()
{
    ReleaseInterface(m_pIBitmap);
}

/* static */ HRESULT
CWICWrapperBitmap::Create(
    __in_ecount(1) IWICBitmap *pIBitmap,
    __deref_out_ecount(1) IWGXBitmap **ppWrapper
    )
{
    HRESULT hr = S_OK;
    
    CWICWrapperBitmap *pWrapper = new CWICWrapperBitmap(pIBitmap);
    IFCOOM(pWrapper);
    pWrapper->AddRef();

    *ppWrapper = pWrapper;

Cleanup:
    RRETURN(hr);
}

STDMETHODIMP
CWICWrapperBitmap::GetSize(
    __out_ecount(1) UINT *puWidth,
    __out_ecount(1) UINT *puHeight
    )
{
    RRETURN(m_pIBitmap->GetSize(puWidth, puHeight));
}

STDMETHODIMP
CWICWrapperBitmap::GetPixelFormat(
    __out_ecount(1) MilPixelFormat::Enum *pPixelFormat
    )
{
    HRESULT hr = S_OK;
    
    WICPixelFormatGUID fmtWIC;
    IFC(m_pIBitmap->GetPixelFormat(&fmtWIC));
    IFC(WicPfToMil(fmtWIC, pPixelFormat));

Cleanup:
    RRETURN(hr);
}

STDMETHODIMP
CWICWrapperBitmap::GetPixelFormat(
    __out_ecount(1) WICPixelFormatGUID *pPixelFormat
    )
{
    RRETURN(m_pIBitmap->GetPixelFormat(pPixelFormat));
}

STDMETHODIMP
CWICWrapperBitmap::GetResolution(
    __out_ecount(1) double *pDpiX,
    __out_ecount(1) double *pDpiY
    )
{
    RRETURN(m_pIBitmap->GetResolution(pDpiX, pDpiY));
}

STDMETHODIMP
CWICWrapperBitmap::CopyPalette(
    __in_ecount(1) IWICPalette *pIPalette
    )
{
    RRETURN(m_pIBitmap->CopyPalette(pIPalette));
}

STDMETHODIMP
CWICWrapperBitmap::CopyPixels(
    __in_ecount(1) const MILRect *prc,
    __in_ecount(1) UINT cbStride,
    __in_ecount(1) UINT cbBufferSize,
    __out_ecount(cbBufferSize) BYTE *pvPixels
    )
{
    RRETURN(m_pIBitmap->CopyPixels(prc, cbStride, cbBufferSize, pvPixels));
}

STDMETHODIMP
CWICWrapperBitmap::Lock(
    __in_ecount(1) const MILRect *prcLock,
    DWORD flags,
    __deref_out_ecount(1) IWGXBitmapLock **ppILock
    )
{   
    HRESULT hr = S_OK;
    IWGXBitmapLock *pLockWrapper = NULL;
    IWICBitmapLock *pIWICLock = NULL;

    // Ensure that flag mapping is the same
    C_ASSERT(MilBitmapLock::Write == WICBitmapLockWrite);
    C_ASSERT(MilBitmapLock::Read == WICBitmapLockRead);

    IFC(m_pIBitmap->Lock(prcLock, flags, &pIWICLock));

    IFC(CWICWrapperBitmapLock::Create(
        pIWICLock,
        &pLockWrapper
        ));

    *ppILock = pLockWrapper;

Cleanup:
    // Since creating the wrapper added an extra ref
    ReleaseInterface(pIWICLock);
    
    RRETURN(hr);
}

STDMETHODIMP
CWICWrapperBitmap::SetPalette(
    __in_ecount(1) IWICPalette *pIPalette
    )
{
    // Ideally we'd compare the contents of two palettes to see if anything really changed,
    // but that involves a copy so we'll skip it.
    UpdateUniqueCount();
    
    RRETURN(m_pIBitmap->SetPalette(pIPalette));
}

STDMETHODIMP
CWICWrapperBitmap::SetResolution(
    double dpiX,
    double dpiY
    )
{
    HRESULT hr = S_OK;
    double currentDpiX, currentDpiY;

    IFC(m_pIBitmap->GetResolution(&currentDpiX, &currentDpiY));

    if (currentDpiX != dpiX || currentDpiY != dpiY)
    {
        UpdateUniqueCount();
    }

Cleanup:
    RRETURN(m_pIBitmap->SetResolution(dpiX, dpiY));
}



