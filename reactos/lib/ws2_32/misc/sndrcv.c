/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 DLL
 * FILE:        misc/sndrcv.c
 * PURPOSE:     Send/receive functions
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 */
#include <ws2_32.h>
#include <catalog.h>
#include <handle.h>


/*
 * @implemented
 */
INT
EXPORT
recv(
  IN  SOCKET s,
  OUT CHAR FAR* buf,
  IN  INT len,
  IN  INT flags)
{
  DWORD BytesReceived;
  WSABUF WSABuf;

  WS_DbgPrint(MAX_TRACE, ("s (0x%X)  buf (0x%X)  len (0x%X) flags (0x%X).\n",
      s, buf, len, flags));

  WSABuf.len = len;
  WSABuf.buf = (CHAR FAR*)buf;

  WSARecv(s, &WSABuf, 1, &BytesReceived, (LPDWORD)&flags, NULL, NULL);

  return BytesReceived;
}


/*
 * @implemented
 */
INT
EXPORT
recvfrom(
  IN      SOCKET s,
  OUT     CHAR FAR* buf,
  IN      INT len,
  IN      INT flags,
  OUT     LPSOCKADDR from,
  IN OUT  INT FAR* fromlen)
{
  DWORD BytesReceived;
  WSABUF WSABuf;

  WS_DbgPrint(MAX_TRACE, ("s (0x%X)  buf (0x%X)  len (0x%X) flags (0x%X).\n",
      s, buf, len, flags));

  WSABuf.len = len;
  WSABuf.buf = (CHAR FAR*)buf;

  WSARecvFrom(s, &WSABuf, 1, &BytesReceived, (LPDWORD)&flags, from, fromlen, NULL, NULL);

  return BytesReceived;
}


/*
 * @implemented
 */
INT
EXPORT
send( 
  IN  SOCKET s, 
  IN  CONST CHAR FAR* buf, 
  IN  INT len, 
  IN  INT flags)
{
  DWORD BytesSent;
  WSABUF WSABuf;

  WS_DbgPrint(MAX_TRACE, ("s (0x%X)  buf (0x%X)  len (0x%X) flags (0x%X).\n",
      s, buf, len, flags));

  WSABuf.len = len;
  WSABuf.buf = (CHAR FAR*)buf;

  return WSASend(s, &WSABuf, 1, &BytesSent, flags, NULL, NULL);
}


/*
 * @implemented
 */
INT
EXPORT
sendto(
  IN  SOCKET s,
  IN  CONST CHAR FAR* buf,
  IN  INT len,
  IN  INT flags,
  IN  CONST LPSOCKADDR to, 
  IN  INT tolen)
{
  DWORD BytesSent;
  WSABUF WSABuf;

  WS_DbgPrint(MAX_TRACE, ("s (0x%X)  buf (0x%X)  len (0x%X) flags (0x%X).\n",
      s, buf, len, flags));

  WSABuf.len = len;
  WSABuf.buf = (CHAR FAR*)buf;

  return WSASendTo(s, &WSABuf, 1, &BytesSent, flags, to, tolen, NULL, NULL);
}


/*
 * @implemented
 */
INT
EXPORT
WSARecv(
  IN      SOCKET s,
  IN OUT  LPWSABUF lpBuffers,
  IN      DWORD dwBufferCount,
  OUT     LPDWORD lpNumberOfBytesRecvd,
  IN OUT  LPDWORD lpFlags,
  IN      LPWSAOVERLAPPED lpOverlapped,
  IN      LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
  PCATALOG_ENTRY Provider;
  INT Errno;
  INT Code;

  WS_DbgPrint(MAX_TRACE, ("Called.\n"));

  if (!ReferenceProviderByHandle((HANDLE)s, &Provider)) {
    WSASetLastError(WSAENOTSOCK);
    return SOCKET_ERROR;
  }

  assert(Provider->ProcTable.lpWSPRecv);

  Code = Provider->ProcTable.lpWSPRecv(s, lpBuffers, dwBufferCount,
    lpNumberOfBytesRecvd, lpFlags, lpOverlapped,
    lpCompletionRoutine, NULL /* lpThreadId */, &Errno);

  DereferenceProviderByPointer(Provider);

  if (Code == SOCKET_ERROR)
    WSASetLastError(Errno);

  return Code;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSARecvDisconnect(
  IN  SOCKET s,
  OUT LPWSABUF lpInboundDisconnectData)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @implemented
 */
INT
EXPORT
WSARecvFrom(
  IN      SOCKET s,
  IN OUT  LPWSABUF lpBuffers,
  IN      DWORD dwBufferCount,
  OUT     LPDWORD lpNumberOfBytesRecvd,
  IN OUT  LPDWORD lpFlags,
  OUT	    LPSOCKADDR lpFrom,
  IN OUT  LPINT lpFromlen,
  IN      LPWSAOVERLAPPED lpOverlapped,
  IN      LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
  PCATALOG_ENTRY Provider;
  INT Errno;
  INT Code;

  WS_DbgPrint(MAX_TRACE, ("Called.\n"));

  if (!ReferenceProviderByHandle((HANDLE)s, &Provider)) {
    WSASetLastError(WSAENOTSOCK);
    return SOCKET_ERROR;
  }

  assert(Provider->ProcTable.lpWSPRecvFrom);

  Code = Provider->ProcTable.lpWSPRecvFrom(s, lpBuffers, dwBufferCount,
    lpNumberOfBytesRecvd, lpFlags, lpFrom, lpFromlen, lpOverlapped,
    lpCompletionRoutine, NULL /* lpThreadId */, &Errno);

  DereferenceProviderByPointer(Provider);

  if (Code == SOCKET_ERROR)
    WSASetLastError(Errno);

  return Code;
}


/*
 * @implemented
 */
INT
EXPORT
WSASend(
  IN  SOCKET s,
  IN  LPWSABUF lpBuffers,
  IN  DWORD dwBufferCount,
  OUT LPDWORD lpNumberOfBytesSent,
  IN  DWORD dwFlags,
  IN  LPWSAOVERLAPPED lpOverlapped,
  IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
  PCATALOG_ENTRY Provider;
  INT Errno;
  INT Code;

  WS_DbgPrint(MAX_TRACE, ("Called.\n"));

  if (!ReferenceProviderByHandle((HANDLE)s, &Provider)) {
    WSASetLastError(WSAENOTSOCK);
    return SOCKET_ERROR;
  }

  assert(Provider->ProcTable.lpWSPSend);

  Code = Provider->ProcTable.lpWSPSend(s, lpBuffers, dwBufferCount,
    lpNumberOfBytesSent, dwFlags, lpOverlapped,
    lpCompletionRoutine, NULL /* lpThreadId */, &Errno);

  DereferenceProviderByPointer(Provider);

  if (Code == SOCKET_ERROR)
    WSASetLastError(Errno);

  return Code;
}


/*
 * @unimplemented
 */
INT
EXPORT
WSASendDisconnect(
  IN  SOCKET s,
  IN  LPWSABUF lpOutboundDisconnectData)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @implemented
 */
INT
EXPORT
WSASendTo(
  IN  SOCKET s,
  IN  LPWSABUF lpBuffers,
  IN  DWORD dwBufferCount,
  OUT LPDWORD lpNumberOfBytesSent,
  IN  DWORD dwFlags,
  IN  CONST LPSOCKADDR lpTo,
  IN  INT iToLen,
  IN  LPWSAOVERLAPPED lpOverlapped,
  IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
  PCATALOG_ENTRY Provider;
  INT Errno;
  INT Code;

  WS_DbgPrint(MAX_TRACE, ("Called.\n"));

  if (!ReferenceProviderByHandle((HANDLE)s, &Provider)) {
    WSASetLastError(WSAENOTSOCK);
    return SOCKET_ERROR;
  }

  assert(Provider->ProcTable.lpWSPSendTo);

  Code = Provider->ProcTable.lpWSPSendTo(s, lpBuffers, dwBufferCount,
    lpNumberOfBytesSent, dwFlags, lpTo, iToLen, lpOverlapped,
    lpCompletionRoutine, NULL /* lpThreadId */, &Errno);

  DereferenceProviderByPointer(Provider);

  if (Code == SOCKET_ERROR)
    WSASetLastError(Errno);

  return Code;
}

/* EOF */
