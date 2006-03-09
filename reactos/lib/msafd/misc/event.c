/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver DLL
 * FILE:        misc/event.c
 * PURPOSE:     Event handling
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *				Alex Ionescu (alex@relsoft.net)
 * REVISIONS:
 *   CSH 15/06-2001 Created
 *	 Alex 16/07/2004 - Complete Rewrite
 */

#include <msafd.h>

#include <debug.h>

int 
WSPAPI 
WSPEventSelect(
	SOCKET Handle, 
	WSAEVENT hEventObject, 
	long lNetworkEvents, 
	LPINT lpErrno)
{
	IO_STATUS_BLOCK				IOSB;
	AFD_EVENT_SELECT_INFO		EventSelectInfo;
	PSOCKET_INFORMATION			Socket = NULL;
	NTSTATUS					Status;
	ULONG						BlockMode;
	HANDLE                                  SockEvent;

	Status = NtCreateEvent( &SockEvent, GENERIC_READ | GENERIC_WRITE,
				NULL, 1, FALSE );

	if( !NT_SUCCESS(Status) ) return -1;

	/* Get the Socket Structure associate to this Socket*/
	Socket = GetSocketStructure(Handle);

	/* Set Socket to Non-Blocking */
	BlockMode = 1;
	SetSocketInformation(Socket, AFD_INFO_BLOCKING_MODE, &BlockMode, NULL);
	Socket->SharedData.NonBlocking = TRUE;

	/* Deactivate Async Select if there is one */
	if (Socket->EventObject) {
		Socket->SharedData.hWnd = NULL;
		Socket->SharedData.wMsg = 0;
		Socket->SharedData.AsyncEvents = 0;
		Socket->SharedData.SequenceNumber++; // This will kill Async Select after the next completion
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
	EventSelectInfo.Events |= AFD_EVENT_DISCONNECT | AFD_EVENT_ABORT;
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

    AFD_DbgPrint(MID_TRACE,("AFD: %x\n", Status));

    /* Wait for return */
    if (Status == STATUS_PENDING) {
	WaitForSingleObject(SockEvent, INFINITE);
    }

    AFD_DbgPrint(MID_TRACE,("Waited\n"));

    NtClose( SockEvent );

    AFD_DbgPrint(MID_TRACE,("Closed event\n"));

    /* Set Socket Data*/
    Socket->EventObject = hEventObject;
    Socket->NetworkEvents = lNetworkEvents;

    AFD_DbgPrint(MID_TRACE,("Leaving\n"));

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
    IO_STATUS_BLOCK				IOSB;
    PSOCKET_INFORMATION			Socket = NULL;
    NTSTATUS					Status;
    HANDLE                                  SockEvent;

    AFD_DbgPrint(MID_TRACE,("Called (lpNetworkEvents %x)\n", lpNetworkEvents));

    Status = NtCreateEvent( &SockEvent, GENERIC_READ | GENERIC_WRITE,
			    NULL, 1, FALSE );

    if( !NT_SUCCESS(Status) ) {
	AFD_DbgPrint(MID_TRACE,("Could not make an event %x\n", Status));
	return -1;
    }

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(Handle);

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

    AFD_DbgPrint(MID_TRACE,("AFD: %x\n", Status));

    /* Wait for return */
    if (Status == STATUS_PENDING) {
	WaitForSingleObject(SockEvent, INFINITE);
	Status = STATUS_SUCCESS;
    }

    AFD_DbgPrint(MID_TRACE,("Waited\n"));

    NtClose( SockEvent );

    AFD_DbgPrint(MID_TRACE,("Closed event\n"));
    AFD_DbgPrint(MID_TRACE,("About to touch struct at %x (%d)\n", 
			    lpNetworkEvents, sizeof(*lpNetworkEvents)));

    lpNetworkEvents->lNetworkEvents = 0;

    AFD_DbgPrint(MID_TRACE,("Zeroed struct\n"));

    /* Set Events to wait for */
    if (EnumReq.PollEvents & AFD_EVENT_RECEIVE) {
	lpNetworkEvents->lNetworkEvents |= FD_READ;
    }

    if (EnumReq.PollEvents & AFD_EVENT_SEND) {
	lpNetworkEvents->lNetworkEvents |= FD_WRITE;
    }

    if (EnumReq.PollEvents & AFD_EVENT_OOB_RECEIVE) {
        lpNetworkEvents->lNetworkEvents |= FD_OOB;
    }

    if (EnumReq.PollEvents & AFD_EVENT_ACCEPT) {
	lpNetworkEvents->lNetworkEvents |= FD_ACCEPT;
    }

    if (EnumReq.PollEvents & 
	(AFD_EVENT_CONNECT | AFD_EVENT_CONNECT_FAIL)) {
        lpNetworkEvents->lNetworkEvents |= FD_CONNECT;
    }

    if (EnumReq.PollEvents & 
	(AFD_EVENT_DISCONNECT | AFD_EVENT_ABORT)) {
	lpNetworkEvents->lNetworkEvents |= FD_CLOSE;
    }

    if (EnumReq.PollEvents & AFD_EVENT_QOS) {
	lpNetworkEvents->lNetworkEvents |= FD_QOS;
    }

    if (EnumReq.PollEvents & AFD_EVENT_GROUP_QOS) {
	lpNetworkEvents->lNetworkEvents |= FD_GROUP_QOS;
    }

    if( NT_SUCCESS(Status) ) *lpErrno = 0;
    else *lpErrno = WSAEINVAL;

    AFD_DbgPrint(MID_TRACE,("Leaving\n"));

    return 0;
}

/* EOF */
