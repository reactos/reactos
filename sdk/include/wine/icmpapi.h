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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_ICMPAPI_H
#define __WINE_ICMPAPI_H

#ifdef __cplusplus
extern "C" {
#endif


HANDLE WINAPI IcmpCreateFile(
    VOID
    );

HANDLE WINAPI Icmp6CreateFile(
    VOID
    );

BOOL WINAPI IcmpCloseHandle(
    HANDLE  IcmpHandle
    );

DWORD WINAPI IcmpParseReplies(
    LPVOID                 ReplyBuffer,
    DWORD                  ReplySize
    );

DWORD WINAPI Icmp6ParseReplies(
    LPVOID                 ReplyBuffer,
    DWORD                  ReplySize
    );

DWORD WINAPI IcmpSendEcho(
    HANDLE                 IcmpHandle,
    IPAddr                 DestinationAddress,
    LPVOID                 RequestData,
    WORD                   RequestSize,
    PIP_OPTION_INFORMATION RequestOptions,
    LPVOID                 ReplyBuffer,
    DWORD                  ReplySize,
    DWORD                  Timeout
    );

DWORD WINAPI IcmpSendEcho2(
    HANDLE                 IcmpHandle,
    HANDLE                 Event,
#ifdef __WINE_WINTERNL_H
    PIO_APC_ROUTINE        ApcRoutine,
#else
    FARPROC                ApcRoutine,
#endif
    PVOID                  ApcContext,
    IPAddr                 DestinationAddress,
    LPVOID                 RequestData,
    WORD                   RequestSize,
    PIP_OPTION_INFORMATION RequestOptions,
    LPVOID                 ReplyBuffer,
    DWORD                  ReplySize,
    DWORD                  Timeout
    );

DWORD WINAPI IcmpSendEcho2Ex(
    HANDLE                 IcmpHandle,
    HANDLE                 Event,
#ifdef __WINE_WINTERNL_H
    PIO_APC_ROUTINE        ApcRoutine,
#else
    FARPROC                ApcRoutine,
#endif
    PVOID                  ApcContext,
    IPAddr                 SourceAddress,
    IPAddr                 DestinationAddress,
    LPVOID                 RequestData,
    WORD                   RequestSize,
    PIP_OPTION_INFORMATION RequestOptions,
    LPVOID                 ReplyBuffer,
    DWORD                  ReplySize,
    DWORD                  Timeout
    );

DWORD WINAPI Icmp6SendEcho2(
    HANDLE                 IcmpHandle,
    HANDLE                 Event,
#ifdef __WINE_WINTERNL_H
    PIO_APC_ROUTINE        ApcRoutine,
#else
    FARPROC                ApcRoutine,
#endif
    PVOID                  ApcContext,
    struct sockaddr_in6*   SourceAddress,
    struct sockaddr_in6*   DestinationAddress,
    LPVOID                 RequestData,
    WORD                   RequestSize,
    PIP_OPTION_INFORMATION RequestOptions,
    LPVOID                 ReplyBuffer,
    DWORD                  ReplySize,
    DWORD                  Timeout
    );


#ifdef __cplusplus
}
#endif

#endif /* __WINE_ICMPAPI_H */
