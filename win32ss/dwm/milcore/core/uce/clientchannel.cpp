// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Implementation of the MIL channel class for use by UI thread callers.
//
//      A channel is always associated with a connection object, which
//      represents the logical connection between a user of the composition
//      engine and the engine itself. The channel defines a streaming
//      protocol for composition engine commands, and it is responsible for
//      accumulating and submitting batches of such commands to the engine
//      via the connection. The command protocol includes the definition of a
//      handle namespace for resource references. Each handle is only valid in
//      the channel in which is created. A handle table is maintained by each
//      channel to keep track of valid handles.
//
//      In addition, each channel also includes a queue of back-channel
//      messages sent from the composition engine back to the application.
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

#if PRERELEASE
//
// When enabled, the back channel fuzzer will flip a random bit in the incoming
// messages every now and then. The expectation is that the code handling the
// message will be able to analyze the corrupted message and properly recover.
//
#define ENABLE_BACKCHANNEL_FUZZING 0
#endif

MtDefine(CMilChannel, MILRender, "CMilChannel");
MtDefine(CMilChannelMessage, MILRender, "CMilClientChannelMessage");

//+-----------------------------------------------------------------------------
//
//    Member:
//        CMilChannel constructor
//
//    Synopsis:
//        Constructor for CMilChannel objects.
//
//------------------------------------------------------------------------------

CMilChannel::CMilChannel(
    __in_ecount(1) CMilConnection *pConnection,
    HMIL_CHANNEL hChannel
    )
{
    // Zero-initialized on creation by DECLARE_METERHEAP_CLEAR

    SetInterface(m_pConnection, pConnection);
    m_hChannel = hChannel;

    //
    // Initialize the message queue structures.
    //

    InitializeListHead(&m_MessageQueue);

    //
    // The first reference goes to the creator of the object
    //

    m_cRef = 1;
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CMilChannel::Initialize
//
//    Synopsis:
//        Initializes a channel by creating an event for synchronization.
//
//------------------------------------------------------------------------------

HRESULT
CMilChannel::Initialize()
{
    HRESULT hr = S_OK;

    IFC(m_csQueue.Init());
    IFCW32(m_eventQueue = CreateEvent(NULL, FALSE, FALSE, NULL));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CMilChannel destructor
//
//    Synopsis:
//        Releases all memory and resources owned by this channel object.
//
//------------------------------------------------------------------------------

CMilChannel::~CMilChannel()
{
    ReleaseInterface(m_pConnection);

    if (m_eventQueue)
    {
        CloseHandle(m_eventQueue);
    }

    while (!IsListEmpty(&m_MessageQueue))
    {
        LOCAL_MESSAGE_QUEUE *p = (LOCAL_MESSAGE_QUEUE*)RemoveHeadList(
            &m_MessageQueue
            );

        FreeHeap(p);
    }

    delete m_pCommands;
    m_pCommands = NULL;
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CMilChannel::CreateOrAddRefOnChannel
//
//    Synopsis:
//        Creates a new resource handle with an intial reference count of one
//        or increments the reference count of an existing handle. The in/out
//        parameter determines whether to create a handle (if NULL) or increment
//        a ref count (if non-NULL).
//
//------------------------------------------------------------------------------

HRESULT
CMilChannel::CreateOrAddRefOnChannel(
    MIL_RESOURCE_TYPE type,
    IN OUT HMIL_RESOURCE *ph
    )
{
    HRESULT hr = S_OK;

    //
    // We can't send a create command if we are in the middle of another command.
    //

    if (m_fIsCommandOpen)
    {
        IFC(WGXERR_UCE_MISSINGENDCOMMAND);
    }

    //
    // Consider moving the marshalling code out of the
    // slave table leaving only the allocation in the table. That way we can
    // cleanly separate handle creation from handle transport.
    //

    IFC(m_handleTable.CreateOrAddRefOnChannel(this, type, ph));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CMilChannel::DuplicateHandle
//
//    Synopsis:
//        Duplicates a handle from this channel's handle table into the handle
//        table of another channel. The target channel must be associated with
//        the same connection and partition as this one.
//
//        Note that this method enqueues a command on the this channel,
//        but does not commit the channel. As a result, the new handle
//        on the target channel will only become known to the composition
//        engine after this channel is committed by the caller. Therefore,
//        this channel must be committed before the target channel commits a
//        batch that includes references to the duplicated handle, or the
//        composition engine will reject that batch as containing an invalid
//        handle. The following is the proper sequence that must be followed
//        by users of DuplicateHandle:
//
//        1.    Create object on channel A with handle Ha.
//        2.    Duplicate handle from channel A to channel B, with new handle Hb.
//        3.    Commit channel A
//        4.    Commit channel B
//
//------------------------------------------------------------------------------

HRESULT
CMilChannel::DuplicateHandle(
    HMIL_RESOURCE hOriginal,
    __in_ecount(1) CMilChannel* pTargetChannel,
    __out_ecount(1) HMIL_RESOURCE* phDuplicate
    )
{
    HRESULT hr = S_OK;


    //
    // We can't send a duplicate command if we are
    // in the middle of another command.
    //

    if (m_fIsCommandOpen)
    {
        IFC(WGXERR_UCE_MISSINGENDCOMMAND);
    }

    //
    // We cannot duplicate handles between two channels belonging to
    // different connections.
    //

    if (pTargetChannel->m_pConnection != m_pConnection)
    {
        IFC(E_INVALIDARG);
    }

    //
    // Ask the client table to duplicate the handle.
    //

    IFC(m_handleTable.DuplicateHandle(
        this,
        hOriginal,
        pTargetChannel,
        phDuplicate
        ));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CMilChannel::ReleaseOnChannel
//
//    Synopsis:
//        Decrements the reference count of a resource handle previously
//        created on this channel. If the reference count reaches zero then
//        the resource is released and the handle is thereafter invalid.
//
//------------------------------------------------------------------------------

HRESULT
CMilChannel::ReleaseOnChannel(
    HMIL_RESOURCE h,
    __out_ecount_opt(1) BOOL *pfDeleted
    )
{
    HRESULT hr = S_OK;

    //
    // We can't send a delete command if we are in the middle of another command.
    //

    if (m_fIsCommandOpen)
    {
        IFC(WGXERR_UCE_MISSINGENDCOMMAND);
    }

    IFC(m_handleTable.ReleaseOnChannel(this, h, pfDeleted));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CMilChannel::GetRefCount
//
//    Synopsis:
//        Returns the reference count of a resource handle previously
//        created on this channel.
//
//------------------------------------------------------------------------------

HRESULT
CMilChannel::GetRefCount(
    HMIL_RESOURCE h,
    __out_ecount_opt(1) UINT *pcRefs
    )
{
    HRESULT hr = S_OK;

    //
    // We can't get the ref count if we are in the middle of another command.
    //

    if (m_fIsCommandOpen)
    {
        IFC(WGXERR_UCE_MISSINGENDCOMMAND);
    }

    IFC(m_handleTable.GetRefCountOnChannel(this, h, pcRefs));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CMilChannel::SendCommand
//
//    Synopsis:
//        Sends a command packet via this channel. The command packet will not
//        be processed by the composition engine until after Commit is called.
//        The sendInSeparateBatch parameter determines whether the command is
//        sent in the currently open batch, or whether it will be added to a
//        new and separate batch which is then immediately closed, leaving the
//        current batch untouched.
//        
//
//------------------------------------------------------------------------------

HRESULT
CMilChannel::SendCommand(
    __in_bcount(cbSize) PVOID pvCommand,
    UINT cbSize,
    bool sendInSeparateBatch
    )
{
    HRESULT hr = S_OK;

    CMilCommandBatch *old_pCommands = NULL;
    int old_idxFree = 0;
    if (sendInSeparateBatch)
    {
        old_pCommands = m_pCommands;
        m_pCommands = NULL;
        old_idxFree = m_idxFree;
        m_idxFree = 0;
    }

    //
    // Let BeginCommand do the parameter and state validation
    //

    IFC(BeginCommand(pvCommand, cbSize, 0));
    IFC(EndCommand());

    if (sendInSeparateBatch)
    {
        IFC(CloseBatch());
        m_pCommands = old_pCommands;
        m_idxFree = old_idxFree;
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CMilChannel::BeginCommand
//
//    Synopsis:
//        Submits the first part of a multi-part command. Additional command
//        data can be submitted via the AppendCommandData method. The command
//        must be completed with a call to the EndCommand method before either
//        another command is submitted or the channel is committed.
//
//------------------------------------------------------------------------------

HRESULT
CMilChannel::BeginCommand(
    __in_bcount(cbSize) PVOID pvCommand,
    UINT cbSize,
    UINT cbExtra
    )
{
    HRESULT hr = S_OK;
    UINT cbCommandTotal = 0;

    //
    // We can't start a command if we are in the middle of another command.
    //

    if (m_fIsCommandOpen)
    {
        IFC(WGXERR_UCE_MISSINGENDCOMMAND);
    }

    //
    // Make sure we have at least a record ID in the input data.
    //

    if (cbSize < sizeof(MILCMD))
    {
        IFC(WGXERR_UCE_MALFORMEDPACKET);
    }

    //
    // Make sure the output buffer is large enough to contain the item
    // and will remain large enough for the following items we have
    // declared.
    //

    IFC(UIntAdd(cbSize, cbExtra, &cbCommandTotal));

    IFC(EnsureSize(cbCommandTotal));

    //
    // Emit the record to the stream.
    //

    IFC(BeginItem());

    MIL_THR(AddItemData(pvCommand, cbSize));

    if (FAILED(hr))
    {
        //
        // Leave the channel in a consistent state
        //
        MIL_THR_SECONDARY(EndItem());
    }

    //
    // Next we will only accept AppendCommandData calls until the next EndCommand
    //

    m_fIsCommandOpen = true;

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CMilChannel::AppendCommandData
//
//    Synopsis:
//        Submits additional data for a multi-part command previously started
//        with a call to the BeginCommand method.
//
//------------------------------------------------------------------------------

HRESULT
CMilChannel::AppendCommandData(
    __in_bcount(cbSize) VOID *pvData,
    UINT cbSize
    )
{
    HRESULT hr = S_OK;

    //
    // We can't append data if we haven't started a multi-part command.
    //

    if (!m_fIsCommandOpen)
    {
        IFC(WGXERR_UCE_MISSINGBEGINCOMMAND);
    }

    IFC(AddItemData(pvData, cbSize));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CMilChannel::EndCommand
//
//    Synopsis:
//        Completes submission of a multi-part command previously started with
//        a call to the BeginCommand method.
//
//------------------------------------------------------------------------------

HRESULT
CMilChannel::EndCommand()
{
    HRESULT hr = S_OK;

    //
    // We can't end a multi-part command if we haven't started one
    //

    if (!m_fIsCommandOpen)
    {
        IFC(WGXERR_UCE_MISSINGBEGINCOMMAND);
    }

    EndItem();

    m_fIsCommandOpen = false;

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CMilChannel::CloseBatch
//
//    Synopsis:
//        Closes the current batch and creates a new one.
//
//------------------------------------------------------------------------------

HRESULT
CMilChannel::CloseBatch()
{
    HRESULT hr = S_OK;

    if (m_pCommands != NULL)
    {
        if (m_fIsCommandOpen)
        {
            IFC(WGXERR_UCE_MISSINGENDCOMMAND);
        }

        // This is needed for channel lookup across packet transports. In
        // proc the handle is null
        m_pCommands->SetChannel(GetChannel());

        m_pCommands->SetFreeIndex(m_idxFree);
        m_idxFree = 0;

        IFC(m_pClosedBatches.Add(m_pCommands));
        m_pCommands = NULL;
    }

  Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CMilChannel::Commit
//
//    Synopsis:
//        Sends all commands in a completed batch to the composition engine
//        for later processing.
//
//------------------------------------------------------------------------------

HRESULT
CMilChannel::Commit()
{
    HRESULT hr = S_OK;

    for (UINT i = 0; i < m_pClosedBatches.GetCount(); i++)
    {
        m_handleTable.FlushChannelHandles(m_pClosedBatches[i]->GetFreeIndex());

        Assert(m_pConnection);
        // SubmitBatch takes ownership of the batch, so we transfer it.
        IFC(m_pConnection->SubmitBatch(m_pClosedBatches[i]));
        m_pClosedBatches[i] = NULL;
    }
    m_pClosedBatches.Reset(FALSE);

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CMilChannel::EnsureSize
//
//    Synopsis:
//        Ensures that there is enough space to record a command packet of the
//        specified size.
//
//------------------------------------------------------------------------------

HRESULT
CMilChannel::EnsureSize(UINT cbSize)
{
    HRESULT hr = S_OK;

    //
    // Make sure we have a recorder to write to.
    //

    IFC(EnsureRecorder());

    //
    // Make sure there is enough space to record an item of the specified
    // length (along with any internal storage which may be needed
    // such as the size of the record).
    //

    IFC(m_pCommands->EnsureItem(cbSize));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CMilChannel::EnsureRecorder
//
//    Synopsis:
//        Ensures that we have a batch to record command packets into.
//
//------------------------------------------------------------------------------

HRESULT
CMilChannel::EnsureRecorder()
{
    HRESULT hr = S_OK;

    if (m_pCommands == NULL)
    {
        IFC(CMilCommandBatch::Create(INITIAL_BATCH_SIZE, &m_pCommands));

        Assert(m_pCommands != NULL);
    }

  Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CMilChannel::BeginItem
//
//    Synopsis:
//        Writes the header for a new command packet to the command recorder.
//
//------------------------------------------------------------------------------

HRESULT
CMilChannel::BeginItem()
{
    Assert(m_pCommands);
    RRETURN(m_pCommands->BeginItem());
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CMilChannel::AddItemData
//
//    Synopsis:
//        Writes additional data for a command packet to the command recorder.
//
//------------------------------------------------------------------------------

HRESULT
CMilChannel::AddItemData(
    __in_bcount(cbSize) VOID *pData,
    UINT cbSize
    )
{
    Assert(m_pCommands);
    RRETURN(m_pCommands->AddItemData(pData, cbSize));
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CMilChannel::EndItem
//
//    Synopsis:
//        Wraps up writing of a command packet to the command recorder.
//
//------------------------------------------------------------------------------

HRESULT
CMilChannel::EndItem()
{
    Assert(m_pCommands);
    RRETURN(m_pCommands->EndItem());
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CMilClientChannel::SetNotificationWindow
// 
//    Synopsis:
//        Selects an HWND to be notified whenever back-channel messages are
//        available. The notification is sent only when the back-channel queue
//        transitions from being empty to being non-empty. The caller is then
//        responsible for either emptying the queue when the notification is
//        received, or remembering that there may be additional items awaiting
//        processing.
//
//------------------------------------------------------------------------------

HRESULT
CMilChannel::SetNotificationWindow(HWND hwnd, UINT message)
{
    CGuard<CCriticalSection> oGuard(m_csQueue);

    m_hNotificationWindow = hwnd;
    m_nNotificationMessage = message;

    return S_OK;
}


//+-----------------------------------------------------------------------------
//
//    Member:
//        CMilChannel::Create
//
//    Synopsis:
//        Creates and initializes a channel on a connection.
//
//------------------------------------------------------------------------------

HRESULT
CMilChannel::Create(
    __in_ecount(1) CMilConnection *pConnection,
    HMIL_CHANNEL hChannel,
    __out_ecount(1) CMilChannel **ppChannel
    )
{
    HRESULT hr = S_OK;

    CMilChannel *pChannel = new CMilChannel(
        pConnection,
        hChannel
        );
    IFCOOM(pChannel);

    //
    // At this point we have a reference to the new object. If everything
    // succeeds we will AddRef to give a reference to our caller. At the end
    // of the method we Release our own reference. On failure, that will also
    // delete the new object.
    //

    IFC(pChannel->Initialize());

    *ppChannel = pChannel;
    pChannel->AddRef();

Cleanup:
    ReleaseInterface(pChannel);

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//    Member:
//        CMilChannel::Destroy
//
//    Synopsis:
//        Removes the channel from the connection and releases this object.
//
//------------------------------------------------------------------------------

HRESULT
CMilChannel::Destroy()
{
    HRESULT hr = S_OK;

    //
    // Wait for all commands in the channel before returning. This will ensure 
    // that all delete cmds in the channel are handled. If the channel was in
    // zombie state, we don't have to wait for the pending commands to execute 
    // as they are going to be ignored by the target (zombie) partition anyway.
    //
    IGNORE_HR(SyncFlush());

    //
    // Tell the transport to remove the channel on the server side.
    //
    MIL_THR(m_pConnection->DestroyChannel(m_hChannel));

    //
    // When we are done with the channel release our reference.
    //    
    Release();

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//    Member:
//        CMilChannel::SyncFlush
//
//    Synopsis:
//        Sends all commands submitted to the channel to this point to the
//        composition engine and waits until the composition engine receives
//        and processes the commands.
//
//------------------------------------------------------------------------------

HRESULT
CMilChannel::SyncFlush()
{
    HRESULT hr = S_OK;

    //
    // The channel is not thread safe by design. The assumption is SyncFlush
    // can not be called while we have a sync flush pending.
    //
    Assert(!m_fWaitingForSyncFlush);

    m_fWaitingForSyncFlush = true;

    IFC(m_pConnection->SynchronizeChannel(GetChannel()));

    {
        //
        // Let the caller know that the partition that the corresponding server
        // channel is attached to has been zombied (it could happen have
        // happened while waiting for the sync flush to be completed).
        //
        // Note that IFC will no-op if m_hrZombie is a success code.
        //

        IFC(m_hrZombie);
    }

Cleanup:
    m_fWaitingForSyncFlush = false;

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CMilChannel::PostMessageToChannel
//
//    Synopsis:
//        Called by the notification transport upon receiving a back channel
//        message.
//
//------------------------------------------------------------------------------

HRESULT
CMilChannel::PostMessageToChannel(
    __in_ecount(1) const MIL_MESSAGE *pmsg
    )
{
    HRESULT hr = S_OK;

    LOCAL_MESSAGE_QUEUE *p = NULL;

    IFC(HrAlloc(
        Mt(CMilChannelMessage),
        sizeof(LOCAL_MESSAGE_QUEUE),
        (VOID**)&p
        ));

    //
    // copy the message content.
    //

    p->msg = *pmsg;


    m_csQueue.Enter();

    //
    // If the list is going from empty to non-empty and the owner of the
    // channel requested window notifications then post the message now. Note
    // that it is a post, not a send, so we can do it from within the lock.
    //

    if (IsListEmpty(&m_MessageQueue) && m_hNotificationWindow != NULL)
    {
        IGNORE_W32(FALSE, ::PostMessage(m_hNotificationWindow, m_nNotificationMessage, 0, 0));
    }

    //
    // push the entry into the back of the queue.
    //

    InsertTailList(&m_MessageQueue, p);

    //
    // Set the queue event, releasing anyone waiting on the message queue.
    //

    SetEvent(m_eventQueue);

    m_csQueue.Leave();

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CMilChannel::PeekNextMessage
//
//    Synopsis:
//        Examines the back-channel queue for messages. If any are available,
//        the first one is returned to the caller. If no messages are available
//        then the method returns immediately.
//
//------------------------------------------------------------------------------

HRESULT
CMilChannel::PeekNextMessage(
    __out_bcount_part(cbSize, sizeof(MIL_MESSAGE)) MIL_MESSAGE *pmsg,
    size_t cbSize,
    __out_ecount(1) BOOL *pfMessageRetrieved
    )
{
    //
    // NOTE: The caller is responsible for flushing the channel before
    //       calling this method, if appropriate.
    //       WaitForNextMessage will do a Flush at the beginning, so
    //       any commands that are pending before the client goes to
    //       sleep will get flushed.
    //

    //
    // The message queue access must be protected by the critical section
    // because we're going to post to it from a different thread. The
    // queue/list operations we use are not atomic so they must be
    // protected.
    //

    m_csQueue.Enter();

    if (IsListEmpty(&m_MessageQueue))
    {
        *pfMessageRetrieved = FALSE;
        RtlZeroMemory(pmsg, min(cbSize, sizeof(MIL_MESSAGE)));
    }
    else
    {
        //
        // Remove the oldest message from the list.
        //

        LOCAL_MESSAGE_QUEUE *p = (LOCAL_MESSAGE_QUEUE*)RemoveHeadList(
            &m_MessageQueue
            );

        Assert(p != NULL);

        //
        // Copy the message to the callers buffer and free the queue item.
        //

        RtlCopyMemory(pmsg, &p->msg, min(cbSize, sizeof(MIL_MESSAGE)));
        FreeHeap(p);

        *pfMessageRetrieved = TRUE;
    }

    m_csQueue.Leave();

    return S_OK;
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CMilChannel::WaitForNextMessage
//
//    Synopsis:
//        Waits until either messages are available in the back-channel queue
//        or the specified set of handles becomes signaled. If any messages are
//        already available when this method is called then the method returns
//        immediately.
//
//------------------------------------------------------------------------------

HRESULT
CMilChannel::WaitForNextMessage(
    __range(0, MAXIMUM_WAIT_OBJECTS - 1) DWORD nCount,
    __in_ecount(nCount) const HANDLE *pHandles,
    BOOL bWaitAll,
    DWORD waitTimeout,
    __out_ecount(1) DWORD *pWaitReturn
    )
{
    HRESULT hr = S_OK;

    //
    // Make sure that the number of handles passed in is reasonable.
    //

    if (nCount > MAXIMUM_WAIT_OBJECTS - 1) // will wait for nCount + 1 events
    {
        RIP("Too many wait objects specified.");
        IFC(E_INVALIDARG);
    }

    //
    // Trigger a crash proactively if the current partition has been
    // zombied for any reason whatsoever. This prevents this method from
    // becoming non-responsive at WaitForMultipleObjects further down which can never be
    // signaled from a zombied partition.
    //
    IFC(m_hrZombie);

    IFC(CloseBatch());
    IFC(Commit());

    //
    // The message queue access must be protected by the critical section
    // because we're going to post to it from a different thread. The
    // queue/list operations we use are not atomic so they must be
    // protected.
    //

    m_csQueue.Enter();

    //
    // If there is a message waiting in the queue, don't bother waiting for
    // the event. This allows us to drain the queue without waiting. Note
    // we clear the event in the case of an empty list so that we can
    // wait on an empty queue without immediately satisfying the wait.
    //

    if (IsListEmpty(&m_MessageQueue))
    {
        //
        // The list is empty and we're under the critical section so nobody
        // can be currently posting to the queue. Ensure that the event is
        // cleared and then wait for someone to post a new message to the
        // queue. The event needs to be cleared because we skip waiting on
        // the event (and hence having it auto-reset) when we determine that
        // there is already stuff to do when we enter this function.
        //

        ResetEvent(m_eventQueue);

        HANDLE handles[MAXIMUM_WAIT_OBJECTS];
        memcpy(handles, pHandles, sizeof(HANDLE) * nCount);
        handles[nCount++] = m_eventQueue;

        m_csQueue.Leave();

        IFCW32X(*pWaitReturn,
                WAIT_FAILED,
                ::WaitForMultipleObjects(nCount, handles, bWaitAll, waitTimeout)
                );
    }
    else
    {
        m_csQueue.Leave();
        *pWaitReturn = nCount;
    }

Cleanup:
    RRETURN(hr);
}


