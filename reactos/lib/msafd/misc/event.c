/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver DLL
 * FILE:        misc/event.c
 * PURPOSE:     Event handling
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 15/06-2001 Created
 */
#include <msafd.h>

INT
WSPAPI
WSPEventSelect(
  IN  SOCKET s,
  IN  WSAEVENT hEventObject,
  IN  LONG lNetworkEvents,
  OUT LPINT lpErrno)
{
  FILE_REQUEST_EVENTSELECT Request;
  FILE_REPLY_EVENTSELECT Reply;
  IO_STATUS_BLOCK Iosb;
  NTSTATUS Status;

  Request.hEventObject   = hEventObject;
  Request.lNetworkEvents = lNetworkEvents;

  Status = NtDeviceIoControlFile(
    (HANDLE)s,
    NULL,
		NULL,
		NULL,   
		&Iosb,
		IOCTL_AFD_EVENTSELECT,
		&Request,
		sizeof(FILE_REQUEST_EVENTSELECT),
		&Reply,
		sizeof(FILE_REPLY_EVENTSELECT));
  if (Status == STATUS_PENDING) {
    AFD_DbgPrint(MAX_TRACE, ("Waiting on transport.\n"));
    /* FIXME: Wait only for blocking sockets */
		Status = NtWaitForSingleObject((HANDLE)s, FALSE, NULL);
  }

  if (!NT_SUCCESS(Status)) {
    AFD_DbgPrint(MAX_TRACE, ("Status (0x%X).\n", Status));
		*lpErrno = WSAENOBUFS;
    return SOCKET_ERROR;
	}

  AFD_DbgPrint(MAX_TRACE, ("Event select successful. Status (0x%X).\n",
    Reply.Status));

  *lpErrno = Reply.Status;

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
  FILE_REQUEST_ENUMNETWORKEVENTS Request;
  FILE_REPLY_ENUMNETWORKEVENTS Reply;
  IO_STATUS_BLOCK Iosb;
  NTSTATUS Status;

  Request.hEventObject = hEventObject;

  Status = NtDeviceIoControlFile(
    (HANDLE)s,
    NULL,
		NULL,
		NULL,   
		&Iosb,
		IOCTL_AFD_ENUMNETWORKEVENTS,
		&Request,
		sizeof(FILE_REQUEST_ENUMNETWORKEVENTS),
		&Reply,
		sizeof(FILE_REPLY_ENUMNETWORKEVENTS));
  if (Status == STATUS_PENDING) {
    AFD_DbgPrint(MAX_TRACE, ("Waiting on transport.\n"));
    /* FIXME: Wait only for blocking sockets */
		Status = NtWaitForSingleObject((HANDLE)s, FALSE, NULL);
  }

  if (!NT_SUCCESS(Status)) {
    AFD_DbgPrint(MAX_TRACE, ("Status (0x%X).\n", Status));
		*lpErrno = WSAENOBUFS;
    return SOCKET_ERROR;
	}

  AFD_DbgPrint(MAX_TRACE, ("EnumNetworkEvents successful. Status (0x%X).\n",
    Reply.Status));

  *lpErrno = Reply.Status;

  return 0;
}

/* EOF */
