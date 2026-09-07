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

#include "precomp.hpp"

MtDefine(CRenderTargetManager, MILRender, "CRenderTargetManager");

//+-----------------------------------------------------------------------------
//
//    Member:
//        CRenderTargetManager constructor
//
//    Synopsis:
//        Stores a pointer to the owner composition object.
//
//------------------------------------------------------------------------------

CRenderTargetManager::CRenderTargetManager(
    __in_ecount(1) CComposition *pComposition
    )
    : m_pfnDwmGetCompositionTimingInfo(NULL)
    , m_pfnDwmpFlush(NULL)
    , m_fInitDwm(false)
    , m_fCompositionEnabled(false)
    , m_uFrameNumber(0)
    , m_renderFailureCount(0)
    , m_lastKnownDisplayCount(-1)
{
    // Note that this class is using DECLARE_METERHEAP_ALLOC.

    m_pCompositionNoRef = pComposition;

    if (QueryPerformanceFrequency(&m_qpcFrequency))
    {
        m_fQPCSupported = true;
    }

    m_iRefreshRateLastFrame = 0;

#if DBG
    m_uDbgNumMissedSleeps = 0;
#endif
}


//+-----------------------------------------------------------------------------
//
//    Member:
//        CRenderTargetManager destructor
//
//    Synopsis:
//        Releases the render targets referenced by this render target manager.
//
//------------------------------------------------------------------------------

CRenderTargetManager::~CRenderTargetManager()
{
    ReleaseTargets();
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CRenderTargetManager::Create
//
//    Synopsis:
//        Render target manager factory.
//
//------------------------------------------------------------------------------

/* static */ HRESULT 
CRenderTargetManager::Create(
    __in_ecount(1) CComposition *pComposition,
    __out_ecount(1) CRenderTargetManager **ppRenderTargetManager
    )
{
    HRESULT hr = S_OK;

    CRenderTargetManager *pRenderTargetManager = 
        new CRenderTargetManager(pComposition);

    IFCOOM(pRenderTargetManager);   

    SetInterface(*ppRenderTargetManager, pRenderTargetManager); // add a reference to render target manager
    pRenderTargetManager = NULL;

Cleanup:
    delete pRenderTargetManager;

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//    Member:
//        CRenderTargetManager::InitializeForDWM
//
//    Synopsis:
//        See if the DWM is running
//
//------------------------------------------------------------------------------

HRESULT
CRenderTargetManager::InitializeForDWM()
{
    HRESULT hr = S_OK;

    //
    // If this fails then we will not gracefully fallback.
    //

    if (SUCCEEDED(DWMAPI::Load()))
    {
        // Start by loading our imports
        m_pfnDwmGetCompositionTimingInfo = 
            reinterpret_cast<PFNDWMGETCOMPOSITIONTIMINGINFO>(DWMAPI::GetProcAddress(
                "DwmGetCompositionTimingInfo"
                ));

        m_pfnDwmpFlush = 
            reinterpret_cast<PFNDWMPFLUSH>(DWMAPI::GetProcAddress(
                "DwmFlush"
                ));

        if (   m_pfnDwmpFlush != NULL
            && m_pfnDwmGetCompositionTimingInfo != NULL)
        {
            BOOL    compositionEnabled = FALSE;
                
            //
            // Check to see whether composition is enabled
            // 
            IFC(DWMAPI::OSCheckedIsCompositionEnabled(&compositionEnabled));

            m_fCompositionEnabled = !!compositionEnabled;
        }
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CRenderTargetManager::ReleaseTargets
//
//    Synopsis:
//        Releases render targets managed by this object.
//
//------------------------------------------------------------------------------

void 
CRenderTargetManager::ReleaseTargets()
{
    Assert(m_pCompositionNoRef != NULL);

    for (UINT i = 0, limit = m_rgpTarget.GetCount(); i < limit; i++)
    {
        CRenderTarget *pTarget = m_rgpTarget[i];
        Assert(pTarget != NULL);

        pTarget->Release();
    }

    m_pCompositionNoRef->ProcessRenderingStatus(CComposition::RENDERING_STATUS_DEVICE_RELEASED);

    m_rgpTarget.Reset(TRUE);
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CRenderTargetManager::NotifyDisplaySetChange 
//
//    Synopsis:
//        Notifies all render targets that the display set has changed
//        Parameters:
//              invalid:      When true, indicates that the new display set 
//                            obtained after a recent mode-change is invalid. 
//              displayCount: Indicates the number of valid displays available
//                            in the new display set.
//
//------------------------------------------------------------------------------
HRESULT 
CRenderTargetManager::NotifyDisplaySetChange(bool invalid, int displayCount)
{
    HRESULT hr = S_OK;
    UINT count = m_rgpTarget.GetCount();

    for(UINT i = 0; i < count; ++i)
    {
        IFC(m_rgpTarget[i]->NotifyDisplaySetChange(invalid, m_lastKnownDisplayCount, displayCount));
    }

Cleanup:

    m_lastKnownDisplayCount = displayCount;
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CRenderTargetManager::UpdateRenderTargetFlags 
//
//    Synopsis:
//        Tells all render targets to update their flags
//
//------------------------------------------------------------------------------
HRESULT
CRenderTargetManager::UpdateRenderTargetFlags()
{
    HRESULT hr = S_OK;

    for (UINT i = 0, count = m_rgpTarget.GetCount(); i < count; ++i)
    {
        IFC(m_rgpTarget[i]->UpdateRenderTargetFlags());
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CRenderTargetManager::IsGPUThrottlingEnabled
//
//    Synopsis:
//        Returns true if GPU throttling should be used.  GPU throttling
//        should not be used:
//
//        * On WDDM drivers
//        * If our RegKey override is set
//
//------------------------------------------------------------------------------

HRESULT
CRenderTargetManager::IsGPUThrottlingEnabled(
    __out_ecount(1) bool *pfEnabled
    )
{
    HRESULT hr = S_OK;
    CMILFactory *pMilFactory = NULL;

    CDisplaySet const *pDisplaySet = NULL;
    *pfEnabled = false;

    //
    // Disable GPU throttling for WDDM drivers.  Busy waits in the driver
    // only occur on XPDM.
    //
    pMilFactory = m_pCompositionNoRef->GetMILFactory();

    //
    // Ask the factory for the display set, this is likely to be more in sync with the rest
    // of composition.
    // 
    MIL_THR(pMilFactory->GetCurrentDisplaySet(&pDisplaySet));

    //
    // WGXERR_DISPLAYSTATEINVALID is expected while we cannot create a display set
    // this should not result in zombieing this partition.
    // 
    if (hr == WGXERR_DISPLAYSTATEINVALID)
    {
        hr = S_OK;
    }

    IFC(hr);

    //
    // We only want to run the gpu throttling on non-WDDM drivers.
    // If we don't have a display set, we are patiently waiting for this
    // partition to be able to render, so, assume we can't use GPU throtting.
    //
    if (pDisplaySet == NULL || pDisplaySet->D3DExObject() != NULL)
    {
        Assert(!*pfEnabled);
        goto Cleanup;
    }

    //
    // Disable GPU throttling if our RegKey was set.
    //
    if (CCommonRegistryData::GPUThrottlingDisabled())
    {
        Assert(!*pfEnabled);
        goto Cleanup;
    }

    *pfEnabled = true;

Cleanup:

    ReleaseInterfaceNoNULL(pMilFactory);
    ReleaseInterfaceNoNULL(pDisplaySet);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CRenderTargetManager::Render
//
//    Synopsis:
//        Renders the render targets managed by this object.
//
//------------------------------------------------------------------------------
HRESULT
CRenderTargetManager::Render(
    __out_ecount(1) bool *pfPresentNeeded
    )
{
    HRESULT hr = S_OK;
    HRESULT hrRenderFailure = S_OK;
    
    for (UINT i = 0, limit = m_rgpTarget.GetCount(); i < limit; i++)
    {
        bool fPresentThisTarget = false;

        CRenderTarget *pTarget = m_rgpTarget[i];
        Assert(pTarget != NULL);

        MIL_THR(pTarget->Render(&fPresentThisTarget));

        HRESULT hrHandled = HandleRenderErrors(hr);

        if (SUCCEEDED(hrHandled) && FAILED(hr))
        {
            hrRenderFailure = hr;
        }

        IFC(hrHandled);

        *pfPresentNeeded |= fPresentThisTarget;
    }

    //
    // This is irrelevant if we didn't render, this also prevents a success
    // notification being sent in the case that there are no render targets.
    // This happens at tear-down.
    // 
    if (m_rgpTarget.GetCount() > 0)
    {
        if (FAILED(hrRenderFailure))
        {
            if (m_renderFailureCount < msc_maxRenderFailuresAllowed)
            {
                if (++m_renderFailureCount == msc_maxRenderFailuresAllowed)
                {
                    //
                    // We have hit our limit for render failures, signal the API factory
                    // that we have a render failure, also indicate what it is.
                    // 
                    IFC(m_pCompositionNoRef->NotifyRenderStatus(hrRenderFailure));
                }
            }
        }
        else
        {
            //
            // If we previously had a failure, clear it.
            // 
            if (m_renderFailureCount == msc_maxRenderFailuresAllowed)
            {
                IFC(m_pCompositionNoRef->NotifyRenderStatus(S_OK));
            }

            m_renderFailureCount = 0;
        }
    }

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------
//
//  Member:
//      CRenderTargetManager::HandleRenderErrors
//
//  Synopsis:
//      Factorization of error handling in Render method
//
//  Returns:
//      Error value to be returned by said functions
//
//------------------------------------------------------------------------

HRESULT
CRenderTargetManager::HandleRenderErrors(
    HRESULT hr
    )
{
    HRESULT hrReturned = hr;
    
    if (FAILED(hr))
    {
        //
        // The core rendering layer delays failures until present, so, we might succeed here
        // set the device state to normal. However, we do want to set the state correctly if
        // there is a failure.
        //
        m_pCompositionNoRef->ProcessRenderingStatus(m_pCompositionNoRef->RenderingStatusFromHr(hr));

        if (hr == D3DERR_OUTOFVIDEOMEMORY ||    // Allow out of video memory to bubble
            IsOOM(hr))                          // Allow out of memory to bubble
        {
            TraceTag((tagMILWarning, "CRenderTargetManager::Render: Encountered low memory condition."));
        }
        else
        {
            switch (hr)
            {
                case D3DERR_NOTAVAILABLE:
                    // Future Consideration:   Task 42738: keep the dwm and mil state in sync with the session state such that the dwm
                    // no longer attempts to render here when we are in a remote session.
                    TraceTag((tagMILWarning, "CRenderTargetManager::Render (intermediate): ignoring D3DERR_NOTAVAILABLE"));
                    hrReturned = S_OK;
                    break;
                    
                case WGXERR_DISPLAYSTATEINVALID:
                    // If rendering returns WGXERR_DISPLAYSTATEINVALID we can ignore it.  Present will also
                    // return WGXERR_DISPLAYSTATEINVALID and we'll handle it there.
                    hrReturned = S_OK;
                    break;

                default:
                    MilUnexpectedError(hr, TEXT("intermediate rendering error"));
            }
        }
    }

    RRETURN(hrReturned);
}


//+-----------------------------------------------------------------------
//
//  Member:
//      CRenderTargetManager::HandlePresentErrors
//
//  Synopsis:
//      Factorization of error handling in Present method
//
//  Returns:
//      Error value to be returned by said functions
//
//------------------------------------------------------------------------

HRESULT
CRenderTargetManager::HandlePresentErrors(
    HRESULT hr
    )
{
    HRESULT hrReturned = hr;
    
    m_pCompositionNoRef->ProcessRenderingStatus(m_pCompositionNoRef->RenderingStatusFromHr(hr));

    if (FAILED(hr))
    {
        if (hr == HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND))
        {
            //this is most likely a failure to present a layered window, safe to ignore.
            TraceTag((tagMILWarning, "CRenderTargetManager::Present: ignoring ERROR_PROC_NOT_FOUND..."));
        }
        else
        {
            if (hr == D3DERR_OUTOFVIDEOMEMORY ||    // Allow out of video memory to bubble
                IsOOM(hr))                          // Allow out of memory to bubble
            {
                TraceTag((tagMILWarning, "CRenderTargetManager::Present: Encountered low memory condition."));
            }
            else
            {
                switch (hr)
                {
                    case D3DERR_NOTAVAILABLE: // Ignoring!
                        //
                        //  Bug: 1237892
                        // Ignoring D3DERR_NOTAVAILABLE here is a mitigation
                        // for this bug
                        //

                        TraceTag((tagMILWarning, "CRenderTargetManager::Present: Ignoring D3DERR_NOTAVAILABLE"));
                        hrReturned = S_OK;
                        break;

                    case WGXERR_DISPLAYSTATEINVALID:
                        // If rendering returns WGXERR_DISPLAYSTATEINVALID we can ignore it.  Present will also
                        // return WGXERR_DISPLAYSTATEINVALID and we'll handle it there.
                        hrReturned = S_OK;
                        break;                        

                    default:
                        MilUnexpectedError(hr, TEXT("presentation error"));
                        break;
                }
            }
        }
    }

    RRETURN1(hrReturned, S_PRESENT_OCCLUDED);
}

//+-----------------------------------------------------------------------
//
//  Member: CRenderTargetManager::GetSyncDisplayID
//
//  Synopsis: Calculates which display the timing engine should sync to.
//            The current algorithm returns the index of whichever monitor
//            has the most total window area on it.
//
//  Returns:  The ID of the monitor that the timing engine should sync to
//
//------------------------------------------------------------------------

UINT CRenderTargetManager::GetSyncDisplayID(
    UINT cDisplays
    )
{
    UINT totalDispWindowArea = 0;           // current total window area for a particular display
    UINT largestDispWindowArea = 0;         // maintains the largest window area found so far for all displays
    UINT iSyncDisplay = 0;                  // index of the display to sync the UI thread on
    UINT cTargets = m_rgpTarget.GetCount();

    //
    // Calculate how much of each window intersects each monitor
    // The monitor with the most window area will be the
    // one the UI thread syncs to.  Other displays will jitter.
    //
    for (UINT iDisplay = 0; iDisplay < cDisplays; iDisplay++)
    {
        totalDispWindowArea = 0;
        for (UINT iTarget = 0; iTarget < cTargets; iTarget++)
        {
            if (m_rgpTarget[iTarget]->IsOfType(TYPE_HWNDRENDERTARGET))
            {
                CMILSurfaceRect rcIntersect;
                CSlaveHWndRenderTarget *pTarget = DYNCAST(CSlaveHWndRenderTarget, m_rgpTarget[iTarget]);
                Assert(pTarget);

                pTarget->GetIntersectionWithDisplay(iDisplay, rcIntersect);
                totalDispWindowArea += ((rcIntersect.right - rcIntersect.left)
                                    * (rcIntersect.bottom - rcIntersect.top));
            }

            if (totalDispWindowArea > largestDispWindowArea)
            {
                iSyncDisplay = iDisplay;
                largestDispWindowArea = totalDispWindowArea;
            }
        }

    }

    return iSyncDisplay;
}


//+-----------------------------------------------------------------------------
//
//    Member:
//        CRenderTargetManager::Present
//
//    Synopsis:
//        Presents the render targets managed by this object.
//
//------------------------------------------------------------------------------

HRESULT
CRenderTargetManager::Present(
    __inout_ecount(1) UINT *puiRefreshRate,
    __out_ecount(1) MilPresentationResults::Enum *pePresentationResults,
    __inout_ecount(1) QPC_TIME *pqpcPresentationTime
    )
{
    HRESULT hr = S_OK;

    bool fGPUThrottlingEnabled = false;

    //
    // We need to call this every time we present because we could
    // transition from a WDDM to non-WDDM driver through a TS session.
    //
    IFC(IsGPUThrottlingEnabled(&fGPUThrottlingEnabled));

    if (fGPUThrottlingEnabled)
    {
        IFC(WaitForGPU());
    }

    IFC(WaitToPresent(
        puiRefreshRate, 
        pePresentationResults, 
        pqpcPresentationTime
        ));

    //
    // Future Consideration:  Pass the correct refresh to WaitForGPU
    // 
    // The GPU throttling code needs to know the refresh rate that avalon
    // is rendering at.  Right now that information is obtained in 
    // WaitToPresent, but WaitForGPU must occur before it.  In the
    // interest of changing as little code as possible we're going to record
    // the refresh rate here, and WaitForGPU will use it on the next frame.
    // So the GPU throttling code will always use a refresh rate 1 frame 
    // behind.
    // 
    // This behavior should be fine, because:
    // 
    //   1.  We expect the refresh rate to rarely change.
    //   2.  We're not expecting it to change by much.
    //   3.  We don't think throttling 1 frame with the incorrect refresh rate
    //       is going to impact our visual quality.
    //
    m_iRefreshRateLastFrame = *puiRefreshRate;

    UINT cTargets = m_rgpTarget.GetCount();

    for (UINT i = 0; i < cTargets; i++)
    {
        CRenderTarget *pTarget = m_rgpTarget[i];
        Assert(pTarget);

        // Only HWND render targets can currently split
        // rendering and present
        if (pTarget->IsOfType(TYPE_HWNDRENDERTARGET))
        {
            MIL_THR(pTarget->Present());
            IFC(HandlePresentErrors(hr));
        }
    }

Cleanup:
    AdvanceFrame();
    
    RRETURN1(hr, S_PRESENT_OCCLUDED);
}


//+-----------------------------------------------------------------------------
//
//    Member:
//        CRenderTargetManager::WaitToPresent
//
//    Synopsis:
//        Waits until we are ready to present, either the DWM has flushed or we
//        have waited until the requested time
//
//------------------------------------------------------------------------------

HRESULT
CRenderTargetManager::WaitToPresent(
    __inout_ecount(1) UINT *puiRefreshRate,
    __out_ecount(1) MilPresentationResults::Enum *pePresentationResults,
    __inout_ecount(1) QPC_TIME *pqpcPresentationTime
    )
{
    HRESULT hr = S_OK;

    //
    // By default We don't support waiting for VSync. Simply notify the listeners
    // that we couldn't wait for VSync and let them choose what to do.
    // 
    *pePresentationResults = MilPresentationResults::VSyncUnsupported;
    *puiRefreshRate = 60;
    
    if (m_rgpVBlankSyncChannels.GetCount() > 0)
    {
        QPC_TIME qpcCurrentTime = 0;

        if (m_fQPCSupported)
        {
            QueryPerformanceCounter((LARGE_INTEGER*) &qpcCurrentTime);
        }


        // On first entry initialize the DWM event pulsing.
        if (!m_fInitDwm)
        {
            m_fInitDwm = true;
            if (DWMAPI::CheckOS())
            {
                IFC(InitializeForDWM());
            }
        }   

        if (m_fCompositionEnabled)
        {
            IFC(WaitForDwm(puiRefreshRate, pePresentationResults, pqpcPresentationTime, qpcCurrentTime));
        }
        else 
        {
            IFC(WaitForTarget(puiRefreshRate, pePresentationResults, pqpcPresentationTime, qpcCurrentTime));
        }
    }

Cleanup:

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CRenderTargetManager::WaitForGPU
//
//    Synopsis:
//        Uses GPUMarkers to try and detect the number of presents we have queued
//        up and inserts sleeps to try to keep the cpu from sending frames too
//        rapidly.
// 
//        This is done to avoid the cpu stalling which can occur if the GPU queue
//        fills up and makes us wait while it processes.
//
//------------------------------------------------------------------------------
HRESULT
CRenderTargetManager::WaitForGPU()
{
    HRESULT hr = S_OK;

    UINT uMaxSizePresentQueue = 0;

    Assert(m_iRefreshRateLastFrame >= 0);

    int iSleepThisFrame = 0;

    if (m_iRefreshRateLastFrame > 0)
    {
        //
        // If the present queue is getting full, it means the gpu is taking 
        // longer than 1 refresh to process each frame.  So sleeping for 1
        // frame will give the gpu a chance to catch up.
        // 
        // We're not too worried about the precision of the sleep here because
        // the important thing is we sleep for enough time to put us into the
        // next frame.
        //
        int iRefreshSleep = 1000/m_iRefreshRateLastFrame;

        IFC(GetNumQueuedPresents(
            &uMaxSizePresentQueue
            ));

        while (   uMaxSizePresentQueue > 2 
               && iSleepThisFrame < MAX_SLEEP_FOR_GPU_THROTTLE
                  )
        {    
            SleepForGPUThrottling(iRefreshSleep);
    
            iSleepThisFrame += iRefreshSleep;            
    
            IFC(GetNumQueuedPresents(
                &uMaxSizePresentQueue
                ));
        }
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CRenderTargetManager::SleepForGPUThrottling
//
//    Synopsis:
//        Sleeps for the specified amount of time, tracking how accurate the
//        the sleep was.  We've found under XP the sleep times are very accurate
//        unless the cpu is overloaded.
//
//------------------------------------------------------------------------------
void
CRenderTargetManager::SleepForGPUThrottling(
    int iTimeToSleep
    )
{
    if (iTimeToSleep > 0)
    {
        QPC_TIME qpcStartTime = 0;
        QPC_TIME qpcEndTime = 0;

        Assert(iTimeToSleep <= MAX_SLEEP_FOR_GPU_THROTTLE);

        if (m_fQPCSupported)
        {
            QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&qpcStartTime));
        }

        Sleep(iTimeToSleep);

        if (m_fQPCSupported)
        {
            QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&qpcEndTime));

            int iActualSleepTime = static_cast<int>((qpcEndTime - qpcStartTime)*1000/m_qpcFrequency.QuadPart);
            int iSleepDifference = abs(iActualSleepTime - iTimeToSleep);

            //
            // It's possible for sleep to be highly inaccurate.  From testing we've seen that it's
            // very consistent in XP while in milcore, but can become inaccurate if CPU load is too
            // high.
            // 
            // If we detect we're inaccurate print out a debug warning.
            //

            //
            // We want to catch errors both when desired sleep is small and large.
            // 
            // We always want the sleep time to be within SleepConstantError of desired.  Once sleep
            // gets large enough though we want to see if it is within 1/SleepMultipleError of desired.
            // 
            // Test for the point where SleepConstantError == sleepThrottle/SleepMultipleError
            // 
            // sleepThrottle == SleepConstantError*SleepMultipleError
            // 
            // If we're less than this value we want to test only with the constant, since the multiple
            // will be too small.  If we're greater than test only with the multiple since the constant
            // will be smaller.
            //
            if (iTimeToSleep < GPU_THROTTLE_CONSTANT_SLEEP_ERROR*GPU_THROTTLE_MULTIPLE_SLEEP_ERROR)
            {
                if (iSleepDifference >= GPU_THROTTLE_CONSTANT_SLEEP_ERROR)
                {
#if DBG
                    m_uDbgNumMissedSleeps++;
#endif
                    TraceTag((tagError, "Warning: GPU Throttling Sleep Expected: %d, Achieved: %d", iTimeToSleep, iActualSleepTime));
                }
            }
            else
            {
                if (iSleepDifference >= iTimeToSleep/GPU_THROTTLE_MULTIPLE_SLEEP_ERROR)
                {
#if DBG
                    m_uDbgNumMissedSleeps++;
#endif
                    TraceTag((tagError, "Warning: GPU Throttling Sleep Expected: %d, Achieved: %d", iTimeToSleep, iActualSleepTime));
                }
            }
        }
    }        
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CRenderTargetManager::AddRenderTarget
//
//    Synopsis:
//        Adds a render target to the registered render target list.
//
//------------------------------------------------------------------------------

HRESULT
CRenderTargetManager::AddRenderTarget(
    __in_ecount(1) CRenderTarget *pTarget
    )
{
    Assert(pTarget != NULL);

    Assert((pTarget->IsOfType(TYPE_HWNDRENDERTARGET)) ||
           (pTarget->IsOfType(TYPE_GENERICRENDERTARGET)));

    pTarget->AddRef();

    // Let the newly added RT know about the state
    // of our display-set.
    pTarget->
        PostDisplayAvailabilityMessage(m_lastKnownDisplayCount);

    RRETURN(m_rgpTarget.Add(pTarget));
}


//+-----------------------------------------------------------------------------
//
//    Member:
//        CRenderTargetManager::RemoveRenderTarget
//
//    Synopsis:
//        Removes a render target from the registered render target list.
//
//    Remark:
//        If the method is called with a render target that is not registered, 
//        the method won't do anything. This simplifies the implementation of 
//        cleanup code in failure cases.
//
//------------------------------------------------------------------------------

void 
CRenderTargetManager::RemoveRenderTarget(
    __in_ecount(1) CRenderTarget *pTarget
    )
{
    UINT i = m_rgpTarget.Find(0, pTarget);

    if (i < m_rgpTarget.GetCount())
    {
        pTarget->Release();

        m_rgpTarget.Remove(pTarget);

        if (m_rgpTarget.GetCount() == 0)
        {
            m_pCompositionNoRef->ProcessRenderingStatus(CComposition::RENDERING_STATUS_DEVICE_RELEASED);
        }
    }
}


/*++

CRenderTargetManager::EnableVBlankSync

Description:
  Enables locking present calls to the vertical blank.

Returns:
  S_OK if succeeded, a failure HRESULT otherwise.

--*/

HRESULT CRenderTargetManager::EnableVBlankSync(
    __in CMilServerChannel* pChannel
    )
{
    HRESULT hr = S_OK;

    // Locate the channel in our table of listeners
    UINT channelIndex = 0;
    UINT nListenerCount = m_rgpVBlankSyncChannels.GetCount();

    while ((channelIndex < nListenerCount)
            && (m_rgpVBlankSyncChannels[channelIndex] != pChannel))
    {
        channelIndex++;
    }

    if (channelIndex == nListenerCount)
    {
        //
        // It's not already in the list. If this is the first channel
        // to ask then determine whether we can enter the mode or not
        //

        if (nListenerCount == 0)
        {
            if (timeBeginPeriod(1) != TIMERR_NOERROR)
            {
                IFC(E_FAIL);
            }
        }

        hr = m_rgpVBlankSyncChannels.InsertAt(pChannel, channelIndex);
        if (FAILED(hr))
        {
            timeEndPeriod(1);
            IFC(hr);
        }
    }

Cleanup:
    //
    // Marshal the result back to the channel
    //

    MIL_MESSAGE message = { MilMessageClass::SyncModeStatus };
    message.syncModeStatusData.hrEnabled = hr;
    MIL_THR_SECONDARY(pChannel->PostMessageToChannel(&message));

    RRETURN(hr);
}


/*++

CRenderTargetManager::DisableVBlankSync

Description:
  Enables locking present calls to the vertical blank.

Returns:
  S_OK if succeeded, a failure HRESULT otherwise.

--*/

void CRenderTargetManager::DisableVBlankSync(
    __in CMilServerChannel* pChannel
    )
{
    //
    // The channel does not want to get notified. Remove it from
    // the list, if it's there. If this was the last channel then
    // also leave sync mode
    //

    if (m_rgpVBlankSyncChannels.Remove(pChannel) && (m_rgpVBlankSyncChannels.GetCount() == 0))
    {
        timeEndPeriod(1);
    }
}


//+-----------------------------------------------------------------------
//
//  Member: CRenderTargetManager::WaitForVBlank
//
//  Synopsis:  Waits for VBlank to occur on the device used by the first HW render target
//
//  Returns: S_OK on success
//           WGXERR_NO_HARDWARE_DEVICE if there are no HWND targets
//
//------------------------------------------------------------------------

HRESULT 
CRenderTargetManager::WaitForVBlank()
{
    BEGIN_MILINSTRUMENTATION_HRESULT_LIST_WITH_DEFAULTS
        WGXERR_NO_HARDWARE_DEVICE
    END_MILINSTRUMENTATION_HRESULT_LIST

    HRESULT hr = WGXERR_NO_HARDWARE_DEVICE;

    if (m_pCompositionNoRef->GetCompositionDeviceState() != MilCompositionDeviceState::Occluded)
    {
        // Wait for VBlank on the first HWND render target we find
        // The DWM should be the only user of this API so there should only
        // be one HWND render target
        UINT count = m_rgpTarget.GetCount();
        if (count > 0)
        {
            UINT i = 0;
            while (i < count && !m_rgpTarget[i]->IsOfType(TYPE_HWNDRENDERTARGET))
            {
                i++;
            }
            if (i < count)
            {
                CSlaveHWndRenderTarget *pTarget = DYNCAST(CSlaveHWndRenderTarget, m_rgpTarget[i]);
                Assert(pTarget);

                hr = pTarget->WaitForVBlank();
                if (FAILED(hr) && (hr != WGXERR_NO_HARDWARE_DEVICE))
                {
                    MIL_CHECKHR(hr);
                }
            }
        }
    }
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//    Member:  
//        CRenderTargetManager::AdvanceFrame
//
//    Synopsis:  
//        Sends frame number to HWND render targets and increments the
//        frame number.
//
//-------------------------------------------------------------------------

void
CRenderTargetManager::AdvanceFrame()
{
    ++m_uFrameNumber;
    UINT count = m_rgpTarget.GetCount();
    for (UINT i = 0; i < count; i++)
    {
        if (m_rgpTarget[i]->IsOfType(TYPE_HWNDRENDERTARGET))
        {
            CSlaveHWndRenderTarget *pTarget = DYNCAST(CSlaveHWndRenderTarget, m_rgpTarget[i]);
            Assert(pTarget);

            pTarget->AdvanceFrame(m_uFrameNumber);
        }
    }
}

//+------------------------------------------------------------------------
//
//    Member:  
//        CRenderTargetManager::InsertGPUMarker
//
//    Synopsis:  
//        Inserts a marker into the GPU command stream
//
//-------------------------------------------------------------------------

HRESULT
CRenderTargetManager::GetNumQueuedPresents(
    __out_ecount(1) UINT *puNumQueuedPresents
    )
{
    HRESULT hr = S_OK;

    // Add a Marker to every hwnd and surface target
    UINT count = m_rgpTarget.GetCount();
    UINT uMaxQueuedPresents = 0;

    if (count > 0)
    {
        for (UINT i = 0;(i < count); i++)
        {
            if (m_rgpTarget[i]->IsOfType(TYPE_HWNDRENDERTARGET))
            {
                UINT uNumQueuedPresents = 0;
                CSlaveHWndRenderTarget *pTarget = DYNCAST(CSlaveHWndRenderTarget, m_rgpTarget[i]);
                Assert(pTarget);

                IFC(pTarget->GetNumQueuedPresents(&uNumQueuedPresents));

                uMaxQueuedPresents = max(uMaxQueuedPresents, uNumQueuedPresents);
            }
        }
    }

    *puNumQueuedPresents = uMaxQueuedPresents;

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:  CRenderTargetManager::WaitForDwm
//
//  Synopsis:  Waits for the dwm to flush and returns the dwm's composition
//             timing info.
//
//------------------------------------------------------------------------
HRESULT 
CRenderTargetManager::WaitForDwm(
    __inout_ecount(1) UINT *puiRefreshRate,
    __inout_ecount(1) MilPresentationResults::Enum *pePresentationResults,
    __inout_ecount(1) QPC_TIME *pqpcPresentationTime,
    __in              QPC_TIME qpcCurrentTime
    )
{
    HRESULT hr = S_OK;

    //
    // If we are asked to present in the future, then wait until VBlank
    //
    if (!m_fQPCSupported || qpcCurrentTime < *pqpcPresentationTime)
    {
        //
        // Wait until VBlank and until all of our current Dx updates are complete.
        // 
        MIL_THR(m_pfnDwmpFlush());

        //
        // Flushes can time-out, this is principally intended to 
        // handle network loss, but, it can also happen during 
        // stress (like firing off 12 instances of XamlPad 
        // simultaneously). We don't want to zombie the partition
        // over this, so, swallow the error.
        // 
        // We could also get a composition disabled call, or, if 
        // the DWM goes down during the call, we could get channel
        // sync abandoned.
        // 
        if (   hr == HRESULT_FROM_WIN32(ERROR_TIMEOUT)
            || hr == DWM_E_COMPOSITIONDISABLED
            || hr == WGXERR_UCE_CHANNELSYNCABANDONED)
        {
            hr = S_OK;
        }
        else 
        {
            IFC(hr);
        }
    }

    ASSERT(m_pfnDwmGetCompositionTimingInfo != NULL);

    {
        DWM_TIMING_INFO info = {0};
        info.cbSize = sizeof(DWM_TIMING_INFO);

        hr = m_pfnDwmGetCompositionTimingInfo(NULL, &info);

        if (FAILED(hr))
        {
            // We should not have changed our result to VSync not supported
            ASSERT(*pePresentationResults == MilPresentationResults::VSyncUnsupported);
            hr = S_OK;
        }
        else
        {
            *pePresentationResults = MilPresentationResults::Dwm;
            *puiRefreshRate = info.rateCompose.uiNumerator / info.rateCompose.uiDenominator;
            *pqpcPresentationTime = info.qpcVBlank;
        }
    }

Cleanup:

    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:  CRenderTargetManager::WaitForTarget
//
//  Synopsis: Waits for a particular target in the set of displays (the sync 
//            display id).
//     
//------------------------------------------------------------------------
HRESULT 
CRenderTargetManager::WaitForTarget(
    __inout_ecount(1) UINT *puiRefreshRate,
    __inout_ecount(1) MilPresentationResults::Enum *pePresentationResults,
    __inout_ecount(1) QPC_TIME *pqpcPresentationTime,
    __in              QPC_TIME qpcCurrentTime
    )
{
    HRESULT hr = S_OK;
    CDisplay const *pDisplay = NULL;
    CDisplaySet const *pDisplaySet = NULL;
    CMILFactory *pMilFactory = NULL;
    UINT cTargets = m_rgpTarget.GetCount();
    UINT cDisplays = 0;    

    if (cTargets > 0)
    {
        //
        // If we are only local, get the display that the window covers the
        // most, this will be the display we sync to. Otherwise we don't
        // support vertical sync.
        //
        pMilFactory = m_pCompositionNoRef->GetMILFactory();

        MIL_THR(pMilFactory->GetCurrentDisplaySet(&pDisplaySet));

        //
        // It is quite normal to not get a display set for a while, this is 
        // handled by sending a SW tier change notification and invalidating
        // our render targets.
        // 
        if (hr == WGXERR_DISPLAYSTATEINVALID)
        {
            hr = S_OK;
        }

        //
        // All other cases an error is a hard fail.
        // 
        IFC(hr);

        if (   pDisplaySet != NULL
            && (cDisplays = pDisplaySet->GetDisplayCount()) > 0
            && !pDisplaySet->IsNonLocalDisplayPresent())
        {
            UINT iSyncDisplay = GetSyncDisplayID(cDisplays);

            IFC(pDisplaySet->GetDisplay(iSyncDisplay, &pDisplay));

            if (m_fQPCSupported)
            {
                *puiRefreshRate = static_cast<UINT>(1000.0/(1000.0/pDisplay->GetRefreshRate()));
                *pePresentationResults = MilPresentationResults::VSync;

                if (qpcCurrentTime < *pqpcPresentationTime)
                {
                    DWORD dwTimeout = (DWORD)((*pqpcPresentationTime - qpcCurrentTime) * 1000 / m_qpcFrequency.QuadPart);
                    // Sleep until we hit the desired presentation time or timeout at 30ms.
                    Sleep(min(dwTimeout, DWORD(30)));
                }

                if (*pqpcPresentationTime == 0)
                {
                    QueryPerformanceCounter((LARGE_INTEGER*) pqpcPresentationTime);
                }
            }               
        }
        else
        {
            ASSERT(*pePresentationResults == MilPresentationResults::VSyncUnsupported && hr == S_OK);
        }
    }

Cleanup:

    ReleaseInterfaceNoNULL(pMilFactory);
    ReleaseInterfaceNoNULL(pDisplay);
    ReleaseInterfaceNoNULL(pDisplaySet);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:  CRenderTargetManager::GetHardwareRenderInterface
//
//  Synopsis: Returns the first hardware render target interface from
//            an HWnd target.
//     
//------------------------------------------------------------------------
HRESULT
CRenderTargetManager::GetHardwareRenderInterface(
    __out_opt IRenderTargetInternal **ppIRT
    )
{
    HRESULT hr = S_OK;

    IRenderTargetInternal *pIRT = NULL;
    
    UINT cTargets = m_rgpTarget.GetCount();

    for (UINT i = 0; i < cTargets; i++)
    {
        CRenderTarget *pTarget = m_rgpTarget[i];
        if (pTarget->IsOfType(TYPE_HWNDRENDERTARGET))
        {
            IFC(pTarget->GetBaseRenderTargetInternal(&pIRT));

            if (pIRT != NULL)
            {
                DWORD renderTargetType;
                IFC(pIRT->GetType(&renderTargetType));
                if (renderTargetType == HWRasterRenderTarget)
                {
                    break;
                }

                // Release the ref and null out pIRT.
                ReleaseInterface(pIRT);
            }
        }
    }

    // Pass reference out (if we found an acceptable RT).
    *ppIRT = pIRT;
    pIRT = NULL;

Cleanup:
    if (FAILED(hr))
    {
        ReleaseInterface(pIRT);
    }
    
    RRETURN(hr);
}

