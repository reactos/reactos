/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             subsys/system/dhcp/api.c
 * PURPOSE:          DHCP client api handlers
 * PROGRAMMER:       arty
 */

#include <rosdhcp.h>

#define NDEBUG
#include <reactos/debug.h>

static CRITICAL_SECTION ApiCriticalSection;

extern HANDLE hAdapterStateChangedEvent;

VOID ApiInit() {
    InitializeCriticalSection( &ApiCriticalSection );
}

VOID ApiLock() {
    EnterCriticalSection( &ApiCriticalSection );
}

VOID ApiUnlock() {
    LeaveCriticalSection( &ApiCriticalSection );
}

VOID ApiFree() {
    DeleteCriticalSection( &ApiCriticalSection );
}

/* This represents the service portion of the DHCP client API */

DWORD
DSAcquireParams(
    _In_ PipeSendFunc Send,
    _In_ HANDLE CommPipe,
    _Out_ COMM_DHCP_REQ *Req)
{
    COMM_DHCP_REPLY Reply;
    PDHCP_ADAPTER Adapter;
    struct protocol* proto;

    DPRINT1("DSAcquireParams()\n");

    Reply.Reply = ERROR_SUCCESS;

    ApiLock();

    Adapter = AdapterFindName(Req->Body.AcquireParams.AdapterName);
    if (Adapter == NULL || Adapter->DhclientState.state == S_STATIC)
    {
        Reply.Reply = ERROR_FILE_NOT_FOUND;
        goto done;
    }

    DPRINT1("Adapter: %p\n", Adapter);

    proto = find_protocol_by_adapter(&Adapter->DhclientInfo);
    if (proto)
        remove_protocol(proto);

    add_protocol(Adapter->DhclientInfo.name,
                 Adapter->DhclientInfo.rfdesc, got_one,
                 &Adapter->DhclientInfo);

    Adapter->DhclientInfo.client->state = S_INIT;
    state_reboot(&Adapter->DhclientInfo);

    if (hAdapterStateChangedEvent != NULL)
        SetEvent(hAdapterStateChangedEvent);

done:
    ApiUnlock();

    return Send(CommPipe, &Reply);
}


DWORD
DSReleaseParams(
    _In_ PipeSendFunc Send,
    _In_ HANDLE CommPipe,
    _Out_ COMM_DHCP_REQ *Req)
{
    COMM_DHCP_REPLY Reply;
    PDHCP_ADAPTER Adapter;
    struct protocol* proto;

    DPRINT1("DSReleaseParams()\n");

    Reply.Reply = ERROR_SUCCESS;

    ApiLock();

    Adapter = AdapterFindName(Req->Body.AcquireParams.AdapterName);
    if (Adapter == NULL)
    {
        Reply.Reply = ERROR_FILE_NOT_FOUND;
        goto done;
    }

    DPRINT1("Adapter: %p\n", Adapter);

    if (Adapter->NteContext)
    {
        DeleteIPAddress(Adapter->NteContext);
        Adapter->NteContext = 0;
    }
    if (Adapter->RouterMib.dwForwardNextHop)
    {
        DeleteIpForwardEntry(&Adapter->RouterMib);
        Adapter->RouterMib.dwForwardNextHop = 0;
    }

    proto = find_protocol_by_adapter(&Adapter->DhclientInfo);
    if (proto)
        remove_protocol(proto);

    Adapter->DhclientInfo.client->active = NULL;
    Adapter->DhclientInfo.client->state = S_INIT;

    if (hAdapterStateChangedEvent != NULL)
        SetEvent(hAdapterStateChangedEvent);

done:
    ApiUnlock();

    return Send(CommPipe, &Reply);
}


DWORD DSLeaseIpAddress( PipeSendFunc Send, HANDLE CommPipe, COMM_DHCP_REQ *Req ) {
    COMM_DHCP_REPLY Reply;
    PDHCP_ADAPTER Adapter;
    struct protocol* proto;

    ApiLock();

    Adapter = AdapterFindIndex( Req->AdapterIndex );

    Reply.Reply = Adapter ? 1 : 0;

    if( Adapter ) {
        proto = find_protocol_by_adapter( &Adapter->DhclientInfo );
        if (proto)
            remove_protocol(proto);

        add_protocol( Adapter->DhclientInfo.name,
                      Adapter->DhclientInfo.rfdesc, got_one,
                      &Adapter->DhclientInfo );

        Adapter->DhclientInfo.client->state = S_INIT;
        state_reboot(&Adapter->DhclientInfo);

        if (hAdapterStateChangedEvent != NULL)
            SetEvent(hAdapterStateChangedEvent);
    }

    ApiUnlock();

    return Send(CommPipe, &Reply );
}

DWORD DSQueryHWInfo( PipeSendFunc Send, HANDLE CommPipe, COMM_DHCP_REQ *Req ) {
    COMM_DHCP_REPLY Reply;
    PDHCP_ADAPTER Adapter;

    ApiLock();

    Adapter = AdapterFindIndex( Req->AdapterIndex );

    Reply.Reply = Adapter ? 1 : 0;

    if (Adapter) {
        Reply.QueryHWInfo.AdapterIndex = Req->AdapterIndex;
        Reply.QueryHWInfo.MediaType = Adapter->IfMib.dwType;
        Reply.QueryHWInfo.Mtu = Adapter->IfMib.dwMtu;
        Reply.QueryHWInfo.Speed = Adapter->IfMib.dwSpeed;
    }

    ApiUnlock();

    return Send(CommPipe,  &Reply );
}

DWORD DSReleaseIpAddressLease( PipeSendFunc Send, HANDLE CommPipe, COMM_DHCP_REQ *Req ) {
    COMM_DHCP_REPLY Reply;
    PDHCP_ADAPTER Adapter;
    struct protocol* proto;

    ApiLock();

    Adapter = AdapterFindIndex( Req->AdapterIndex );

    Reply.Reply = Adapter ? 1 : 0;

    if( Adapter ) {
        if (Adapter->NteContext)
        {
            DeleteIPAddress( Adapter->NteContext );
            Adapter->NteContext = 0;
        }
        if (Adapter->RouterMib.dwForwardNextHop)
        {
            DeleteIpForwardEntry( &Adapter->RouterMib );
            Adapter->RouterMib.dwForwardNextHop = 0;
        }

        proto = find_protocol_by_adapter( &Adapter->DhclientInfo );
        if (proto)
           remove_protocol(proto);

        Adapter->DhclientInfo.client->active = NULL;
        Adapter->DhclientInfo.client->state = S_INIT;

        if (hAdapterStateChangedEvent != NULL)
            SetEvent(hAdapterStateChangedEvent);
    }

    ApiUnlock();

    return Send(CommPipe,  &Reply );
}

DWORD DSRenewIpAddressLease( PipeSendFunc Send, HANDLE CommPipe, COMM_DHCP_REQ *Req ) {
    COMM_DHCP_REPLY Reply;
    PDHCP_ADAPTER Adapter;
    struct protocol* proto;

    ApiLock();

    Adapter = AdapterFindIndex( Req->AdapterIndex );

    if( !Adapter || Adapter->DhclientState.state == S_STATIC ) {
        Reply.Reply = 0;
        ApiUnlock();
        return Send(CommPipe,  &Reply );
    }

    Reply.Reply = 1;

    proto = find_protocol_by_adapter( &Adapter->DhclientInfo );
    if (proto)
        remove_protocol(proto);

    add_protocol( Adapter->DhclientInfo.name,
                  Adapter->DhclientInfo.rfdesc, got_one,
                  &Adapter->DhclientInfo );

    Adapter->DhclientInfo.client->state = S_INIT;
    state_reboot(&Adapter->DhclientInfo);

    if (hAdapterStateChangedEvent != NULL)
        SetEvent(hAdapterStateChangedEvent);

    ApiUnlock();

    return Send(CommPipe,  &Reply );
}

DWORD DSStaticRefreshParams( PipeSendFunc Send, HANDLE CommPipe, COMM_DHCP_REQ *Req ) {
    NTSTATUS Status;
    COMM_DHCP_REPLY Reply;
    PDHCP_ADAPTER Adapter;
    struct protocol* proto;

    ApiLock();

    Adapter = AdapterFindIndex( Req->AdapterIndex );

    Reply.Reply = Adapter ? 1 : 0;

    if( Adapter ) {
        if (Adapter->NteContext)
        {
            DeleteIPAddress( Adapter->NteContext );
            Adapter->NteContext = 0;
        }
        if (Adapter->RouterMib.dwForwardNextHop)
        {
            DeleteIpForwardEntry( &Adapter->RouterMib );
            Adapter->RouterMib.dwForwardNextHop = 0;
        }

        Adapter->DhclientState.state = S_STATIC;
        proto = find_protocol_by_adapter( &Adapter->DhclientInfo );
        if (proto)
            remove_protocol(proto);
        Status = AddIPAddress( Req->Body.StaticRefreshParams.IPAddress,
                               Req->Body.StaticRefreshParams.Netmask,
                               Req->AdapterIndex,
                               &Adapter->NteContext,
                               &Adapter->NteInstance );
        Reply.Reply = NT_SUCCESS(Status);

        if (hAdapterStateChangedEvent != NULL)
            SetEvent(hAdapterStateChangedEvent);
    }

    ApiUnlock();

    return Send(CommPipe,  &Reply );
}
