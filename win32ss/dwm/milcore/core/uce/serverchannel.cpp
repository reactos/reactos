// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CMilServerChannel, MILRender, "CMilServerChannel");

ExternTag(tagMILServerChannel);

/*++

Routine Description:

    CMilServerChannel::Create

    Creates a channel server side stub for use over packet transports.

--*/

HRESULT
CMilServerChannel::Create(
    __in CConnectionContext *pTransport,
    __in IMilBatchDevice *pdevTarget,
    HMIL_CHANNEL hChannel,
    __out CMilServerChannel **ppChannel
    )
{
    HRESULT hr = S_OK;

    Assert(pdevTarget);

    CMilSlaveHandleTable *pHandleTable = NULL;
    CMilServerChannel *p = new CMilServerChannel(
        pTransport,
        pdevTarget,
        hChannel
        );
    IFCOOM(p);
    p->AddRef();


    pHandleTable = new CMilSlaveHandleTable();
    IFCOOM(pHandleTable);    
    pHandleTable->AddRef();

    p->m_pServerTable = pHandleTable;
    pHandleTable = NULL;
    
    *ppChannel = p;
    p = NULL;

Cleanup:
    ReleaseInterface(p);
    ReleaseInterface(pHandleTable);

    RRETURN(hr);
}

//================================================================================================================

CMilServerChannel::CMilServerChannel(
    __in CConnectionContext *pTransport,
    __in IMilBatchDevice *pdevTarget,
    HMIL_CHANNEL hChannel
    )
{
    SetInterface(m_pTransport, pTransport);
    m_pDevice = pdevTarget;
    m_hChannel = hChannel;
    m_pServerTable = NULL;
    m_hSyncFlushEvent = NULL;

    TraceTag((tagMILServerChannel, 
              "CMilServerChannel::CMilServerChannel: channel 0x%08p connection 0x%08p assigned at handle 0x%08p",
              this,
              m_pTransport,
              m_hChannel
              ));

}

CMilServerChannel::~CMilServerChannel()
{
    ReleaseInterface(m_pServerTable);
    ReleaseInterface(m_pTransport);
}

//================================================================================================================

HRESULT
CMilServerChannel::SubmitBatch(__in CMilCommandBatch* pBatch)
{
    pBatch->SetChannelPtr(this);
    RRETURN(m_pDevice->SubmitBatch(pBatch));
}

//================================================================================================================

VOID
CMilServerChannel::SignalFinishedFlush(HRESULT hrReported)
{
    HRESULT hr = S_OK;

    if (m_hSyncFlushEvent)
    {
        //
        // The sync flush event is used to synchronize the partition cleanup.
        // See CloseChannelForced for more details.
        //
        
        MIL_TW32(::SetEvent(m_hSyncFlushEvent));
        m_hSyncFlushEvent = NULL;
    }
    else
    {
        //
        // Build a channel notification with the sync flush reply message
        // and send it through the notification transport.
        //
        
        MIL_MESSAGE msg = { MilMessageClass::SyncFlushReply };

        msg.syncFlushReplyData.hr = hrReported;

        MIL_THR(PostMessageToChannel(&msg));
    }

    IGNORE_HR(hr);
}

//==============================================================================

VOID
CMilServerChannel::SetServerSideFlushEvent(HANDLE hSyncFlushEvent)
{
    m_hSyncFlushEvent = hSyncFlushEvent;
}

//------------------------------------------------------------------------------
//
//   CMilServerChannel::GetChannelTable
//
//   Used by the compositor to get the handle table used to translate
//   resource handles sent over this channel.
//
//------------------------------------------------------------------------------

CMilSlaveHandleTable*
CMilServerChannel::GetChannelTable()
{
    return m_pServerTable;
}

//==============================================================================

CComposition *CMilServerChannel::GetComposition()
{
    return DYNCAST(CComposition, m_pDevice);
}

//+-----------------------------------------------------------------------------
//
//    Member:
//        CMilServerChannel::PostMessageToChannel
// 
//    Synopsis:
//        Sends a back-channel notification to the client-side channel.
// 
//------------------------------------------------------------------------------

HRESULT 
CMilServerChannel::PostMessageToChannel(
    __in_ecount(1) const MIL_MESSAGE *pNotification
    )
{
    RRETURN(m_pTransport->PostMessageToClient(pNotification, m_hChannel));    
}


