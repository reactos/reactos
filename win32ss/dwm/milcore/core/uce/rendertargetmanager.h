// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Abstract:
//      Render target manager.
//
//------------------------------------------------------------------------------

MtExtern(CRenderTargetManager);

#define MAX_SLEEP_FOR_GPU_THROTTLE 1000

#define GPU_THROTTLE_CONSTANT_SLEEP_ERROR 5
#define GPU_THROTTLE_MULTIPLE_SLEEP_ERROR 8


typedef HRESULT (WINAPI *PFNDWMGETCOMPOSITIONTIMINGINFO)(
    HWND hwnd,
    __out_ecount(1) DWM_TIMING_INFO *pTimingInfo
    );

typedef HRESULT (WINAPI *PFNDWMPFLUSH)();    

typedef HRESULT (WINAPI *PFNDWMISCOMPOSITIONENABLED)(
    __out_ecount(1) BOOL *pfEnabled                                                 
    );
    

   
class CRenderTargetManager : public CMILRefCountBase
{
private:
    // Constructor
    CRenderTargetManager(
        __in_ecount(1) CComposition *pComposition
        );

    // Destructor
    virtual ~CRenderTargetManager();

protected:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CRenderTargetManager));

public:
    // CRenderTargetManager factory
    static HRESULT Create(
        __in_ecount(1) CComposition *pComposition,
        __out_ecount(1) CRenderTargetManager **ppRenderTargetManager
        );

    // Render the render targets managed by this object
    HRESULT Render(
        __out_ecount(1) bool *pfPresentNeeded
        );

    // Present the render targets managed by this object    
    HRESULT Present(
        __inout_ecount(1) UINT *puiRefreshRate,
        __out_ecount(1) MilPresentationResults::Enum *pePresentationResults,
        __inout_ecount(1) QPC_TIME *pqpcPresentationTime
        );

    // Wait until we are ready to present.
    HRESULT WaitToPresent(
        __inout_ecount(1) UINT *puiRefreshRate,
        __out_ecount(1) MilPresentationResults::Enum *pePresentationResults,
        __inout_ecount(1) QPC_TIME *pqpcPresentationTime
        );

    HRESULT WaitForGPU();

    void SleepForGPUThrottling(
        int iTimeToSleep
        );

    //
    // Add and remove targets from this device. Targets are typically window
    // or offscreen surfaces which are the target of a rendering tree
    // (nodes and render-data)
    //

    HRESULT AddRenderTarget(
        __in_ecount(1) CRenderTarget *pTarget
        );

    void RemoveRenderTarget(
        __in_ecount(1) CRenderTarget *pTarget
        );

    HRESULT EnableVBlankSync(
        __in CMilServerChannel* pChannel
        );

    void DisableVBlankSync(
        __in CMilServerChannel* pChannel
        );

    // Advances frame count on hwnds
    void AdvanceFrame();

    HRESULT GetNumQueuedPresents(
        __out_ecount(1) UINT *puNumQueuedPresents
        );

    HRESULT WaitForVBlank();

    // Releases the render targets managed by this object.
    void ReleaseTargets();

    HRESULT IsGPUThrottlingEnabled(
            __out_ecount(1) bool *pfEnabled
            );

    HRESULT NotifyDisplaySetChange(bool invalid, int displayCount);

    HRESULT UpdateRenderTargetFlags();

    HRESULT GetHardwareRenderInterface(__out_opt IRenderTargetInternal **ppIRT);

private:

    UINT GetSyncDisplayID(
        UINT cDisplays
        );

    HRESULT HandleRenderErrors(
        HRESULT hr
        );

    HRESULT HandlePresentErrors(
        HRESULT hr
        );

    HRESULT InitializeForDWM();

    HRESULT WaitForDwm(
        __inout_ecount(1) UINT *puiRefreshRate,
        __inout_ecount(1) MilPresentationResults::Enum *pePresentationResults,
        __inout_ecount(1) QPC_TIME *pqpcPresentationTime,
        __in              QPC_TIME qpcCurrentTime
        );

    HRESULT WaitForTarget(
        __inout_ecount(1) UINT *puiRefreshRate,
        __inout_ecount(1) MilPresentationResults::Enum *pePresentationResults,
        __inout_ecount(1) QPC_TIME *pqpcPresentationTime,
        __in              QPC_TIME qpcCurrentTime
        );

private:
    // The owning compositor. Note that we do not add-ref it 
    // to avoid cyclical dependencies.
    CComposition *m_pCompositionNoRef;

    // The list of render targets managed by this object.
    DynArray<CRenderTarget*, TRUE> m_rgpTarget;

    DynArray<CMilServerChannel*> m_rgpVBlankSyncChannels;


    // Information to get present under the DWM and get timing info back
    PFNDWMGETCOMPOSITIONTIMINGINFO m_pfnDwmGetCompositionTimingInfo;
    PFNDWMPFLUSH                   m_pfnDwmpFlush;
    
   
    LARGE_INTEGER m_qpcFrequency;
    bool m_fQPCSupported;    
    bool m_fInitDwm;

    int m_iRefreshRateLastFrame;

    UINT m_uFrameNumber;
    
#if DBG
    UINT m_uDbgNumMissedSleeps;
#endif
    bool m_fCompositionEnabled;

    UINT m_renderFailureCount;

    // Count of displays reported during the last 
    // display-set update.
    int m_lastKnownDisplayCount;

    static const UINT msc_maxRenderFailuresAllowed = 5;
};


