// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_composition
//      $Keywords:  composition
//
//  $Description:
//      The composition device.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#pragma once

MtExtern(CComposition);

//
// Forward declarations
//

class CRenderTargetManager;
class CVisualCacheManager;
class CSlaveEtwEventResource;
class CDrawingContext;
class CMilSlaveVideo;
class CSlaveHWndRenderTarget;
class CMilSlaveGlyphCache;

//+-----------------------------------------------------------------------------
//
//  Enumeration:
//      RoundTripRequestState
// 
//  Synopsis:
//      Tracks the state of round trip requests that can be used to measure
//      latency of the composition pipe.
// 
//------------------------------------------------------------------------------

enum RoundTripRequestState
{
    RoundTripRequestState_None,
    RoundTripRequestState_Pending,
    RoundTripRequestState_WaitingOnDXQueue
};


//+-----------------------------------------------------------------------------
//
//  Class:
//      CComposition
// 
//  Synopsis:
//      The composition device.
// 
//------------------------------------------------------------------------------

class CComposition :
    public CMILRefCountBase,
    public Partition,
    public IMilBatchDevice
{
protected:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CComposition));

    CComposition(__in MilMarshalType::Enum marshalType);

    virtual ~CComposition();

public:
    DEFINE_REF_COUNT_BASE


    //+-------------------------------------------------------------------------
    //
    //  Composition device interface
    //
    //--------------------------------------------------------------------------

public:
    // Requests that an extra composition pass is executed.
    virtual void ScheduleCompositionPass() = 0;

protected:
    // Called by ProcessComposition after ensuring the display set.
    virtual HRESULT OnBeginComposition() = 0;

    // Called by ProcessComposition after the composition pass is over.
    virtual HRESULT OnEndComposition() = 0;

    // Called by the composition device on shutdown.
    virtual void OnShutdownComposition() = 0;

    // Called by Compose after the partition has been zombied.
    virtual HRESULT OnZombieComposition() = 0;


public:
    //+-------------------------------------------------------------------------
    //
    //  Composition properties, timing and statistics
    //
    //--------------------------------------------------------------------------

    MilMarshalType::Enum GetMarshalType() const { return m_mType;}

    // Returns the time at the last composition pass
    UTC_TIME GetComposedTime() const { Assert(m_mType != MilMarshalType::SameThread); return m_tComposed; };

    // Publicly exposed counter to determine if we are still in the same composition frame
    static UTC_TIME GetFrameLastComposed() { return s_frameLastComposed; };
    

    //+-------------------------------------------------------------------------
    //
    //  Access to composition device components
    //
    //--------------------------------------------------------------------------

    __out_ecount(1) CMILFactory *GetMILFactory()
    {
        m_pFactory->AddRef();
        return m_pFactory;
    }

    __out_ecount(1) CMilScheduleManager *GetScheduleManager() { return &m_scheduleManager; }

    //+-------------------------------------------------------------------------
    //
    //  Partition interface
    //
    //--------------------------------------------------------------------------

    // Runs any necessary updates to the composition - all the resources
    // participating in the composition, the thread, or device management.
    override HRESULT Compose(
        __out_ecount(1) bool *pfPresentNeeded
        );

    override HRESULT WaitForVBlank();

    // present any target with unpresented rendering
    override HRESULT Present(
        __in_ecount(1) CPartitionManager* pPartitionManager
        );

    override void FlushChannels(
        bool fForceAllChannels = false
        );

    // This method sends a "this partition is in zombie state" notification 
    // back to the server side on all channels that have registered 
    // to receive them.
    override HRESULT NotifyPartitionIsZombie();

    override virtual MilCompositionDeviceState::Enum GetCompositionDeviceState()
    {
        return m_deviceState;
    }

    HRESULT RegisterVideo(__in_ecount(1) CMilSlaveVideo *pVideo);

    void UnregisterVideo(__in_ecount(1) CMilSlaveVideo *pVideo);

    //+-------------------------------------------------------------------------
    //
    //  Render target interface
    //
    //--------------------------------------------------------------------------

    // Rendering status tracks the state of the D3D devices used by this composition device.
    enum RENDERING_STATUS
    {
        RENDERING_STATUS_DEVICE_RELEASED,
        RENDERING_STATUS_DEVICE_LOST,
        RENDERING_STATUS_DEVICE_OCCLUDED,
        RENDERING_STATUS_NORMAL
    };

    // Process current status of the rendering device
    void ProcessRenderingStatus(RENDERING_STATUS status);

    static RENDERING_STATUS RenderingStatusFromHr(HRESULT hr);

    HRESULT NotifyRenderStatus(__in HRESULT hrRender);


    //+-------------------------------------------------------------------------
    //
    //  Misc. public methods
    //
    //--------------------------------------------------------------------------

    // Adds a performance tracing event. This event will be signaled
    // when flushing the channels (see CComposition::FlushChannels).
    HRESULT AddEtwEvent(
        __in_ecount(1) CSlaveEtwEventResource* pEtwEvent
        );

    // Removes a performance tracing event.
    void RemoveEtwEvent(
        __in_ecount(1) CSlaveEtwEventResource* pEtwEvent
        );

    // Releases a resource associated with this composition
    static HRESULT ReleaseResource(
        __in_ecount(1) CMilSlaveHandleTable *pHandleTable,
        __in HMIL_RESOURCE hResource,
        __in_ecount(1) CMilSlaveResource *pResource,
        bool fShutdownCleanup
        );

    // Set up a bad shader notification to be sent after the render pass. 
    void SetPendingBadShaderNotification()
    {
        m_bNeedBadShaderNotification = true;
    }

    __out CMilSlaveGlyphCache *GetGlyphCache()
    {
        return m_pGlyphCache;
    }


    CVisualCacheManager* GetVisualCacheManagerNoRef();

    CRenderTargetManager* GetRenderTargetManagerNoRef();

    bool GetLastForceSoftwareForProcessValue()
    {
        return m_fLastForceSoftwareForProcessValue;
    }

    CMilSlaveResource* GetCurrentResourceNoRef() { return m_pCurrentResourceNoRef; }

    void SetCurrentResource(CMilSlaveResource* pCurrentResourceNoRef) { m_pCurrentResourceNoRef = pCurrentResourceNoRef; }

protected:
    // Initializes this instance of the CComposition class.
    HRESULT Initialize();

    // Signals that a channel has been connected to this composition.
    HRESULT AttachChannel(
        __in HMIL_CHANNEL hChannel,
        __in CMilServerChannel *pChannel
        );

    // Signals that a channel has been disconnected from this composition.
    HRESULT DetachChannel(
        __in HMIL_CHANNEL hChannel
        );

    // Looks up a channel attached to this composition
    HRESULT GetAttachedChannel(
        __in HMIL_CHANNEL hChannel,
        __out CMilServerChannel **ppChannel
        );

    HRESULT BeginProcessVideo(
        __in_ecount(1) bool displaySetChanged
        );

    void EndProcessVideo();

    // Processes a partition command, optionally skipping command batches
    HRESULT ProcessPartitionCommand(
        __in_ecount(1) CMilCommandBatch* pBatch,
        __in bool fProcessBatchCommands
        );

    // Processes a command batch
    HRESULT ProcessCommandBatch(
        __in_ecount(1) CMilCommandBatch* pBatch
        );

    // Performs the compositor duties by processing any pending batches,
    // updating the video subsystem, rendering and ticking animations.
    HRESULT ProcessComposition(
        __out_ecount(1) bool *pfPresentNeeded
        );

    // Render all of the dirty render targets
    HRESULT Render(        
        __out_ecount(1) bool *pfPresentNeeded
        );

    HRESULT NotifyHelper(__in_ecount(1) MIL_MESSAGE *pMessage);

protected:
    //+-------------------------------------------------------------------------
    //
    //  Composition device components
    //
    //--------------------------------------------------------------------------

    // MIL rendering factory should be used when creating MIL rendering objects. 
    CMILFactory *m_pFactory;

    // Enables resources to schedule additional composition passes
    CMilScheduleManager m_scheduleManager;

    // Represents the collection of render targets used by this compositor
    CRenderTargetManager *m_pRenderTargetManager;


    //+-------------------------------------------------------------------------
    //
    //  Transport integration
    //
    //--------------------------------------------------------------------------

    // The effective marshal type for the channels attached to this composition device.
    MilMarshalType::Enum m_mType;

    // Maximum allowed number of channels attached to this composition.
    static const size_t s_cMaxAttachedChannels = 64 * 1024;

    // Array of channels that are attached to this composition.
    DynArray<CMilServerChannel*, TRUE> m_rgpAttachedChannels; 


    //+-------------------------------------------------------------------------
    //
    //  Timing
    //
    //--------------------------------------------------------------------------

    // If true, the hardware supports high-resolution performance counters.
    BOOL m_fQPCSupported;

    LARGE_INTEGER m_qpcFrequency;

    UTC_TIME m_tComposed;

    QPC_TIME m_qpcComposed;    

    static UTC_TIME s_frameLastComposed;


    //+-------------------------------------------------------------------------
    //
    //  Scheduling and statistics
    //
    //--------------------------------------------------------------------------

    MilCompositionDeviceState::Enum m_deviceState;


    //+-------------------------------------------------------------------------
    //
    //  Notifications and events
    //
    //--------------------------------------------------------------------------

    // Array of channels that are blocked on channel processing and composition pass
    DynArray<CMilServerChannel*, TRUE> m_rgpFlushChannels;

    // Array of channels that have requested to receive async notifications
    DynArray<CMilServerChannel*, TRUE> m_rgpNotificationChannel;

    // structure to keep track of which channels want to be notified when
    // composition is finished.
    struct NotifyPresentInfo
    {
        CMilServerChannel *pChannel;
        QPC_TIME           qpcFrameTime;
    };

    DynArray<NotifyPresentInfo> m_rgpPresentTimeListeners;

    RoundTripRequestState m_roundTripRequestState;

private:
    // ------------------------------------------------------------------------
    //
    //   Special case handlers for command processing
    //
    // ------------------------------------------------------------------------

    HRESULT CComposition::Partition_NotifyPolicyChangeForNonInteractiveMode(
        __in CMilServerChannel *pChannel,
        __in CMilSlaveHandleTable *pHandleTable,
        __in const MILCMD_PARTITION_NOTIFYPOLICYCHANGEFORNONINTERACTIVEMODE* pCmd
        );

    HRESULT Partition_RegisterForNotifications(
        __in CMilServerChannel* pChannel,
        __in CMilSlaveHandleTable* pHandleTable,
        __in const MILCMD_PARTITION_REGISTERFORNOTIFICATIONS * pCmd
        );

    HRESULT Transport_DestroyResourcesOnChannel(
        __in CMilServerChannel* pChannel,
        __in CMilSlaveHandleTable* pHandleTable,
        __in const MILCMD_TRANSPORT_DESTROYRESOURCESONCHANNEL* pCmd
        );

    HRESULT Transport_SyncFlush(
        __in CMilServerChannel* pChannel,
        __in CMilSlaveHandleTable* pHandleTable,
        __in const MILCMD_TRANSPORT_SYNCFLUSH* pCmd
        );

    HRESULT Channel_DeleteResource(
        __in CMilServerChannel* pChannel,
        __in CMilSlaveHandleTable* pHandleTable,
        __in const MILCMD_CHANNEL_DELETERESOURCE* pCmd
        );

    HRESULT Channel_CreateResource(
        __in CMilServerChannel* pChannel,
        __in CMilSlaveHandleTable* pHandleTable,
        __in const MILCMD_CHANNEL_CREATERESOURCE* pCmd
        );

    HRESULT Channel_DuplicateHandle(
        __in CMilServerChannel* pChannel,
        __in CMilSlaveHandleTable* pHandleTable,
        __in const MILCMD_CHANNEL_DUPLICATEHANDLE* pCmd
        );

    // Requests that the current hardware caps be determined and sent back to the channel.
    HRESULT Channel_RequestTier(
        __in CMilServerChannel* pChannel,
        __in CMilSlaveHandleTable* pHandleTable,
        __in const MILCMD_CHANNEL_REQUESTTIER* pCmd
        );

    HRESULT Partition_SetVBlankSyncMode(
        __in CMilServerChannel* pChannel,
        __in CMilSlaveHandleTable* pHandleTable,
        __in const MILCMD_PARTITION_SETVBLANKSYNCMODE* pCmd
        );

    HRESULT Partition_NotifyPresent(
        __in CMilServerChannel* pChannel,
        __in CMilSlaveHandleTable* pHandleTable,
        __in const MILCMD_PARTITION_NOTIFYPRESENT* pCmd
        );

    HRESULT GlyphRun_Create(
        __in CMilServerChannel* pChannel,
        __in CMilSlaveHandleTable* pHandleTable,
        __in const MILCMD_GLYPHRUN_CREATE* pCmd,
        __in_bcount(cbPayload) LPCVOID pcvPayload,
        UINT cbPayload
        );

    HRESULT HwndTarget_Create(
        __in CMilServerChannel* pChannel,
        __in CMilSlaveHandleTable* pHandleTable,
        __in const MILCMD_HWNDTARGET_CREATE* pCmd
        );

    HRESULT GenericTarget_Create(
        __in CMilServerChannel* pChannel,
        __in CMilSlaveHandleTable* pHandleTable,
        __in const MILCMD_GENERICTARGET_CREATE* pCmd
        );
    
private:
    // Cleans up compositor resources.
    void CleanupOnShutdown();

    // Releases the table of channels registered for notifications.
    void ReleaseNotificationChannels();

    // Notify our listeners that we presented a frame
    void NotifyPresentListeners(
        MilPresentationResults::Enum ePresentationResults,
        UINT uiRefreshRate,
        QPC_TIME qpcPresentationTime
        );

    void NotifyTierChange();


private:
    //+-------------------------------------------------------------------------
    // 
    //  Resource management
    // 
    //--------------------------------------------------------------------------

    // Glyph cache for this CComposition
    CMilSlaveGlyphCache *m_pGlyphCache;

    // List of all video resources currently registered with this composition device.
    DynArray<CMilSlaveVideo *, TRUE> m_rgpVideo;

    // Array of ETW Event resources for performance tracing.
    DynArray<CSlaveEtwEventResource*, TRUE> m_rgpEtwEvent;

    CVisualCacheManager* m_pVisualCacheManager;
    
    bool m_bNeedBadShaderNotification;

    // A copy of the current Visual/SlaveResource being drawn for etw IRT creation tracking.
    CMilSlaveResource* m_pCurrentResourceNoRef;

    bool m_fLastForceSoftwareForProcessValue;
   
    //+-------------------------------------------------------------------------
    //
    //  Debugging support
    //
    //--------------------------------------------------------------------------

#if DBG   
    // Number of currently registered video resources.
    UINT m_dbgVideoCount;
#endif
};


