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
    UNIMPLEMENTED

    return FALSE;
}


WSAEVENT
EXPORT
WSACreateEvent(VOID)
{
    UNIMPLEMENTED

    return (WSAEVENT)0;
}


BOOL
EXPORT
WSAResetEvent(
    IN  WSAEVENT hEvent)
{
    UNIMPLEMENTED

    return FALSE;
}


BOOL
EXPORT
WSASetEvent(
    IN  WSAEVENT hEvent)
{
    UNIMPLEMENTED

    return FALSE;
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
    UNIMPLEMENTED

    return 0;
}

/* EOF */
