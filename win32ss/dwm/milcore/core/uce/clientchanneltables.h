// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
// 

// 
//  Abstract:
//      Definitions for client and server-side channel handle tables.
// 
//------------------------------------------------------------------------------

MtExtern(CMilConnection);

#define DEVICE_ENTRY 1
#define DEVICE_ENTRY_MASTER 2
#define DEVICE_ENTRY_SLAVE 3

interface IMilTransportQueue;
class CMilChannel;

struct CLIENT_CHANNEL_HANDLE_ENTRY
{
    DWORD type;
    CMilChannel *pMilChannel;
    IMilBatchDevice *pCompDevice;
    CMilSlaveHandleTable *pht;
    HANDLE hSyncFlushEvent;
};

class CMilClientChannelTable : public HANDLE_TABLE
{
public:
    CMilClientChannelTable();
    ~CMilClientChannelTable();

    HRESULT Initialize();

    HRESULT AssignChannelEntry(
        __in HMIL_CHANNEL hChannel,
        __out_ecount(1) CLIENT_CHANNEL_HANDLE_ENTRY **ppEntry
        );

    HRESULT GetNewChannelEntry(
        __out_ecount(1) HMIL_CHANNEL *phChannel,
        __out_ecount(1) CLIENT_CHANNEL_HANDLE_ENTRY **ppEntry
        );

    VOID DestroyHandle(
        __in HMIL_CHANNEL object
        );

    HRESULT GetMasterTableEntry(
        __in HMIL_CHANNEL hChannel,
        __out_ecount(1) CLIENT_CHANNEL_HANDLE_ENTRY **ppMasterEntry
        );

    HRESULT GetMasterTableEntryThreadSafe(
        __in HMIL_CHANNEL hChannel,
        __out_ecount(1) CLIENT_CHANNEL_HANDLE_ENTRY *pMasterEntry
        );

private:
    UINT m_nOfChannels;
    CCriticalSection m_csChannelTable;
};


