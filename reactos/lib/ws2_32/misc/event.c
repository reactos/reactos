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

/* EOF */
