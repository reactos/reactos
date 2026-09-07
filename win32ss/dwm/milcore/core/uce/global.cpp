// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//---------------------------------------------------------------------------------
//

//
//  Abstract:
//    Global composition engine functionality/data structures go into this file.
//
//---------------------------------------------------------------------------------

#include "precomp.hpp"

DeclareTag(tagMILResources, "MIL", "MIL Resource Model Debugging");
DeclareTag(tagMILRedirection, "MIL", "MIL Redirection Debugging");
DeclareTag(tagMILRedirectionSpriteMap, "MIL", "MIL Redirection Sprite Map Debugging");
DeclareTag(tagMILConnectionHosting, "MIL", "MIL Connection hosting");
DeclareTag(tagMILConnectionHostingUpdates, "MIL", "MIL Connection hosting Updates");
DeclareTag(tagMILTransport, "MIL", "MIL Transport Layer Debugging");
DeclareTag(tagTSPerf, "TS", "Avalon Terminal Services Performance");
DeclareTag(tagTSDebug, "TS", "Avalon Terminal Services Debugging");
DeclareTag(tagMILConnection, "MIL", "MIL Connection Debugging");
DeclareTag(tagMILConnectionCtx, "MIL", "MIL ConnectionCtx Debugging");
DeclareTag(tagMILRPC, "RPC", "Avalon RPC Transport Debugging");
DeclareTag(tagMILTransportForwardTraffic, "MIL", "MIL Forward traffic");
DeclareTag(tagMILTransportBackwardTraffic, "MIL", "MIL Backward traffic");
DeclareTag(tagTSConnector, "TS", "Avalon Terminal Services Debugging:ts connector");
DeclareTag(tagMILServerChannel, "MIL", "MIL Server Channel Debugging");
DeclareTag(tagMILTierRequest, "MIL", "MIL Server Tier request");

//---------------------------------------------------------------------------------
//
// Global composition engine critical section.
//
//---------------------------------------------------------------------------------

CCriticalSection g_csCompositionEngine;


CMediaControl *g_pMediaControl;
LONG g_cRefInitialization = 0;
LONG g_cRefTransportInitialization = 0;
extern CMilSlaveHandleTable *g_pht;

//---------------------------------------------------------------------------------
//
// Global transport setting overrides.
//
//---------------------------------------------------------------------------------

#if defined(PRERELEASE)
// If set to true, will enable recording T-transport.
bool s_fRecordingTTransport = false;
#endif /* defined(PRERELEASE) */


//+-----------------------------------------------------------------------------
//
//    Function:
//        EnsurePartitionManager
// 
//    Synopsis:
//        Ensures the existence of the partition manager and increases its
//        reference count.
// 
//    Error handling:
//        The reference count will be increased even in the event of partition
//        manager creation failure. Caller will be responsible for calling
//        ReleasePartitionManager in such a case.
//
//------------------------------------------------------------------------------

HRESULT 
EnsurePartitionManager(
    int nPriority
    )
{
    HRESULT hr = S_OK;

    //
    // Enable these tags to get more information about resource model,
    // redirection, transport and terminal services integration.
    //

    EnableTag(tagMILResources, FALSE);
    EnableTag(tagMILRedirection, FALSE);
    EnableTag(tagMILRedirectionSpriteMap, TRUE);
    EnableTag(tagMILConnectionHosting, FALSE);
    EnableTag(tagMILConnectionHostingUpdates, FALSE);
    EnableTag(tagMILTransport, FALSE);
    EnableTag(tagTSDebug, FALSE);
    EnableTag(tagTSPerf, FALSE);
    EnableTag(tagMILConnection, FALSE);
    EnableTag(tagMILConnectionCtx, FALSE);
    EnableTag(tagMILRPC, TRUE);
    EnableTag(tagMILTransportForwardTraffic, FALSE);
    EnableTag(tagMILTransportBackwardTraffic, FALSE);
    EnableTag(tagTSConnector, FALSE);
    EnableTag(tagMILServerChannel, FALSE);
    EnableTag(tagMILTierRequest, FALSE);

    //
    // Initialize may be called multiple times in the case of multiple
    // app-domains. Use reference counting to ensure we only initialize
    // once.
    //

    CGuard<CCriticalSection> oGuard(g_csCompositionEngine);

    if (g_cRefInitialization++ == 0)
    {            
        //
        // Create the partition manager.
        //

        IFC(CPartitionManager::Create(nPriority, &g_pPartitionManager));
    }

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//    Function:
//        ReleasePartitionManager
// 
//    Synopsis:
//        Will decrease the partition  manager reference count and shut
//        it down if necessary.
// 
//    Error handling:
//        This method should be called even if EnsurePartitionManager
//        call failed.
//
//------------------------------------------------------------------------------

void 
ReleasePartitionManager()
{
    CGuard<CCriticalSection> oGuard(g_csCompositionEngine);

    //
    // Deinitialize may be called multiple times in the case of multiple
    // app-domains. Use reference counting to ensure we only deinitialize
    // once.
    //

    if (0 == --g_cRefInitialization)
    {
        //
        // g_pPartitionManager could be null if the first EnsurePartitionManager 
        // call failed. We require the release function to be called to simplify
        // error handling on caller side.
        //

        if (g_pPartitionManager != NULL)
        {
            g_pPartitionManager->Shutdown();

            SAFE_DELETE(g_pPartitionManager);
        }
    }

#ifdef PRERELEASE
    if (g_cRefInitialization < 0) 
    {
        MilUnexpectedError(E_UNEXPECTED, TEXT("Partition manager reference counting error."));
    }
#endif
}

//+-----------------------------------------------------------------------------
//
//    Function:
//        UpdateSchedulerSettings
//
//    Synopsis:
//        Causes a scheduler type change. This will trigger shutdown of any 
//        existing worker threads.  Scheduler will then be re-created and 
//        new worker threads will be started.
//
//------------------------------------------------------------------------------

HRESULT UpdateSchedulerSettings(
    int nPriority
    )
{
    HRESULT hr = S_OK;
    g_csCompositionEngine.Enter();
    IFC(g_pPartitionManager->UpdateSchedulerSettings(nPriority));

Cleanup:
    g_csCompositionEngine.Leave();
    RRETURN(hr);
}

//---------------------------------------------------------------------------------
//
//   GetCompositionEngineComposedEventId()
//
//   Get the id used to create the named event signaled after every composition pass
//
//---------------------------------------------------------------------------------
HRESULT GetCompositionEngineComposedEventId(
    __out_ecount(1) UINT *pcEventId
)
{
    HRESULT hr = S_OK;
    g_csCompositionEngine.Enter();
    IFC(g_pPartitionManager->GetComposedEventId(pcEventId));

Cleanup:
    g_csCompositionEngine.Leave();
    RRETURN(hr);
}




