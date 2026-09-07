// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//      DeviceBitmap used by D3DImage 
//
//------------------------------------------------------------------------------

MtExtern(CInteropDeviceBitmap);

class CInteropDeviceBitmap : 
    public CDeviceBitmap,
    public IAdapterStatusListener
{
public:

    // fIsFrontBufferAvailable is a BOOL because the pointer will come from managed code
    // and bool has ambiguous size
    typedef void (*FrontBufferAvailableCallbackPtr)(
        BOOL fIsFrontBufferAvailable,
        UINT uVersion
        );
    
    override ~CInteropDeviceBitmap();
    
    static HRESULT Create(
        __in IUnknown *pIUserSurface,
        __in_range(0, DBL_MAX) double dpiX,
        __in_range(0, DBL_MAX) double dpiY,
        UINT uVersion,
        __in FrontBufferAvailableCallbackPtr pfnAvailable,
        bool isSoftwareFallbackEnabled,
        __deref_out CInteropDeviceBitmap **ppInteropDeviceBitmap
        );

    HRESULT AddUserDirtyRect(__in const CMilRectU &rc);

    HRESULT Present();

    void Detach();

    override void NotifyAdapterStatus(UINT uAdapter, bool fIsValid);

    override bool TryCreateDependentDeviceColorSource(
        __in const LUID &luidNewDevice,
        __in CHwBitmapCache *pNewCache
        );

    bool IsHwRenderingDisabled() const { CGuard<CCriticalSection> oGuard(m_cs); return m_fIsHwRenderingDisabled; }
    bool IsSoftwareFallbackEnabled() const { CGuard<CCriticalSection> oGuard(m_cs); return m_fIsSoftwareFallbackEnabled; }

    HRESULT GetAsSoftwareBitmap(__deref_out IWICBitmapSource **ppIBitmapSource);
    HRESULT GetSoftwareBitmapSource(__deref_out IWGXBitmapSource **ppBitmapSource);

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CInteropDeviceBitmap));

protected:
    enum FrontBufferUpdateMethod
    {
        // Front buffer's handle will be opened on the back buffer's device and StretchRect'd
        SharedSurface,
        // Back buffer will be BitBlt to front buffer 
        BitBlt,
        // Back buffer will be copied to front buffer through software
        Software
    };
    
    CInteropDeviceBitmap(
        UINT uVersion,
        __in FrontBufferAvailableCallbackPtr pfnAvailable,
        bool isSoftwareFallbackEnabled,
        __in_range(0, SURFACE_RECT_MAX) UINT uWidth,
        __in_range(0, SURFACE_RECT_MAX) UINT uHeight,
        MilPixelFormat::Enum fmtPixel,
        FrontBufferUpdateMethod oUpdateMethod,
        UINT uAdapter,
        __in IDirect3DSurface9 *pUserSurface
        );

private:
    
    static FrontBufferUpdateMethod GetUpdateMethod(
        __in IDirect3DDevice9 *pID3DDevice,
        __in_opt const IDirect3DDevice9Ex *pID3DDeviceEx,
        __in IDirect3DSurface9 *pID3DSurface
        );
    
    HRESULT UpdateFrontBuffer();

    HRESULT CreateFrontBuffer();
    HRESULT GetDisplayFromUserDevice(__deref_out const CDisplay **ppDisplay); 

    void NotifyAdapterStatusInternal(UINT uAdapter, bool fIsValid);

    HRESULT CopyToSoftwareBitmap(__deref_inout IWGXBitmap **ppBitmap);

    //
    // Critical section that is entered by all public APIs. Just about every bit of private data
    // below can be accessed from either thread.
    //
    mutable CCriticalSection m_cs;
    
    // Will be NULL after Detach() or NotifyAdapterStatus(m_uAdapter, false)
    IDirect3DSurface9 *m_pIUserSurface;

    FrontBufferAvailableCallbackPtr m_pfnAvailable;
    UINT m_uVersion;
    
    //
    // Unfortunately we need to keep a second set of dirty rects. This
    // set of dirty rects is used to copy from the user's surface to
    // our main color source each Present(). The IWGXBitmap dirty rects
    // are always accumulated but only used for copying our main color 
    // source to a different color source on another adapter. 
    //
    enum { c_maxBitmapDirtyListSize = 5 };
    CMilRectU m_rgUserDirtyRects[c_maxBitmapDirtyListSize];
    UINT m_cUserDirtyRects;  

    LUID m_luidDevice;
    UINT m_uAdapter;

    FrontBufferUpdateMethod m_oUpdateMethod;
    
    // A disabled CInteropDeviceBitmap has no front buffer. It will never
    // be enabled again unless software fallback is enabled.
    bool m_fIsHwRenderingDisabled;

    bool m_fIsSoftwareFallbackEnabled;

    // Software copy of the user's surface. NULL unless software fallback is enabled
    // and the front buffer is unavailable.
    IWGXBitmap *m_pISoftwareBitmap;
};


