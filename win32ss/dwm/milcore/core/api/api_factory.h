// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:
//      MILCore factory. Contains factory methods accessible to product code
//
//

#ifndef __API_FACTORY_H__
#define __API_FACTORY_H__

MtExtern(CMILFactory);
MtExtern(CMILFactoryObjectEntry);


/*=========================================================================*\
    CMILFactory - Top-level MIL Factory object
\*=========================================================================*/

class CMILFactory :
    public IMILCoreFactory,
    public CMILCOMBase
{
public:

    static
    HRESULT
    Create(
        __deref_out_ecount(1) CMILFactory **ppCMILFactory
        );

    DECLARE_COM_BASE;
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMILFactory));

    __success(true) STDMETHOD(UpdateDisplayState)(
        __out_ecount(1) bool *pDisplayStateChanged, 
        __out_ecount(1) int *pDisplayCount
        ) override;

    // Query graphics accleration capabilities
    STDMETHOD_(void, QueryCurrentGraphicsAccelerationCaps)(
        bool fReturnCommonMinimum,
        __out_ecount(1) ULONG *pulDisplayUniqueness,
        __out_ecount(1) MilGraphicsAccelerationCaps *pCaps
        );

    // Bitmap Render Target
    STDMETHOD(CreateBitmapRenderTarget)(
        UINT width,
        UINT height,
        MilPixelFormat::Enum format,
        FLOAT dpiX,
        FLOAT dpiY,
        MilRTInitialization::Flags dwFlags,
        __deref_out_ecount(1)IMILRenderTargetBitmap **ppIRenderTargetBitmap
        );

    // Audio/Video
    STDMETHOD(CreateMediaPlayer)(
        __inout_ecount(1) IUnknown *pEventProxy,
        bool canOpenAnyMedia,
        __deref_out_ecount(1) IMILMedia **ppMedia
        );

    // Render Target for a client supplied Bitmap
    STDMETHOD(CreateSWRenderTargetForBitmap)(
        __inout_ecount(1) IWICBitmap *pIBitmap,
        __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTargetBitmap
        );

private:

    inline bool IsDisplaySetInitialized() const
    {
        return m_pDisplaySet != nullptr;
    }

public:
    //
    // Normal public methods
    //

    static HRESULT
    ComputeRenderTargetTypeAndPresentTechnique(
        __in_opt HWND hwnd,
        MilWindowProperties::Flags flWindowProperties,
        MilWindowLayerType::Enum eWindowLayerType,
        MilRTInitialization::Flags InFlags,
        __out_ecount(1) MilRTInitialization::Flags * const pOutFlags
        );

    HRESULT
    CreateDesktopRenderTarget(
        __in_opt HWND hwnd,
        MilWindowLayerType::Enum eWindowLayerType,
        MilRTInitialization::Flags dwFlags,
        __deref_out_ecount(1) IMILRenderTargetHWND **ppIRenderTarget
        );

    HRESULT
    GetCurrentDisplaySet(
        const CDisplaySet **ppDisplaySet
        );

protected:

    CMILFactory();
    virtual ~CMILFactory();

    HRESULT
    Init(
        void
        );

    /* QI support method */
    STDMETHOD(HrFindInterface)(__in_ecount(1) REFIID riid, __deref_out void **ppvObject);

private:

    const CDisplaySet               *m_pDisplaySet;
    CCriticalSection                m_lock;

    //
    // This stores the result of the last attempt to create a display set, this
    // is to store the hresult of the last failure (for debugging purposes) and
    // also to ensure that we only send a SW tier notification if the display set
    // changes once.
    //
    HRESULT                         m_hrLastDisplaySetUpdate;
};

#endif // !__API_FACTORY_H__


