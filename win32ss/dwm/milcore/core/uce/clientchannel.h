// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Abstract:
//
//    Api implemented by the channel server side (composition side).
//    Comments on the use of IMilRefCount. The server side api is used by the
//    code running server side as follows:
//       - when running in proc by the composition loop. It is passed to the loop
//       by setting its pointer on command batches that need it during handle
//       lookup.
//       - when running cross proc by the channel tables and the composition loop
//       (same as above).
//    In proc. Starts with one reference on creation, is incremented when a batch
//       goes through the channel, is decremented when the command batch comes out
//      of the channel. The command buffer references are manipulated via the
//       CMilCommandBatch::SetChannelPtr function. When the channel is destroyed
//       a MilCmdDeviceDestroyChannel is enqueued as the last command in the last
//       batch sent through the channel. On execution of this command, the
//       refcount is decremented. In the in proc case, the refcount controls the
//       lifetime of the combined channel object.
//    Cross proc. This api is used on an object that is created only server side.
//       Starts with one reference on creation, and get incremented when it is
//       added to the channel table. The original refcount corresponds to the
//       reference on the client side. Is incremented when a batch goes through
//       the channel, is decremented when the command batch comes out of the
//       channel. The command buffer references are manipulated via the
//       CMilCommandBatch::SetChannelPtr function. When the channel is destroyed
//       a MilCmdDeviceDestroyChannel is enqueued as the last command in the last
//       batch sent through the channel. On execution of this command, the
//       refcount is decremented. Finally a transport command that removes the
//       channel from the channel tables is sent through the transport. This
//       removes another reference.
//
//------------------------------------------------------------------------------

MtExtern(CMilChannel);

class CMilSlaveHandleTable;
interface IMilBatchDevice;

class CMilChannel :
    public CMILRefCountBase
{
    friend class CMilMasterHandleTable;

private:
    CMilChannel(
        __in_ecount(1) CMilConnection *pConnection,
        HMIL_CHANNEL hChannel
        );

    HRESULT Initialize();

    ~CMilChannel();

public:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilChannel));
    DEFINE_REF_COUNT_BASE

    static HRESULT Create(
        __in_ecount(1) CMilConnection *pConnection,
        HMIL_CHANNEL hChannel,
        __out_ecount(1) CMilChannel **ppChannel
        );

    HRESULT CreateOrAddRefOnChannel(
        MIL_RESOURCE_TYPE type,
        IN OUT HMIL_RESOURCE *ph
        );

    HRESULT DuplicateHandle(
        HMIL_RESOURCE hOriginal,
        __in_ecount(1) CMilChannel* pTargetChannel,
        __out_ecount(1) HMIL_RESOURCE* phDuplicate
        );

    HRESULT ReleaseOnChannel(
        HMIL_RESOURCE h,
        __out_ecount_opt(1) BOOL *pfDeleted
        );

    HRESULT GetRefCount(
        HMIL_RESOURCE h,
        __out_ecount_opt(1) UINT *pcRefs
        );

    HRESULT SendCommand(
        __in_bcount(cbSize) PVOID pvCommand,
        UINT cbSize,
        bool sendInNewBatch = false
        );

    HRESULT BeginCommand(
        __in_bcount(cbSize) PVOID pvCommand,
        UINT cbSize,
        UINT cbExtra
        );

    HRESULT AppendCommandData(
        __in_bcount(cbSize) void *pvData,
        UINT cbSize
        );

    HRESULT EndCommand();

    HRESULT CloseBatch();

    //
    // Commits a closed batch, if it exists.
    //
    HRESULT Commit();

    HMIL_CHANNEL GetChannel() const
    {
        return m_hChannel;
    }

    MilMarshalType::Enum GetMarshalType() const
    {
        return m_pConnection->GetMarshalType();
    }

    //
    // Free list head in the master handle table.
    // temporary methods until we spin of tables inside the channel
    // these methods are neded to control handle deletion.
    //

    UINT GetFreeIndex() const
    {
        return m_idxFree;
    }

    void SetFreeIndex(UINT idx)
    {
        m_idxFree = idx;
    }

  
    HRESULT SetNotificationWindow(HWND hwnd, UINT message);

    HRESULT PostMessageToChannel(
        __in_ecount(1) const MIL_MESSAGE *pmsg
        );

    HRESULT PeekNextMessage(
        __out_bcount_part(cbSize, sizeof(MIL_MESSAGE)) MIL_MESSAGE *pmsg,
        size_t cbSize,
        __out_ecount(1) BOOL *pfMessageRetrieved
        );

    HRESULT WaitForNextMessage(
        __range(0, MAXIMUM_WAIT_OBJECTS - 1) DWORD nCount,
        __in_ecount(nCount) const HANDLE *pHandles,
        BOOL bWaitAll,
        DWORD waitTimeout,
        __out_ecount(1) DWORD *pWaitReturn
        );

    HRESULT Destroy();

    HRESULT SyncFlush();

    void Zombie(HRESULT hrZombie) { m_hrZombie = hrZombie; }

    void Disconnect() { m_fIsDisconnected = true; }

    void SetReceiveBroadcastMessages(bool fReceiveBroadcast)
    {
        m_fReceivesBroadcastMessages = fReceiveBroadcast;
    }

    bool ReceivesBroadcastMessages() const
    {
        return m_fReceivesBroadcastMessages;
    }

private:

    HRESULT BeginItem();
    HRESULT AddItemData(__in_bcount(cbSize) void *pData, UINT cbSize);
    HRESULT EndItem();


    HRESULT EnsureSize(UINT cbSize);

    //
    // The batch memory is created lazily. Ensure that it exists and if
    // necessary create it.
    //

    HRESULT EnsureRecorder();

    UINT m_idxFree;
    CMilMasterHandleTable m_handleTable;
    CMilConnection *m_pConnection;
    HMIL_CHANNEL m_hChannel;

    //
    // Message queue for the channel object. This is used to queue messages
    // from the server to the client. For this implementation, a LIST_ENTRY
    // queue is used, protected by a critical section. The event is signaled
    // whenever anything is posted to the queue, allowing the WaitForNextMessage
    // function to block.
    //

    struct LOCAL_MESSAGE_QUEUE :
        public LIST_ENTRY
    {
        MIL_MESSAGE msg;
    };

    LIST_ENTRY m_MessageQueue;
    HANDLE m_eventQueue;
    CCriticalSection m_csQueue;

    CMilCommandBatch *m_pCommands;
    DynArray<CMilCommandBatch *> m_pClosedBatches;

    HWND m_hNotificationWindow;
    UINT m_nNotificationMessage;

    //
    // Flags
    //

    bool m_fWaitingForSyncFlush     : 1;
    bool m_fIsCommandOpen           : 1;
    bool m_fReceivesBroadcastMessages : 1;
    bool m_fIsDisconnected : 1;

    // If set to a failure code, the partition that the corresponding server channel 
    // is attached to has been zombied because of a render thread failure.
    HRESULT m_hrZombie;
};

inline CMilChannel *HandleToPointer(MIL_CHANNEL hChannel)
{
    return reinterpret_cast<CMilChannel*>(hChannel);
}

inline MIL_CHANNEL PointerToHandle(__in_ecount_opt(1) CMilChannel *pChannel)
{
    return reinterpret_cast<MIL_CHANNEL>(pChannel);
}

