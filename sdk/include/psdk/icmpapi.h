/*
 * PROJECT:     ReactOS PSDK
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ICMP API definitions
 * COPYRIGHT:   Copyright 2025 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#ifndef _ICMP_INCLUDED_
#define _ICMP_INCLUDED_

#pragma once

#ifndef IPHLPAPI_DLL_LINKAGE
#ifdef DECLSPEC_IMPORT
#define IPHLPAPI_DLL_LINKAGE DECLSPEC_IMPORT
#else
#define IPHLPAPI_DLL_LINKAGE
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

IPHLPAPI_DLL_LINKAGE
HANDLE
WINAPI
IcmpCreateFile(
    VOID);

#if (NTDDI_VERSION >= NTDDI_WINXP)
IPHLPAPI_DLL_LINKAGE
HANDLE
WINAPI
Icmp6CreateFile(
    VOID);
#endif

IPHLPAPI_DLL_LINKAGE
BOOL
WINAPI
IcmpCloseHandle(
    _In_ HANDLE IcmpHandle);

IPHLPAPI_DLL_LINKAGE
DWORD
WINAPI
IcmpSendEcho(
    _In_ HANDLE IcmpHandle,
    _In_ IPAddr DestinationAddress,
    _In_reads_bytes_(RequestSize) LPVOID RequestData,
    _In_ WORD RequestSize,
    _In_opt_ PIP_OPTION_INFORMATION RequestOptions,
    _Out_writes_bytes_(ReplySize) LPVOID ReplyBuffer,
    _In_range_(>=, sizeof(ICMP_ECHO_REPLY) + RequestSize + 8)
        DWORD ReplySize,
    _In_ DWORD Timeout);

IPHLPAPI_DLL_LINKAGE
DWORD
WINAPI
IcmpSendEcho2(
    _In_ HANDLE IcmpHandle,
    _In_opt_ HANDLE Event,
#ifdef PIO_APC_ROUTINE_DEFINED
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
#else
    _In_opt_ FARPROC ApcRoutine,
#endif
    _In_opt_ PVOID ApcContext,
    _In_ IPAddr DestinationAddress,
    _In_reads_bytes_(RequestSize) LPVOID RequestData,
    _In_ WORD RequestSize,
    _In_opt_ PIP_OPTION_INFORMATION RequestOptions,
    _Out_writes_bytes_(ReplySize) LPVOID ReplyBuffer,
    _In_range_(>=, sizeof(ICMP_ECHO_REPLY) + RequestSize + 8)
        DWORD ReplySize,
    _In_ DWORD Timeout);

#if (NTDDI_VERSION >= NTDDI_VISTASP1)
IPHLPAPI_DLL_LINKAGE
DWORD
WINAPI
IcmpSendEcho2Ex(
    _In_ HANDLE IcmpHandle,
    _In_opt_ HANDLE Event,
#ifdef PIO_APC_ROUTINE_DEFINED
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
#else
    _In_opt_ FARPROC ApcRoutine,
#endif
    _In_opt_ PVOID ApcContext,
    _In_ IPAddr SourceAddress,
    _In_ IPAddr DestinationAddress,
    _In_reads_bytes_(RequestSize) LPVOID RequestData,
    _In_ WORD RequestSize,
    _In_opt_ PIP_OPTION_INFORMATION RequestOptions,
    _Out_writes_bytes_(ReplySize) LPVOID ReplyBuffer,
    _In_range_(>=, sizeof(ICMP_ECHO_REPLY) + RequestSize + 8 + sizeof(IO_STATUS_BLOCK))
        DWORD ReplySize,
    _In_ DWORD Timeout);
#endif

#if (NTDDI_VERSION >= NTDDI_WINXP)
IPHLPAPI_DLL_LINKAGE
DWORD
WINAPI
Icmp6SendEcho2(
    _In_ HANDLE IcmpHandle,
    _In_opt_ HANDLE Event,
#ifdef PIO_APC_ROUTINE_DEFINED
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
#else
    _In_opt_ FARPROC ApcRoutine,
#endif
    _In_opt_ PVOID ApcContext,
    _In_ struct sockaddr_in6 *SourceAddress,
    _In_ struct sockaddr_in6 *DestinationAddress,
    _In_reads_bytes_(RequestSize) LPVOID RequestData,
    _In_ WORD RequestSize,
    _In_opt_ PIP_OPTION_INFORMATION RequestOptions,
    _Out_writes_bytes_(ReplySize) LPVOID ReplyBuffer,
    _In_range_(>=, sizeof(ICMPV6_ECHO_REPLY) + RequestSize + 8 + sizeof(IO_STATUS_BLOCK))
        DWORD ReplySize,
    _In_ DWORD Timeout);
#endif

IPHLPAPI_DLL_LINKAGE
DWORD
WINAPI
IcmpParseReplies(
    _Out_writes_bytes_(ReplySize) LPVOID ReplyBuffer,
    _In_range_(>=, sizeof(ICMP_ECHO_REPLY) + 8)
        DWORD ReplySize);

#if (NTDDI_VERSION >= NTDDI_WINXP)
IPHLPAPI_DLL_LINKAGE
DWORD
WINAPI
Icmp6ParseReplies(
    _Out_writes_bytes_(ReplySize) LPVOID ReplyBuffer,
    _In_range_(>=, sizeof(ICMPV6_ECHO_REPLY) + 8)
        DWORD ReplySize);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _ICMP_INCLUDED_ */
