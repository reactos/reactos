/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 DLL
 * FILE:        dll/win32/ws2_32/include/upcall.h
 * PURPOSE:     Upcall function defintions
 */

#ifndef __UPCALL_H
#define __UPCALL_H

BOOL
WSPAPI
WPUCloseEvent(
    IN  WSAEVENT hEvent,
    OUT LPINT lpErrno);

INT
WSPAPI
WPUCloseSocketHandle(
    IN  SOCKET s,
    OUT LPINT lpErrno);

INT
WSPAPI
WPUCloseThread(
    IN  LPWSATHREADID lpThreadId,
    OUT LPINT lpErrno);

WSAEVENT
WSPAPI
WPUCreateEvent(
    OUT LPINT lpErrno);

SOCKET
WSPAPI
WPUCreateSocketHandle(
    IN  DWORD dwCatalogEntryId,
    IN  DWORD_PTR dwContext,
    OUT LPINT lpErrno);

int
WSPAPI
WPUFDIsSet(
    IN  SOCKET s,
    IN  LPFD_SET set);

INT
WSPAPI
WPUGetProviderPath(
    IN      LPGUID lpProviderId,
    OUT     LPWSTR lpszProviderDllPath,
    IN OUT  LPINT lpProviderDllPathLen,
    OUT     LPINT lpErrno);

SOCKET
WSPAPI
WPUModifyIFSHandle(
    IN  DWORD dwCatalogEntryId,
    IN  SOCKET ProposedHandle,
    OUT LPINT lpErrno);

INT
WSPAPI
WPUOpenCurrentThread(
    OUT LPWSATHREADID lpThreadId,
    OUT LPINT lpErrno);

INT
WSPAPI
WPUQueryBlockingCallback(
    IN  DWORD dwCatalogEntryId,
    OUT LPBLOCKINGCALLBACK FAR* lplpfnCallback,
    OUT PDWORD_PTR lpdwContext,
    OUT LPINT lpErrno);

INT
WSPAPI
WPUQuerySocketHandleContext(
    IN  SOCKET s,
    OUT PDWORD_PTR lpContext,
    OUT LPINT lpErrno);

INT
WSPAPI
WPUQueueApc(
    IN  LPWSATHREADID lpThreadId,
    IN  LPWSAUSERAPC lpfnUserApc,
    IN  DWORD_PTR dwContext,
    OUT LPINT lpErrno);

BOOL
WSPAPI
WPUResetEvent(
    IN  WSAEVENT hEvent,
    OUT LPINT lpErrno);

BOOL
WSPAPI
WPUSetEvent(
    IN  WSAEVENT hEvent,
    OUT LPINT lpErrno);

#endif /* __UPCALL_H */
