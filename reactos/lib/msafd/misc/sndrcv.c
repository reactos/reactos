/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver DLL
 * FILE:        misc/sndrcv.c
 * PURPOSE:     Send/receive routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 */
#include <msafd.h>

INT
WSPAPI
WSPAsyncSelect(
    IN  SOCKET s, 
    IN  HWND hWnd, 
    IN  UINT wMsg, 
    IN  LONG lEvent, 
    OUT LPINT lpErrno)
{
  UNIMPLEMENTED

  return 0;
}


INT
WSPAPI
WSPRecv(
    IN      SOCKET s,
    IN OUT  LPWSABUF lpBuffers,
    IN      DWORD dwBufferCount,
    OUT     LPDWORD lpNumberOfBytesRecvd,
    IN OUT  LPDWORD lpFlags,
    IN      LPWSAOVERLAPPED lpOverlapped,
    IN      LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    IN      LPWSATHREADID lpThreadId,
    OUT     LPINT lpErrno)
{
  PFILE_REQUEST_RECV Request;
  FILE_REPLY_RECV Reply;
  IO_STATUS_BLOCK Iosb;
  NTSTATUS Status;
  DWORD Size;

  AFD_DbgPrint(MAX_TRACE, ("Called.\n"));

  Size = dwBufferCount * sizeof(WSABUF);

  Request = (PFILE_REQUEST_RECV)HeapAlloc(
    GlobalHeap, 0, sizeof(FILE_REQUEST_RECV) + Size);
  if (!Request) {
    AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
    *lpErrno = WSAENOBUFS;
    return SOCKET_ERROR;
  }

  /* Put buffer pointers after request structure */
  Request->Buffers     = (LPWSABUF)(Request + 1);
  Request->BufferCount = dwBufferCount;
  Request->Flags       = lpFlags;

  RtlCopyMemory(Request->Buffers, lpBuffers, Size);

  Status = NtDeviceIoControlFile(
    (HANDLE)s,
    NULL,
		NULL,
		NULL,   
		&Iosb,
		IOCTL_AFD_RECV,
		Request,
		sizeof(FILE_REQUEST_RECV) + Size,
		&Reply,
		sizeof(FILE_REPLY_RECV));

  HeapFree(GlobalHeap, 0, Request);

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

  AFD_DbgPrint(MAX_TRACE, ("Receive successful (0x%X).\n",
    Reply.NumberOfBytesRecvd));

  *lpNumberOfBytesRecvd = Reply.NumberOfBytesRecvd;
  //*lpFlags = 0;

  return 0;
}


INT
WSPAPI
WSPRecvDisconnect(
    IN  SOCKET s,
    OUT LPWSABUF lpInboundDisconnectData,
    OUT LPINT lpErrno)
{
  UNIMPLEMENTED

  return 0;
}

INT
WSPAPI
WSPRecvFrom(
    IN      SOCKET s,
    IN OUT  LPWSABUF lpBuffers,
    IN      DWORD dwBufferCount,
    OUT     LPDWORD lpNumberOfBytesRecvd,
    IN OUT  LPDWORD lpFlags,
    OUT     LPSOCKADDR lpFrom,
    IN OUT  LPINT lpFromLen,
    IN      LPWSAOVERLAPPED lpOverlapped,
    IN      LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    IN      LPWSATHREADID lpThreadId,
    OUT     LPINT lpErrno)
{
  PFILE_REQUEST_RECVFROM Request;
  FILE_REPLY_RECVFROM Reply;
  IO_STATUS_BLOCK Iosb;
  NTSTATUS Status;
  DWORD Size;

  AFD_DbgPrint(MAX_TRACE, ("Called.\n"));

  Size = dwBufferCount * sizeof(WSABUF);

  Request = (PFILE_REQUEST_RECVFROM)HeapAlloc(
    GlobalHeap, 0, sizeof(FILE_REQUEST_RECVFROM) + Size);
  if (!Request) {
    AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
    *lpErrno = WSAENOBUFS;
    return SOCKET_ERROR;
  }

  /* Put buffer pointers after request structure */
  Request->Buffers     = (LPWSABUF)(Request + 1);
  Request->BufferCount = dwBufferCount;
  Request->Flags       = lpFlags;
  Request->From        = lpFrom;
  Request->FromLen     = lpFromLen;

  RtlCopyMemory(Request->Buffers, lpBuffers, Size);

  Status = NtDeviceIoControlFile(
    (HANDLE)s,
    NULL,
		NULL,
		NULL,   
		&Iosb,
		IOCTL_AFD_RECVFROM,
		Request,
		sizeof(FILE_REQUEST_RECVFROM) + Size,
		&Reply,
		sizeof(FILE_REPLY_RECVFROM));

  HeapFree(GlobalHeap, 0, Request);

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

  AFD_DbgPrint(MAX_TRACE, ("Receive successful (0x%X).\n",
    Reply.NumberOfBytesRecvd));

  *lpNumberOfBytesRecvd = Reply.NumberOfBytesRecvd;
  //*lpFlags = 0;
  ((PSOCKADDR_IN)lpFrom)->sin_family = AF_INET;
  ((PSOCKADDR_IN)lpFrom)->sin_port = 0;
  ((PSOCKADDR_IN)lpFrom)->sin_addr.S_un.S_addr = 0x0100007F;
  *lpFromLen = sizeof(SOCKADDR_IN);

  return 0;
}


INT
WSPAPI
WSPSend(
    IN  SOCKET s,
    IN  LPWSABUF lpBuffers,
    IN  DWORD dwBufferCount,
    OUT LPDWORD lpNumberOfBytesSent,
    IN  DWORD dwFlags,
    IN  LPWSAOVERLAPPED lpOverlapped,
    IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    IN  LPWSATHREADID lpThreadId,
    OUT LPINT lpErrno)
{
  PFILE_REQUEST_SENDTO Request;
  FILE_REPLY_SENDTO Reply;
  IO_STATUS_BLOCK Iosb;
  NTSTATUS Status;
  DWORD Size;

  AFD_DbgPrint(MAX_TRACE, ("Called.\n"));

  Size = dwBufferCount * sizeof(WSABUF);

  Request = (PFILE_REQUEST_SENDTO)HeapAlloc(
    GlobalHeap, 0, sizeof(FILE_REQUEST_SEND) + Size);
  if (!Request) {
    *lpErrno = WSAENOBUFS;
    return SOCKET_ERROR;
  }

  /* Put buffer pointers after request structure */
  Request->Buffers     = (LPWSABUF)(Request + 1);
  Request->BufferCount = dwBufferCount;
  Request->Flags       = dwFlags;

  RtlCopyMemory(Request->Buffers, lpBuffers, Size);

  Status = NtDeviceIoControlFile(
    (HANDLE)s,
    NULL,
		NULL,
		NULL,
		&Iosb,
		IOCTL_AFD_SEND,
		Request,
		sizeof(FILE_REQUEST_SEND) + Size,
		&Reply,
		sizeof(FILE_REPLY_SEND));

  HeapFree(GlobalHeap, 0, Request);

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

  AFD_DbgPrint(MAX_TRACE, ("Send successful.\n"));

  return 0;
}


INT
WSPAPI
WSPSendDisconnect(
    IN  SOCKET s,
    IN  LPWSABUF lpOutboundDisconnectData,
    OUT LPINT lpErrno)
{
  UNIMPLEMENTED

  return 0;
}


INT
WSPAPI
WSPSendTo(
    IN  SOCKET s,
    IN  LPWSABUF lpBuffers,
    IN  DWORD dwBufferCount,
    OUT LPDWORD lpNumberOfBytesSent,
    IN  DWORD dwFlags,
    IN  CONST LPSOCKADDR lpTo,
    IN  INT iToLen,
    IN  LPWSAOVERLAPPED lpOverlapped,
    IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    IN  LPWSATHREADID lpThreadId,
    OUT LPINT lpErrno)
{
  PFILE_REQUEST_SENDTO Request;
  FILE_REPLY_SENDTO Reply;
  IO_STATUS_BLOCK Iosb;
  NTSTATUS Status;
  DWORD Size;

  AFD_DbgPrint(MAX_TRACE, ("Called.\n"));

  Size = dwBufferCount * sizeof(WSABUF);

  Request = (PFILE_REQUEST_SENDTO)HeapAlloc(
    GlobalHeap, 0, sizeof(FILE_REQUEST_SENDTO) + Size);
  if (!Request) {
    *lpErrno = WSAENOBUFS;
    return SOCKET_ERROR;
  }

  /* Put buffer pointers after request structure */
  Request->Buffers     = (LPWSABUF)(Request + 1);
  Request->BufferCount = dwBufferCount;
  Request->Flags       = dwFlags;
  Request->ToLen       = iToLen;

  RtlCopyMemory(&Request->To, lpTo, sizeof(SOCKADDR));

  RtlCopyMemory(Request->Buffers, lpBuffers, Size);

  Status = NtDeviceIoControlFile(
    (HANDLE)s,
    NULL,
		NULL,
		NULL,
		&Iosb,
		IOCTL_AFD_SENDTO,
		Request,
		sizeof(FILE_REQUEST_SENDTO) + Size,
		&Reply,
		sizeof(FILE_REPLY_SENDTO));

  HeapFree(GlobalHeap, 0, Request);

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

  AFD_DbgPrint(MAX_TRACE, ("Send successful.\n"));

  return 0;
}

/* EOF */
