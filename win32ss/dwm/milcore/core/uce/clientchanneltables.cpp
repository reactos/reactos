// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*++--------------------------------------------------------------------------



Module Name:
     channeltables.cpp


------------------------------------------------------------------------*/
#include "precomp.hpp"

/*++--------------------------------------------------------------------------
class CMilClientChannelTable
-----------------------------------------------------------------------------*/

CMilClientChannelTable::CMilClientChannelTable()
    : HANDLE_TABLE(sizeof(CLIENT_CHANNEL_HANDLE_ENTRY))
{
    m_nOfChannels = 0;
}

CMilClientChannelTable::~CMilClientChannelTable()
{
}

HRESULT CMilClientChannelTable::Initialize()
{
    HRESULT hr = S_OK;

    IFC(m_csChannelTable.Init());

Cleanup:
    RRETURN(hr);
}


HRESULT CMilClientChannelTable::GetMasterTableEntry(
    __in HMIL_CHANNEL hChannel,
    __out_ecount(1) CLIENT_CHANNEL_HANDLE_ENTRY **ppMasterEntry
    )
{
    HRESULT hr = S_OK;

    if(!ValidEntry(hChannel))
    {
        IFC(E_HANDLE);
    }

    *ppMasterEntry = reinterpret_cast<CLIENT_CHANNEL_HANDLE_ENTRY*>(ENTRY_RECORD((*this), hChannel));

Cleanup:
    RRETURN(hr);
}

HRESULT CMilClientChannelTable::GetMasterTableEntryThreadSafe(
    __in HMIL_CHANNEL hChannel,
    __out_ecount(1) CLIENT_CHANNEL_HANDLE_ENTRY *pMasterEntry
    )
{
    HRESULT hr = S_OK;
    CLIENT_CHANNEL_HANDLE_ENTRY *pMasterEntryInternal = NULL;
    CGuard<CCriticalSection> oGuard(m_csChannelTable);

    IFC(GetMasterTableEntry(hChannel, &pMasterEntryInternal));

    // copy the table entry out of table under the guard. Returning the entry internal pointer
    // out of the table is not thread safe because during a realloc that entry can change location
    // in memory.
    *pMasterEntry = *pMasterEntryInternal;

Cleanup:
    RRETURN(hr);
}


HRESULT CMilClientChannelTable::GetNewChannelEntry(
    __out_ecount(1) HMIL_CHANNEL *phChannel,
    __out_ecount(1) CLIENT_CHANNEL_HANDLE_ENTRY **ppEntry
    )
{
    HRESULT hr = S_OK;
    CLIENT_CHANNEL_HANDLE_ENTRY *pMasterEntry = NULL;
    HMIL_CHANNEL hChannel;
    CGuard<CCriticalSection> oGuard(m_csChannelTable);

    IFC(GetNewEntry(DEVICE_ENTRY, &hChannel));

    IFC(GetMasterTableEntry(hChannel, &pMasterEntry));

    pMasterEntry->pMilChannel = NULL;
    pMasterEntry->pCompDevice = NULL;
    pMasterEntry->pht = NULL;
    IFCW32(pMasterEntry->hSyncFlushEvent = CreateEvent(NULL, FALSE, FALSE, NULL));

    *phChannel = hChannel;
    *ppEntry = pMasterEntry;

    m_nOfChannels++;

Cleanup:
    RRETURN(hr);
}

HRESULT CMilClientChannelTable::AssignChannelEntry(
    __in HMIL_CHANNEL hChannel,
    __out_ecount(1) CLIENT_CHANNEL_HANDLE_ENTRY **ppEntry
    )
{
    HRESULT hr = S_OK;
    CLIENT_CHANNEL_HANDLE_ENTRY *pMasterEntry = NULL;

    //
    // Allocate a handle table to manage the rendering resources.
    //

    IFC(AssignEntry(hChannel, DEVICE_ENTRY));


    IFC(GetMasterTableEntry(hChannel, &pMasterEntry));

    *ppEntry = pMasterEntry;

Cleanup:
    RRETURN(hr);
}


VOID CMilClientChannelTable::DestroyHandle(
    __in HMIL_CHANNEL hChannel
    )
{
    HRESULT hr = S_OK;
    CLIENT_CHANNEL_HANDLE_ENTRY *pMasterEntry = NULL;
    CGuard<CCriticalSection> oGuard(m_csChannelTable);

    IFC(GetMasterTableEntry(hChannel, &pMasterEntry));
    CloseHandle(pMasterEntry->hSyncFlushEvent);

    HANDLE_TABLE::DestroyHandle(hChannel);

    m_nOfChannels--;

Cleanup:
    ;
}



