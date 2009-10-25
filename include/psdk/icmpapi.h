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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_ICMPAPI_H
#define __WINE_ICMPAPI_H

HANDLE WINAPI  IcmpCreateFile(
    VOID
    );

HANDLE WINAPI  Icmp6CreateFile(
    VOID
    );

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
    FARPROC                  ApcRoutine,
    PVOID                    ApcContext,
    IPAddr                   DestinationAddress,
    LPVOID                   RequestData,
    WORD                     RequestSize,
    PIP_OPTION_INFORMATION   RequestOptions,
    LPVOID                   ReplyBuffer,
    DWORD                    ReplySize,
    DWORD                    Timeout
    );

DWORD
WINAPI
Icmp6SendEcho2(
    HANDLE                   IcmpHandle,
    HANDLE                   Event,
    FARPROC                  ApcRoutine,
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

DWORD
WINAPI
IcmpParseReplies(
    LPVOID                   ReplyBuffer,
    DWORD                    ReplySize
    );

DWORD
WINAPI
Icmp6ParseReplies(
    LPVOID                   ReplyBuffer,
    DWORD                    ReplySize
    );

#endif /* __WINE_ICMPAPI_H */
