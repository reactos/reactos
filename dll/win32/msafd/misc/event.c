/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver DLL
 * FILE:        dll/win32/msafd/misc/event.c
 * PURPOSE:     Event handling
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Alex Ionescu (alex@relsoft.net)
 * REVISIONS:
 *   CSH  15/06/2001 - Created
 *   Alex 16/07/2004 - Complete Rewrite
 */

#include <msafd.h>

int
WSPAPI
WSPEventSelect(
    IN SOCKET Handle,
    IN WSAEVENT hEventObject,
    IN long lNetworkEvents,
    OUT LPINT lpErrno)
{
    IO_STATUS_BLOCK        IOSB;
    AFD_EVENT_SELECT_INFO  EventSelectInfo;
    PSOCKET_INFORMATION    Socket = NULL;
    NTSTATUS               Status;
    BOOLEAN                BlockMode;
    HANDLE                 SockEvent;

    TRACE("WSPEventSelect (%lx) %lx %lx\n", Handle, hEventObject, lNetworkEvents);

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(Handle);
    if (!Socket)
    {
        if (lpErrno) *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }

    Status = NtCreateEvent(&SockEvent, EVENT_ALL_ACCESS,
                           NULL, SynchronizationEvent, FALSE);

    if (!NT_SUCCESS(Status)) return SOCKET_ERROR;

    /* Set Socket to Non-Blocking */
    BlockMode = TRUE;
    SetSocketInformation(Socket, AFD_INFO_BLOCKING_MODE, &BlockMode, NULL, NULL, NULL, NULL);
    Socket->SharedData->NonBlocking = TRUE;

    /* Deactivate Async Select if there is one */
    if (Socket->EventObject) {
        Socket->SharedData->hWnd = NULL;
        Socket->SharedData->wMsg = 0;
        Socket->SharedData->AsyncEvents = 0;
        Socket->SharedData->SequenceNumber++; // This will kill Async Select after the next completion
    }

    /* Set Structure Info */
    EventSelectInfo.EventObject = hEventObject;
    EventSelectInfo.Events = 0;

    /* Set Events to wait for */
    if (lNetworkEvents & FD_READ) {
        EventSelectInfo.Events |= AFD_EVENT_RECEIVE;
    }

    if (lNetworkEvents & FD_WRITE) {
        EventSelectInfo.Events |= AFD_EVENT_SEND;
    }

    if (lNetworkEvents & FD_OOB) {
        EventSelectInfo.Events |= AFD_EVENT_OOB_RECEIVE;
    }

    if (lNetworkEvents & FD_ACCEPT) {
        EventSelectInfo.Events |= AFD_EVENT_ACCEPT;
    }

    if (lNetworkEvents & FD_CONNECT) {
        EventSelectInfo.Events |= AFD_EVENT_CONNECT | AFD_EVENT_CONNECT_FAIL;
    }

    if (lNetworkEvents & FD_CLOSE) {
        EventSelectInfo.Events |= AFD_EVENT_DISCONNECT | AFD_EVENT_ABORT | AFD_EVENT_CLOSE;
    }

    if (lNetworkEvents & FD_QOS) {
        EventSelectInfo.Events |= AFD_EVENT_QOS;
    }

    if (lNetworkEvents & FD_GROUP_QOS) {
        EventSelectInfo.Events |= AFD_EVENT_GROUP_QOS;
    }

    /* Send IOCTL */
    Status = NtDeviceIoControlFile((HANDLE)Handle,
                                   SockEvent,
                                   NULL,
                                   NULL,
                                   &IOSB,
                                   IOCTL_AFD_EVENT_SELECT,
                                   &EventSelectInfo,
                                   sizeof(EventSelectInfo),
                                   NULL,
                                   0);

    /* Wait for return */
    if (Status == STATUS_PENDING) {
        WaitForSingleObject(SockEvent, INFINITE);
        Status = IOSB.Status;
    }

    NtClose (SockEvent);

    if (Status != STATUS_SUCCESS)
    {
        ERR("Got status 0x%08x.\n", Status);
        return MsafdReturnWithErrno(Status, lpErrno, 0, NULL);
    }

    /* Set Socket Data*/
    Socket->EventObject = hEventObject;
    Socket->NetworkEvents = lNetworkEvents;

    TRACE("Leaving\n");

    return 0;
}


INT
WSPAPI
WSPEnumNetworkEvents(
    IN  SOCKET Handle,
    IN  WSAEVENT hEventObject,
    OUT LPWSANETWORKEVENTS lpNetworkEvents,
    OUT LPINT lpErrno)
{
    AFD_ENUM_NETWORK_EVENTS_INFO EnumReq;
    IO_STATUS_BLOCK              IOSB;
    PSOCKET_INFORMATION          Socket = NULL;
    NTSTATUS                     Status;
    HANDLE                       SockEvent;

    TRACE("Called (lpNetworkEvents %x)\n", lpNetworkEvents);

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(Handle);
    if (!Socket)
    {
       if (lpErrno) *lpErrno = WSAENOTSOCK;
       return SOCKET_ERROR;
    }
    if (!lpNetworkEvents)
    {
       if (lpErrno) *lpErrno = WSAEFAULT;
       return SOCKET_ERROR;
    }

    Status = NtCreateEvent(&SockEvent, EVENT_ALL_ACCESS,
                           NULL, SynchronizationEvent, FALSE);

    if( !NT_SUCCESS(Status) ) {
        ERR("Could not make an event %x\n", Status);
        return SOCKET_ERROR;
    }

    EnumReq.Event = hEventObject;

    /* Send IOCTL */
    Status = NtDeviceIoControlFile((HANDLE)Handle,
                                   SockEvent,
                                   NULL,
                                   NULL,
                                   &IOSB,
                                   IOCTL_AFD_ENUM_NETWORK_EVENTS,
                                   &EnumReq,
                                   sizeof(EnumReq),
                                   NULL,
                                   0);

    /* Wait for return */
    if (Status == STATUS_PENDING) {
        WaitForSingleObject(SockEvent, INFINITE);
        Status = IOSB.Status;
    }

    NtClose (SockEvent);

    if (Status != STATUS_SUCCESS)
    {
        ERR("Status 0x%08x\n", Status);
        return MsafdReturnWithErrno(Status, lpErrno, 0, NULL);
    }

    lpNetworkEvents->lNetworkEvents = 0;

    /* Set Events to wait for */
    if (EnumReq.PollEvents & AFD_EVENT_RECEIVE) {
        lpNetworkEvents->lNetworkEvents |= FD_READ;
        lpNetworkEvents->iErrorCode[FD_READ_BIT] = TranslateNtStatusError(EnumReq.EventStatus[FD_READ_BIT]);
    }

    if (EnumReq.PollEvents & AFD_EVENT_SEND) {
        lpNetworkEvents->lNetworkEvents |= FD_WRITE;
        lpNetworkEvents->iErrorCode[FD_WRITE_BIT] = TranslateNtStatusError(EnumReq.EventStatus[FD_WRITE_BIT]);
    }

    if (EnumReq.PollEvents & AFD_EVENT_OOB_RECEIVE) {
        lpNetworkEvents->lNetworkEvents |= FD_OOB;
        lpNetworkEvents->iErrorCode[FD_OOB_BIT] = TranslateNtStatusError(EnumReq.EventStatus[FD_OOB_BIT]);
    }

    if (EnumReq.PollEvents & AFD_EVENT_ACCEPT) {
        lpNetworkEvents->lNetworkEvents |= FD_ACCEPT;
        lpNetworkEvents->iErrorCode[FD_ACCEPT_BIT] = TranslateNtStatusError(EnumReq.EventStatus[FD_ACCEPT_BIT]);
    }

    if (EnumReq.PollEvents &
            (AFD_EVENT_CONNECT | AFD_EVENT_CONNECT_FAIL)) {
        lpNetworkEvents->lNetworkEvents |= FD_CONNECT;
        lpNetworkEvents->iErrorCode[FD_CONNECT_BIT] = TranslateNtStatusError(EnumReq.EventStatus[FD_CONNECT_BIT]);
    }

    if (EnumReq.PollEvents &
            (AFD_EVENT_DISCONNECT | AFD_EVENT_ABORT | AFD_EVENT_CLOSE)) {
        lpNetworkEvents->lNetworkEvents |= FD_CLOSE;
        lpNetworkEvents->iErrorCode[FD_CLOSE_BIT] = TranslateNtStatusError(EnumReq.EventStatus[FD_CLOSE_BIT]);
    }

    if (EnumReq.PollEvents & AFD_EVENT_QOS) {
        lpNetworkEvents->lNetworkEvents |= FD_QOS;
        lpNetworkEvents->iErrorCode[FD_QOS_BIT] = TranslateNtStatusError(EnumReq.EventStatus[FD_QOS_BIT]);
    }

    if (EnumReq.PollEvents & AFD_EVENT_GROUP_QOS) {
        lpNetworkEvents->lNetworkEvents |= FD_GROUP_QOS;
        lpNetworkEvents->iErrorCode[FD_GROUP_QOS_BIT] = TranslateNtStatusError(EnumReq.EventStatus[FD_GROUP_QOS_BIT]);
    }

    TRACE("Leaving\n");

    return MsafdReturnWithErrno(STATUS_SUCCESS, lpErrno, 0, NULL);
}

/* EOF */
