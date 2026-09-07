// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/**************************************************************************
*

*
* Abstract:
*
*   Contains bitmap base class declaration
*
**************************************************************************/

/**************************************************************************
*
* Class Description:
*
*   CWGXBitmapLockState
*
*   This class implements the lock/unlock for multiple read and single write.  If there is
*   is contention it returns an error code.
*
**************************************************************************/

const LONG lockNone = 0x0000000;
const LONG lockWrite = 0x80000000;

class CWGXBitmapLockState
{
protected:
    LONG volatile m_lockState;

    // Prevent heap allocation of this object.
    inline void * __cdecl operator new(size_t cb);
    inline void * __cdecl operator new[](size_t cb);
    inline void __cdecl operator delete(void * pv);
    inline void __cdecl operator delete[](void * pv);
    inline void * __cdecl operator new(size_t cb, void * pv);

public:
    CWGXBitmapLockState();
    ~CWGXBitmapLockState();

    HRESULT LockRead();
    VOID UnlockRead();

    HRESULT LockWrite();
    VOID UnlockWrite();

    HRESULT CheckNoLocks();
};

/**************************************************************************
*
* Class Description:
*
*   CBaseWGXBitmap
*
*   This is the base class providing common IWGXBitmap functionality
*
**************************************************************************/

class CBaseWGXBitmap :
    public CMILCOMBase,
    public CObjectUniqueness,
    public IWGXBitmap,
    public CMILResourceCache,
    // Always inherit from CAssertEntry to promote checked/retail build
    // compatibility as CBaseWGXBitmap implementation is defined in multiple modules.
    public CAssertEntry
{
public:
    virtual ~CBaseWGXBitmap();
    
    // IUnknown
    DECLARE_COM_BASE;

    // IWGXBitmap
    STDMETHOD(AddDirtyRect(__in_ecount(1) const RECT *prcDirtyRect));
    
    __success(true) STDMETHOD_(bool, GetDirtyRects)(
        __deref_out_ecount(*pcDirtyRects) MilRectU const **const prgDirtyRects,   // disallow assignment to prgDirtyRects directly
        __deref_out_range(0,5) UINT * const pcDirtyRects,
        __inout_ecount(1) UINT * const pCachedUniqueness
        );

    STDMETHOD_(SourceState::Enum, SourceState)() const; 

    // IMILResourceCache
    STDMETHOD_(void, GetUniquenessToken)(
        __out_ecount(1) UINT *puToken
        ) const override;

protected:

    // Internal CMILCOMBase method providing support for QueryInterface.
    STDMETHOD(HrFindInterface)(__in_ecount(1) REFIID riid, __deref_out void **ppv);

    CBaseWGXBitmap();

private:    
    // Accumulated dirty rectangles
    __field_ecount_part_opt(5,m_cDirtyRects) CMilRectU *m_rgDirtyRects;
    __field_range(0,5) UINT m_cDirtyRects;

    UINT m_nUniquenessBeforeFirstDirtyRect;
    UINT m_nUniquenessAtLastDirtyRect;

    bool m_fClearDirtyListOnNextAdd;
};

/**************************************************************************
*
* Class Description:
*
*   CWGXBitmap
*
*   Common code for all custom IWGXBitmap implementations
*
**************************************************************************/

MtExtern(CWGXBitmap);

class CWGXBitmapLock;

class CWGXBitmap : public CBaseWGXBitmap
{
public:

    CWGXBitmap();
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CWGXBitmap));

    virtual HRESULT Init(
        __in_ecount(1) IWGXBitmapSource *pISource
        );

    // The base bitmap class implements Copy in terms of Lock as a convenience
    // for bitmaps which can easily implement Lock because they have in-memory
    // pixel arrays. This should be overridden if there is a more optimal way
    // of implementing CopyPixels.

    STDMETHOD(CopyPixels)(
        __in_ecount_opt(1) const WICRect *prc,
        __in UINT cbStride,
        __in UINT cbBufferSize,
        __out_ecount(cbBufferSize) BYTE *pvPixels
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

    STDMETHOD(SetPalette)(
        __in_ecount(1) IWICPalette *pIPalette
        );

    STDMETHOD(Lock)(
        __in_ecount_opt(1) const MILRect *prcLock,
        __in DWORD flags,
        __deref_out_ecount(1) IWGXBitmapLock **ppILock
        ) = 0;

    STDMETHOD(Unlock)(
        __in_ecount(1) CWGXBitmapLock *pBitmapLock
        );

    STDMETHOD(SetResolution)(
        __in double dblDpiX,
        __in double dblDpiY
        );

protected:

    // Bitmap dimensions.

    UINT m_nWidth;
    UINT m_nHeight;

    // Bitmap resolution.

    FLOAT m_fltDpiX;
    FLOAT m_fltDpiY;

    // How to interpret the pixel format.

    MilPixelFormat::Enum m_PixelFormat;
    IWICPalette *m_pPalette;

    // Currently active lock.

    CWGXBitmapLockState m_lockState;

protected:
    virtual ~CWGXBitmap();

    HRESULT HrCheckPixelRect(
        __in_ecount_opt(1) const WICRect *prcInput,
        __out_ecount(1) RECT *prcOutput);

    HRESULT HrLock(
        __in_ecount(1) const RECT &rcLock,
        __in MilPixelFormat::Enum pxlFormat,
        __in UINT cbStride,
        __in UINT cbBufferSize,
        __in_bcount(cbBufferSize) VOID *pvPixels,
        __in DWORD dwFlags,
        __deref_out_ecount(1) IWGXBitmapLock **ppILock,
        __in BOOL fLockOwnsPixelData = FALSE
        );

    HRESULT HrLockUnaligned(
        __in_ecount(1) const RECT &rcLock,
        MilPixelFormat::Enum pxlFormat,
        __in_range(1, 7) UINT nBitsStart,
        UINT nBitsTotal,
        UINT cbSrcStride,
        UINT cbBufferSize,
        __in_bcount(cbBufferSize) VOID *pvPixels,
        DWORD dwFlags,
        __deref_out_ecount(1) IWGXBitmapLock **ppILock
        );

    HRESULT HrInitDataFromBitmapSource(
        __in_ecount(1) IWGXBitmapSource *pISource,
        __in_ecount_opt(1) WICRect * srcRect = NULL);

private:

    HRESULT CopyPixelsHelper(
        __in MilPixelFormat::Enum pixelFormat,
        __in UINT width,
        __in UINT height,
        __in UINT cbInputStride,
        __in UINT cbInputBufferSize,
        __in_bcount(cbInputBufferSize) BYTE *pbInputBuffer,
        __in UINT cbOutputStride,
        __in UINT cbOutputBufferSize,
        __out_bcount(cbOutputBufferSize) BYTE *pbOutputBuffer
        );
};

// We can't use WIC's version of this because it hard codes TRUE for fCopySource
HRESULT CreateBitmapFromSourceRect(
    __in_ecount(1) IWGXBitmapSource *pISource,
    __in UINT x,
    __in UINT y,
    __in UINT width,
    __in UINT height,
    __in BOOL fCopySource,
    __deref_out_ecount(1) IWGXBitmap **ppBitmap
    );

/**************************************************************************
*
* Class Description:
*
*   CWGXBitmapLock
*
*   This class implements the IWGXBitmapLock class. This supports the
*   IWGXBitmap::Lock method.
*
**************************************************************************/

MtExtern(CWGXBitmapLock);

class CWGXBitmapLock :
    public CMILCOMBase,
    public IWGXBitmapLock
{

protected:
    
    // Is lock initialized?
    BOOL m_fValid;
    BOOL m_fOwnsPixelData;
    struct SurfaceData
    {
        UINT Width;
        UINT Height;
        UINT Stride;
        MilPixelFormat::Enum PixelFormat;
        void *Pixels;
        UINT BufferSize;
    } m_BitmapData;

    // Flags passed into Lock.
    DWORD m_dwFlags;

    // Reference to the bitmap we're locking on.
    CWGXBitmap *m_pIBitmap;

    // Internal method providing support for QueryInterface.
    STDMETHOD(HrFindInterface)(__in_ecount(1) REFIID riid, __deref_out void **ppv);

    void Unlock();

public:

    CWGXBitmapLock();
    virtual ~CWGXBitmapLock();
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CWGXBitmapLock));

    // IUnknown
    DECLARE_COM_BASE;

    // Implement the IWGXBitmapLock interface.
    STDMETHOD(GetSize)(
        __out_ecount(1) UINT *puiWidth,
        __out_ecount(1) UINT *puiHeight);
    STDMETHOD(GetStride)(
        __out_ecount(1) UINT *puiStride);
    STDMETHOD(GetDataPointer)(
        __out_ecount(1) UINT *pcbBufferSize,
        __deref_out_bcount(*pcbBufferSize) BYTE **ppvData);
    STDMETHOD(GetPixelFormat)(
        __out_ecount(1) MilPixelFormat::Enum *pPixelFormat);

    HRESULT Init(
        __in_ecount(1) CWGXBitmap *pIBitmap,
        __in UINT nWidth,
        __in UINT nHeight,
        __in MilPixelFormat::Enum pxlFormat,
        __in UINT cbStride,
        __in UINT cbBufferSize,
        __in_bcount(cbBufferSize) VOID *pvPixels,
        __in DWORD dwFlags,
        __in BOOL fOwnsPixelData = FALSE
        );

    HRESULT CheckValid();
    DWORD GetFlags();
};

/**************************************************************************
*
* Class Description:
*
*   CWGXBitmapLockUnaligned
*
*   This class implements the IWGXBitmapLock class. This supports the
*   IWGXBitmap::Lock method.
*   This class is specifically designed to handle the unaligned packed
*   pixel format cases (1bpp, 2bpp and 4bpp, either indexed or gray).
*
**************************************************************************/

MtExtern(CWGXBitmapLockUnaligned);

class CWGXBitmapLockUnaligned : public CWGXBitmapLock
{
protected:

    // Store the state pertaining to the original Lock location. This is used
    // on Unlock to propagate the changes back into the source data if the
    // bitmap was locked for write.

    // Pointer to the byte containing the top-left corner of the Lock rectangle
    // in the source data.

    void *m_pvOriginalPixels;
    unsigned int m_cbOriginalStride;
    unsigned int m_cbOriginalBufferSize;

    // Number of bits between the left and right edges.

    unsigned int m_nBitsTotal;

    // Bit offset of the left edge of the lock rectangle within the byte
    // pointed to by m_pvOriginalPixels.

    unsigned int m_nBitsStart;

    void UnlockUnaligned();

public:

    CWGXBitmapLockUnaligned();
    virtual ~CWGXBitmapLockUnaligned();
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CWGXBitmapLockUnaligned));

    HRESULT HrInit(
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
        );
};



