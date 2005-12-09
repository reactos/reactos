/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 Helper DLL
 * FILE:        include/libs/winsock/ws2help.h
 * PURPOSE:     WinSock 2 Helper DLL header
 */

#ifndef __WS2HELP_H
#define __WS2HELP_H

typedef struct _WSH_HANDLE_TABLE *PWAH_HANDLE_TABLE;

typedef struct _WSH_HANDLE
{
    LONG RefCount;
    HANDLE Handle;
} WSH_HANDLE, *PWAH_HANDLE;

typedef BOOL
(WINAPI *PWAH_HANDLE_ENUMERATE_PROC)(
    IN PVOID Context,
    IN PWAH_HANDLE Handle
);

PWAH_HANDLE
WINAPI
WahReferenceContextByHandle(
    IN PWAH_HANDLE_TABLE Table,
    IN HANDLE Handle
);

DWORD
WINAPI
WahRemoveHandleContext(
    IN PWAH_HANDLE_TABLE Table,
    IN PWAH_HANDLE Handle
);

DWORD
WINAPI
WahCloseSocketHandle(
    IN HANDLE HelperHandle,
    IN SOCKET Socket
);

DWORD
WINAPI
WahOpenCurrentThread(
    IN HANDLE HelperHandle,
    OUT LPWSATHREADID ThreadId
);

DWORD
WINAPI
WahCloseApcHelper(
    IN HANDLE HelperHandle
);

DWORD
WINAPI
WahCloseThread(
    IN HANDLE HelperHandle,
    IN LPWSATHREADID ThreadId
);

DWORD
WINAPI
WahCloseHandleHelper(
    IN HANDLE HelperHandle
);

DWORD
WINAPI
WahCloseNotificationHandleHelper(
    IN HANDLE HelperHandle
);

DWORD
WINAPI
WahOpenNotificationHandleHelper(
    OUT PHANDLE HelperHandle
);

DWORD
WINAPI
WahCreateNotificationHandle(
    IN HANDLE HelperHandle,
    OUT PHANDLE NotificationHelperHandle
);

INT
WINAPI 
WahWaitForNotification(
    IN HANDLE NotificationHelperHandle,
    IN HANDLE lpNotificationHandle,
    IN LPWSAOVERLAPPED lpOverlapped,
    IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
);

INT
WINAPI
WahNotifyAllProcesses(
    IN HANDLE NotificationHelperHandle
);

BOOL
WINAPI
WahEnumerateHandleContexts(
    IN PWAH_HANDLE_TABLE Table,
    IN PWAH_HANDLE_ENUMERATE_PROC Callback,
    IN PVOID Context
);

DWORD
WINAPI
WahCreateHandleContextTable(
    OUT PWAH_HANDLE_TABLE *Table
);

DWORD
WINAPI
WahDestroyHandleContextTable(
    IN PWAH_HANDLE_TABLE Table
);

PWAH_HANDLE
WINAPI
WahInsertHandleContext(
    IN PWAH_HANDLE_TABLE Table,
    IN PWAH_HANDLE Handle
);

DWORD
WINAPI
WahOpenApcHelper(
    OUT HANDLE ApcHelperHandle
);

#endif
