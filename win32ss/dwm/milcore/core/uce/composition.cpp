// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//------------------------------------------------------------------------------

#include "precomp.hpp"

#if PRERELEASE
//
// One of the requirements for the composition engine is to be able to accept
// as input command streams originating in hostile environments. In order to
// facilitate security testing, we've implemented a high-level fuzzer that
// operates on recorded command streams (see ENABLE_FUZZING) and a low-level
// fuzzer that randomly flips bits in the input -- thus allowing it to be
// run online (see ENABLE_INLING_FUZZING). The expectation is that the code
// will be able to analyze and correctly handle invalid data. Note that 
// enabling the fuzzing actually turns off parts of the defense-in-depth
// code to ensure that the underlying code gets appropriate coverage.
//
#define ENABLE_INLINE_FUZZING 0
#define ENABLE_FUZZING 0
#endif

MtDefine(CComposition, MILRender, "CComposition");

ExternTag(tagMILResources);

UTC_TIME CComposition::s_frameLastComposed = 0;


//+-----------------------------------------------------------------------------
// 
//  Member:
//      CComposition constructor
// 
//------------------------------------------------------------------------------       

CComposition::CComposition(__in MilMarshalType::Enum marshalType) 
      : m_mType(marshalType),
        m_deviceState(MilCompositionDeviceState::NoDevice),
        m_fLastForceSoftwareForProcessValue(false)
{
    // Check if the hardware supports high-resolution performance counters.
    m_fQPCSupported = QueryPerformanceFrequency(&m_qpcFrequency);

    // Use ReleaseInterface to destroy this object
    AddRef();
}


//+-----------------------------------------------------------------------------
// 
//  Member:
//      CComposition destructor
// 
//------------------------------------------------------------------------------       

CComposition::~CComposition()
{    
    m_rgpEtwEvent.Reset(FALSE);
    m_rgpFlushChannels.Reset(FALSE);


    //
    // Release the table of the channels attached to this composition.
    //
    // Note that at this time nobody should be holding a reference to
    // this object and taking the attached channels critical section
    // is not necessary.
    //

    for (UINT i = 0, limit = m_rgpAttachedChannels.GetCount(); i < limit; i++)
    {
        CMilServerChannel *pChannel = m_rgpAttachedChannels[i];

        ReleaseInterface(pChannel);
    }

    m_rgpAttachedChannels.Reset(TRUE);

    ReleaseNotificationChannels();

    // Release the external components of the composition engine
    ReleaseInterface(m_pFactory);
    ReleaseInterface(m_pRenderTargetManager);
    ReleaseInterface(m_pVisualCacheManager);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CComposition::Initialize
// 
//  Synopsis:
//      Initializes this instance of the CComposition class.
// 
//------------------------------------------------------------------------------

HRESULT 
CComposition::Initialize()
{
    HRESULT hr = S_OK;

    // Create the MIL factory.
    CMILFactory *pFactory = NULL;
    CRenderTargetManager *pRenderTargetManager = NULL;

    IFC(CMILFactory::Create(&pFactory));

    // Create the render target manager.
    IFC(CRenderTargetManager::Create(
            this,
            &pRenderTargetManager
            ));

    // Create the cache manager.
    IFC(CVisualCacheManager::Create(
            this,
            pFactory,
            &m_pVisualCacheManager
            ));

    // Create the glyph cache
    IFC(CMilSlaveGlyphCache::Create(this, &m_pGlyphCache));

    // Now that initialization succeeded, store the MIL factory reference.
    SetInterface(m_pFactory, pFactory);
    SetInterface(m_pRenderTargetManager, pRenderTargetManager);
    
    m_pCurrentResourceNoRef = NULL;

Cleanup:    
    ReleaseInterfaceNoNULL(pFactory); // MIL factory was created with reference count equal to one
    ReleaseInterfaceNoNULL(pRenderTargetManager); // MIL RTManager was created with reference count equal to one

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CComposition::ProcessPartitionCommand
// 
//  Synopsis:
//      Processes a partition command.
// 
//-----------------------------------------------------------------------------

HRESULT 
CComposition::ProcessPartitionCommand(
    __in_ecount(1) CMilCommandBatch* pBatch,
    __in bool fProcessBatchCommands
    )
{
    HRESULT hr = S_OK;

    switch (pBatch->m_commandType)
    {
    case PartitionCommandBatch:
        {
            //
            // Now process the commands in the given batch.
            //

            AssertConstMsg(!(IsZombie() && fProcessBatchCommands), "Should never attempt to process command batches in zombie state.");

            if (fProcessBatchCommands)
            {
                IFC(ProcessCommandBatch(pBatch));
            }
            else
            {
                pBatch->SetChannelPtr(NULL);

                //
                // The command batches are allocated with a custom allocator
                // and need to be freed with the same allocator.
                //
                delete pBatch;
            }
        }
        break;

    case PartitionCommandOpenChannel:
        {
            IFC(AttachChannel(
                pBatch->GetChannel(),
                pBatch->GetChannelPtr()));

            pBatch->SetChannelPtr(NULL);

            delete pBatch;
        }
        break;

    case PartitionCommandCloseChannel:
        {
            IFC(DetachChannel(
                pBatch->GetChannel()));

            pBatch->SetChannelPtr(NULL);

            delete pBatch;
        }
        break;
    }

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:
//      CComposition::ProcessCommandBatch
//
//  Synopsis:
//      Executes a batch of MIL commands.
//
//-----------------------------------------------------------------------------

HRESULT 
CComposition::ProcessCommandBatch(
    __in_ecount(1) CMilCommandBatch* pBatch
    )
{
    HRESULT hr = S_OK;

    MILCMD nCmdType = MilCmdInvalid;
    LPCVOID pcvData = NULL;
    UINT cbSize = 0;

#if ENABLE_INLINE_FUZZING && PRERELEASE && !DBG
    static UINT c_FuzzThreshold = 0;
#endif

    //
    // Retrieve the channel the batch was sent to. Get the channel's
    // handle table and associate it with the current compositor.
    //

    CMilServerChannel *pChannel = pBatch->GetChannelPtr();
    Assert(pChannel != NULL);

    CMilSlaveHandleTable *pHandleTable = pChannel->GetChannelTable();
    Assert(pHandleTable != NULL);

    pHandleTable->SetComposition(this);

    //
    // Trace the execution of this method.
    //

    if (ETW_ENABLED_CHECK(TRACE_LEVEL_VERBOSE))
    {
        UINT64 data = pBatch->GetTotalWrittenByteCount();
        EventWriteWClientUceProcessQueueInfo(data);
    }

    //
    // Attach the batch buffer to the reader.
    //

    CMilDataBlockReader DataDecoder(pBatch->FlushData());

    //
    // Retrieve the first command.
    // We can't assume that the data is trustable so use the Safe methods
    //

    MIL_THR(DataDecoder.GetFirstItemSafe(
        (UINT*)&nCmdType,
        const_cast<void**>(&pcvData),
        &cbSize
        ));

    //
    // Check the batch record alignment. The record structures are compiled
    // with pack(1) because the format is what we use to communicate across
    // architectures in the TS case. Pack(1) means that the compiler can
    // figure out which fields in the struct are UNALIGNED (IA64), however this
    // requires the struct to start on an aligned boundary. Force this here.
    //

    Assert((cbSize & 0x3) == 0);

    while (SUCCEEDED(hr))
    {
        //
        // Check whether we reached the end of the stream.
        //

        if (hr == S_FALSE)
        {
            hr = S_OK;
            break;
        }

#if ENABLE_INLINE_FUZZING && PRERELEASE && !DBG

        //
        // The purpose of the inline fuzzer is to test the composition code
        // for resiliency to invalid input coming over the wire. 
        //
        // Although the recording-session based fuzzer provides better repro
        // steps, the inline fuzzer is useful to test DWM, magnifier, TS
        // and other hard-to-record scenarios.
        //
        // In order to enable the inline fuzzer:
        //
        //  1. Set the ENABLE_INLINE_FUZZING to 1 (see the top of this file).
        //  2. Adjust the c_FuzzThreshold constant.
        //  3. Rebuild mil/core under x86fre.
        //  4. Use the resulting milcore.dll as you would normally do, 
        //     looking for AVs resulting from the invalid data. 
        //
        // Note that every now and then you'll break your application either
        // visually or structurally (failure to negotiate transport version,
        // etc.). Adjust the fuzz threshold constant and/or add some more
        // exceptions below (by default we fuzz everything except for the
        // MilCmdTransportSyncFlush command which is too difficult to work
        // around, see the recording session fuzzer code below).
        //

        {
            static bool s_fCheckInlineFuzzingRegKey = true;
            static bool s_fRandomSeeded = false;
            static UINT s_cCommands = 0;
            static UINT s_cFlippedBits = 0;
            
            if (s_fCheckInlineFuzzingRegKey)
            {
                s_fCheckInlineFuzzingRegKey = false;
                TCHAR regValue[MAX_PATH];

                if (RegGetHKLMString(
                    TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"),
                    TEXT("MilCoreInlineFuzzing"),
                    regValue,
                    sizeof(regValue)))
                {
                    c_FuzzThreshold = _tstoi(regValue);
                }
            }


            if (c_FuzzThreshold > 0 && nCmdType != MilCmdTransportSyncFlush)
            {
                if (!s_fRandomSeeded)
                {
                    srand((unsigned)time(NULL));
                    s_fRandomSeeded = true;
                }

                ++s_cCommands;

                while ((rand() % c_FuzzThreshold) == 0)
                {
                    LPBYTE pbData = reinterpret_cast<LPBYTE>(const_cast<LPVOID>(pcvData));
                    
                    pbData[rand() % cbSize] ^= (1 << (rand() % 8));
                    
                    ++s_cFlippedBits;
                }
            }
        }

#endif /* ENABLE_INLINE_FUZZING && PRERELEASE && !DBG */

        //
        // Process the current message. The packet routing switch instruction
        // is automatically generated. It accepts the following parameters:
        //
        //      nCmdType        -- the command type
        //      pcvData         -- pointer to the beginning of the command
        //      cbSize          -- size of the command (including payload)
        //      pChannel        -- the channel this batch was sent to
        //      pHandleTable    -- the channel's handle table
        //

        //
        // Ignore false-positive PreFast warning about possible infinite loop when using
        // IFC macro inside this .inl file - PreFast can't seem to parse this correctly.
        #pragma warning (push)
        #pragma warning (disable:25062)

        #include "generated_process_message.inl"

        #pragma warning (pop)

        // Watchdog for bugs
        CFloatFPU::AssertPrecisionAndRoundingMode();            


        //
        // Retrieve the next command if there is one.
        //

        MIL_THR(DataDecoder.GetNextItemSafe(
            (UINT*)&nCmdType,
            const_cast<void**>(&pcvData),
            &cbSize
            ));
    }


Cleanup:

#if ENABLE_FUZZING && PRERELEASE && !DBG
    //
    // In order to enable fuzzing:
    //
    //  1. Set the ENABLE_FUZZING definition to 1 (see the top of this file).
    //  2. Rebuild mil/core under x86fre.
    //  3. Build in mil/devtest/tools.
    //  4. Run 'batchfuzz.exe' for further instructions.
    //

    if (FAILED(hr))
    {
        //
        // Make sure that the record packet player will not be blocked
        // by an unprocessed sync flush command... Note that in case of
        // animation smoothing, we will signal sync flush anyway, so skip
        // in that case.
        //

        bool needToSignalSyncFlush =
            m_rgpFlushChannels.Find(0, pChannel) == m_rgpFlushChannels.GetCount();

        if (needToSignalSyncFlush)
        {
            MIL_THR(m_rgpFlushChannels.Add(pChannel));

            if (SUCCEEDED(hr))
            {
                pChannel->AddRef();
            }
            else
            {
                MilUnexpectedError(hr, TEXT("failed to override flush channel action for fuzzing"));
            }
        }


        //
        // Do not break on detected and properly handled errors -- we
        // are only interested in access violations.
        //

        hr = S_OK;
    }
#endif /* ENABLE_FUZZING && PRERELEASE && !DBG */

#if ENABLE_INLINE_FUZZING && PRERELEASE && !DBG
    if (c_FuzzThreshold > 0)
    {
        hr = S_OK;
    }
#endif

    //
    // Conditionally (see MilUnexpectedError implementation) break into the
    // debugger to preserve debugability of batch processing.
    //

    if (FAILED(hr))
    {
        if (hr != D3DERR_NOTAVAILABLE)
        {
            MilUnexpectedError(hr, TEXT("batch processing error"));
        }
        else
        {
            //
            //  Bug: 1237892
            // Ignoring D3DERR_NOTAVAILABLE here is a mitigation for this bug.
            //

            TraceTag((tagMILWarning, "CComposition::ProcessCommandBatch: ignoring D3DERR_NOTAVAILABLE"));

            hr = S_OK;
        }
    }


    //
    // No matter what free the batch and assign it to the lookaside.
    //

    pBatch->SetChannelPtr(NULL);
    delete pBatch;

    //
    // ERROR HANDLING NOTE: any failure error code returned from this
    // method will result in putting the current partition into zombie
    // state. Partition manager will attempt to notify the server
    // (or, the UI thread) of the failure.
    //

    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:
//      CComposition::BeginProcessVideo
//
//  Synopsis:
//      Checks if video content is ready to be updated and notifies
//      the registered video resources.
//
//-----------------------------------------------------------------------------

HRESULT 
CComposition::BeginProcessVideo(
    __in_ecount(1) bool displaySetChanged
    )
{
    HRESULT hr = S_OK;

    //
    // Check if the VideoReady event is set. Report Win32 errors
    // if WaitForSingleObject call returns WAIT_FAILED.
    //
    // investigate making the videos animated resources instead (just like flipchains)

    for (UINT i = 0; i < m_rgpVideo.GetCount(); i++)
    {
        bool bNewFrameReady = false;

        //
        // Also tell the video that we are beginning a composition pass.
        //
        IFC(m_rgpVideo[i]->BeginComposition(displaySetChanged, &bNewFrameReady));

        //
        // If the video has a new frame then indicate that the video has
        // changed.
        //
        if (bNewFrameReady)
        {
            m_rgpVideo[i]->NotifyOnChanged(m_rgpVideo[i]);
        }
    }

    //
    // ERROR HANDLING NOTE: any failure error code returned from this
    // method will result in putting the current partition into zombie
    // state. Partition manager will attempt to notify the server
    // (or, the UI thread) of the failure.
    //

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:
//      CComposition::EndProcessVideo
//
//  Synopsis:
//      After rendering, tell all the videos that we are at the end of 
//      a composition pass.
//
//-----------------------------------------------------------------------------

void 
CComposition::EndProcessVideo()
{
    for (UINT i = 0; i < m_rgpVideo.GetCount(); i++)
    {
        IGNORE_HR(m_rgpVideo[i]->EndComposition());
    }
}

//+----------------------------------------------------------------------------
//
//    Member:
//        CComposition::ProcessComposition
//
//    Synopsis:
//        Performs the compositor duties by processing any pending batches,
//        updating the video subsystem, rendering and ticking animations.
//
//-----------------------------------------------------------------------------

HRESULT CComposition::ProcessComposition(
    __out_ecount(1) bool *pfPresentNeeded
    )
{
    HRESULT hr = S_OK;
    bool displaySetChanged = false;
    bool doRenderPass = true;
    HRESULT hrUpdateDisplayState = S_OK;

    *pfPresentNeeded = false;

    //
    // Update the display set for this render pass (if possible).
    // NOTE: this needs to be done before ProcessBatches otherwise RequestTier
    //       will get a stale display set.
    //
    int displayCount = 0;
    MIL_THRX(hrUpdateDisplayState, m_pFactory->UpdateDisplayState(&displaySetChanged, &displayCount));

    // Multiple threads could be changing the RenderOptions at once so it is important that we only
    // read and change the value of our value ONCE. It is also important that we call this before
    // OnBeginComposition() because that processes batches and if a render target gets created
    // it needs the latest version of this bool.
    if (m_fLastForceSoftwareForProcessValue != !!RenderOptions::IsSoftwareRenderingForcedForProcess())
    {
        m_fLastForceSoftwareForProcessValue = !m_fLastForceSoftwareForProcessValue;
        IFC(m_pRenderTargetManager->UpdateRenderTargetFlags());
    }

    //
    // Allow for extra composition steps to be taken by specialized compositors.
    //
    IFC(OnBeginComposition());

    //
    // If the display state is invalid, then we report a software tier,
    // the DWM will turn off compositing in this case, WPF should invalidate
    // its render targets to ensure that we get another WM_PAINT message in.
    // 
    if (hrUpdateDisplayState == WGXERR_DISPLAYSTATEINVALID)
    {        
        //
        // We don't render if we can't get a new display set, unless
        // the UI thread has requested that we try to render anyway.
        // 
        auto& compatSettings = g_pPartitionManager->GetCompatSettings();
        
        doRenderPass = compatSettings.ShouldRenderEvenWhenNoDisplayDevicesAreAvailable();
        //
        // Make sure that we invalidate all of the render targets and caches,
        // and notify any listeners that display set is not valid
        // If the UI thread has requested that we try to render despite this, 
        // then override and lie to listeners that displays are valid.
        IFC(m_pRenderTargetManager->NotifyDisplaySetChange(doRenderPass, displayCount));
        GetVisualCacheManagerNoRef()->NotifyDeviceLost();
    }
    else
    {
        IFC(m_pRenderTargetManager->NotifyDisplaySetChange(displaySetChanged, displayCount));

        //
        // All other failures, go to Cleanup and fail.
        // 
        IFC(hrUpdateDisplayState);
    }

    //
    // If the display set changed, then we want to send a tier change notification
    // over to either the DWM or WPF. We don't notify on failure because the partition
    // will be zombied anyway.
    // 
    if (displaySetChanged)
    {
        // If the display set changed we lost the device and therefore let all our 
        // listeners know.
        ProcessRenderingStatus(RENDERING_STATUS_DEVICE_LOST);
        NotifyTierChange();
    }
    
    //
    // Skip the render pass if we don't have a display set.
    // 
    if (!doRenderPass)
    {
        goto Cleanup;
    }

    //
    // Make sure videos are updated as needed
    //
    IFC(BeginProcessVideo(displaySetChanged));

    // 
    // Check for glyphs that need updating
    //
    m_pGlyphCache->ProcessPendingAnimations();

    //
    // We need to be extra careful not to overwrite success codes returned 
    // by Render while performing the post-render actions. We are particularly
    // interested in preserving S_PRESENT_OCCLUDED success code.
    // 
    // the success code preservation is very fragile. Perhaps
    //  we should drop the attempt to preserve it and explicitly let the caller know
    //  that S_PRESENT_OCCLUDED was returned via an extra out parameter.
    //
    {
        HRESULT hrRender = S_OK;

        //
        // Perform the render pass.
        //

        MIL_THRX(hrRender, Render(pfPresentNeeded));

        if (m_bNeedBadShaderNotification)
        {
            // If a user-supplied pixel shader was bad, just send a notification
            // up, and continue on without an error.  This will then repeatedly
            // happen every frame until the app on the UI thread makes a change
            // to prevent it from happening (like removing the offending
            // shader).
            MIL_MESSAGE badShaderMessage = { MilMessageClass::BadPixelShader };
            IGNORE_HR(NotifyHelper(&badShaderMessage));
            m_bNeedBadShaderNotification = false;
        }

        //
        // After rendering, tell all the videos that we are at the end of a
        // composition pass.
        //
        EndProcessVideo();

        //
        // Allow for extra composition steps to be taken by specialized compositors.
        //
        IFC(OnEndComposition());

        //
        // If the steps taken after the Render call succeeded, report the HRESULT
        // as returned by the Render call.
        //
        hr = hrRender;
    }

Cleanup:

    //
    // Consider backbuffer completely composed when present is not needed or
    // when there is failure (false is the default state of *pfPresentNeeded).
    //
    if (!(*pfPresentNeeded))
    {

        //
        // If we aren't going to present, we need to still notify the listeners
        // that no present is occuring.
        //

        NotifyPresentListeners(MilPresentationResults::NoPresent, 0, 0);
    }

    RRETURN1(hr, S_PRESENT_OCCLUDED);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CComposition::Compose
//
//  Synopsis:
//      Runs any necessary updates to the composition - all the resources
//      participating in the composition, the thread, or device management.
//
//------------------------------------------------------------------------------
HRESULT 
CComposition::Compose(
    __out_ecount(1) bool *pfPresentNeeded
    )
{
    HRESULT hr = S_OK;

    // Increment the composition frame counter
    s_frameLastComposed++;

#if ENABLE_PARTITION_MANAGER_LOG
    CPartitionManager::LogEvent(PartitionManagerEvent::Composing, static_cast<DWORD>(reinterpret_cast<UINT_PTR>(this)));
#endif /* ENABLE_PARTITION_MANAGER_LOG */

    bool fPresentNeeded = false;

    //
    // Our rendering code is tested with single precision floating point
    // which is also the mode we like to run DX with.  This mode is
    // enforced at thread start time, here we only need to ensure
    // that FPU mode is still correct.
    //

    CFloatFPU::AssertPrecisionAndRoundingMode();

    if (!IsZombie())
    {
        //
        // If not zombie, perform the render and optional present passes...
        //

        IFC(ProcessComposition(&fPresentNeeded));
    }
    else
    {
        //
        // Zombie partitions do not need to be composed.
        //

        IFC(OnZombieComposition());
    }

    *pfPresentNeeded = fPresentNeeded;

Cleanup:

    if (SUCCEEDED(hr))
    {
        hr = S_OK; // don't return success codes other than S_OK
    }

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member: 
//      CComposition::Render
//
//  Synopsis:  
//      Ticks the schedule manager, updates animate resources and asks the
//      render target manager to start a render pass.
//
//------------------------------------------------------------------------------

HRESULT 
CComposition::Render(    
    __out_ecount(1) bool *pfPresentNeeded
    )
{
    HRESULT hr = S_OK;

    *pfPresentNeeded = false;

    //
    // Tick schedule manager
    //

    GetScheduleManager()->Tick();

    //
    // Ask the render target manager to render the updated content.
    //

    IFC(m_pRenderTargetManager->Render(pfPresentNeeded));


Cleanup:
    //
    // ERROR HANDLING NOTE: any failure error code returned from this
    // method will result in putting the current partition into zombie
    // state. Partition manager will attempt to notify the server
    // (or, the UI thread) of the failure.
    //

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member: 
//      CComposition::RenderingStatusFromHr
//
//  Synopsis:  
//      Converts HRESULTs returned from Render or Present into rendering status.
//
//------------------------------------------------------------------------------

CComposition::RENDERING_STATUS
CComposition::RenderingStatusFromHr(HRESULT hr)
{
    RENDERING_STATUS status = RENDERING_STATUS_NORMAL;
    switch (hr)
    {
        case S_OK:
            status = RENDERING_STATUS_NORMAL;
            break;
        
        case S_PRESENT_OCCLUDED:
            status = RENDERING_STATUS_DEVICE_OCCLUDED;
            break;
            
        case WGXERR_NO_HARDWARE_DEVICE:
        case WGXERR_DISPLAYSTATEINVALID:
            status = RENDERING_STATUS_DEVICE_LOST;
            break;
            
        default:
            // Treat unknown errors as device lost
            status = RENDERING_STATUS_DEVICE_LOST;
            break;
    }

    return status;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CComposition::NotifyRenderStatus
//
//  Synopsis:
//      Notifies the composition engine of a change in rendering status,
//      we then forward this to any interested notification channels. 
//  
//      This is only sent for a sequence of failed Render calls where the 
//      error is swallowed. In the DWM case this indicates a black screen
//      and is something that needs to be stop composition if it persists.
//
//      This is sent as a separate notification to zombie to ensure that
//      the DWM and WPF do not terminate in this case and to ensure that 
//      WPF applications don't change their behavior (the message will be
//      ignored).
// 
//      This cannot be sent as a SW tier notification because the DWM will
//      tear down the transport and requery capabilities, this will succeed
//      resulting in another render failure and another tier request etc. 
//      etc. This is correct behavior for when the display set cannot be 
//      created but not the correct behavior for this case.
// 
//      In this case we want the DWM to stay active until the next display
//      change where it can again attempt to render etc. 
//
//------------------------------------------------------------------------------

HRESULT
CComposition::NotifyRenderStatus(
    __in HRESULT hrRender
    )
{
    HRESULT hr = S_OK;

    MIL_MESSAGE msgNotify = { MilMessageClass::RenderStatus };

    msgNotify.renderStatusData.hrCode = hrRender;

    IFC(NotifyHelper(&msgNotify));    

Cleanup:

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CComposition::Present
//
//  Synopsis:
//      Presents render targets with any unpresented content
//
//------------------------------------------------------------------------------

HRESULT 
CComposition::Present(
    __in_ecount(1) CPartitionManager* pPartitionManager
    )
{
    HRESULT hr = S_OK;
    QPC_TIME qpcPresentationTime = UINT64_MAX;

    if (m_deviceState == MilCompositionDeviceState::Occluded)
    {
        hr = S_PRESENT_OCCLUDED;
        goto Cleanup;
    }

    if (ETW_ENABLED_CHECK(TRACE_LEVEL_INFORMATION))
    {
        LARGE_INTEGER qpcCurrentTime;

        if (m_fQPCSupported)
        {
            QueryPerformanceCounter(&qpcCurrentTime);
        }
        else
        {
            qpcCurrentTime.QuadPart = 0;
        }

        EventWriteWClientUcePresentBegin((UINT64)this, qpcCurrentTime.QuadPart);
    }

    //
    // Ask the render target manager to present its render targets.
    //
    // In case we can't get the refresh rate of the device set the default as 60
    //

    MilPresentationResults::Enum ePresentationResults = MilPresentationResults::VSyncUnsupported;

    UINT uiRefreshRate = 0;

    //
    //  If someone has requested a presentation notification see if there is a
    //  frame time associated with this.
    //
    for (UINT i = 0; i < m_rgpPresentTimeListeners.GetCount(); i++)
    {
        if (m_rgpPresentTimeListeners[i].qpcFrameTime != 0 && m_rgpPresentTimeListeners[i].qpcFrameTime < qpcPresentationTime)
        {
            qpcPresentationTime = m_rgpPresentTimeListeners[i].qpcFrameTime;
        }
    }

    // If we didn't find a frame time, then set the time to 0 (present immediately).
    if (qpcPresentationTime == UINT64_MAX)
    {
        qpcPresentationTime = 0;
    }

    IFC(m_pRenderTargetManager->Present(&uiRefreshRate, 
                                        &ePresentationResults, 
                                        &qpcPresentationTime));

    NotifyPresentListeners(ePresentationResults, uiRefreshRate, qpcPresentationTime);

Cleanup:
    if (SUCCEEDED(hr) &&
        ETW_ENABLED_CHECK(TRACE_LEVEL_INFORMATION))
    {
        LARGE_INTEGER qpcCurrentTime;

        if (m_fQPCSupported)
        {
            QueryPerformanceCounter(&qpcCurrentTime);
        }
        else
        {
            qpcCurrentTime.QuadPart = 0;
        }

        EventWriteWClientUcePresentEnd((UINT64)this, qpcCurrentTime.QuadPart);
    }

#ifdef DEBUG
    m_pGlyphCache->ValidateCache();
#endif

    // Give glyph caches opportunity to trim their realization size if necessary.
    m_pGlyphCache->TrimCache();

    //
    // ERROR HANDLING NOTE: any failure error code returned from this
    // method will result in putting the current partition into zombie
    // state. Partition manager will attempt to notify the server
    // (or, the UI thread) of the failure.
    //

    RRETURN1(hr, S_PRESENT_OCCLUDED);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CComposition::RegisterVideo
// 
//  Synopsis:
//      Adds a video resource to the list of currently playing videos.
// 
//------------------------------------------------------------------------------

HRESULT
CComposition::RegisterVideo(
    __in_ecount(1) CMilSlaveVideo *pVideo
    )
{
    HRESULT hr = S_OK;

    IFC((m_rgpVideo.Add(pVideo)));

#if DBG
    m_dbgVideoCount++;
#endif

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CComposition::UnregisterVideo
// 
//  Synopsis:
//      Removes a video resource from the list of currently playing videos.
// 
//------------------------------------------------------------------------------

void
CComposition::UnregisterVideo(
    __in_ecount(1) CMilSlaveVideo *pVideo
    )
{
    //
    // The resource may not be in the list in TS scenarios
    //
    if (m_rgpVideo.Remove(pVideo))
    {
#if DBG
        m_dbgVideoCount--;
#endif
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CComposition::GetVisualCacheManagerNoRef
// 
//  Synopsis:
//      Returns a pointer to the visual cache manager for this composition.
// 
//------------------------------------------------------------------------------

CVisualCacheManager*
CComposition::GetVisualCacheManagerNoRef()
{
    Assert(m_pVisualCacheManager);
    return m_pVisualCacheManager;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CComposition::GetRenderTargetManagerNoRef
// 
//  Synopsis:
//      Returns a pointer to the render target manager for this composition.
// 
//------------------------------------------------------------------------------

CRenderTargetManager*
CComposition::GetRenderTargetManagerNoRef()
{
    Assert(m_pRenderTargetManager);
    return m_pRenderTargetManager;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CComposition::AddEtwEvent
// 
//  Synopsis:
//      Adds an event to the ETW event execution list.
// 
//------------------------------------------------------------------------------

HRESULT 
CComposition::AddEtwEvent(
    __in_ecount(1) CSlaveEtwEventResource *pEtwEvent
    )
{
    RRETURN(m_rgpEtwEvent.Add(pEtwEvent));
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CComposition::RemoveEtwEvent
// 
//  Synopsis:
//      Removes an event from the ETW event execution list.
// 
//  Remarks:
//      It is valid to call this method with a EtwEvent that is not 
//      registered. The current implementation will ignore this. This 
//      implementation simplifies the destructor implementation of the 
//      CSlaveEtwEvent class since that is where the EtwEvents are 
//      getting removed.
// 
//------------------------------------------------------------------------------

void 
CComposition::RemoveEtwEvent(
    __in_ecount(1) CSlaveEtwEventResource *pEtwEvent
    )
{
    IGNORE_FAIL(FALSE, m_rgpEtwEvent.Remove(pEtwEvent));
}


// ----------------------------------------------------------------------------
//
//   Special case handlers for command processing
//
// ----------------------------------------------------------------------------

//+-----------------------------------------------------------------------
//
//  Member:  CComposition::Channel_NotifyPolicyChangeForNonInteractiveMode
//
//  Synopsis:  
//          See the summary comments on MediaContext.ForceRenderingInNonInteractiveMode 
//          for details. 
//
//------------------------------------------------------------------------
HRESULT CComposition::Partition_NotifyPolicyChangeForNonInteractiveMode(
    __in CMilServerChannel *pChannel, 
    __in CMilSlaveHandleTable *pHandleTable, 
    __in const MILCMD_PARTITION_NOTIFYPOLICYCHANGEFORNONINTERACTIVEMODE* pCmd
)
{
    bool fForceRender = 
        (pCmd->ShouldRenderEvenWhenNoDisplayDevicesAreAvailable ? true : false);
    auto& compatSettings = g_pPartitionManager->GetCompatSettings();
    compatSettings.SetRenderPolicyForNonInteractiveMode(fForceRender);

    return S_OK;
}

//+-----------------------------------------------------------------------
//
//  Member:  CComposition::Partition_RegisterForNotifications
//
//  Synopsis:  Adds the calling channel to the list of channels
//             async notifications are sent to
//
//------------------------------------------------------------------------
HRESULT CComposition::Partition_RegisterForNotifications(
    __in CMilServerChannel *pChannel,
    __in CMilSlaveHandleTable *pHandleTable,
    __in const MILCMD_PARTITION_REGISTERFORNOTIFICATIONS *pCmd
    )
{
    HRESULT hr = S_OK;

    if (pCmd->Enable)
    {
        // only add the channel if it is not already in the list
        if (m_rgpNotificationChannel.Find(0, pChannel) == m_rgpNotificationChannel.GetCount())
        {
            IFC(m_rgpNotificationChannel.Add(pChannel));
            pChannel->AddRef();

            // Nobody is listening to the CompositionDeviceStateChange
            // message anymore and should probably be removed. Care should be taken to ensure that any 
            // side effects that yield specific behavior during mode changes are identified (if any exist) 
            // and are preserved (if appropriate).

            // Send the current state of the device to work around 
            // a race condition in the DWM.  The device may become
            // ready before the DWM has registered for notifications
            // resulting in the DWM staying disabled when it shouldn't
            MIL_MESSAGE msgNotify = {MilMessageClass::CompositionDeviceStateChange};
            msgNotify.deviceStateChangeData.deviceStateOld = m_deviceState;
            msgNotify.deviceStateChangeData.deviceStateNew = m_deviceState;
            IGNORE_HR(NotifyHelper(&msgNotify));

        }
    }
    else
    {
        if (m_rgpNotificationChannel.Find(0, pChannel) < m_rgpNotificationChannel.GetCount())
        {
            if (m_rgpNotificationChannel.Remove(pChannel))
            {
                pChannel->Release();
            }
        }
    }
  Cleanup:
    return S_OK;
}


//+-----------------------------------------------------------------------------
//
//   Special case handlers for command processing
//
//------------------------------------------------------------------------------

/*++

CComposition::Transport_DestroyResourcesOnChannel

Description:
  Shuts down a transport channel.

Returns:
  S_OK -- this method will never fail.

--*/

HRESULT CComposition::Transport_DestroyResourcesOnChannel(
    __in CMilServerChannel* pChannel,
    __in CMilSlaveHandleTable* pHandleTable,
    __in const MILCMD_TRANSPORT_DESTROYRESOURCESONCHANNEL* pCmd
    )
{
    //
    // If this is the last channel shutting closing on forced TS client
    // shutdown we need to release all render targets and resources.
    //

    CleanupOnShutdown();

    return S_OK;
}


// ----------------------------------------------------------------------------
//
//   Special case handlers for command processing
//
// ----------------------------------------------------------------------------

/*++

CComposition::Transport_SyncFlush

Description:
  Puts a channel on the list of channels to be signaled
  when composition is done.

Returns:
  This method can only fail on a DynArray::Add call.

--*/

HRESULT CComposition::Transport_SyncFlush(
    __in CMilServerChannel* pChannel,
    __in CMilSlaveHandleTable* pHandleTable,
    __in const MILCMD_TRANSPORT_SYNCFLUSH* pCmd
    )
{
    HRESULT hr = S_OK;

    //
    // The channel has issued a sync flush. Add it to the
    // array of channels that will be signaled when composition is done.
    //

    IFC(m_rgpFlushChannels.Add(pChannel));
    pChannel->AddRef();

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
// 
//  Member:
//      CComposition::Channel_DeleteResource
//
//  Synopsis:
//      Releases a resource on a channel.
//
//  Returns:
//      WGXERR_UCE_MALFORMEDPACKET if the resource handle is invalid. This method
//      can also fail on a CMilSlaveHandleTable::DeleteHandle call.
// 
//------------------------------------------------------------------------------

HRESULT 
CComposition::Channel_DeleteResource(
    __in CMilServerChannel* pChannel,
    __in CMilSlaveHandleTable* pHandleTable,
    __in const MILCMD_CHANNEL_DELETERESOURCE* pCmd
    )
{
    HRESULT hr = S_OK;

    TraceTag((tagMILResources, 
              "CComposition::Channel_DeleteResource: handle 0x%08x, type 0x%08x",
              pCmd->Handle,
              pCmd->resType
              ));

    MIL_RESOURCE_TYPE resType = pCmd->resType;

    //
    // Retrieve the resource (this also verifies the resource type).
    //

    CMilSlaveResource *pResource =
        pHandleTable->GetResource(
            pCmd->Handle,
            resType
            );

    if (pResource == NULL
        || pHandleTable->GetObjectType(pCmd->Handle) != resType) /* need exact type */
    {
        RIP("Invalid resource handle.");
        IFC(WGXERR_UCE_MALFORMEDPACKET);
    }


    //
    //  Added code for removing
    // composition targets from ProcessReleaseResource. Should
    // we remove targets when refcount drops to zero and
    // resource is actually going away?
    //

    //
    //  adding/removing targets from the list is implicit.
    // This should probably
    // be made explicit and via command packets
    //

    if ((resType == TYPE_HWNDRENDERTARGET) ||
        (resType == TYPE_GENERICRENDERTARGET))
    {
        CRenderTarget* pTarget =
            static_cast<CRenderTarget*>(pResource);

        m_pRenderTargetManager->RemoveRenderTarget(pTarget);
    }

    IFC(ReleaseResource(pHandleTable, pCmd->Handle, pResource, false /* fShutdownCleanup */));

Cleanup:
    RRETURN(hr);
}


/*++

CComposition::Channel_CreateResource

Description:
  Creates a resource on a channel.

Returns:
  WGXERR_UCE_MALFORMEDPACKET if the resource type is invalid. This method
  can also fail on a CMilSlaveHandleTable::CreateEmptyResource call.

--*/

HRESULT CComposition::Channel_CreateResource(
    __in CMilServerChannel* pChannel,
    __in CMilSlaveHandleTable* pHandleTable,
    __in const MILCMD_CHANNEL_CREATERESOURCE* pCmd
    )
{
    HRESULT hr = S_OK;

    CMilSlaveResource *pResource = NULL;

    TraceTag((tagMILResources, 
              "CComposition::Channel_CreateResource: handle 0x%08x, type 0x%08x",
              pCmd->Handle,
              pCmd->resType
              ));

    // Glyph run resources must be created and initialized explicitly
    // with the MilCmdGlyphRunCreate command, as they are not usable
    // if empty. Therefore, ignore implicit creation here.

    if (pCmd->resType == TYPE_GLYPHRUN)
    {
        goto Cleanup;
    }


    //
    // Check for handle collisions.
    //

    Assert(pHandleTable->GetObjectType(pCmd->Handle) == EMPTY_ENTRY);


    //
    // Create an empty resource.
    //

    IFC(pHandleTable->CreateEmptyResource(
        this,
        pChannel,
        pCmd,
        &pResource
        ));


    //
    // Make sure that a resource has been created...
    //

    Assert(pHandleTable->GetObjectType(pCmd->Handle) != EMPTY_ENTRY);


Cleanup:
    ReleaseInterface(pResource);

    RRETURN(hr);
}


/*++

CComposition::Channel_DuplicateHandle

Description:
  Performs client-side handle duplication.

--*/

HRESULT CComposition::Channel_DuplicateHandle(
    __in CMilServerChannel* pChannel,
    __in CMilSlaveHandleTable* pHandleTable,
    __in const MILCMD_CHANNEL_DUPLICATEHANDLE* pCmd
    )
{
    HRESULT hr = S_OK;

    CMilServerChannel *pTargetChannel = NULL;

    IFC(GetAttachedChannel(
            pCmd->TargetChannel,
            &pTargetChannel
            ));

    Assert(pHandleTable->GetObjectType(pCmd->Original) != EMPTY_ENTRY);
    Assert((pTargetChannel->GetChannelTable())->GetObjectType(pCmd->Duplicate) == EMPTY_ENTRY);

    IFC(pHandleTable->DuplicateHandle(
        pChannel,
        pCmd->Original,
        pTargetChannel,
        pCmd->Duplicate
        ));

    Assert((pTargetChannel->GetChannelTable())->GetObjectType(pCmd->Duplicate) == pHandleTable->GetObjectType(pCmd->Original));

Cleanup:
    ReleaseInterface(pTargetChannel);

    RRETURN(hr);
}



//+-----------------------------------------------------------------------------
//
//    Member:
//        CComposition::Channel_RequestTier
//
//    Synopsis:
//        Determines the current hardware tier and some other vital 
//        statistics. Sends the information over the back channel.
//
//------------------------------------------------------------------------------
HRESULT 
CComposition::Channel_RequestTier(
    __in CMilServerChannel *pChannel,
    __in CMilSlaveHandleTable *pHandleTable,
    __in const MILCMD_CHANNEL_REQUESTTIER *pCmd
    )
{
    HRESULT hr = S_OK;

    // The tier message sent over the back channel
    MIL_MESSAGE tierMessage = { MilMessageClass::Tier };

    //
    // Get the current hardware caps information.
    //
    //Future Consideration:  display set uniqueness is be part of the
    //  reply, use that value to make sure the tier is valid by the time
    //  we get to create the render target.
    //

    tierMessage.tierData.CommonMinimumCaps = pCmd->ReturnCommonMinimum;

    ULONG DisplayUniqueness = tierMessage.tierData.DisplayUniqueness;

    m_pFactory->QueryCurrentGraphicsAccelerationCaps(
                !!tierMessage.tierData.CommonMinimumCaps,
                &DisplayUniqueness,
                &tierMessage.tierData.Caps);

    tierMessage.tierData.DisplayUniqueness = DisplayUniqueness;

    //
    // Grab relevant WinSAT data -- video memory bandwidth in kilobytes 
    // per second and video memory size estimation, in bytes. The caller
    // can use this information to perform display machine assessments.
    //

    {
        static const PTSTR szWinSatKey = _T("Software\\Microsoft\\Windows NT\\CurrentVersion\\WinSAT");
        
        // Grab WinSAT dropped bandwidth number
        DWORD VideoMemoryBandwidth = 0;
        DWORD VideoMemorySize = 0;

        if (!RegGetHKLMDword(szWinSatKey, _T("VideoMemoryBandwidth"), &VideoMemoryBandwidth))
        {
            VideoMemoryBandwidth = 0;  // assume no bandwidth if there's no assessment
        }
    
        tierMessage.tierData.Assessment.VideoMemoryBandwidth = VideoMemoryBandwidth;

        // Read the video memory size
        if (!RegGetHKLMDword(szWinSatKey, _T("VideoMemorySize"), &VideoMemorySize))
        {
            VideoMemorySize = 0; // assume no memory if there's no assessment
        }
    
        tierMessage.tierData.Assessment.VideoMemorySize = VideoMemorySize;
    }

    //
    // Send the obtained caps information over the back channel.
    //

    IFC(pChannel->PostMessageToChannel(&tierMessage));

Cleanup:
    RRETURN(hr);
}


/*++

CComposition::Partition_SetVBlankSyncMode

Description:
  Sets the presentation mode to either wait for vertical blank or not.

Returns:
  Presently this function always succeeds and returns S_OK.

--*/

HRESULT CComposition::Partition_SetVBlankSyncMode(
    __in CMilServerChannel* pChannel,
    __in CMilSlaveHandleTable* pHandleTable,
    __in const MILCMD_PARTITION_SETVBLANKSYNCMODE* pCmd
    )
{
    HRESULT hr = S_OK;

    if (pCmd->Enable)
    {
        IFC(m_pRenderTargetManager->EnableVBlankSync(pChannel));
    }
    else
    {
        m_pRenderTargetManager->DisableVBlankSync(pChannel);
    }

Cleanup:
    RRETURN(hr);
}


/*++

CComposition::Partition_NotifyPresent

Description:
  Adds a channel to the list of channels that get notified of each
  presentation time.

Returns:
  Presently this function always succeeds and returns S_OK.

--*/

HRESULT CComposition::Partition_NotifyPresent(
    __in CMilServerChannel* pChannel,
    __in CMilSlaveHandleTable* pHandleTable,
    __in const MILCMD_PARTITION_NOTIFYPRESENT* pCmd
    )
{
    NotifyPresentInfo notifyInfo;

    notifyInfo.pChannel = pChannel;
    RtlCopyMemory(&notifyInfo.qpcFrameTime, &(pCmd->FrameTime), sizeof(QPC_TIME));
    RRETURN(m_rgpPresentTimeListeners.Add(notifyInfo));
}


/*++

CComposition::GlyphRun_Create

Description:
  Creates a glyph run or updates an existing glyph run.

Returns:
  WGXERR_UCE_MALFORMEDPACKET if the glyph run resource handle is invalid.
  This method can also fail on the calls to CMilSlaveHandleTable
  ::CreateEmptyResource and CGlyphRun::ProcessCreate methods.

--*/

HRESULT CComposition::GlyphRun_Create(
    __in CMilServerChannel* pChannel,
    __in CMilSlaveHandleTable* pHandleTable,
    __in const MILCMD_GLYPHRUN_CREATE* pCmd,
    __in_bcount(cbPayload) LPCVOID pcvPayload,
    UINT cbPayload
    )
{
    HRESULT hr = S_OK;

    //
    // Check to see if the resource needs to be created.
    //
    // If it already exists, drop through and process the packet
    // rather than executing the creation routine.
    //

    if (pHandleTable->GetObjectType(pCmd->Handle) == TYPE_NULL)
    {
        CGlyphRunResource* pRes = NULL;

        MILCMD_CHANNEL_CREATERESOURCE cmd = { MilCmdChannelCreateResource };

        cmd.Handle = pCmd->Handle;
        cmd.resType = TYPE_GLYPHRUN;

        IFC(pHandleTable->CreateEmptyResource(
            this,
            pChannel,
            &cmd,
            reinterpret_cast<CMilSlaveResource**>(&pRes)
            ));

        IFC(pRes->ProcessCreate(
            pHandleTable,
            pCmd,
            pcvPayload,
            cbPayload
            ));

        Verify(pRes->Release() == 1);
    }
    else
    {
        CGlyphRunResource* pResource =
            static_cast<CGlyphRunResource*>(pHandleTable->GetResource(
                pCmd->Handle,
                TYPE_GLYPHRUN
                ));

        if (pResource == NULL)
        {
            IFC(WGXERR_UCE_MALFORMEDPACKET);
        }

        //
        // Dispatch the command to the given resource.
        //

        IFC(pResource->ProcessCreate(
            pHandleTable,
            pCmd,
            pcvPayload,
            cbPayload
            ));
    }

Cleanup:
    RRETURN(hr);
}


/*++

CComposition::HwndTarget_Create

Description:
  Creates an HWND render target. 

Returns:
  WGXERR_UCE_MALFORMEDPACKET if the HWND render target handle is invalid.
  This method can also fail on calls to CSlaveHWndRenderTarget
  ::ProcessCreate and CSlaveIntermediateRenderTarget
  ::ProcessCreateAsHwndTarget methods.

--*/

HRESULT CComposition::HwndTarget_Create(
    __in CMilServerChannel* pChannel,
    __in CMilSlaveHandleTable* pHandleTable,
    __in const MILCMD_HWNDTARGET_CREATE* pCmd
    )
{
    HRESULT hr = S_OK;

    if (pHandleTable->GetObjectType(pCmd->Handle) == TYPE_HWNDRENDERTARGET)
    {
        CSlaveHWndRenderTarget* pResource =
            static_cast<CSlaveHWndRenderTarget*>(pHandleTable->GetResource(
                pCmd->Handle,
                TYPE_HWNDRENDERTARGET
                ));

        if (pResource == NULL)
        {
            IFC(WGXERR_UCE_MALFORMEDPACKET);
        }

        // Dispatch the command to the HWND render target.
        IFC(pResource->ProcessCreate(
            pHandleTable,
            pCmd
            ));

        // Associate this render target with our render target manager.
        IFC(m_pRenderTargetManager->AddRenderTarget(pResource));

    }
    else
    {
        RIP("Invalid resource specified as target of MilCmdHwndTargetCreate.");
        IFC(WGXERR_UCE_MALFORMEDPACKET);
    }

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//    Member:
//        CComposition::GenericTarget_Create
//
//    Synopsis:
//        Creates a generic render target and registers it with
//        the render target manager.
//
//------------------------------------------------------------------------------

HRESULT
CComposition::GenericTarget_Create(
    __in CMilServerChannel* pChannel,
    __in CMilSlaveHandleTable* pHandleTable,
    __in const MILCMD_GENERICTARGET_CREATE* pCmd
    )
{
    HRESULT hr = S_OK;

    CSlaveGenericRenderTarget* pResource =
        static_cast<CSlaveGenericRenderTarget*>(
            pHandleTable->GetResource(
                pCmd->Handle,
                TYPE_GENERICRENDERTARGET
                ));
    
    if (pResource == NULL)
    {
        RIP("Invalid resource handle.");
        IFC(WGXERR_UCE_MALFORMEDPACKET);
    }
    
    IFC(pResource->ProcessCreate(pHandleTable, pCmd));

    // Associate this render target with our render target manager.
    IFC(m_pRenderTargetManager->AddRenderTarget(pResource));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member: CComposition::WaitForVBlank
//
//  Synopsis:  Waits for VBlank to occur on the device used by the first HW render target
//
//  Returns: S_OK on success
//           WGXERR_NO_HARDWARE_DEVICE if there are no HWND targets
//
//------------------------------------------------------------------------
HRESULT CComposition::WaitForVBlank()
{
    BEGIN_MILINSTRUMENTATION_HRESULT_LIST_WITH_DEFAULTS
        WGXERR_NO_HARDWARE_DEVICE
    END_MILINSTRUMENTATION_HRESULT_LIST

    RRETURN(m_pRenderTargetManager->WaitForVBlank());
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CComposition::AttachChannel
//
//    Synopsis:
//        Called by the packet player to signal a channel has been
//        connected to this composition.
//
//------------------------------------------------------------------------------

HRESULT CComposition::AttachChannel(
    __in HMIL_CHANNEL hChannel,
    __in CMilServerChannel *pChannel
    )
{
    HRESULT hr = S_OK;

    if (hChannel >= s_cMaxAttachedChannels
        || (hChannel < m_rgpAttachedChannels.GetCount()
            && m_rgpAttachedChannels[hChannel] != NULL))
    {
        RIP("CComposition::AttachChannel: invalid channel handle.");
        IFC(E_INVALIDARG);
    }


    //
    // Grow the channel table if necessary.
    //

    if (hChannel >= m_rgpAttachedChannels.GetCount())
    {
        IFC(m_rgpAttachedChannels.AddAndSet(
                hChannel - m_rgpAttachedChannels.GetCount() + 1,
                NULL
                ));
    }

    Assert(hChannel < m_rgpAttachedChannels.GetCount());

    m_rgpAttachedChannels[hChannel] = pChannel;
    pChannel->AddRef();

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//    Member:
//        CComposition::DetachChannel
//
//    Synopsis:
//        Called by the packet player to signal a channel has been
//        disconnected from this composition.
//
//------------------------------------------------------------------------------

HRESULT CComposition::DetachChannel(
    __in HMIL_CHANNEL hChannel
    )
{
    HRESULT hr = S_OK;

    CMilServerChannel *pChannel = NULL;

    IFC(GetAttachedChannel(hChannel, &pChannel));

    //
    // Remove the channel from the notification array.
    //

    if (m_rgpNotificationChannel.Find(0, pChannel) < m_rgpNotificationChannel.GetCount())
    {
        if (m_rgpNotificationChannel.Remove(pChannel))
        {
            pChannel->Release();
        }
    }

    //
    // Release the specified channel twice, once for the storage reference
    // count and once for the GetAttachedChannel call, and remove its table 
    // entry.
    //

    ReleaseInterfaceNoNULL(pChannel);
    ReleaseInterface(pChannel);

    m_rgpAttachedChannels[hChannel] = NULL;


    //
    // Shrink the table size if possible.
    //

    bool fShouldShrink = false;

    while (m_rgpAttachedChannels.GetCount() > 0
           && m_rgpAttachedChannels.Last() == NULL)
    {
        IFC(m_rgpAttachedChannels.RemoveAt(m_rgpAttachedChannels.GetCount() - 1));

        fShouldShrink = true;
    }

    if (fShouldShrink)
    {
        m_rgpAttachedChannels.ShrinkToSize();
    }

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//    Member:
//        CComposition::GetAttachedChannel
//
//    Synopsis:
//        Looks up a channel attached to this composition
//
//------------------------------------------------------------------------------

HRESULT CComposition::GetAttachedChannel(
    __in HMIL_CHANNEL hChannel,
    __out CMilServerChannel **ppChannel
    )
{
    HRESULT hr = S_OK;

    CMilServerChannel *pChannel = NULL;

    if (hChannel >= s_cMaxAttachedChannels
        || hChannel >= m_rgpAttachedChannels.GetCount()
        || m_rgpAttachedChannels[hChannel] == NULL)
    {
        RIP("CComposition::GetAttachedChannel: invalid channel handle");
        IFC(E_INVALIDARG);
    }

    pChannel = m_rgpAttachedChannels[hChannel];
    pChannel->AddRef();

    *ppChannel = pChannel;
    pChannel = NULL;

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------
//
//  Member: CComposition::FlushChannels
//
//  Synopsis: Signals channels that are waiting for a channel flush, ETW
//            events are fired here also
//
//  Notes:
//      This code was previously in Present(), but since we don't always
//      call Present() after Render if all rendered rectangles are occluded
//      it needs to be factored out so we can still call it regardless of whether
//      Present happens. If we don't, the compositor will become non-responsive on startup.
//
//------------------------------------------------------------------------

void
CComposition::FlushChannels(
    bool fForceAllChannels
    )
{
    //
    // Output the ETW Event performance traces from the ETW resources
    // accumulated in the array.
    //

    for (UINT i = 0; i < m_rgpEtwEvent.GetCount(); i++)
    {
        CSlaveEtwEventResource *pEtw = m_rgpEtwEvent[i];
        Assert(pEtw);

        pEtw->OutputEvent();
    }


    //
    // Signal all channels waiting for a sync flush. When done
    // clear the sync flush array.
    //
    // 
    for (UINT i = 0, limit = m_rgpFlushChannels.GetCount(); i < limit; i++)
    {
        CMilServerChannel* pChannel = m_rgpFlushChannels[i];

        pChannel->SignalFinishedFlush(m_hrZombieNotificationFailureReason);
        pChannel->Release();
    }

    m_rgpFlushChannels.Reset();

    //
    // In abortive shutdown situations we want to signal all channels
    // attached to this composition.
    //
    if (fForceAllChannels)
    {
        for (UINT i = 0, limit = m_rgpAttachedChannels.GetCount(); i < limit; i++)
        {
            CMilServerChannel* pChannel = m_rgpAttachedChannels[i];

            if (pChannel != NULL)
            {
                pChannel->SignalFinishedFlush(m_hrZombieNotificationFailureReason);
            }
        }
    }
}

//+-----------------------------------------------------------------------
//
//  Member:
//      CComposition::NotifyPartitionIsZombie
//
//  Synopsis:
//      This method sends a "partition is in zombie state" notification
//      back to the server side on all channels that have registered
//      to receive them.
//
//------------------------------------------------------------------------

HRESULT
CComposition::NotifyPartitionIsZombie()
{
    HRESULT hr = S_OK, hrPost = S_OK;

    UINT cNotificationsToPost = 0;

    Assert(NeedsZombieNotification());


    //
    // Clean up the compositor.
    //

    CleanupOnShutdown();


    //
    // Signal all the channels that work has been completed (the UI thread
    // might be waiting for us to signal sync flush event) and that the
    // partition is now in zombie state.
    //

    FlushChannels(/* fForceAllChannels */ true);

    {
        for (UINT i = 0, limit = m_rgpAttachedChannels.GetCount(); i < limit; i++)
        {
            CMilServerChannel *pChannel = m_rgpAttachedChannels[i];
            
            if (pChannel)
            {
                //
                // Keep track of the number of channels attached to this composition
                // and balance it with the notifications successfully posted.
                //
                
                ++cNotificationsToPost;
                
                //
                // Send out the notifications saying that this partition is now a zombie.
                //
                
                MIL_MESSAGE msg = { MilMessageClass::PartitionIsZombie };
                msg.partitionIsZombieData.hrFailureCode = m_hrZombieNotificationFailureReason;               
                MIL_THRX(hrPost, pChannel->PostMessageToChannel(&msg));

                
                if (SUCCEEDED(hrPost))
                {
                    --cNotificationsToPost;
                }
            }
        }
    }

    if (cNotificationsToPost > 0)
    {
        //
        // Failure to post notifications will require a retry later...
        //
        // Note that we do not consider the case of a partition
        // without any channels attached as it is degenerate.
        //
        //  consider disallowing to open a channel to a zombie partition.
        //

        TraceTag((tagMILVerbose,
                  "CComposition::NotifyPartitionIsZombie: failed to send notifications to %i channels",
                  cNotificationsToPost
                  ));

        MIL_THR(WGXERR_UCE_RENDERTHREADFAILURE);
    }

    RRETURN(hr);
}


//+-----------------------------------------------------------------------
//
//  Member: CComposition::ProcessRenderingStatus
//
//  Synopsis:  Changes the deivce state based on the status supplied
//             and notifies all intersted channels of the change
//
//------------------------------------------------------------------------
void
CComposition::ProcessRenderingStatus(RENDERING_STATUS status)
{
    MilCompositionDeviceState::Enum stateNew = m_deviceState;
    MilCompositionDeviceState::Enum stateOld = m_deviceState;

    switch (status)
    {
        case RENDERING_STATUS_NORMAL:
        {
            stateNew = MilCompositionDeviceState::Normal;        
        }
        break;
    
        case RENDERING_STATUS_DEVICE_LOST:
        case RENDERING_STATUS_DEVICE_RELEASED:
        {
            GetVisualCacheManagerNoRef()->NotifyDeviceLost();
            stateNew = MilCompositionDeviceState::NoDevice;
        }
        break;
    
        case RENDERING_STATUS_DEVICE_OCCLUDED:
        {
            ScheduleCompositionPass();
            stateNew = MilCompositionDeviceState::Occluded;
        }
        break;
    }
    m_deviceState = stateNew;

    if (stateNew != stateOld)
    {
#if DBG
        static const LPCSTR rgRenderingStatus[] = { "NORMAL", "NO_DEVICE", "OCCLUDED" };
        TraceTag((tagMILWarning,
                  "CComposition::ProcessRenderingStatus: Status %s -> %s.",
                  rgRenderingStatus[stateOld],
                  rgRenderingStatus[stateNew]
                  ));
#endif
        
        MIL_MESSAGE msgNotify = {MilMessageClass::CompositionDeviceStateChange};
        msgNotify.deviceStateChangeData.deviceStateOld = stateOld;
        msgNotify.deviceStateChangeData.deviceStateNew = stateNew;
        IGNORE_HR(NotifyHelper(&msgNotify));
    }
}

//+-----------------------------------------------------------------------
//
//  Member: CComposition::NotifyHelper
//
//  Synopsis:  Sends a notifications to all interested channels
//
//------------------------------------------------------------------------

HRESULT
CComposition::NotifyHelper(__in_ecount(1) MIL_MESSAGE *pMessage)
{
    HRESULT hr = S_OK;

    UINT count = m_rgpNotificationChannel.GetCount();
    for  (UINT i = 0; i < count; i++)
    {
        MIL_THR_SECONDARY(m_rgpNotificationChannel[i]->PostMessageToChannel(pMessage));
    }

    RRETURN(hr)
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CComposition::CleanupOnShutdown
//
//    Synopsis:
//        Cleans up compositor resources.
//
//------------------------------------------------------------------------------

void
CComposition::CleanupOnShutdown()
{
    // Let the specialized composition devices know that we are shutting down.
    OnShutdownComposition();

    //
    // Release the notification channels.
    //

    ReleaseNotificationChannels();

    //
    // As we are aborting the composition and stopping batch processing, all
    // resources (including render targets) need to be released. It's up to
    // the UI thread to clean up the master handle table entries.
    //

    m_pRenderTargetManager->ReleaseTargets();

    {
        for (UINT i = 0, limit = m_rgpAttachedChannels.GetCount(); i < limit; i++)
        {
            CMilServerChannel *pChannel = m_rgpAttachedChannels[i];
            
            if (pChannel)
            {
                //
                // Release the resources belonging to this channel.
                //
                
                pChannel->GetChannelTable()->ReleaseHandleTableEntries(this);
            }
        }
    }
}


//+-----------------------------------------------------------------------------
//
//    Member:
//        CComposition::ReleaseNotificationChannels
//
//    Synopsis:
//        Releases the table of channels registered for notifications.
//
//------------------------------------------------------------------------------

void
CComposition::ReleaseNotificationChannels()
{
    //
    // Release the table of channels registered for notifications.
    //

    for (unsigned int i = 0; i < m_rgpNotificationChannel.GetCount(); i++)
    {
        m_rgpNotificationChannel[i]->Release();
    }

    m_rgpNotificationChannel.Reset(TRUE);
}


//+-----------------------------------------------------------------------
//
//  Member: CComposition::NotifyPresentListeners
//
//  Synopsis:  Sends a notifications to all interested channels
//             if fPresentationTime is false, then we send 0 as the
//             presentation time so that it get ignored (we didn't present)
//
//------------------------------------------------------------------------

void
CComposition::NotifyPresentListeners(
    MilPresentationResults::Enum ePresentationResults,
    UINT uiRefreshRate,
    QPC_TIME qpcPresentationTime
    )
{
    HRESULT hr = S_OK;
    //
    // Notify any listener channels about the presentation time
    //

    UINT nListenerCount = m_rgpPresentTimeListeners.GetCount();
    if (nListenerCount > 0)
    {
        //
        // Notice that currently this function only waits on the sync
        // display when this is no longer the case check that the correct
        // display is in vsync and something was presented before updating
        // the last present time.
        //

        MIL_MESSAGE message = { MilMessageClass::Presented };

        RtlCopyMemory(&message.presentationTimeData.presentationResults, &ePresentationResults, sizeof(MilPresentationResults::Enum));
        RtlCopyMemory(&message.presentationTimeData.refreshRate, &uiRefreshRate, sizeof(UINT));
        RtlCopyMemory(&message.presentationTimeData.presentationTime, &qpcPresentationTime, sizeof(LARGE_INTEGER));

        for (UINT channel = 0; channel < nListenerCount; channel++)
        {
            MIL_THR(m_rgpPresentTimeListeners[channel].pChannel->PostMessageToChannel(&message));
        }

        //
        // The notification subscription is only valid for one frame, so clear
        // the array now that the notification is issued. However, as we
        // expect these channels to subscribe again in their next frames do
        // not trim the listeners array.
        //

        m_rgpPresentTimeListeners.Reset(FALSE);
    }
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CComposition::ReleaseResource
//
//    Synopsis:
//        Releases a resource associated with this composition
//
//------------------------------------------------------------------------------

HRESULT 
CComposition::ReleaseResource(
    __in_ecount(1) CMilSlaveHandleTable *pHandleTable,
    __in HMIL_RESOURCE hResource,
    __in_ecount(1) CMilSlaveResource *pResource,
    bool fShutdownCleanup
    )
{
    HRESULT hr = S_OK;

    if (pResource->IsOfType(TYPE_GLYPHRUN))
    {
        //
        // Managed side already received confirmation from
        // CMilMasterHandleTable::ReleaseOnChannel() that the resource has been
        // released, and removed related data from glyph cache. However,
        // real glyph run destruction can be deferred because of renderdata
        // that can hold it. In theory this should not happen.
        // In practice it does happen from time to time, because of various
        // errors in life time control. When happens, we see either
        // assertion or AV in CMilSlaveGlyphCache::GetBitmaps() that never
        // helped to figure out what's going wrong.
        // To avoid it, we are disabling glyph run rendering so that it
        // will no more apply to glyph cache.
        //
        CGlyphRunResource *pGlyphRunResource = DYNCAST(CGlyphRunResource, pResource);
        Assert(pGlyphRunResource);

        pGlyphRunResource->Disable();
    }


    //
    // After performing the special steps, delete the resource in the handle table.
    //

    IFC(pHandleTable->DeleteHandle(hResource));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CComposition::NotifyTierChange
//
//    Synopsis:
//        Notifies any register listeners that there was a device change (and
//        hence a potential tier change).
// 
//------------------------------------------------------------------------------
void 
CComposition::NotifyTierChange()
{
    MIL_MESSAGE tierMessage = { MilMessageClass::Tier };

    tierMessage.tierData.CommonMinimumCaps = TRUE;

    ULONG DisplayUniqueness = tierMessage.tierData.DisplayUniqueness;

    m_pFactory->QueryCurrentGraphicsAccelerationCaps(
                !!tierMessage.tierData.CommonMinimumCaps,
                &DisplayUniqueness,
                &tierMessage.tierData.Caps);

    tierMessage.tierData.DisplayUniqueness = DisplayUniqueness;

    IGNORE_HR(NotifyHelper(&tierMessage));
}




