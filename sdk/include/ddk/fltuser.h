/*
 * fltuser.h
 *
 * This file is part of the ReactOS DDK package.
 *
 * Contributors:
 *   Amine Khaldi (amine.khaldi@reactos.org)
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

#ifndef __FLTUSER_H__
#define __FLTUSER_H__

#define FLT_MGR_BASELINE (((OSVER(NTDDI_VERSION) == NTDDI_WIN2K) && (SPVER(NTDDI_VERSION) >= SPVER(NTDDI_WIN2KSP4))) || \
                          ((OSVER(NTDDI_VERSION) == NTDDI_WINXP) && (SPVER(NTDDI_VERSION) >= SPVER(NTDDI_WINXPSP2))) || \
                          ((OSVER(NTDDI_VERSION) == NTDDI_WS03)  && (SPVER(NTDDI_VERSION) >= SPVER(NTDDI_WS03SP1))) ||  \
                          (NTDDI_VERSION >= NTDDI_VISTA))

#define FLT_MGR_AFTER_XPSP2 (((OSVER(NTDDI_VERSION) == NTDDI_WIN2K) && (SPVER(NTDDI_VERSION) >= SPVER(NTDDI_WIN2KSP4))) ||  \
                             ((OSVER(NTDDI_VERSION) == NTDDI_WINXP) && (SPVER(NTDDI_VERSION) >  SPVER(NTDDI_WINXPSP2))) ||  \
                             ((OSVER(NTDDI_VERSION) == NTDDI_WS03)  && (SPVER(NTDDI_VERSION) >= SPVER(NTDDI_WS03SP1))) ||   \
                             (NTDDI_VERSION >= NTDDI_VISTA))

#define FLT_MGR_LONGHORN (NTDDI_VERSION >= NTDDI_VISTA)
#define FLT_MGR_WIN7 (NTDDI_VERSION >= NTDDI_WIN7)

#include <fltuserstructures.h>

#ifdef __cplusplus
extern "C" {
#endif

#if FLT_MGR_BASELINE

#if FLT_MGR_LONGHORN
#define FLT_ASSERT(_e) NT_ASSERT(_e)
#define FLT_ASSERTMSG(_m, _e) NT_ASSERTMSG(_m, _e)
#else
#define FLT_ASSERT(_e) ASSERT(_e)
#define FLT_ASSERTMSG(_m, _e) ASSERTMSG(_m, _e)
#endif /* FLT_MGR_LONGHORN */

_Must_inspect_result_
HRESULT
WINAPI
FilterLoad(
    _In_ LPCWSTR lpFilterName);

_Must_inspect_result_
HRESULT
WINAPI
FilterUnload(
    _In_ LPCWSTR lpFilterName);

_Must_inspect_result_
HRESULT
WINAPI
FilterCreate(
    _In_ LPCWSTR lpFilterName,
    _Outptr_ HFILTER *hFilter);

HRESULT
WINAPI
FilterClose(
    _In_ HFILTER hFilter);

_Must_inspect_result_
HRESULT
WINAPI
FilterInstanceCreate(
    _In_ LPCWSTR lpFilterName,
    _In_ LPCWSTR lpVolumeName,
    _In_opt_ LPCWSTR lpInstanceName,
   _Outptr_ HFILTER_INSTANCE *hInstance);

HRESULT
WINAPI
FilterInstanceClose(
    _In_ HFILTER_INSTANCE hInstance);

_Must_inspect_result_
HRESULT
WINAPI
FilterAttach(
    _In_ LPCWSTR lpFilterName,
    _In_ LPCWSTR lpVolumeName,
    _In_opt_ LPCWSTR lpInstanceName ,
    _In_opt_ DWORD dwCreatedInstanceNameLength ,
    _Out_writes_bytes_opt_(dwCreatedInstanceNameLength) LPWSTR lpCreatedInstanceName);

_Must_inspect_result_
HRESULT
WINAPI
FilterAttachAtAltitude(
    _In_ LPCWSTR lpFilterName,
    _In_ LPCWSTR lpVolumeName,
    _In_ LPCWSTR lpAltitude,
    _In_opt_ LPCWSTR lpInstanceName ,
    _In_opt_ DWORD dwCreatedInstanceNameLength ,
    _Out_writes_bytes_opt_(dwCreatedInstanceNameLength) LPWSTR lpCreatedInstanceName);

_Must_inspect_result_
HRESULT
WINAPI
FilterDetach(
    _In_ LPCWSTR lpFilterName,
    _In_ LPCWSTR lpVolumeName,
    _In_opt_ LPCWSTR lpInstanceName);

_Must_inspect_result_
HRESULT
WINAPI
FilterFindFirst(
    _In_ FILTER_INFORMATION_CLASS dwInformationClass,
    _Out_writes_bytes_to_(dwBufferSize,*lpBytesReturned) LPVOID lpBuffer,
    _In_ DWORD dwBufferSize,
    _Out_ LPDWORD lpBytesReturned,
    _Out_ LPHANDLE lpFilterFind);

_Must_inspect_result_
HRESULT
WINAPI
FilterFindNext(
    _In_ HANDLE hFilterFind,
    _In_ FILTER_INFORMATION_CLASS dwInformationClass,
    _Out_writes_bytes_to_(dwBufferSize,*lpBytesReturned) LPVOID lpBuffer,
    _In_ DWORD dwBufferSize,
    _Out_ LPDWORD lpBytesReturned);

_Must_inspect_result_
HRESULT
WINAPI
FilterFindClose(
    _In_ HANDLE hFilterFind);

_Must_inspect_result_
HRESULT
WINAPI
FilterVolumeFindFirst(
    _In_ FILTER_VOLUME_INFORMATION_CLASS dwInformationClass,
    _Out_writes_bytes_to_(dwBufferSize,*lpBytesReturned) LPVOID lpBuffer,
    _In_ DWORD dwBufferSize,
    _Out_ LPDWORD lpBytesReturned,
    _Out_ PHANDLE lpVolumeFind);

_Must_inspect_result_
HRESULT
WINAPI
FilterVolumeFindNext(
    _In_ HANDLE hVolumeFind,
    _In_ FILTER_VOLUME_INFORMATION_CLASS dwInformationClass,
    _Out_writes_bytes_to_(dwBufferSize,*lpBytesReturned) LPVOID lpBuffer,
    _In_ DWORD dwBufferSize,
    _Out_ LPDWORD lpBytesReturned);

HRESULT
WINAPI
FilterVolumeFindClose(
    _In_ HANDLE hVolumeFind);

_Must_inspect_result_
HRESULT
WINAPI
FilterInstanceFindFirst(
    _In_ LPCWSTR lpFilterName,
    _In_ INSTANCE_INFORMATION_CLASS dwInformationClass,
    _Out_writes_bytes_to_(dwBufferSize,*lpBytesReturned) LPVOID lpBuffer,
    _In_ DWORD dwBufferSize,
    _Out_ LPDWORD lpBytesReturned,
    _Out_ LPHANDLE lpFilterInstanceFind);

_Must_inspect_result_
HRESULT
WINAPI
FilterInstanceFindNext(
    _In_ HANDLE hFilterInstanceFind,
    _In_ INSTANCE_INFORMATION_CLASS dwInformationClass,
    _Out_writes_bytes_to_(dwBufferSize,*lpBytesReturned) LPVOID lpBuffer,
    _In_ DWORD dwBufferSize,
    _Out_ LPDWORD lpBytesReturned);

_Must_inspect_result_
HRESULT
WINAPI
FilterInstanceFindClose(
    _In_ HANDLE hFilterInstanceFind);

_Must_inspect_result_
HRESULT
WINAPI
FilterVolumeInstanceFindFirst(
    _In_ LPCWSTR lpVolumeName,
    _In_ INSTANCE_INFORMATION_CLASS dwInformationClass,
    _Out_writes_bytes_to_(dwBufferSize,*lpBytesReturned) LPVOID lpBuffer,
    _In_ DWORD dwBufferSize,
    _Out_ LPDWORD lpBytesReturned,
    _Out_ LPHANDLE lpVolumeInstanceFind);

_Must_inspect_result_
HRESULT
WINAPI
FilterVolumeInstanceFindNext(
    _In_ HANDLE hVolumeInstanceFind,
    _In_ INSTANCE_INFORMATION_CLASS dwInformationClass,
    _Out_writes_bytes_to_(dwBufferSize,*lpBytesReturned) LPVOID lpBuffer,
    _In_ DWORD dwBufferSize,
    _Out_ LPDWORD lpBytesReturned);

HRESULT
WINAPI
FilterVolumeInstanceFindClose(
    _In_ HANDLE hVolumeInstanceFind);

_Must_inspect_result_
HRESULT
WINAPI
FilterGetInformation(
    _In_ HFILTER hFilter,
    _In_ FILTER_INFORMATION_CLASS dwInformationClass,
    _Out_writes_bytes_to_(dwBufferSize,*lpBytesReturned) LPVOID lpBuffer,
    _In_ DWORD dwBufferSize,
    _Out_ LPDWORD lpBytesReturned);

_Must_inspect_result_
HRESULT
WINAPI
FilterInstanceGetInformation(
    _In_ HFILTER_INSTANCE hInstance,
    _In_ INSTANCE_INFORMATION_CLASS dwInformationClass,
    _Out_writes_bytes_to_(dwBufferSize,*lpBytesReturned) LPVOID lpBuffer,
    _In_ DWORD dwBufferSize,
    _Out_ LPDWORD lpBytesReturned);

_Must_inspect_result_
HRESULT
WINAPI
FilterConnectCommunicationPort(
    _In_ LPCWSTR lpPortName,
    _In_ DWORD dwOptions,
    _In_reads_bytes_opt_(wSizeOfContext) LPCVOID lpContext,
    _In_ WORD wSizeOfContext,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes ,
    _Outptr_ HANDLE *hPort);

_Must_inspect_result_
HRESULT
WINAPI
FilterSendMessage(
    _In_ HANDLE hPort,
    _In_reads_bytes_opt_(dwInBufferSize) LPVOID lpInBuffer,
    _In_ DWORD dwInBufferSize,
    _Out_writes_bytes_to_opt_(dwOutBufferSize,*lpBytesReturned) LPVOID lpOutBuffer,
    _In_ DWORD dwOutBufferSize,
    _Out_ LPDWORD lpBytesReturned);

_Must_inspect_result_
HRESULT
WINAPI
FilterGetMessage(
    _In_ HANDLE hPort,
    _Out_writes_bytes_(dwMessageBufferSize) PFILTER_MESSAGE_HEADER lpMessageBuffer,
    _In_ DWORD dwMessageBufferSize,
    _Inout_ LPOVERLAPPED lpOverlapped);

_Must_inspect_result_
HRESULT
WINAPI
FilterReplyMessage(
    _In_ HANDLE hPort,
    _In_reads_bytes_(dwReplyBufferSize) PFILTER_REPLY_HEADER lpReplyBuffer,
    _In_ DWORD dwReplyBufferSize);

_Must_inspect_result_
HRESULT
WINAPI
FilterGetDosName(
    _In_ LPCWSTR lpVolumeName,
    _Out_writes_(dwDosNameBufferSize) LPWSTR lpDosName,
    _In_ DWORD dwDosNameBufferSize);

#endif /* FLT_MGR_BASELINE */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __FLTUSER_H__ */
