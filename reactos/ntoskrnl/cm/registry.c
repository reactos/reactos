/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cm/registry.c
 * PURPOSE:         Registry functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
STDCALL 
NtInitializeRegistry(
	BOOLEAN SetUpBoot
	)
{
}

NTSTATUS
STDCALL 
ZwInitializeRegistry(
	BOOLEAN SetUpBoot
	)
{
}

NTSTATUS RtlCheckRegistryKey(ULONG RelativeTo, PWSTR Path)
{
   UNIMPLEMENTED;
}

NTSTATUS RtlCreateRegistryKey(ULONG RelativeTo, PWSTR Path)
{
   UNIMPLEMENTED;
}

NTSTATUS RtlDeleteRegistryValue(ULONG RelativeTo, PWSTR Path,
				PWSTR ValueName)
{
   UNIMPLEMENTED;
}

NTSTATUS RtlQueryRegistryValues(ULONG RelativeTo,
				PWSTR Path,
				PRTL_QUERY_REGISTRY_TABLE QueryTable,
				PVOID Context,
				PVOID Environment)
{
   UNIMPLEMENTED;
}

NTSTATUS RtlWriteRegistryValue(ULONG RelativeTo,
			       PWSTR Path,
			       PWSTR ValueName,
			       ULONG ValueType,
			       PVOID ValueData,
			       ULONG ValueLength)
{
   UNIMPLEMENTED;
}

NTSTATUS NtCreateKey(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess,
		     POBJECT_ATTRIBUTES ObjectAttributes, ULONG TitleIndex,
		     PUNICODE_STRING Class, ULONG CreateOptions, 
		     PULONG Disposition)
{
}

NTSTATUS ZwCreateKey(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess,
		     POBJECT_ATTRIBUTES ObjectAttributes, ULONG TitleIndex,
		     PUNICODE_STRING Class, ULONG CreateOptions, 
		     PULONG Disposition)
{
   UNIMPLEMENTED;
}

NTSTATUS NtDeleteKey(HANDLE KeyHandle)
{
}

NTSTATUS ZwDeleteKey(HANDLE KeyHandle)
{
   UNIMPLEMENTED;
}

NTSTATUS NtEnumerateKey(HANDLE KeyHandle, ULONG Index, 
			KEY_INFORMATION_CLASS KeyInformationClass,
			PVOID KeyInformation,
			ULONG Length,
			PULONG ResultLength)
{
}

NTSTATUS ZwEnumerateKey(HANDLE KeyHandle, ULONG Index, 
			KEY_INFORMATION_CLASS KeyInformationClass,
			PVOID KeyInformation,
			ULONG Length,
			PULONG ResultLength)
{
   UNIMPLEMENTED;
}

NTSTATUS NtEnumerateValueKey(HANDLE KeyHandle, ULONG Index, 
			     KEY_VALUE_INFORMATION_CLASS KeyInformationClass,
			     PVOID KeyInformation,
			     ULONG Length,
			     PULONG ResultLength)
{
}

NTSTATUS ZwEnumerateValueKey(HANDLE KeyHandle, ULONG Index, 
			     KEY_VALUE_INFORMATION_CLASS KeyInformationClass,
			     PVOID KeyInformation,
			     ULONG Length,
			     PULONG ResultLength)
{
   UNIMPLEMENTED;
}

NTSTATUS NtFlushKey(HANDLE KeyHandle)
{
}

NTSTATUS ZwFlushKey(HANDLE KeyHandle)
{
   UNIMPLEMENTED;
}

NTSTATUS NtOpenKey(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess,
		   POBJECT_ATTRIBUTES ObjectAttributes)
{
}

NTSTATUS ZwOpenKey(PHANDLE KeyHandle, ACCESS_MASK DesiredAccess,
		   POBJECT_ATTRIBUTES ObjectAttributes)
{
   UNIMPLEMENTED;
}

NTSTATUS NtQueryKey(HANDLE KeyHandle, 
		    KEY_INFORMATION_CLASS KeyInformationClass,
		    PVOID KeyInformation,
		    ULONG Length,
		    PULONG ResultLength)
{
}

NTSTATUS ZwQueryKey(HANDLE KeyHandle, 
		    KEY_INFORMATION_CLASS KeyInformationClass,
		    PVOID KeyInformation,
		    ULONG Length,
		    PULONG ResultLength)
{
   UNIMPLEMENTED;
}

NTSTATUS NtQueryValueKey(HANDLE KeyHandle,
		    PUNICODE_STRING ValueName,
		    KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
		    PVOID KeyValueInformation,
		    ULONG Length,
		    PULONG ResultLength)
{
}

NTSTATUS ZwQueryValueKey(HANDLE KeyHandle,
		    PUNICODE_STRING ValueName,
		    KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
		    PVOID KeyValueInformation,
		    ULONG Length,
		    PULONG ResultLength)
{
   UNIMPLEMENTED;
}

NTSTATUS NtSetValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName,
		       ULONG TitleIndex, ULONG Type, PVOID Data,
		       ULONG DataSize)
{
}

NTSTATUS ZwSetValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName,
		       ULONG TitleIndex, ULONG Type, PVOID Data,
		       ULONG DataSize)
{
   UNIMPLEMENTED;
}

NTSTATUS
STDCALL
NtDeleteValueKey(
	IN HANDLE KeyHandle,
	IN PUNICODE_STRING ValueName
	)
{
}

NTSTATUS
STDCALL
ZwDeleteValueKey(
	IN HANDLE KeyHandle,
	IN PUNICODE_STRING ValueName	
		 )
{
}

NTSTATUS
STDCALL 
NtLoadKey(
	PHANDLE KeyHandle,
	OBJECT_ATTRIBUTES ObjectAttributes
	)
{
}

NTSTATUS
STDCALL 
ZwLoadKey(
	PHANDLE KeyHandle,
	OBJECT_ATTRIBUTES ObjectAttributes
	)
{
}

NTSTATUS STDCALL NtLoadKey2(VOID)
{
}

NTSTATUS
STDCALL
NtNotifyChangeKey(
	IN HANDLE KeyHandle,
	IN HANDLE Event,
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, 
	IN PVOID ApcContext OPTIONAL, 
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG CompletionFilter,
	IN BOOLEAN Asynchroneous, 
	OUT PVOID ChangeBuffer,
	IN ULONG Length,
	IN BOOLEAN WatchSubtree
	)
{
}

NTSTATUS
STDCALL
ZwNotifyChangeKey(
	IN HANDLE KeyHandle,
	IN HANDLE Event,
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, 
	IN PVOID ApcContext OPTIONAL, 
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG CompletionFilter,
	IN BOOLEAN Asynchroneous, 
	OUT PVOID ChangeBuffer,
	IN ULONG Length,
	IN BOOLEAN WatchSubtree
	)
{
}

NTSTATUS
STDCALL
NtQueryMultipleValueKey(
   HANDLE KeyHandle,	
   PVALENT ListOfValuesToQuery,	
   ULONG NumberOfItems,	
   PVOID MultipleValueInformation,		
   ULONG Length,
   PULONG  ReturnLength
)
{
}

NTSTATUS
STDCALL
ZwQueryMultipleValueKey(
   HANDLE KeyHandle,	
   PVALENT ListOfValuesToQuery,	
   ULONG NumberOfItems,	
   PVOID MultipleValueInformation,		
   ULONG Length,
   PULONG  ReturnLength
)
{
}

NTSTATUS
STDCALL
NtReplaceKey(
	IN POBJECT_ATTRIBUTES ObjectAttributes, 
	IN HANDLE Key,
	IN POBJECT_ATTRIBUTES ReplacedObjectAttributes 
	)
{
}

NTSTATUS
STDCALL
ZwReplaceKey(
	IN POBJECT_ATTRIBUTES ObjectAttributes, 
	IN HANDLE Key,
	IN POBJECT_ATTRIBUTES ReplacedObjectAttributes 
	)
{
}

NTSTATUS
STDCALL
NtRestoreKey(
	HANDLE KeyHandle,
	HANDLE FileHandle,
	ULONG RestoreFlags
	)
{
}

NTSTATUS
STDCALL
ZwRestoreKey(
	HANDLE KeyHandle,
	HANDLE FileHandle,
	ULONG RestoreFlags
	)
{
}

NTSTATUS
STDCALL
NtSaveKey(
	IN HANDLE KeyHandle,
	IN HANDLE FileHandle
	)
{
}

NTSTATUS
STDCALL
ZwSaveKey(
	IN HANDLE KeyHandle,
	IN HANDLE FileHandle
	)
{
}

NTSTATUS
STDCALL
NtSetInformationKey(
	IN HANDLE KeyHandle,
	IN CINT KeyInformationClass,
	IN PVOID KeyInformation,
	IN ULONG KeyInformationLength
	)
{
}

NTSTATUS
STDCALL
ZwSetInformationKey(
	IN HANDLE KeyHandle,
	IN CINT KeyInformationClass,
	IN PVOID KeyInformation,
	IN ULONG KeyInformationLength
	)
{
}

NTSTATUS 
STDCALL
NtUnloadKey(
	HANDLE KeyHandle
	)
{
}

NTSTATUS 
STDCALL
ZwUnloadKey(
	HANDLE KeyHandle
	)
{
}
