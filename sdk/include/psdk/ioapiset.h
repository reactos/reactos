/*
 * ioapiset.h
 *
 * Interface definitions for api-ms-win-core-io-l1
 *
 * This file is part of the ReactOS SDK.
 *
 * Contributors:
 *   Created by Timo Kreuzer <timo.kreuzer@reactos.org>
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

#ifndef _IO_APISET_H_
#define _IO_APISET_H_

#pragma once

#include <minwindef.h>

#ifdef __cplusplus
extern "C" {
#endif

WINBASEAPI
BOOL
WINAPI
CancelIo(
    _In_ HANDLE hFile);

WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
CreateIoCompletionPort(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE ExistingCompletionPort,
    _In_ ULONG_PTR CompletionKey,
    _In_ DWORD NumberOfConcurrentThreads);

WINBASEAPI
BOOL
WINAPI
DeviceIoControl(
    _In_ HANDLE hDevice,
    _In_ DWORD dwIoControlCode,
    _In_reads_bytes_opt_(nInBufferSize) LPVOID lpInBuffer,
    _In_ DWORD nInBufferSize,
    _Out_writes_bytes_to_opt_(nOutBufferSize,*lpBytesReturned) LPVOID lpOutBuffer,
    _In_ DWORD nOutBufferSize,
    _Out_opt_ LPDWORD lpBytesReturned,
    _Inout_opt_ LPOVERLAPPED lpOverlapped);

WINBASEAPI
BOOL
WINAPI
GetOverlappedResult(
    _In_ HANDLE hFile,
    _In_ LPOVERLAPPED lpOverlapped,
    _Out_ LPDWORD lpNumberOfBytesTransferred,
    _In_ BOOL bWait);

WINBASEAPI
BOOL
WINAPI
GetOverlappedResultEx(
    _In_ HANDLE hFile,
    _In_ LPOVERLAPPED lpOverlapped,
    _Out_ LPDWORD lpNumberOfBytesTransferred,
    _In_ DWORD dwMilliseconds,
    _In_ BOOL bAlertable);

WINBASEAPI
BOOL
WINAPI
GetQueuedCompletionStatus(
    _In_ HANDLE CompletionPort,
    _Out_ LPDWORD lpNumberOfBytesTransferred,
    _Out_ PULONG_PTR lpCompletionKey,
    _Out_ LPOVERLAPPED* lpOverlapped,
    _In_ DWORD dwMilliseconds);

WINBASEAPI
BOOL
WINAPI
PostQueuedCompletionStatus(
    _In_ HANDLE CompletionPort,
    _In_ DWORD dwNumberOfBytesTransferred,
    _In_ ULONG_PTR dwCompletionKey,
    _In_opt_ LPOVERLAPPED lpOverlapped);

#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)

WINBASEAPI
BOOL
WINAPI
GetQueuedCompletionStatusEx(
    _In_ HANDLE CompletionPort,
    _Out_writes_to_(ulCount,*ulNumEntriesRemoved) LPOVERLAPPED_ENTRY lpCompletionPortEntries,
    _In_ ULONG ulCount,
    _Out_ PULONG ulNumEntriesRemoved,
    _In_ DWORD dwMilliseconds,
    _In_ BOOL fAlertable);

WINBASEAPI
BOOL
WINAPI
CancelIoEx(
    _In_ HANDLE hFile,
    _In_opt_ LPOVERLAPPED lpOverlapped);

WINBASEAPI
BOOL
WINAPI
CancelSynchronousIo(
    _In_ HANDLE hThread);

#endif // (_WIN32_WINNT >= _WIN32_WINNT_VISTA)

#ifdef __cplusplus
}
#endif

#endif // _IO_APISET_H_
