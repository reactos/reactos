/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver DLL
 * FILE:        include/msafd.h
 * PURPOSE:     Ancillary Function Driver DLL header
 */
#ifndef __MSAFD_H
#define __MSAFD_H

#include <windows.h>
#include <ws2spi.h>
#define NTOS_USER_MODE
#include <ntos.h>
#include <ddk/tdi.h>
#include <wsahelp.h>
#include <winsock2.h>
#include <afd/shared.h>
#include <debug.h>

extern HANDLE GlobalHeap;
extern WSPUPCALLTABLE Upcalls;
extern LPWPUCOMPLETEOVERLAPPEDREQUEST lpWPUCompleteOverlappedRequest;

SOCKET
WSPAPI
WSPAccept(
    IN      SOCKET s,
    OUT     LPSOCKADDR addr,
    IN OUT  LPINT addrlen,
    IN      LPCONDITIONPROC lpfnCondition,
    IN      DWORD dwCallbackData,
    OUT     LPINT lpErrno);

INT
WSPAPI
WSPAddressToString(
    IN      LPSOCKADDR lpsaAddress,
    IN      DWORD dwAddressLength,
    IN      LPWSAPROTOCOL_INFOW lpProtocolInfo,
    OUT     LPWSTR lpszAddressString,
    IN OUT  LPDWORD lpdwAddressStringLength,
    OUT     LPINT lpErrno);

INT
WSPAPI
WSPAsyncSelect(
    IN  SOCKET s, 
    IN  HWND hWnd, 
    IN  UINT wMsg, 
    IN  LONG lEvent, 
    OUT LPINT lpErrno);

INT
WSPAPI WSPBind(
    IN  SOCKET s,
    IN  CONST LPSOCKADDR name, 
    IN  INT namelen, 
    OUT LPINT lpErrno);

INT
WSPAPI
WSPCancelBlockingCall(
    OUT LPINT lpErrno);

INT
WSPAPI
WSPCleanup(
    OUT LPINT lpErrno);

INT
WSPAPI
WSPCloseSocket(
    IN	SOCKET s,
    OUT	LPINT lpErrno);

INT
WSPAPI
WSPConnect(
    IN  SOCKET s,
    IN  CONST LPSOCKADDR name,
    IN  INT namelen,
    IN  LPWSABUF lpCallerData,
    OUT LPWSABUF lpCalleeData,
    IN  LPQOS lpSQOS,
    IN  LPQOS lpGQOS,
    OUT LPINT lpErrno);

INT
WSPAPI
WSPDuplicateSocket(
    IN  SOCKET s,
    IN  DWORD dwProcessId,
    OUT LPWSAPROTOCOL_INFOW lpProtocolInfo,
    OUT LPINT lpErrno);

INT
WSPAPI
WSPEnumNetworkEvents(
    IN  SOCKET s, 
    IN  WSAEVENT hEventObject, 
    OUT LPWSANETWORKEVENTS lpNetworkEvents, 
    OUT LPINT lpErrno);

INT
WSPAPI
WSPEventSelect(
    IN  SOCKET s,
    IN  WSAEVENT hEventObject,
    IN  LONG lNetworkEvents,
    OUT LPINT lpErrno);

BOOL
WSPAPI
WSPGetOverlappedResult(
    IN  SOCKET s,
    IN  LPWSAOVERLAPPED lpOverlapped,
    OUT LPDWORD lpcbTransfer,
    IN  BOOL fWait,
    OUT LPDWORD lpdwFlags,
    OUT LPINT lpErrno);

INT
WSPAPI
WSPGetPeerName(
    IN      SOCKET s, 
    OUT     LPSOCKADDR name, 
    IN OUT  LPINT namelen, 
    OUT     LPINT lpErrno);

BOOL
WSPAPI
WSPGetQOSByName(
    IN      SOCKET s, 
    IN OUT  LPWSABUF lpQOSName, 
    OUT     LPQOS lpQOS, 
    OUT     LPINT lpErrno);

INT
WSPAPI
WSPGetSockName(
    IN      SOCKET s,
    OUT     LPSOCKADDR name,
    IN OUT  LPINT namelen,
    OUT     LPINT lpErrno);

INT
WSPAPI
WSPGetSockOpt(
    IN      SOCKET s,
    IN      INT level,
    IN      INT optname,
    OUT	    CHAR FAR* optval,
    IN OUT  LPINT optlen,
    OUT     LPINT lpErrno);

INT
WSPAPI
WSPIoctl(
    IN  SOCKET s,
    IN  DWORD dwIoControlCode,
    IN  LPVOID lpvInBuffer,
    IN  DWORD cbInBuffer,
    OUT LPVOID lpvOutBuffer,
    IN  DWORD cbOutBuffer,
    OUT LPDWORD lpcbBytesReturned,
    IN  LPWSAOVERLAPPED lpOverlapped,
    IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    IN  LPWSATHREADID lpThreadId,
    OUT LPINT lpErrno);

SOCKET
WSPAPI
WSPJoinLeaf(
    IN  SOCKET s,
    IN  CONST LPSOCKADDR name,
    IN  INT namelen,
    IN  LPWSABUF lpCallerData,
    OUT LPWSABUF lpCalleeData,
    IN  LPQOS lpSQOS,
    IN  LPQOS lpGQOS,
    IN  DWORD dwFlags,
    OUT LPINT lpErrno);

INT
WSPAPI
WSPListen(
    IN  SOCKET s,
    IN  INT backlog,
    OUT LPINT lpErrno);

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
    OUT     LPINT lpErrno);

INT
WSPAPI
WSPRecvDisconnect(
    IN  SOCKET s,
    OUT LPWSABUF lpInboundDisconnectData,
    OUT LPINT lpErrno);

INT
WSPAPI
WSPRecvFrom(
    IN      SOCKET s,
    IN OUT  LPWSABUF lpBuffers,
    IN      DWORD dwBufferCount,
    OUT     LPDWORD lpNumberOfBytesRecvd,
    IN OUT  LPDWORD lpFlags,
    OUT     LPSOCKADDR lpFrom,
    IN OUT  LPINT lpFromlen,
    IN      LPWSAOVERLAPPED lpOverlapped,
    IN      LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    IN      LPWSATHREADID lpThreadId,
    OUT     LPINT lpErrno);

INT
WSPAPI
WSPSelect(
    IN      INT nfds,
    IN OUT  LPFD_SET readfds,
    IN OUT  LPFD_SET writefds,
    IN OUT  LPFD_SET exceptfds,
    IN      CONST LPTIMEVAL timeout,
    OUT     LPINT lpErrno);

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
    OUT LPINT lpErrno);

INT
WSPAPI
WSPSendDisconnect(
    IN  SOCKET s,
    IN  LPWSABUF lpOutboundDisconnectData,
    OUT LPINT lpErrno);

INT
WSPAPI
WSPSendTo(
    IN  SOCKET s,
    IN  LPWSABUF lpBuffers,
    IN  DWORD dwBufferCount,
    OUT LPDWORD lpNumberOfBytesSent,
    IN  DWORD dwFlags,
    IN  CONST LPSOCKADDR lpTo,
    IN  INT iTolen,
    IN  LPWSAOVERLAPPED lpOverlapped,
    IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    IN  LPWSATHREADID lpThreadId,
    OUT LPINT lpErrno);

INT
WSPAPI
WSPSetSockOpt(
    IN  SOCKET s,
    IN  INT level,
    IN  INT optname,
    IN  CONST CHAR FAR* optval,
    IN  INT optlen,
    OUT LPINT lpErrno);

INT
WSPAPI
WSPShutdown(
    IN  SOCKET s,
    IN  INT how,
    OUT LPINT lpErrno);

SOCKET
WSPAPI
WSPSocket(
    IN  INT af,
    IN  INT type,
    IN  INT protocol,
    IN  LPWSAPROTOCOL_INFOW lpProtocolInfo,
    IN  GROUP g,
    IN  DWORD dwFlags,
    OUT LPINT lpErrno);

INT
WSPAPI
WSPStringToAddress(
    IN      LPWSTR AddressString,
    IN      INT AddressFamily,
    IN      LPWSAPROTOCOL_INFOW lpProtocolInfo,
    OUT     LPSOCKADDR lpAddress,
    IN OUT  LPINT lpAddressLength,
    OUT     LPINT lpErrno);

#endif /* __MSAFD_H */

/* EOF */
