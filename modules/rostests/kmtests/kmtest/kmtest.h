/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Kernel-Mode Test Suite loader application declarations
 * COPYRIGHT:   Copyright 2011-2018 Thomas Faber <thomas.faber@reactos.org>
 *              Copyright 2017 Ged Murphy <gedmurphy@reactos.org>
 */

#ifndef _KMTESTS_H_
#define _KMTESTS_H_

extern PCSTR ErrorFileAndLine;

#ifndef KMT_STRINGIZE
#define KMT_STRINGIZE(x) #x
#endif /* !defined KMT_STRINGIZE */

#define location(file, line)                    do { ErrorFileAndLine = file ":" KMT_STRINGIZE(line); } while (0)
#define error_value(Error, value)               do { location(__FILE__, __LINE__); Error = value; } while (0)
#define error(Error)                            error_value(Error, GetLastError())
#define error_goto(Error, label)                do { error(Error); goto label; } while (0)
#define error_value_goto(Error, value, label)   do { error_value(Error, value); goto label; } while (0)

/* service management functions */
DWORD
KmtServiceInit(VOID);

DWORD
KmtServiceCleanup(
    BOOLEAN IgnoreErrors);

DWORD
KmtCreateService(
    IN PCWSTR ServiceName,
    IN PCWSTR ServicePath,
    IN PCWSTR DisplayName OPTIONAL,
    OUT SC_HANDLE *ServiceHandle);

DWORD
KmtStartService(
    IN PCWSTR ServiceName OPTIONAL,
    IN OUT SC_HANDLE *ServiceHandle);

DWORD
KmtCreateAndStartService(
    IN PCWSTR ServiceName,
    IN PCWSTR ServicePath,
    IN PCWSTR DisplayName OPTIONAL,
    OUT SC_HANDLE *ServiceHandle,
    IN BOOLEAN RestartIfRunning);

DWORD
KmtStopService(
    IN PCWSTR ServiceName OPTIONAL,
    IN OUT SC_HANDLE *ServiceHandle);

DWORD
KmtDeleteService(
    IN PCWSTR ServiceName OPTIONAL,
    IN OUT SC_HANDLE *ServiceHandle);

DWORD KmtCloseService(
    IN OUT SC_HANDLE *ServiceHandle);


#ifdef KMT_FLT_USER_MODE

DWORD
KmtFltLoad(
    _In_z_ PCWSTR ServiceName);

DWORD
KmtFltCreateAndStartService(
    _In_z_ PCWSTR ServiceName,
    _In_z_ PCWSTR ServicePath,
    _In_z_ PCWSTR DisplayName OPTIONAL,
    _Out_ SC_HANDLE *ServiceHandle,
    _In_ BOOLEAN RestartIfRunning);

DWORD
KmtFltConnect(
    _In_z_ PCWSTR ServiceName,
    _Out_ HANDLE *hPort);

DWORD
KmtFltDisconnect(
    _Out_ HANDLE *hPort);

DWORD
KmtFltSendMessage(
    _In_ HANDLE hPort,
    _In_reads_bytes_(dwInBufferSize) LPVOID lpInBuffer,
    _In_ DWORD dwInBufferSize,
    _Out_writes_bytes_to_opt_(dwOutBufferSize, *lpBytesReturned) LPVOID lpOutBuffer,
    _In_ DWORD dwOutBufferSize,
    _Out_opt_ LPDWORD lpBytesReturned);

DWORD
KmtFltGetMessage(
    _In_ HANDLE hPort,
    _Out_writes_bytes_(dwMessageBufferSize) PFILTER_MESSAGE_HEADER lpMessageBuffer,
    _In_ DWORD dwMessageBufferSize,
    _In_opt_ LPOVERLAPPED Overlapped);

DWORD
KmtFltReplyMessage(
    _In_ HANDLE hPort,
    _In_reads_bytes_(dwReplyBufferSize) PFILTER_REPLY_HEADER lpReplyBuffer,
    _In_ DWORD dwReplyBufferSize);

DWORD
KmtFltGetMessageResult(
    _In_ HANDLE hPort,
    _In_ LPOVERLAPPED Overlapped,
    _Out_ LPDWORD BytesTransferred);

DWORD
KmtFltUnload(
    _In_z_ PCWSTR ServiceName);

DWORD
KmtFltDeleteService(
    _In_z_ PCWSTR ServiceName OPTIONAL,
    _Inout_ SC_HANDLE *ServiceHandle);

DWORD KmtFltCloseService(
    _Inout_ SC_HANDLE *ServiceHandle);

#endif /* KMT_FLT_USER_MODE */

#endif /* !defined _KMTESTS_H_ */
