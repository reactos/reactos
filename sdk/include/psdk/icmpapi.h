/*
 * Interface to the ICMP functions.
 *
 * Copyright (C) 1999 Francois Gouget
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _ICMP_INCLUDED_
#define _ICMP_INCLUDED_

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

HANDLE WINAPI  IcmpCreateFile(
    VOID
    );

#if (NTDDI_VERSION >= NTDDI_WINXP)
HANDLE WINAPI  Icmp6CreateFile(
    VOID
    );
#endif

BOOL WINAPI  IcmpCloseHandle(
    HANDLE  IcmpHandle
    );

DWORD WINAPI  IcmpSendEcho(
    HANDLE                 IcmpHandle,
    IPAddr                 DestinationAddress,
    LPVOID                 RequestData,
    WORD                   RequestSize,
    PIP_OPTION_INFORMATION RequestOptions,
    LPVOID                 ReplyBuffer,
    DWORD                  ReplySize,
    DWORD                  Timeout
    );

DWORD
WINAPI
IcmpSendEcho2(
    HANDLE                   IcmpHandle,
    HANDLE                   Event,
#ifdef PIO_APC_ROUTINE_DEFINED
    PIO_APC_ROUTINE          ApcRoutine,
#else
    FARPROC                  ApcRoutine,
#endif
    PVOID                    ApcContext,
    IPAddr                   DestinationAddress,
    LPVOID                   RequestData,
    WORD                     RequestSize,
    PIP_OPTION_INFORMATION   RequestOptions,
    LPVOID                   ReplyBuffer,
    DWORD                    ReplySize,
    DWORD                    Timeout
    );

#if (NTDDI_VERSION >= NTDDI_VISTASP1)
DWORD
WINAPI
IcmpSendEcho2Ex(
    HANDLE                   IcmpHandle,
    HANDLE                   Event,
#ifdef PIO_APC_ROUTINE_DEFINED
    PIO_APC_ROUTINE          ApcRoutine,
#else
    FARPROC                  ApcRoutine,
#endif
    PVOID                    ApcContext,
    IPAddr                   SourceAddress,
    IPAddr                   DestinationAddress,
    LPVOID                   RequestData,
    WORD                     RequestSize,
    PIP_OPTION_INFORMATION   RequestOptions,
    LPVOID                   ReplyBuffer,
    DWORD                    ReplySize,
    DWORD                    Timeout
    );
#endif

#if (NTDDI_VERSION >= NTDDI_WINXP)
DWORD
WINAPI
Icmp6SendEcho2(
    HANDLE                   IcmpHandle,
    HANDLE                   Event,
#ifdef PIO_APC_ROUTINE_DEFINED
    PIO_APC_ROUTINE          ApcRoutine,
#else
    FARPROC                  ApcRoutine,
#endif
    PVOID                    ApcContext,
    struct sockaddr_in6     *SourceAddress,
    struct sockaddr_in6     *DestinationAddress,
    LPVOID                   RequestData,
    WORD                     RequestSize,
    PIP_OPTION_INFORMATION   RequestOptions,
    LPVOID                   ReplyBuffer,
    DWORD                    ReplySize,
    DWORD                    Timeout
    );
#endif

DWORD
WINAPI
IcmpParseReplies(
    LPVOID                   ReplyBuffer,
    DWORD                    ReplySize
    );

#if (NTDDI_VERSION >= NTDDI_WINXP)
DWORD
WINAPI
Icmp6ParseReplies(
    LPVOID                   ReplyBuffer,
    DWORD                    ReplySize
    );
#endif

#ifdef __cplusplus
}
#endif

#endif /* _ICMP_INCLUDED_ */
