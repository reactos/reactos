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

MtExtern(CMilConnection);

class CMilChannel;

class CMilConnection :
    public IMilBatchDevice,
    public CMILCOMBase 
{
private:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilConnection));

    CMilConnection();
    HRESULT InitializeClientTransport(MilMarshalType::Enum marshalType);

protected:
    ~CMilConnection();

public:
    //
    // IUnknown
    //
    DECLARE_COM_BASE

    static HRESULT Create(
        __in MilMarshalType::Enum marshalType, 
        __out CMilConnection** ppConnection);


    void ShutdownClientTransport();

    HRESULT CreateChannel(
        HMIL_CHANNEL hChannel,
        __out_ecount(1) CMilChannel **ppChannel
        );

    HRESULT DestroyChannel(HMIL_CHANNEL hChannel);

    HRESULT SynchronizeChannel(HMIL_CHANNEL hChannel);
    
    MilMarshalType::Enum GetMarshalType() const
    {
        return m_marshalType;
    }

    override HRESULT SubmitBatch(__in CMilCommandBatch *pBatch);

    HRESULT PostMessageToClient(
        __in const MIL_MESSAGE *pMessage,
        HMIL_CHANNEL hChannel
        );

    HRESULT PresentAllPartitions();

private:  
    // Called when the version reply notification has been received.
    HRESULT OnVersionReplyNotification(
        __in_ecount(1) const MIL_MESSAGE *pNotification, 
        __in_bcount(cbNotificationPayload) LPCVOID pcvNotificationPayload, 
        UINT cbNotificationPayload
        );

    HRESULT CreateChannelHelper(
        HMIL_CHANNEL hChannel,
        HMIL_CHANNEL hChannelSource,
        __in_ecount(1) CLIENT_CHANNEL_HANDLE_ENTRY *pEntry,
        __out_ecount(1) CMilChannel **ppChannel
        );

private:
    
    CConnectionContext *m_pConnectionContext;

    // Cross thread or same thread.
    MilMarshalType::Enum m_marshalType;


    // this is the channel table used to send notifications
    // to their corresponding channels.
    CMilClientChannelTable m_channelTable;

    CMilCrossThreadTransport*m_pCmdTransport;

    // crit sect that protects access to the channel table.
    CCriticalSection m_csConnection;
};

inline CMilConnection *HandleToPointer(HMIL_CONNECTION hTransport)
{
    return reinterpret_cast<CMilConnection*>(hTransport);
}

inline HMIL_CONNECTION PointerToHandle(__in_ecount_opt(1) CMilConnection *pTransport)
{
    return reinterpret_cast<HMIL_CONNECTION>(pTransport);
}


