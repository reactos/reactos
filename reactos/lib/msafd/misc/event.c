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

	/* Wait for return */
	if (Status == STATUS_PENDING) {
		WaitForSingleObject(SockEvent, 0);
	}

	NtClose( SockEvent );

	/* Set Socket Data*/
	Socket->EventObject = hEventObject;
	Socket->NetworkEvents = lNetworkEvents;

	return 0;
}


INT
WSPAPI
WSPEnumNetworkEvents(
  IN  SOCKET s, 
  IN  WSAEVENT hEventObject, 
  OUT LPWSANETWORKEVENTS lpNetworkEvents, 
  OUT LPINT lpErrno)
{
   return 0;
}

/* EOF */
