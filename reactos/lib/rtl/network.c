/*
 *  ReactOS kernel
 *  Copyright (C) 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: network.c,v 1.1 2004/08/05 18:17:37 ion Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Network Address Translation implementation
 * FILE:              lib/rtl/network.c
 */

#include <ddk/ntddk.h>

#define NDEBUG
#include <debug.h>
/* FUNCTIONS *****************************************************************/

/*
* @unimplemented
*/
NTSTATUS
STDCALL
RtlIpv4AddressToStringA(
	PULONG IP,
	LPSTR Buffer
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
STDCALL
RtlIpv4AddressToStringExA(
	PULONG IP,
	PULONG Port,
	LPSTR Buffer,
	PULONG MaxSize
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
STDCALL
RtlIpv4AddressToStringW(
	PULONG IP,
	LPWSTR Buffer
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
STDCALL
RtlIpv4AddressToStringExW(
	PULONG IP,
	PULONG Port,
	LPWSTR Buffer,
	PULONG MaxSize
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
STDCALL
RtlIpv4StringToAddressA(
	IN LPSTR IpString,
	IN ULONG Base,
	OUT PVOID PtrToIpAddr,
	OUT ULONG IpAddr
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
STDCALL
RtlIpv4StringToAddressExA(
	IN LPSTR IpString,
	IN ULONG Base,
	OUT PULONG IpAddr,
	OUT PULONG Port
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
STDCALL
RtlIpv4StringToAddressW(
	IN LPWSTR IpString, 
	IN ULONG Base, 
	OUT PVOID PtrToIpAddr,
	OUT ULONG IpAddr
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
STDCALL
RtlIpv4StringToAddressExW(
	IN LPWSTR IpString,
	IN ULONG Base,
	OUT PULONG IpAddr,
	OUT PULONG Port
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
STDCALL
RtlIpv6AddressToStringA(
	PULONG IP,
	LPSTR Buffer
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
STDCALL
RtlIpv6AddressToStringExA(
	PULONG IP,
	PULONG Port,
	LPSTR Buffer,
	PULONG MaxSize
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
STDCALL
RtlIpv6AddressToStringW(
	PULONG IP,
	LPWSTR Buffer
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
STDCALL
RtlIpv6AddressToStringExW(
	PULONG IP,
	PULONG Port,
	LPWSTR Buffer,
	PULONG MaxSize
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
STDCALL
RtlIpv6StringToAddressA(
	IN LPSTR IpString,
	IN ULONG Base,
	OUT PVOID PtrToIpAddr,
	OUT ULONG IpAddr
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
STDCALL
RtlIpv6StringToAddressExA(
	IN LPSTR IpString,
	IN ULONG Base,
	OUT PULONG IpAddr,
	OUT PULONG Port
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
STDCALL
RtlIpv6StringToAddressW(
	IN LPWSTR IpString,
	IN ULONG Base,
	OUT PVOID PtrToIpAddr,
	OUT ULONG IpAddr
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
STDCALL
RtlIpv6StringToAddressExW(
	IN LPWSTR IpString,
	IN ULONG Base,
	OUT PULONG IpAddr,
	OUT PULONG Port
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}


/* EOF */
