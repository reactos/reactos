// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*++--------------------------------------------------------------------------



Module Name:
     CConnectionContext.cpp

  Defines the interface class for transporting data to slave devices
  and defines a basic implementation of the class.
------------------------------------------------------------------------*/
#include "precomp.hpp"

MtDefine(CConnectionContext, Mem, "CConnectionContext");
MtDefine(CRecorderConnectionContext, Mem, "CRecorderConnectionContext");

ExternTag(tagMILConnectionCtx);
ExternTag(tagMILTransportBackwardTraffic);

//----------------------------------------------
// CConnectionContext::CConnectionContext
//
//  - pTransport: notification interface given to the server channels as a message sink. This is a pointer to
//                the api used by the server channel to post messages back to the client channel.
//  - pSurfManager: interface that abstracts access to sprite layer
//  - pOwningRDP: owning rdp adaptor passed to the partition(CComposition) so resources can determine
//                  if they are being remoted. 
//                  This should be changed to a flag in the future
//----------------------------------------------


CConnectionContext::CConnectionContext(
    MilMarshalType::Enum mType,
    __in CMilConnection *pNotifTransport
    ) : m_channelTable(sizeof(SERVER_CHANNEL_HANDLE_ENTRY))
{
    m_mType = mType;
    AddRef();

    m_pTransportNoRef = pNotifTransport;
}

CConnectionContext::~CConnectionContext()
{
}

//----------------------------------------------
// CConnectionContext::EnsureRecorder
//
// the recorder is used during disconnects to enqueue
// close and sync commands directly into the server channel.
//----------------------------------------------

HRESULT
CConnectionContext::EnsureRecorder()
{
    HRESULT hr = S_OK;

    // The command batch and the event are used to execute a channel
    // shutdown from the rdp side of the transport, without commands coming from
    // the pipe. We do this when the ts plugin tells us that the session has lost
    // connection.

    if (m_hSyncFlushEvent == NULL)
    {
        IFCW32(m_hSyncFlushEvent = CreateEvent(NULL, FALSE, FALSE, NULL));
    }

    if (m_pCommands == NULL)
    {
        IFC(CMilCommandBatch::Create(INITIAL_BATCH_SIZE, &m_pCommands));

        Assert(m_pCommands != NULL);
    }

  Cleanup:
    RRETURN(hr);
}

//----------------------------------------------
// CConnectionContext::GetExistingComposition
//
// when opening channels, this function is used to
// get an existing composition
//----------------------------------------------

HRESULT
CConnectionContext::GetExistingComposition(
    HMIL_CHANNEL hChannel,
    CComposition **ppComposition
    )
{
    HRESULT hr = S_OK;
    SERVER_CHANNEL_HANDLE_ENTRY *pServerEntry = NULL;

    IFC(m_channelTable.GetServerChannelTableEntry(hChannel, &pServerEntry));
    *ppComposition = pServerEntry->pCompDevice;

Cleanup:
    RRETURN(hr);
}

//----------------------------------------------
// CConnectionContext::AssignChannelInTable
//
// create a new entry in the channel table
//----------------------------------------------

HRESULT
CConnectionContext::AssignChannelInTable(
    HMIL_CHANNEL hChannel,
    HMIL_CHANNEL hSourceChannel,
    CMilServerChannel *pmilChannel,
    CComposition *pCompDevice)
{
    HRESULT hr = S_OK;
    SERVER_CHANNEL_HANDLE_ENTRY *pServerEntry = NULL;
    BOOL fDestroyHandleOnFail = FALSE;

    // inserts the channel at its assigned handle location.
    IFC(m_channelTable.AssignChannelEntry(hChannel));

    // If we fail passed this point we need to destroy the entry that was
    // allocated
    fDestroyHandleOnFail = TRUE;
    IFC(m_channelTable.GetServerChannelTableEntry(hChannel, &pServerEntry));

    pServerEntry->pCompDevice = pCompDevice;
    if (pCompDevice)
    {
        pCompDevice->AddRef();
    }
    pServerEntry->pServerChannel = pmilChannel;
    pmilChannel->AddRef();

    pServerEntry->hSourceChannel= hSourceChannel;

    m_nrChannels++;
Cleanup:

    if (FAILED(hr) && fDestroyHandleOnFail)
    {
        m_channelTable.DestroyHandle(hChannel);
    }

    RRETURN(hr);
}

//----------------------------------------------
// CConnectionContext::RemoveChannelFromTable
//
// This function is called on a connection disconnect. The channel is
// removed from the channel table and all the resources associated with
// this channel are released.
//----------------------------------------------

HRESULT
CConnectionContext::RemoveChannelFromTable(HMIL_CHANNEL hChannel)
{
    HRESULT hr = S_OK;
    SERVER_CHANNEL_HANDLE_ENTRY *pServerEntry = NULL;

    IFC(m_channelTable.GetServerChannelTableEntry(hChannel, &pServerEntry));

    // need to tell this partition to remove its rt's
    ReleaseInterface(pServerEntry->pCompDevice);
    ReleaseInterface(pServerEntry->pServerChannel);

    // remove handle from the channel table.
    m_channelTable.DestroyHandle(hChannel);

    m_nrChannels--;
Cleanup:
    RRETURN(hr);
}

//----------------------------------------------
// CConnectionContext::OpenChannel
//
// Opens a channel on the connection
//   - hChannelIn: the handle of the newly created channel this handle was allocated
//                 client side and the method associates the newly created channel
//                 to this handle.
//   - hSourceChannel: if not NULL it is the handle of the channel we use to
//                     find the partition this channel will run on. If NULL we 
//                     will create a new partition for this channel
//   - pRDPAdaptor: owning rdp adaptor passed to the partition(CComposition) so resources can determine
//                  if they are been remoted. Should be changed to a flag.
//   - pSurfManager: used during remoting to recover remoted sprites.
//----------------------------------------------

HRESULT CConnectionContext::OpenChannel(
    HMIL_CHANNEL hChannel,
    HMIL_CHANNEL hSourceChannel
    )
{
    HRESULT hr = S_OK;
    CComposition *pOwningComposition = NULL;
    CMilServerChannel *pChannel = NULL;
    bool fAssignedChannel = false;
    CMilCommandBatch *pPartitionCommand = NULL;

    IFC(CMilCommandBatch::Create(&pPartitionCommand));

    pPartitionCommand->m_commandType = PartitionCommandOpenChannel;

    IFC(GetOwningComposition(
        hSourceChannel,
        &pOwningComposition
        ));

    IFC(CMilServerChannel::Create(
        this,
        pOwningComposition,
        hChannel,
        &pChannel
        ));

    IFC(AssignChannelInTable(
        hChannel,
        hSourceChannel,
        pChannel,
        pOwningComposition
        ));
        
    fAssignedChannel = true;

    //
    // Attach the channel to its composition device.
    // If we fail here we return the error and rely on the 
    // controlling code on the client side to properly manage
    // the channel table integrity. This method is currently synchronous.
    // it modifies a table looked up on the render thread taking a crit sect.
    // The table should be manipulated by marshalling commands into the change queue.
    //
    pPartitionCommand->SetChannel(hChannel);
    pPartitionCommand->SetChannelPtr(pChannel);


    // Note that the ownership of the command batch is transferred to the connection
    // in this call. Hence the connection is responsible for cleaning up the batch
    // even on failure.

    hr = pOwningComposition->SubmitBatch(pPartitionCommand);
    pPartitionCommand = NULL;
    IFC(hr);
        
Cleanup:

    // GSchneid: This code does not look robust because it can leave a channel entry in the
    // channel table and still be unable to create the matching data structures on the composition 
    // side leaving the engine in an inconsistent state. 
    
    if (FAILED(hr) && fAssignedChannel)
    {
        CMilCommandBatch *pPartitionCommandRetry = NULL;
        
        MIL_THR_SECONDARY(CMilCommandBatch::Create(&pPartitionCommandRetry));

        if (!pPartitionCommandRetry)
        {
            MIL_THR_SECONDARY(E_OUTOFMEMORY);
        }
        else
        {
            pPartitionCommandRetry->SetChannel(hChannel);
            pPartitionCommandRetry->m_commandType = PartitionCommandCloseChannel;

            // Note that the ownership of the command batch is transferred to the connection
            // in this call. Hence the connection is responsible for cleaning up the batch
            // even on failure.
            MIL_THR_SECONDARY(pOwningComposition->SubmitBatch(pPartitionCommandRetry));
            pPartitionCommandRetry = NULL;
            
        }

        delete pPartitionCommandRetry;
    }

    ReleaseInterface(pChannel);
    ReleaseInterface(pOwningComposition);
    delete pPartitionCommand;
   
    RRETURN(hr);
}

//----------------------------------------------
//
// CConnectionContext::CloseChannel
//
//----------------------------------------------

HRESULT CConnectionContext::CloseChannel(
    HMIL_CHANNEL hChannel
    )
{
    HRESULT hr = S_OK;
    CMilServerChannel *pChannel = NULL;
    CComposition *pOwningComposition = NULL;

    CMilCommandBatch *pPartitionCommand = NULL;

    IFC(CMilCommandBatch::Create(&pPartitionCommand));

    pPartitionCommand->m_commandType = PartitionCommandCloseChannel;

    //
    // Detach the channel from its composition device.
    //

    IFC(GetServerChannel(hChannel, &pChannel));
    
    pOwningComposition = pChannel->GetComposition();
    Assert(pOwningComposition);

    pPartitionCommand->SetChannel(hChannel);

    //
    // Note that we purposedly set the channel pointer to NULL -- as 
    // we are closing the channel, we are not really interested in 
    // receiving any future notifications (zombie, etc.).
    //

    // Note that the ownership of the command batch is transferred to the connection
    // in this call. Hence the connection is responsible for cleaning up the batch
    // even on failure.
    Assert(pPartitionCommand->GetChannelPtr() == NULL);
    pOwningComposition->SubmitBatch(pPartitionCommand);
    pPartitionCommand = NULL;

Cleanup:

    delete pPartitionCommand;

    //
    // remove the channel from the table and marshal a close
    // command to the render thread.
    //

    MIL_THR(RemoveChannelFromTable(hChannel));
    RRETURN(hr);
}


//----------------------------------------------
// CConnectionContext::SendBatchToChannel
//
// 
// Extracts a batch out of the packet and submits it to the channel
//----------------------------------------------

HRESULT CConnectionContext::SendBatchToChannel(
    HMIL_CHANNEL hChannel,
    __in_ecount(1) CMilCommandBatch *pBatch
    )
{
    HRESULT hr = S_OK;
    CMilServerChannel *pChannel = NULL;

    Assert(pBatch != NULL);

    //
    // Look up the channel by handle
    //

    IFC(GetServerChannel(hChannel, &pChannel));

    //
    // Now submit the batch, transferring ownership of the memory to the
    // destination device.
    //

    hr = pChannel->SubmitBatch(pBatch);
    pBatch = NULL;
    IFC(hr);
    

Cleanup:
    delete pBatch;
    RRETURN(hr);
}

//----------------------------------------------
// CConnectionContext::CloseChannelForced
//
// This function is called to close the channel. It
// sends a close command followed by a sync flush command. It is used 
// by the host of the composition render thread side objects to force a 
// a clean shutdown in the absence of communication with the ui side components.
//----------------------------------------------

HRESULT CConnectionContext::CloseChannelForced(HMIL_CHANNEL hChannel, BOOL fCleanResource)
{
    HRESULT hr = S_OK;
    SERVER_CHANNEL_HANDLE_ENTRY *pServerEntry = NULL;
    MILCMD_TRANSPORT_SYNCFLUSH cmdFlush = { MilCmdTransportSyncFlush };
    MILCMD_TRANSPORT_DESTROYRESOURCESONCHANNEL cmdDestroy = { MilCmdTransportDestroyResourcesOnChannel };

    IFC(m_channelTable.GetServerChannelTableEntry(hChannel, &pServerEntry));

    IFC(EnsureRecorder());

    // send channel close command to partition

    // if it is time to clean out all resources marshal the MilCmdDeviceDestroyChannelReleaseAllResources
    // command. The assumption made is that all channels corresponding to client composition context
    // have one corresponding CComposition (partition).
    cmdDestroy.hChannel = hChannel;
    //
    // Make sure there is enough space to record an item of the specified
    // length (along with any internal storage which may be needed
    // such as the size of the record).
    //
    IFC(m_pCommands->EnsureItem(sizeof(cmdDestroy)));

    IFC(m_pCommands->BeginAddEndItem(&cmdDestroy, sizeof(cmdDestroy)));

    IFC(m_pCommands->EnsureItem(sizeof(cmdFlush)));

    IFC(m_pCommands->BeginAddEndItem(&cmdFlush, sizeof(cmdFlush)));

    pServerEntry->pServerChannel->SetServerSideFlushEvent(m_hSyncFlushEvent);

    // Note that the ownership of the command batch is transferred to the connection
    // in this call. Hence the connection is responsible for cleaning up the batch
    // even on failure.
    hr = pServerEntry->pServerChannel->SubmitBatch(m_pCommands);
    m_pCommands = NULL;
    IFC(hr);

    // wait for channel delete and flush command to be completed.
    WaitForSingleObject(m_hSyncFlushEvent, INFINITE);

Cleanup:
    delete m_pCommands;
    
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//    Member:
//        CConnectionContext::PresentAllPartitions
//
//    Synopsis: 
//        Presents all partitions in a same thread connection context. This is used 
//        to trigger batch processing and rendering passes on synchronous compositors 
//        attached to this connection context. Note that presentation is not necessary 
//        as the rendering results will be accessed through a generic render target's 
//        IMILRenderTargetBitmap). See WPF's BitmapVisualManager for example of use.
//
//------------------------------------------------------------------------------

HRESULT
CConnectionContext::PresentAllPartitions()
{
    HRESULT hr = S_OK;
    SERVER_CHANNEL_HANDLE_ENTRY *pServerEntry = NULL;

    // if we have no channels left bail.
    if (m_nrChannels == 0)
    {
        goto Cleanup;
    }

    for (HMIL_CHANNEL hChannel = 1; hChannel < m_channelTable.m_cHandleCount; hChannel++)
    {
        if (!m_channelTable.ValidEntry(hChannel))
        {
            continue;
        }

        IFC(m_channelTable.GetServerChannelTableEntry(hChannel, &pServerEntry));

        if (pServerEntry->hSourceChannel == NULL)
        {
            //
            // We need this guard since all the rendering
            // is supposed to work with single floating point
            // precision. In case when synchronous device
            // will need default double settings (say, for codecs)
            // then "CDoubleFPU oGuard" should be used on
            // corresponding call (2003/11/17 mikhaill)
            //

            CFloatFPU oGuard;

            //
            //  rgbrast can fail to render 3D content with 
            //  WGXERR_DISPLAYSTATEINVALID if a mode change has occurred between creating the 
            //  D3D device and render time. In such case, we will attempt to update the display 
            //  set and render again.
            //

            const UINT cMaxComposeAttempts = 3;

            for (UINT cComposeAttempt = 0; cComposeAttempt < cMaxComposeAttempts; ++cComposeAttempt) 
            {
                bool fPresentNeeded = false;

                MIL_THR(pServerEntry->pCompDevice->Compose(&fPresentNeeded));

                if (hr != WGXERR_DISPLAYSTATEINVALID) 
                {
                    break;
                }

                Sleep(500); // sleep to let the system stabilize after the mode change
            }

            if (FAILED(hr))
            {
                goto Cleanup; // break from the loop if all attempts to compose have failed
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

//----------------------------------------------
// CConnectionContext::ShutDownAllChannels
//
// This function is called on a connection disconnect. All channels will be destroyed. It is used 
// by the host of the composition render thread side objects to force a 
// a clean shutdown in the absence of communication with the ui side components.
//----------------------------------------------

HRESULT
CConnectionContext::ShutDownAllChannels()
{
    HRESULT hr = S_OK;
    SERVER_CHANNEL_HANDLE_ENTRY *pServerEntry = NULL;
    UINT nrChannels = 0;
    UINT nrTotalChannels = m_nrChannels;

    // if we have no channels left bail.
    if (m_nrChannels == 0)
    {
        goto Cleanup;
    }

    for (HMIL_CHANNEL hChannel = 1; hChannel < m_channelTable.m_cHandleCount; hChannel++)
    {
        if (!m_channelTable.ValidEntry(hChannel))
        {
            continue;
        }

        IFC(m_channelTable.GetServerChannelTableEntry(hChannel, &pServerEntry));

        // if we are force shutting down the last channel of this process connection
        // we need to clean out the resources from its corresponding slave table. This needs to 
        // happen on the composition thread.
        if(FAILED((CloseChannelForced(hChannel, (nrChannels == nrTotalChannels - 1)))))
        {
            Assert(FALSE);
            continue;
        }

        nrChannels++;

        //
        // Detach the channel from its composition device.
        //

        IFC(CloseChannel(hChannel));
    }

Cleanup:
    RRETURN(hr);
}

//----------------------------------------------
// CConnectionContext::GetOwningComposition
//
// called to get the owning composition for a channel that is about
// to be created.
//----------------------------------------------

HRESULT
CConnectionContext::GetOwningComposition(
    HMIL_CHANNEL hSourceChannel,
    __out_ecount(1) CComposition **ppComposition
    )
{
    HRESULT         hr = S_OK;
    CComposition    *pComposition = NULL;

    if (hSourceChannel)
    {
        //
        // Use the same composition object as the one associated with the
        // source channel
        //

        SERVER_CHANNEL_HANDLE_ENTRY *pServerSourceEntry = NULL;
        IFC(m_channelTable.GetServerChannelTableEntry(hSourceChannel, &pServerSourceEntry));
        pComposition = pServerSourceEntry->pCompDevice;
        pComposition->AddRef();
    }
    else
    {
        //
        // Create a new composition object for this channel
        //

        if (m_mType == MilMarshalType::SameThread)
        {
            CSameThreadComposition *pSameThreadComposition = NULL;

            IFC(CSameThreadComposition::Create(
                m_mType,
                &pSameThreadComposition
                ));

            pComposition = pSameThreadComposition;
        }
        else
        {
            CCrossThreadComposition *pCrossThreadComposition = NULL;

            IFC(CCrossThreadComposition::Create(
                m_mType,
                &pCrossThreadComposition
                ));

            pComposition = pCrossThreadComposition;
        }
    }

    Assert(pComposition);
    *ppComposition = pComposition;

Cleanup:
    RRETURN(hr);
}

//
// This method is used for an optimization that allows client channels opened for 
// the cross thread transport to send batches directly to the server channels.
// 
HRESULT
CConnectionContext::GetServerChannel(HMIL_CHANNEL hChannel, __out CMilServerChannel **ppServerChannel)
{
    RRETURN(m_channelTable.GetServerChannel(hChannel, ppServerChannel));
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CConnectionContext::PostMessageToClient
//
//  Synopsis:
//      Channels call this method to post messages back to the app. The calling
//      code owns the lifetime of the input packet
//
//----------------------------------------------------------------------------

HRESULT
CConnectionContext::PostMessageToClient(
    __in const MIL_MESSAGE *pMsg,
    HMIL_CHANNEL hChannel
    )
{
    RRETURN(m_pTransportNoRef->PostMessageToClient(pMsg, hChannel));
}






