/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 DLL
 * FILE:        misc/event.c
 * PURPOSE:     Event handling
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 */
#include <ws2_32.h>
#include <handle.h>


/*
 * @implemented
 */
BOOL
EXPORT
WSACloseEvent(
  IN  WSAEVENT hEvent)
{
  BOOL Success;

  if (!WSAINITIALIZED) {
    WSASetLastError(WSANOTINITIALISED);
    return FALSE;
  }

  Success = CloseHandle((HANDLE)hEvent);

  if (!Success)
    WSASetLastError(WSA_INVALID_HANDLE);

  return Success;
}


/*
 * @implemented
 */
WSAEVENT
EXPORT
WSACreateEvent(VOID)
{
  HANDLE Event;

  if (!WSAINITIALIZED) {
    WSASetLastError(WSANOTINITIALISED);
    return FALSE;
  }

  Event = CreateEvent(NULL, TRUE, FALSE, NULL);

  if (Event == INVALID_HANDLE_VALUE)
    WSASetLastError(WSA_INVALID_HANDLE);
  
  return (WSAEVENT)Event;
}


/*
 * @implemented
 */
BOOL
EXPORT
WSAResetEvent(
  IN  WSAEVENT hEvent)
{
  BOOL Success;

  if (!WSAINITIALIZED) {
    WSASetLastError(WSANOTINITIALISED);
    return FALSE;
  }

  Success = ResetEvent((HANDLE)hEvent);

  if (!Success)
    WSASetLastError(WSA_INVALID_HANDLE);

  return Success;
}


/*
 * @implemented
 */
BOOL
EXPORT
WSASetEvent(
  IN  WSAEVENT hEvent)
{
  BOOL Success;

  if (!WSAINITIALIZED) {
    WSASetLastError(WSANOTINITIALISED);
    return FALSE;
  }

  Success = SetEvent((HANDLE)hEvent);

  if (!Success)
    WSASetLastError(WSA_INVALID_HANDLE);

  return Success;
}


/*
 * @implemented
 */
DWORD
EXPORT
WSAWaitForMultipleEvents(
  IN  DWORD cEvents,
  IN  CONST WSAEVENT FAR* lphEvents,
  IN  BOOL fWaitAll,
  IN  DWORD dwTimeout,
  IN  BOOL fAlertable)
{
  DWORD Status;

  if (!WSAINITIALIZED) {
    WSASetLastError(WSANOTINITIALISED);
    return FALSE;
  }

  Status = WaitForMultipleObjectsEx(cEvents, lphEvents, fWaitAll, dwTimeout, fAlertable);
  if (Status == WAIT_FAILED) {
    Status = GetLastError();

    if (Status == ERROR_NOT_ENOUGH_MEMORY)
      WSASetLastError(WSA_NOT_ENOUGH_MEMORY);
    else if (Status == ERROR_INVALID_HANDLE)
      WSASetLastError(WSA_INVALID_HANDLE);
    else
      WSASetLastError(WSA_INVALID_PARAMETER);

    return WSA_WAIT_FAILED;
  }

  return Status;
}


/*
 * @implemented
 */
INT
EXPORT
WSAEnumNetworkEvents(
  IN  SOCKET s,
  IN  WSAEVENT hEventObject,
  OUT LPWSANETWORKEVENTS lpNetworkEvents)
{
  PCATALOG_ENTRY Provider;
  INT Status;
  INT Errno;

  if (!lpNetworkEvents) {
    WSASetLastError(WSAEINVAL);
    return SOCKET_ERROR;
  }

  if (!WSAINITIALIZED) {
    WSASetLastError(WSANOTINITIALISED);
    return SOCKET_ERROR;
  }

  if (!ReferenceProviderByHandle((HANDLE)s, &Provider)) {
    WSASetLastError(WSAENOTSOCK);
    return SOCKET_ERROR;
  }

  Status = Provider->ProcTable.lpWSPEnumNetworkEvents(
    s, hEventObject, lpNetworkEvents, &Errno);

  DereferenceProviderByPointer(Provider);

  if (Status == SOCKET_ERROR)
    WSASetLastError(Errno);

  return Status;
}


/*
 * @implemented
 */
INT
EXPORT
WSAEventSelect(
    IN  SOCKET s,
    IN  WSAEVENT hEventObject,
    IN  LONG lNetworkEvents)
{
  PCATALOG_ENTRY Provider;
  INT Status;
  INT Errno;

  if (!WSAINITIALIZED) {
    WSASetLastError(WSANOTINITIALISED);
    return SOCKET_ERROR;
  }

  if (!ReferenceProviderByHandle((HANDLE)s, &Provider)) {
    WSASetLastError(WSAENOTSOCK);
    return SOCKET_ERROR;
  }

  Status = Provider->ProcTable.lpWSPEventSelect(
    s, hEventObject, lNetworkEvents, &Errno);

  DereferenceProviderByPointer(Provider);

  if (Status == SOCKET_ERROR)
    WSASetLastError(Errno);

  return Status;
}

/* EOF */
