/* $Id: registry.c,v 1.1 2000/08/11 12:35:47 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Rtl registry functions
 * FILE:              lib/ntdll/rtl/registry.c
 * PROGRAMER:         Eric Kohl
 * REVISION HISTORY:
 *                    2000/08/11: Created
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <ntdll/ntdll.h>


/* FUNCTIONS ***************************************************************/

NTSTATUS
STDCALL
RtlCheckRegistryKey (
	IN	ULONG	RelativeTo,
	IN	PWSTR	Path
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
STDCALL
RtlCreateRegistryKey (
	IN	ULONG	RelativeTo,
	IN	PWSTR	Path
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
STDCALL
RtlDeleteRegistryValue (
	IN	ULONG	RelativeTo,
	IN	PWSTR	Path,
	IN	PWSTR	ValueName
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
STDCALL
RtlQueryRegistryValues (
	IN	ULONG				RelativeTo,
	IN	PWSTR				Path,
	IN	PRTL_QUERY_REGISTRY_TABLE	QueryTable,
	IN	PVOID				Context,
	IN	PVOID				Environment
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
STDCALL
RtlWriteRegistryValue (
	IN	ULONG	RelativeTo,
	IN	PWSTR	Path,
	IN	PWSTR	ValueName,
	IN	ULONG	ValueType,
	IN	PVOID	ValueData,
	IN	ULONG	ValueLength
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS STDCALL
RtlpNtMakeTemporaryKey(HANDLE KeyHandle)
{
   return NtDeleteKey(KeyHandle);
}

/* EOF */
