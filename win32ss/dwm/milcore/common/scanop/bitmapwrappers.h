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

MtExtern(CWGXWrapperBitmapSource);

class CWGXWrapperBitmapSource : 
    public CMILCOMBase,
    public IWICBitmapSource
{
public:
    static HRESULT Create(
        __in_ecount(1) IWGXBitmapSource *pIWGXBitmapSource, 
        __deref_out_ecount(1) IWICBitmapSource **ppWrapper
        );

    virtual ~CWGXWrapperBitmapSource();

    DECLARE_COM_BASE;
    
    STDMETHOD(GetSize)(
        __out_ecount(1) UINT *puWidth,
        __out_ecount(1) UINT *puHeight
        );

    STDMETHOD(GetPixelFormat)(
        __out_ecount(1) WICPixelFormatGUID *pPixelFormat
        );

    STDMETHOD(GetResolution)(
        __out_ecount(1) double *pDpiX,
        __out_ecount(1) double *pDpiY
        );

    STDMETHOD(CopyPalette)(
        __in_ecount(1) IWICPalette *pIPalette
        );

    STDMETHOD(CopyPixels)(
        __in_ecount(1) const WICRect *prc,
        __in_ecount(1) UINT cbStride,
        __in_ecount(1) UINT cbBufferSize,
        __out_ecount(cbBufferSize) BYTE *pbPixels
        );

protected:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CWGXWrapperBitmapSource));

    CWGXWrapperBitmapSource(__in_ecount(1) IWGXBitmapSource *pIWGXBitmapSource);
    
    STDMETHOD(HrFindInterface)(__in_ecount(1) REFIID riid, __deref_out void **ppv);

private:
    IWGXBitmapSource *m_pIWGXBitmapSource;
    
};

MtExtern(CWGXWrapperBitmap);

class CWGXWrapperBitmap : 
    public CMILCOMBase,
    public IWICBitmap
{
public:
    static HRESULT Create(
        __in_ecount(1) IWGXBitmap *pIWGXBitmap, 
        __deref_out_ecount(1) IWICBitmap **ppWrapper
        );

    virtual ~CWGXWrapperBitmap();

    DECLARE_COM_BASE;
    
    STDMETHOD(GetSize)(
        __out_ecount(1) UINT *puWidth,
        __out_ecount(1) UINT *puHeight
        );

    STDMETHOD(GetPixelFormat)(
        __out_ecount(1) WICPixelFormatGUID *pPixelFormat
        );

    STDMETHOD(GetResolution)(
        __out_ecount(1) double *pDpiX,
        __out_ecount(1) double *pDpiY
        );

    STDMETHOD(CopyPalette)(
        __in_ecount(1) IWICPalette *pIPalette
        );

    STDMETHOD(CopyPixels)(
        __in_ecount(1) const WICRect *prc,
        __in_ecount(1) UINT cbStride,
        __in_ecount(1) UINT cbBufferSize,
        __out_ecount(cbBufferSize) BYTE *pbPixels
        );

    STDMETHOD(SetPalette)(
        __in_ecount(1) IWICPalette *pIPalette
        );

    STDMETHOD(Lock)(
        __in_ecount_opt(1) const WICRect *prcLock,
        DWORD flags,
        __deref_out_ecount(1) IWICBitmapLock **ppILock
        );

    STDMETHOD(SetResolution)(
        double dblDpiX,
        double dblDpiY
        );

protected:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CWGXWrapperBitmap));

    CWGXWrapperBitmap(__in_ecount(1) IWGXBitmap *pIWGXBitmap);
    
    STDMETHOD(HrFindInterface)(__in_ecount(1) REFIID riid, __deref_out void **ppv);

private:
    IWGXBitmap *m_pIWGXBitmap;  
};

/**************************************************************************
*
* Class Description:
*
*   CWGXWrapperBitmapLock
*
*   IWICWGXBitmapLock that forwards WIC interface calls to a wrapped 
*   IWGXBitmapLock
*
**************************************************************************/

MtExtern(CWGXWrapperBitmapLock);

class CWGXWrapperBitmapLock : 
    public CMILCOMBase,
    public IWICBitmapLock
{
public:
    static HRESULT Create(
        __in_ecount(1) IWGXBitmapLock *pIWGXLock,
        __deref_out_ecount(1) IWICBitmapLock **ppLock
        );
    
    // IUnknown
    DECLARE_COM_BASE;

    // Implement the IWGXBitmapLock interface.
    STDMETHOD(GetSize)(
        __out_ecount(1) UINT *puWidth,
        __out_ecount(1) UINT *puHeight
        );
    
    STDMETHOD(GetStride)(
        __out_ecount(1) UINT *puStride
        );
    
    STDMETHOD(GetDataPointer)(
        __out_ecount(1) UINT *pcbBufferSize,
        __deref_out_bcount(*pcbBufferSize) BYTE **ppbData
        );
    
    STDMETHOD(GetPixelFormat)(
        __out_ecount(1) WICPixelFormatGUID *pPixelFormat
        );

protected:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CWGXWrapperBitmapLock));

    CWGXWrapperBitmapLock(__in_ecount(1) IWGXBitmapLock *pIWGXLock);
    virtual ~CWGXWrapperBitmapLock();
    STDMETHOD(HrFindInterface)(__in_ecount(1) REFIID riid, __deref_out void **ppv);

private:
    IWGXBitmapLock *m_pIWGXLock;

};

HRESULT
WrapInClosestBitmapInterface(
    __in_ecount(1) IWICBitmapSource *pIWICBitmapSource,
    __deref_out_ecount(1) IWGXBitmapSource **ppIWGXBitmapSource
    );

HRESULT
WrapInClosestBitmapInterface(
    __in_ecount(1) IWGXBitmapSource *pIWGXBitmapSource,
    __deref_out_ecount(1) IWICBitmapSource **ppIWICBitmapSource
    );

/**************************************************************************
*
* Class Description:
*
*   CWICWrapperBitmapSource
*
*   IWGXBitmapSource that forwards WIC interface calls to a wrapped IWICBitmapSource
*
**************************************************************************/

MtExtern(CWICWrapperBitmapSource);

class CWICWrapperBitmapSource : 
    public CMILCOMBase,
    public IWGXBitmapSource
{
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CWICWrapperBitmapSource));
    virtual ~CWICWrapperBitmapSource();
    
    static HRESULT Create(
        __in_ecount(1) IWICBitmapSource *pIWICBitmapSource,
        __deref_out_ecount(1) IWGXBitmapSource **ppWrapper
        );

    DECLARE_COM_BASE;

    // IWGXBitmapSource
    STDMETHOD(GetSize)(
        __out_ecount(1) UINT *puWidth,
        __out_ecount(1) UINT *puHeight
        );

    STDMETHOD(GetPixelFormat)(
        __out_ecount(1) MilPixelFormat::Enum *pPixelFormat
        );

    STDMETHOD(GetResolution)(
        __out_ecount(1) double *pDpiX,
        __out_ecount(1) double *pDpiY
        );

    STDMETHOD(CopyPalette)(
        __in_ecount(1) IWICPalette *pIPalette
        );

    STDMETHOD(CopyPixels)(
        __in_ecount(1) const MILRect *prc,
        __in_ecount(1) UINT cbStride,
        __in_ecount(1) UINT cbBufferSize,
        __out_ecount(cbBufferSize) BYTE *pvPixels
        );

protected:    
    STDMETHOD(HrFindInterface)(__in_ecount(1) REFIID riid, __deref_out void **ppv);

private:
    CWICWrapperBitmapSource(__in_ecount(1) IWICBitmapSource *pIWICBitmapSource);

    IWICBitmapSource *m_pIWICBitmapSource;
};



/**************************************************************************
*
* Class Description:
*
*   CWICWrapperBitmap
*
*   IWGXBitmap that forwards WIC interface calls to a wrapped IWICBitmap
*
**************************************************************************/

MtExtern(CWICWrapperBitmap);

class CWICWrapperBitmap :
    public CBaseWGXBitmap,
    public IWICBitmapSource
{
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CWICWrapperBitmap));
    virtual ~CWICWrapperBitmap();
    
    static HRESULT Create(
        __in_ecount(1) IWICBitmap *pIBitmap,
        __deref_out_ecount(1) IWGXBitmap **ppWrapper
        );

    // Fixes ambiguity errors between IUnknown::AddRef/Release/QI
    // implemented through IWICBitmapSource and AddRef/Release/QI
    // inherited from CBaseWGXBitmap
    DECLARE_COM_BASE;

    // IWGXBitmapSource
    STDMETHOD(GetSize)(
        __out_ecount(1) UINT *puWidth,
        __out_ecount(1) UINT *puHeight
        );

    STDMETHOD(GetPixelFormat)(
        __out_ecount(1) MilPixelFormat::Enum *pPixelFormat
        );

    STDMETHOD(GetResolution)(
        __out_ecount(1) double *pDpiX,
        __out_ecount(1) double *pDpiY
        );

    STDMETHOD(CopyPalette)(
        __in_ecount(1) IWICPalette *pIPalette
        );

    STDMETHOD(CopyPixels)(
        __in_ecount(1) const MILRect *prc,
        __in_ecount(1) UINT cbStride,
        __in_ecount(1) UINT cbBufferSize,
        __out_ecount(cbBufferSize) BYTE *pvPixels
        );

    // IWICBitmapSource
    STDMETHOD(GetPixelFormat)(
        __out_ecount(1) WICPixelFormatGUID *pPixelFormat
        );

    // IWGXBitmap
    STDMETHOD(Lock)(
        __in_ecount(1) const MILRect *prcLock,
        DWORD flags,
        __deref_out_ecount(1) IWGXBitmapLock **ppILock
        );

    STDMETHOD(SetPalette)(
        __in_ecount(1) IWICPalette *pIPalette
        );

    STDMETHOD(SetResolution)(
        double dpiX,
        double dpiY
        );

private:
    CWICWrapperBitmap(__in_ecount(1) IWICBitmap *pIBitmap);

    IWICBitmap *m_pIBitmap;
};

/**************************************************************************
*
* Class Description:
*
*   CWICWrapperBitmapLock
*
*   IWGXBitmapLock that forwards WIC interface calls to a wrapped 
*   IWICWGXBitmapLock
*
**************************************************************************/

MtExtern(CWICWrapperBitmapLock);

class CWICWrapperBitmapLock : 
    public CMILCOMBase,
    public IWGXBitmapLock
{
public:
    static HRESULT Create(
        __in_ecount(1) IWICBitmapLock *pIWICLock,
        __deref_out_ecount(1) IWGXBitmapLock **ppLock
        );

    // IUnknown
    DECLARE_COM_BASE;

    // Implement the IWGXBitmapLock interface.
    STDMETHOD(GetSize)(
        __out_ecount(1) UINT *puWidth,
        __out_ecount(1) UINT *puHeight
        );
    
    STDMETHOD(GetStride)(
        __out_ecount(1) UINT *puStride
        );
    
    STDMETHOD(GetDataPointer)(
        __out_ecount(1) UINT *pcbBufferSize,
        __deref_out_bcount(*pcbBufferSize) BYTE **ppbData
        );
    
    STDMETHOD(GetPixelFormat)(
        __out_ecount(1) MilPixelFormat::Enum *pPixelFormat
        );

protected:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CWICWrapperBitmapLock));

    CWICWrapperBitmapLock(__in_ecount(1) IWICBitmapLock *pIWICLock);
    virtual ~CWICWrapperBitmapLock();
    STDMETHOD(HrFindInterface)(__in_ecount(1) REFIID riid, __deref_out void **ppv);

private:
    IWICBitmapLock *m_pIWICLock;

};


