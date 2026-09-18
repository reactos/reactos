// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*++--------------------------------------------------------------------------



Module Name:
     connectioncontext.h
     
Abstract:

    This object is the server side peer to the client side transport and channel
    objects. Client side channels and transport take api calls and convert them
    into packets. These packets are posted into the transport. The transport
    posts these commands to the CConnectionContext which decodes the packets.
    
------------------------------------------------------------------------*/

MtExtern(CConnectionContext);
MtExtern(CRecorderConnectionContext);

class CMilConnection;


class CConnectionContext :
    public CMILCOMBase
{
public:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CConnectionContext));

    CConnectionContext(
        MilMarshalType::Enum mType,
        __in CMilConnection *pNotifTransport
        );

    virtual ~CConnectionContext();
    //
    // IUnknown
    //
    DECLARE_COM_BASE

    HRESULT PostMessageToClient(
        __in const MIL_MESSAGE *Msg,
        HMIL_CHANNEL hChannel);

    HRESULT PresentAllPartitions();
    HRESULT ShutDownAllChannels();


    HRESULT OpenChannel(HMIL_CHANNEL hChannel, HMIL_CHANNEL hSourceChannel);
    HRESULT CloseChannel(HMIL_CHANNEL hChannel);
    HRESULT SendBatchToChannel(HMIL_CHANNEL hChannel, __in_ecount(1) CMilCommandBatch* pBatch);


private:

    HRESULT GetServerChannel(HMIL_CHANNEL hChannel, __out CMilServerChannel **ppServerChannel);
    
    HRESULT GetOwningComposition(
        HMIL_CHANNEL hSourceChannel,
        __out_ecount(1) CComposition **ppComposition
        );

    virtual HRESULT RemoveChannelFromTable(HMIL_CHANNEL hChannel);

    HRESULT AssignChannelInTable(
        HMIL_CHANNEL hChannel,
        HMIL_CHANNEL hSourceChannel,
        CMilServerChannel *pmilChannel,
        CComposition *pOwningComposition
        );

    HRESULT CloseChannelForced(HMIL_CHANNEL hChannel, BOOL fCleanResource = FALSE);

    HRESULT GetExistingComposition(
        HMIL_CHANNEL hChannel,
        CComposition **ppComposition
        );

    HRESULT EnsureRecorder();

    UINT m_nrChannels;

    //
    // This is used for commands that need to be sent to the
    // running partitions.
    //
    CMilCommandBatch *m_pCommands;
    // event used to sync flush channels in absence of ui side communication. This
    // is done during shutdown effected by the server side host of the composition.
    HANDLE m_hSyncFlushEvent;

    // this is the channel table used to send command buffers
    // to their corresponding channels.
    CMilServerChannelTable m_channelTable;

    // notification interface given to the server channels as a message sync.
    // This is a back pointer to the object that controls the life time
    // of the connection context so it is a weak reference.
    CMilConnection *m_pTransportNoRef;

    MilMarshalType::Enum m_mType;
};



