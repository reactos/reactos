// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/**************************************************************************
*

*
* Abstract:
*
*   Contains system memory based bitmap class declarations
*
*
**************************************************************************/

MtExtern(CSystemMemoryBitmap);

class CSystemMemoryBitmap : 
    public IMILDynamicResource,
    public CWGXBitmap
{   
public:
    DECLARE_COM_BASE;

    static HRESULT Create(
        UINT width,
        UINT height,
        MilPixelFormat::Enum pixelFormat,
        BOOL fClear,
        BOOL fIsDynamic,
        __deref_out CSystemMemoryBitmap **ppSystemMemoryBitmap
        );

    CSystemMemoryBitmap();
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CSystemMemoryBitmap));

    HRESULT Init(
        __in_ecount(1) IWGXBitmapSource *pISource
        );

    HRESULT Init(
        __in_ecount(1) IWGXBitmapSource *pISource,
        __in_ecount(1) WICRect * srcRect,
        BOOL fCopySource
        );

    HRESULT Init(
        __in UINT nWidth,
        __in UINT nHeight,
        __in MilPixelFormat::Enum pxlFormat,
        __in BOOL fClear = FALSE,
        __in BOOL fIsDynamic = FALSE
        );

    HRESULT UnsafeUpdateFromSource(
        __in_ecount(1) IWGXBitmapSource *pISource,
        __in_ecount(1) MilRectU &rcSrc,
        UINT uDstLeft,
        UINT uDstTop
        );

    // IWGXBitmap interfaces.

    STDMETHOD(Lock)(
        __in_ecount_opt(1) IN const WICRect *prcLock,
        __in DWORD dwFlags,
        __deref_out_ecount(1) IWGXBitmapLock **ppILock
        );

    // IMILDynamicResource interfaces
    STDMETHOD(IsDynamicResource)(
        __out_ecount(1) bool *pfIsDynamic
        );

    

protected:

    STDMETHOD(HrFindInterface)(__in_ecount(1) REFIID riid, __deref_out void **ppv) override;

    virtual ~CSystemMemoryBitmap();

    VOID *m_pPixels;
    UINT m_nStride;

    //
    // Indicates to the hardware pipeline that this bitmap will be updated often 
    // so the pipeline may want to keep a system memory texture around rather than 
    // repeatedly creating and deleting it on every update (on non-LDDM).  This gives large 
    // perf gains for rapidly changing sources, however it will use more memory
    // in multi-monitor scenarios.
    //
    bool m_fIsDynamic;
};

MtExtern(CClientMemoryBitmap);

class CClientMemoryBitmap : public CSystemMemoryBitmap
{
public:
    virtual ~CClientMemoryBitmap();
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CClientMemoryBitmap));

    HRESULT HrInit(
        __in UINT nWidth,
        __in UINT nHeight,
        __in MilPixelFormat::Enum pxlFormat,
        __in UINT cbBufferSize,
        __in_bcount(cbBufferSize) void *pvPixels,
        __in UINT cbStride
        );
};

MtExtern(CDummySource);

class CDummySource :
    public CMILCOMBase,
    public IWGXBitmapSource
{
protected:
    STDMETHOD(HrFindInterface)(__in_ecount(1) REFIID riid, __deref_out void **ppv);

public:
    CDummySource(
        __in UINT nWidth,
        __in UINT nHeight,
        __in MilPixelFormat::Enum pxlFormat
        );
    virtual ~CDummySource();
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CDummySource));

    // IUnknown

    DECLARE_COM_BASE;

    // IWGXBitmapSource methods

    STDMETHOD(CopyPixels)(
        __in_ecount_opt(1) const MILRect *prc,
        __in UINT cbStride,
        __in UINT cbBufferSize,
        __out_ecount(cbBufferSize) BYTE *pbPixels
        );

    STDMETHOD(CopyPalette)(
        __in_ecount(1) IWICPalette *pIPalette
        );

    STDMETHOD(GetSize)(
        __out_ecount(1) UINT *pnWidth,
        __out_ecount(1) UINT *pnHeight
        );

    STDMETHOD(GetPixelFormat)(
        __out_ecount(1) MilPixelFormat::Enum *pPixelFormat
        );

    STDMETHOD(GetResolution)(
        __out_ecount(1) double *pDpiX,
        __out_ecount(1) double *pDpiY
        );

private:

    UINT m_nWidth;
    UINT m_nHeight;
    MilPixelFormat::Enum m_PixelFormat;
};




