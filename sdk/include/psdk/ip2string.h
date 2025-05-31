/*
 * Copyright (C) 2019 Alex Henrie
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

#ifndef __WINE_IP2STRING_H
#define __WINE_IP2STRING_H

#ifdef __cplusplus
extern "C" {
#endif

NTSYSAPI char * WINAPI RtlIpv4AddressToStringA(const IN_ADDR *address, char *str);
NTSYSAPI WCHAR * WINAPI RtlIpv4AddressToStringW(const IN_ADDR *address, WCHAR *str);
#define RtlIpv4AddressToString WINELIB_NAME_AW(RtlIpv4AddressToString)
NTSYSAPI NTSTATUS WINAPI RtlIpv4AddressToStringExA(const IN_ADDR *address, USHORT port, char *str, ULONG *size);
NTSYSAPI NTSTATUS WINAPI RtlIpv4AddressToStringExW(const IN_ADDR *address, USHORT port, WCHAR *str, ULONG *size);
#define RtlIpv4AddressToStringEx WINELIB_NAME_AW(RtlIpv4AddressToStringEx)
NTSYSAPI char * WINAPI RtlIpv6AddressToStringA(const IN6_ADDR *address, char *str);
NTSYSAPI WCHAR * WINAPI RtlIpv6AddressToStringW(const IN6_ADDR *address, WCHAR *str);
#define RtlIpv6AddressToString WINELIB_NAME_AW(RtlIpv6AddressToString)
NTSYSAPI NTSTATUS WINAPI RtlIpv6AddressToStringExA(const IN6_ADDR *address, LONG scope, USHORT port, char *str, ULONG *size);
NTSYSAPI NTSTATUS WINAPI RtlIpv6AddressToStringExW(const IN6_ADDR *address, LONG scope, USHORT port, WCHAR *str, ULONG *size);
#define RtlIpv6AddressToStringEx WINELIB_NAME_AW(RtlIpv6AddressToStringEx)
NTSYSAPI NTSTATUS WINAPI RtlIpv4StringToAddressA(const char *str, BOOLEAN strict, const char **terminator, IN_ADDR *address);
NTSYSAPI NTSTATUS WINAPI RtlIpv4StringToAddressW(const WCHAR *str, BOOLEAN strict, const WCHAR **terminator, IN_ADDR *address);
#define RtlIpv4StringToAddress WINELIB_NAME_AW(RtlIpv4StringToAddress)
NTSYSAPI NTSTATUS WINAPI RtlIpv4StringToAddressExA(const char *str, BOOLEAN strict, IN_ADDR *address, USHORT *port);
NTSYSAPI NTSTATUS WINAPI RtlIpv4StringToAddressExW(const WCHAR *str, BOOLEAN strict, IN_ADDR *address, USHORT *port);
#define RtlIpv4StringToAddressEx WINELIB_NAME_AW(RtlIpv4StringToAddressEx)
NTSYSAPI NTSTATUS WINAPI RtlIpv6StringToAddressA(const char *str, const char **terminator, IN6_ADDR *address);
NTSYSAPI NTSTATUS WINAPI RtlIpv6StringToAddressW(const WCHAR *str, const WCHAR **terminator, IN6_ADDR *address);
#define RtlIpv6StringToAddress WINELIB_NAME_AW(RtlIpv6StringToAddress)
NTSYSAPI NTSTATUS WINAPI RtlIpv6StringToAddressExA(const char *str, IN6_ADDR *address, ULONG *scope, USHORT *port);
NTSYSAPI NTSTATUS WINAPI RtlIpv6StringToAddressExW(const WCHAR *str, IN6_ADDR *address, ULONG *scope, USHORT *port);
#define RtlIpv6StringToAddressEx WINELIB_NAME_AW(RtlIpv6StringToAddressEx)

#ifdef __cplusplus
}
#endif

#endif
