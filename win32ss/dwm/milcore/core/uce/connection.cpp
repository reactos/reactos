// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
// 

// 
//  Abstract:
//      Client-side connection holds to a command transport implementation,
//      providing a back-channel end-point and managing transport channels.
// 
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CMilConnection, Mem, "CMilConnection");

ExternTag(tagMILConnection);

//+---------------------------------------------------------------------------
//
//  Member:
//      CMilConnection::CMilConnection
//
//  Synopsis:
//      Default constructor for a CMilConnection object.
//
//----------------------------------------------------------------------------

CMilConnection::CMilConnection()
{
    // Zero-initialized on creation by DECLARE_METERHEAP_CLEAR
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CMilConnection::~CMilConnection
//
//  Synopsis:
//      Destructor for a CMilConnection object.
//
//----------------------------------------------------------------------------

CMilConnection::~CMilConnection()
{
    ReleaseInterface(m_pConnectionContext);
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CMilConnection::Create
//
//----------------------------------------------------------------------------

HRESULT CMilConnection::Create(
    __in MilMarshalType::Enum marshalType,
    __out CMilConnection** ppConnection)
{   
    HRESULT hr = S_OK;
    CMilConnection *pConnection = NULL;

    pConnection = new CMilConnection();
    IFCOOM(pConnection);
    
    pConnection->AddRef();
    
    IFC(pConnection->InitializeClientTransport(marshalType));

    *ppConnection = pConnection;
    pConnection = NULL;
    
Cleanup:
    ReleaseInterface(pConnection);
    
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:
//      CMilConnection::InitializeClientTransport
//
//  Synopsis:
//      Initializes data structures needed for establishing and maintaining
//      a connection with a composition engine and opens a low-level
//      connection.
//
//  Return value:
//      WGXERR_UCE_UNSUPPORTEDTRANSPORTVERSION
//          The composition engine to which we attempted to connect does not
//          support the same protocol version as we do.
//
//----------------------------------------------------------------------------

HRESULT
CMilConnection::InitializeClientTransport(MilMarshalType::Enum marshalType)
{
    HRESULT hr = S_OK;

    m_marshalType = marshalType;

    //
    // Initialize the critical section used to protect access to the channel
    // handle table and the table itself.
    //
    IFC(m_csConnection.Init());
    IFC(m_channelTable.Initialize());

    //
    // Initialize connection context.
    //
    IFCOOM(m_pConnectionContext = new CConnectionContext(m_marshalType, this));

Cleanup:

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CMilConnection::ShutdownClientTransport
//
//  Synopsis:
//      Closes all connections managed by this transport and cleans up
//      internal data structures.
//
//----------------------------------------------------------------------------

VOID
CMilConnection::ShutdownClientTransport()
{
    if (m_pConnectionContext)
    {
        // Ignoring HR since this code can currently not handle HRESULT failures.
        // Not to critical since we are shutting down anyway.
        IGNORE_HR(m_pConnectionContext->ShutDownAllChannels());
        ReleaseInterface(m_pConnectionContext);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:
//      CMilConnection::CreateChannelHelper
//
//  Synopsis:
//      Internal helper method that creates a channel at given handle location.
//
//----------------------------------------------------------------------------

HRESULT
CMilConnection::CreateChannelHelper(
    HMIL_CHANNEL hChannel,
    HMIL_CHANNEL hChannelSource,
    __in_ecount(1) CLIENT_CHANNEL_HANDLE_ENTRY *pEntry,
    __out_ecount(1) CMilChannel **ppChannel
    )
{
    HRESULT hr = S_OK;
    CMilChannel *pChannel = NULL;
    bool serverChannelCreated = false;

    // 
    // Open a channel on the server.
    //
    IFC(m_pConnectionContext->OpenChannel(hChannel, hChannelSource));
    serverChannelCreated = true;

    //
    // Create the client channel matching the server channel.
    //
    IFC(CMilChannel::Create(this, hChannel, &pChannel));

    // take a reference corresponding to the channel being in the channel table.
    pEntry->pMilChannel = pChannel;
    pChannel->AddRef();
    
    *ppChannel = pChannel;
    pChannel = NULL;

Cleanup:
    //
    // If we failed, make sure not to leak a server-side channel end-point
    //

    if (FAILED(hr) && serverChannelCreated)
    {
        IGNORE_HR(m_pConnectionContext->CloseChannel(hChannel));
    }

    ReleaseInterface(pChannel);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CMilConnection::CreateChannel
//
//  Synopsis:
//      Creates a channel over the connection maintained by this transport.
//
//----------------------------------------------------------------------------

HRESULT
CMilConnection::CreateChannel(
    HMIL_CHANNEL hChannelSource,
    __out_ecount(1) CMilChannel **ppChannel
    )
{
    CGuard<CCriticalSection> oGuard(m_csConnection);
    HRESULT hr = S_OK;
    HMIL_CHANNEL hChannel = NULL;
    CLIENT_CHANNEL_HANDLE_ENTRY *pEntry = NULL;

    IFC(m_channelTable.GetNewChannelEntry(&hChannel, &pEntry));

    IFC(CreateChannelHelper(hChannel, hChannelSource, pEntry, ppChannel));

    TraceTag((tagMILConnection, 
              "CMilConnection::CreateChannel: connection 0x%08p created at handle 0x%08p, object 0x%08p",
              this,
              hChannel, 
              *ppChannel
              ));

Cleanup:

    if (FAILED(hr))
    {       
        // if we have failed remove the handle if necessary.
        if (hChannel)
        {
            m_channelTable.DestroyHandle(hChannel);
        }
    }        

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CMilConnection::DestroyChannel
//
//  Synopsis:
//      Removes the specified channel from the list of channels managed by
//      this transport and sends a command over the connection to instruct
//      the composition engine to release its receiving channel object.
//
//----------------------------------------------------------------------------

HRESULT
CMilConnection::DestroyChannel(HMIL_CHANNEL hChannel)
{
    CGuard<CCriticalSection> oGuard(m_csConnection);
    HRESULT hr = S_OK;
    CLIENT_CHANNEL_HANDLE_ENTRY *pMasterEntry = NULL;

    IFC(m_channelTable.GetMasterTableEntry(hChannel, &pMasterEntry));

    TraceTag((tagMILConnection, 
              "CMilConnection::DestroyChannel: connection 0x%08p destroyed at handle 0x%08p, object 0x%08p",
              this,
              hChannel, 
              pMasterEntry->pMilChannel
              ));

    //
    // Remove the channel table references
    //

    Assert(pMasterEntry->pMilChannel);
    ReleaseInterface(pMasterEntry->pMilChannel);
    m_channelTable.DestroyHandle(hChannel);

    //
    // Remove the server end-point of the channel
    //
    IFC(m_pConnectionContext->CloseChannel(hChannel));

Cleanup:

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CMilConnection::SynchronizeChannel
//
//  Synopsis:
//      Sends a control command to the composition engine and blocks the
//      calling thread until the composition engine processes it. This method
//      also flushes all commands pending on the specified channel.
//
//----------------------------------------------------------------------------

HRESULT
CMilConnection::SynchronizeChannel(HMIL_CHANNEL hChannel)
{
    HRESULT hr = S_OK;
    DWORD dwWaitResult = WAIT_FAILED;
    CLIENT_CHANNEL_HANDLE_ENTRY *pMasterEntry = NULL, masterEntry;
    MILCMD_TRANSPORT_SYNCFLUSH cmd = {MilCmdTransportSyncFlush};

    {
        m_csConnection.Enter();

        MIL_THR(m_channelTable.GetMasterTableEntry(hChannel, &pMasterEntry));
        if (FAILED(hr))
        {
            m_csConnection.Leave();
            goto Cleanup;
        }
        masterEntry = *pMasterEntry;
        m_csConnection.Leave();
    }

    // Send sync flush request to compositor.

    IFC(masterEntry.pMilChannel->SendCommand(&cmd, sizeof(cmd)));

    IFC(masterEntry.pMilChannel->CloseBatch());
    IFC(masterEntry.pMilChannel->Commit());

    {
        //
        //   Infinite wait on a single object is
        // dangerous. This should probably be a wait on multiple objects, like
        // the event and the thread that is supposed to signal the event.  If
        // the thread dies, we would stop waiting and could return an error.
        //
        

        IFCW32X(dwWaitResult, 
                WAIT_FAILED, // failureCode
                ::WaitForSingleObject(masterEntry.hSyncFlushEvent, INFINITE)
                );

        switch (dwWaitResult) 
        {
        case WAIT_OBJECT_0:
            // No-op. The wait has completed successfully.
            break;

        case WAIT_ABANDONED:
            IFC(WGXERR_UCE_CHANNELSYNCABANDONED);
            break;

        case WAIT_TIMEOUT:
            IFC(WGXERR_UCE_CHANNELSYNCTIMEDOUT);
            break;

        default:
            RIP("Unexpected sync flush wait result.");
            IFC(E_UNEXPECTED);
            break;
        }
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:
//      CMilConnection::SubmitBatch
//
//  Synopsis:
//      Submits a batch of commands to the composition engine.
//
//----------------------------------------------------------------------------

HRESULT
CMilConnection::SubmitBatch(__in CMilCommandBatch *pBatch)
{
    HRESULT hr = S_OK;
    
    // Note that the ownership of the command batch is transferred to the connection
    // context with the following call. Hence the connection context is responsible 
    // for cleaning up the batch even on failure.
    IFC(m_pConnectionContext->SendBatchToChannel(pBatch->GetChannel(), pBatch));                     

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:
//      CMilTransport::PostMessageToClient
//
//  Synopsis:
//      This method is used to queue messages to the channels. In the cross thread
//      transport the channels call this directly from the server channel
//      CMilServerChannel::PostMessageToClient method. In the TS case
//      CMilServerChannel::PostMessageToClient sends the message via CMilRDPAdaptor::PostMessageToClient
//      which posts the message to the dynamic virtual channel. It is then read for the client in
//      CMilTsTransport::BackChannelThread. CMilTsTransport::BackChannelThread then uses 
//      this method to queue the message to the channel.
//
//
//
//----------------------------------------------------------------------------
HRESULT
CMilConnection::PostMessageToClient(
    __in const MIL_MESSAGE *pNotification,
    HMIL_CHANNEL hChannel
    )
{
    HRESULT hr = S_OK;

    CGuard<CCriticalSection> oGuard(m_csConnection);


    {
        CMilChannel *pChannelNoRef = NULL;
        CLIENT_CHANNEL_HANDLE_ENTRY *pMasterEntry = NULL;

        if (SUCCEEDED(m_channelTable.GetMasterTableEntry(hChannel, &pMasterEntry))) 
            // Ignoring messages for channels that have been destroyed and are no longer in the table!
        {
            pChannelNoRef = pMasterEntry->pMilChannel;

            switch (pNotification->type) 
            {
                case MilMessageClass::PartitionIsZombie:
                {
                    HRESULT hrZombie = pNotification->partitionIsZombieData.hrFailureCode;
                    pChannelNoRef->Zombie(hrZombie);
                    break;              
                }
                        
                case MilMessageClass::SyncFlushReply:
                    // for sync messages we need to signal the corresponding client channel.
                    {
                        HRESULT hrSyncFlush = (pNotification->syncFlushReplyData).hr;
                        if (FAILED(hrSyncFlush))
                        {
                            pChannelNoRef->Zombie(hrSyncFlush);
                        }
                        IFCW32(::SetEvent(pMasterEntry->hSyncFlushEvent));
                    }
                    break;

                default:
                    // Pass any other message directly to the target channel.
                    IFC(pMasterEntry->pMilChannel->PostMessageToChannel(pNotification));
                    break;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//    Member:
//        CMilConnection::PresentAllPartitions
// 
//------------------------------------------------------------------------------

HRESULT
CMilConnection::PresentAllPartitions()
{
    HRESULT hr = S_OK;

    if (m_marshalType == MilMarshalType::SameThread)
    {
        IFC(m_pConnectionContext->PresentAllPartitions());
    }
    else
    {
        RIP("CMilConnection::PresentAllPartitions can not present cross thread transport");
        IFC(E_UNEXPECTED);
    }

Cleanup:
    RRETURN(hr);
}



