/*
 * ws2san.h
 *
 * WinSock Direct (SAN) support
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Casper S. Hornstrup <chorns@users.sourceforge.net>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#pragma once

#ifndef _WS2SAN_H_
#define _WS2SAN_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SO_MAX_RDMA_SIZE                  0x700D
#define SO_RDMA_THRESHOLD_SIZE            0x700E

#define MEM_READ                          1
#define MEM_WRITE                         2
#define MEM_READWRITE                     3

#define WSAID_REGISTERMEMORY \
  {0xC0B422F5, 0xF58C, 0x11d1, {0xAD, 0x6C, 0x00, 0xC0, 0x4F, 0xA3, 0x4A, 0x2D}}

#define WSAID_DEREGISTERMEMORY \
  {0xC0B422F6, 0xF58C, 0x11d1, {0xAD, 0x6C, 0x00, 0xC0, 0x4F, 0xA3, 0x4A, 0x2D}}

#define WSAID_REGISTERRDMAMEMORY \
  {0xC0B422F7, 0xF58C, 0x11d1, {0xAD, 0x6C, 0x00, 0xC0, 0x4F, 0xA3, 0x4A, 0x2D}}

#define WSAID_DEREGISTERRDMAMEMORY \
  {0xC0B422F8, 0xF58C, 0x11d1, {0xAD, 0x6C, 0x00, 0xC0, 0x4F, 0xA3, 0x4A, 0x2D}}

#define WSAID_RDMAWRITE \
  {0xC0B422F9, 0xF58C, 0x11d1, {0xAD, 0x6C, 0x00, 0xC0, 0x4F, 0xA3, 0x4A, 0x2D}}

#define WSAID_RDMAREAD \
  {0xC0B422FA, 0xF58C, 0x11d1, {0xAD, 0x6C, 0x00, 0xC0, 0x4F, 0xA3, 0x4A, 0x2D}}

#if(_WIN32_WINNT >= 0x0501)
#define WSAID_MEMORYREGISTRATIONCACHECALLBACK \
  {0xE5DA4AF8, 0xD824, 0x48CD, {0xA7, 0x99, 0x63, 0x37, 0xA9, 0x8E, 0xD2, 0xAF}}
#endif

typedef struct _WSPUPCALLTABLEEX {
  LPWPUCLOSEEVENT lpWPUCloseEvent;
  LPWPUCLOSESOCKETHANDLE lpWPUCloseSocketHandle;
  LPWPUCREATEEVENT lpWPUCreateEvent;
  LPWPUCREATESOCKETHANDLE lpWPUCreateSocketHandle;
  LPWPUFDISSET lpWPUFDIsSet;
  LPWPUGETPROVIDERPATH lpWPUGetProviderPath;
  LPWPUMODIFYIFSHANDLE lpWPUModifyIFSHandle;
  LPWPUPOSTMESSAGE lpWPUPostMessage;
  LPWPUQUERYBLOCKINGCALLBACK lpWPUQueryBlockingCallback;
  LPWPUQUERYSOCKETHANDLECONTEXT lpWPUQuerySocketHandleContext;
  LPWPUQUEUEAPC lpWPUQueueApc;
  LPWPURESETEVENT lpWPUResetEvent;
  LPWPUSETEVENT lpWPUSetEvent;
  LPWPUOPENCURRENTTHREAD lpWPUOpenCurrentThread;
  LPWPUCLOSETHREAD lpWPUCloseThread;
  LPWPUCOMPLETEOVERLAPPEDREQUEST lpWPUCompleteOverlappedRequest;
} WSPUPCALLTABLEEX, FAR *LPWSPUPCALLTABLEEX;

typedef struct _WSABUFEX {
  u_long len;
  _Field_size_bytes_(len) char FAR *buf;
  HANDLE handle;
} WSABUFEX, FAR * LPWSABUFEX;

typedef
_Must_inspect_result_
int
(WSPAPI *LPWSPSTARTUPEX)(
  _In_ WORD wVersionRequested,
  _Out_ LPWSPDATA lpWSPData,
  _In_ LPWSAPROTOCOL_INFOW lpProtocolInfo,
  _In_ LPWSPUPCALLTABLEEX lpUpcallTable,
  _Out_ LPWSPPROC_TABLE lpProcTable);

typedef
_Must_inspect_result_
HANDLE
(WSPAPI *LPFN_WSPREGISTERMEMORY)(
  _In_ SOCKET s,
  _In_reads_bytes_(dwBufferLength) PVOID lpBuffer,
  _In_ DWORD dwBufferLength,
  _In_ DWORD dwFlags,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPFN_WSPDEREGISTERMEMORY)(
  _In_ SOCKET s,
  _In_ HANDLE Handle,
  _Out_ LPINT lpErrno);

typedef
_Must_inspect_result_
int
(WSPAPI *LPFN_WSPREGISTERRDMAMEMORY)(
  _In_ SOCKET s,
  _In_reads_bytes_(dwBufferLength) PVOID lpBuffer,
  _In_ DWORD dwBufferLength,
  _In_ DWORD dwFlags,
  _Out_writes_bytes_(*lpdwDescriptorLength) LPVOID lpRdmaBufferDescriptor,
  _Inout_ LPDWORD lpdwDescriptorLength,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPFN_WSPDEREGISTERRDMAMEMORY)(
  _In_ SOCKET s,
  _In_reads_bytes_(dwDescriptorLength) LPVOID lpRdmaBufferDescriptor,
  _In_ DWORD dwDescriptorLength,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPFN_WSPRDMAWRITE)(
  _In_ SOCKET s,
  _In_reads_(dwBufferCount) LPWSABUFEX lpBuffers,
  _In_ DWORD dwBufferCount,
  _In_reads_bytes_(dwTargetDescriptorLength) LPVOID lpTargetBufferDescriptor,
  _In_ DWORD dwTargetDescriptorLength,
  _In_ DWORD dwTargetBufferOffset,
  _Out_ LPDWORD lpdwNumberOfBytesWritten,
  _In_ DWORD dwFlags,
  _In_opt_ LPWSAOVERLAPPED lpOverlapped,
  _In_opt_ LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
  _In_ LPWSATHREADID lpThreadId,
  _Out_ LPINT lpErrno);

typedef int
(WSPAPI *LPFN_WSPRDMAREAD)(
  _In_ SOCKET s,
  _In_reads_(dwBufferCount) LPWSABUFEX lpBuffers,
  _In_ DWORD dwBufferCount,
  _In_reads_bytes_(dwTargetDescriptorLength) LPVOID lpTargetBufferDescriptor,
  _In_ DWORD dwTargetDescriptorLength,
  _In_ DWORD dwTargetBufferOffset,
  _Out_ LPDWORD lpdwNumberOfBytesRead,
  _In_ DWORD dwFlags,
  _In_opt_ LPWSAOVERLAPPED lpOverlapped,
  _In_opt_ LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
  _In_ LPWSATHREADID lpThreadId,
  _Out_ LPINT lpErrno);

#if(_WIN32_WINNT >= 0x0501)
typedef
_Must_inspect_result_
int
(WSPAPI *LPFN_WSPMEMORYREGISTRATIONCACHECALLBACK)(
  _In_reads_bytes_(Size) PVOID lpvAddress,
  _In_ SIZE_T Size,
  _Out_ LPINT lpErrno);
#endif

_Must_inspect_result_
int
WSPAPI
WSPStartupEx(
  _In_ WORD wVersionRequested,
  _Out_ LPWSPDATA lpWSPData,
  _In_ LPWSAPROTOCOL_INFOW lpProtocolInfo,
  _In_ LPWSPUPCALLTABLEEX lpUpcallTable,
  _Out_ LPWSPPROC_TABLE lpProcTable);

_Must_inspect_result_
HANDLE
WSPAPI
WSPRegisterMemory(
  _In_ SOCKET s,
  _In_reads_bytes_(dwBufferLength) PVOID lpBuffer,
  _In_ DWORD dwBufferLength,
  _In_ DWORD dwFlags,
  _Out_ LPINT lpErrno);

int
WSPAPI
WSPDeregisterMemory(
  _In_ SOCKET s,
  _In_ HANDLE Handle,
  _Out_ LPINT lpErrno);

_Must_inspect_result_
int
WSPAPI
WSPRegisterRdmaMemory(
  _In_ SOCKET s,
  _In_reads_bytes_(dwBufferLength) PVOID lpBuffer,
  _In_ DWORD dwBufferLength,
  _In_ DWORD dwFlags,
  _Out_writes_bytes_(*lpdwDescriptorLength) LPVOID lpRdmaBufferDescriptor,
  _Inout_ LPDWORD lpdwDescriptorLength,
  _Out_ LPINT lpErrno);

int
WSPAPI
WSPDeregisterRdmaMemory(
  _In_ SOCKET s,
  _In_reads_bytes_(dwDescriptorLength) LPVOID lpRdmaBufferDescriptor,
  _In_ DWORD dwDescriptorLength,
  _Out_ LPINT lpErrno);

int
WSPAPI
WSPRdmaWrite(
  _In_ SOCKET s,
  _In_reads_(dwBufferCount) LPWSABUFEX lpBuffers,
  _In_ DWORD dwBufferCount,
  _In_reads_bytes_(dwTargetDescriptorLength) LPVOID lpTargetBufferDescriptor,
  _In_ DWORD dwTargetDescriptorLength,
  _In_ DWORD dwTargetBufferOffset,
  _Out_ LPDWORD lpdwNumberOfBytesWritten,
  _In_ DWORD dwFlags,
  _In_opt_ LPWSAOVERLAPPED lpOverlapped,
  _In_opt_ LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
  _In_ LPWSATHREADID lpThreadId,
  _Out_ LPINT lpErrno);

int
WSPAPI
WSPRdmaRead(
  _In_ SOCKET s,
  _In_reads_(dwBufferCount) LPWSABUFEX lpBuffers,
  _In_ DWORD dwBufferCount,
  _In_reads_bytes_(dwTargetDescriptorLength) LPVOID lpTargetBufferDescriptor,
  _In_ DWORD dwTargetDescriptorLength,
  _In_ DWORD dwTargetBufferOffset,
  _Out_ LPDWORD lpdwNumberOfBytesRead,
  _In_ DWORD dwFlags,
  _In_opt_ LPWSAOVERLAPPED lpOverlapped,
  _In_opt_ LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
  _In_ LPWSATHREADID lpThreadId,
  _Out_ LPINT lpErrno);

#if(_WIN32_WINNT >= 0x0501)
_Must_inspect_result_
int
WSPAPI
WSPMemoryRegistrationCacheCallback(
  _In_reads_bytes_(Size) PVOID lpvAddress,
  _In_ SIZE_T Size,
  _Out_ LPINT lpErrno);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _WS2SAN_H_ */
