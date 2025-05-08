// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*++--------------------------------------------------------------------------



Module Name:
     channeltables.cpp


------------------------------------------------------------------------*/
#include "precomp.hpp"

CMilServerChannelTable::CMilServerChannelTable(UINT cbEntry) : HANDLE_TABLE(cbEntry)
{
}

CMilServerChannelTable::~CMilServerChannelTable()
{
}

HRESULT CMilServerChannelTable::AssignChannelEntry(
    __in HMIL_CHANNEL hChannel
    )
{
    HRESULT hr = S_OK;

    //
    // Allocate a handle table to manage the rendering resources.
    //

    IFC(AssignEntry(hChannel, DEVICE_ENTRY));

Cleanup:
    RRETURN(hr);
}

HRESULT CMilServerChannelTable::GetServerChannelTableEntry(
    __in HMIL_CHANNEL hChannel,
    __out_ecount(1) SERVER_CHANNEL_HANDLE_ENTRY **ppMasterEntry
    )
{
    HRESULT hr = S_OK;

    if(!ValidEntry(hChannel))
    {
        IFC(E_HANDLE);
    }

    *ppMasterEntry = reinterpret_cast<SERVER_CHANNEL_HANDLE_ENTRY*>(ENTRY_RECORD((*this), hChannel));

Cleanup:
    RRETURN(hr);
}

VOID CMilServerChannelTable::DestroyHandle(
    __in HMIL_CHANNEL hChannel
    )
{
    HRESULT hr = S_OK;
    SERVER_CHANNEL_HANDLE_ENTRY *pSlaveEntry = NULL;

    IFC(GetSlaveTableEntry(hChannel, &pSlaveEntry));

    ReleaseInterface(pSlaveEntry->pServerChannel);

    HANDLE_TABLE::DestroyHandle(hChannel);

Cleanup:
    ;
}

HRESULT CMilServerChannelTable::GetServerChannel(
    __in HMIL_CHANNEL hChannel,
    __out_ecount(1) CMilServerChannel **ppServerChannel
    )
{
    HRESULT hr = S_OK;
    SERVER_CHANNEL_HANDLE_ENTRY *pSlaveEntry = NULL;

    IFC(GetSlaveTableEntry(hChannel, &pSlaveEntry));

    if(pSlaveEntry->pServerChannel == NULL)
    {
        IFC(E_HANDLE);
    }

    *ppServerChannel = pSlaveEntry->pServerChannel;

Cleanup:
    RRETURN(hr);
}

HRESULT CMilServerChannelTable::GetSlaveTableEntry(
    __in HMIL_CHANNEL hChannel,
    __out_ecount(1) SERVER_CHANNEL_HANDLE_ENTRY **ppSlaveEntry
    )
{
    HRESULT hr = S_OK;

    if(!ValidEntry(hChannel))
    {
        IFC(E_HANDLE);
    }

    *ppSlaveEntry = reinterpret_cast<SERVER_CHANNEL_HANDLE_ENTRY*>(ENTRY_RECORD((*this), hChannel));

Cleanup:
    RRETURN(hr);
}


