/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             ntoskrnl/cm/rtlfunc.c
 * PURPOSE:          Rtlxxx function for registry access
 * UPDATE HISTORY:
*/

#include <ddk/ntddk.h>
#include <internal/config.h>
#include <internal/ob.h>
#include <limits.h>
#include <string.h>
#include <internal/pool.h>
#include <internal/registry.h>

#define NDEBUG
#include <internal/debug.h>

#include "cm.h"

NTSTATUS STDCALL
RtlCheckRegistryKey(IN ULONG RelativeTo,
		    IN PWSTR Path)
{
   HANDLE KeyHandle;
   NTSTATUS Status;

   Status = RtlpGetRegistryHandle(RelativeTo,
				  Path,
				  FALSE,
				  &KeyHandle);
   if (!NT_SUCCESS(Status))
     return Status;

   NtClose(KeyHandle);

   return STATUS_SUCCESS;
}


NTSTATUS STDCALL
RtlCreateRegistryKey(IN ULONG RelativeTo,
		     IN PWSTR Path)
{
   HANDLE KeyHandle;
   NTSTATUS Status;

   Status = RtlpGetRegistryHandle(RelativeTo,
				  Path,
				  TRUE,
				  &KeyHandle);
   if (!NT_SUCCESS(Status))
     return Status;

   NtClose(KeyHandle);

   return STATUS_SUCCESS;
}


NTSTATUS STDCALL
RtlDeleteRegistryValue(IN ULONG RelativeTo,
		       IN PWSTR Path,
		       IN PWSTR ValueName)
{
   HANDLE KeyHandle;
   NTSTATUS Status;
   UNICODE_STRING Name;

   Status = RtlpGetRegistryHandle(RelativeTo,
				  Path,
				  TRUE,
				  &KeyHandle);
   if (!NT_SUCCESS(Status))
     return Status;

   RtlInitUnicodeString(&Name,
			ValueName);

   NtDeleteValueKey(KeyHandle,
		    &Name);

   NtClose(KeyHandle);

   return STATUS_SUCCESS;
}


NTSTATUS STDCALL
RtlOpenCurrentUser(IN ACCESS_MASK DesiredAccess,
		   OUT PHANDLE KeyHandle)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING KeyPath;
   NTSTATUS Status;

   Status = RtlFormatCurrentUserKeyPath(&KeyPath);
   if (NT_SUCCESS(Status))
     {
	InitializeObjectAttributes(&ObjectAttributes,
				   &KeyPath,
				   OBJ_CASE_INSENSITIVE,
				   NULL,
				   NULL);
	Status = NtOpenKey(KeyHandle,
			   DesiredAccess,
			   &ObjectAttributes);
	RtlFreeUnicodeString(&KeyPath);
	if (NT_SUCCESS(Status))
	  return STATUS_SUCCESS;
     }

   RtlInitUnicodeString(&KeyPath,
			L"\\Registry\\User\\.Default");

   InitializeObjectAttributes(&ObjectAttributes,
			      &KeyPath,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);
   Status = NtOpenKey(KeyHandle,
		      DesiredAccess,
		      &ObjectAttributes);
   return(Status);
}


NTSTATUS STDCALL
RtlQueryRegistryValues(IN ULONG RelativeTo,
		       IN PWSTR Path,
		       IN PRTL_QUERY_REGISTRY_TABLE QueryTable,
		       IN PVOID Context,
		       IN PVOID Environment)
{
	UNIMPLEMENTED;
}


NTSTATUS STDCALL
RtlWriteRegistryValue(IN ULONG RelativeTo,
		      IN PWSTR Path,
		      IN PWSTR ValueName,
		      IN ULONG ValueType,
		      IN PVOID ValueData,
		      IN ULONG ValueLength)
{
   HANDLE KeyHandle;
   NTSTATUS Status;
   UNICODE_STRING Name;

   Status = RtlpGetRegistryHandle(RelativeTo,
				  Path,
				  TRUE,
				  &KeyHandle);
   if (!NT_SUCCESS(Status))
     return Status;

   RtlInitUnicodeString(&Name,
			ValueName);

   NtSetValueKey(KeyHandle,
		 &Name,
		 0,
		 ValueType,
		 ValueData,
		 ValueLength);

   NtClose(KeyHandle);

   return STATUS_SUCCESS;
}

NTSTATUS STDCALL
RtlFormatCurrentUserKeyPath(IN OUT PUNICODE_STRING KeyPath)
{
   /* FIXME: !!! */
   RtlCreateUnicodeString(KeyPath,
			  L"\\Registry\\User\\.Default");

   return STATUS_SUCCESS;
}

/*  ------------------------------------------  Private Implementation  */


NTSTATUS
RtlpGetRegistryHandle(ULONG RelativeTo,
		      PWSTR Path,
		      BOOLEAN Create,
		      PHANDLE KeyHandle)
{
   UNICODE_STRING KeyName;
   WCHAR KeyBuffer[MAX_PATH];
   OBJECT_ATTRIBUTES ObjectAttributes;
   NTSTATUS Status;

   if (RelativeTo & RTL_REGISTRY_HANDLE)
     {
	*KeyHandle = (HANDLE)Path;
	return STATUS_SUCCESS;
     }

   if (RelativeTo & RTL_REGISTRY_OPTIONAL)
     RelativeTo &= ~RTL_REGISTRY_OPTIONAL;

   if (RelativeTo >= RTL_REGISTRY_MAXIMUM)
     return STATUS_INVALID_PARAMETER;

   KeyName.Length = 0;
   KeyName.MaximumLength = MAX_PATH;
   KeyName.Buffer = KeyBuffer;
   KeyBuffer[0] = 0;

   switch (RelativeTo)
     {
	case RTL_REGISTRY_SERVICES:
	  RtlAppendUnicodeToString(&KeyName,
				   L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
	  break;

	case RTL_REGISTRY_CONTROL:
	  RtlAppendUnicodeToString(&KeyName,
				   L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\");
	  break;

	case RTL_REGISTRY_WINDOWS_NT:
	  RtlAppendUnicodeToString(&KeyName,
				   L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\");
	  break;

	case RTL_REGISTRY_DEVICEMAP:
	  RtlAppendUnicodeToString(&KeyName,
				   L"\\Registry\\Machine\\Hardware\\DeviceMap\\");
	  break;

	case RTL_REGISTRY_USER:
	  Status = RtlFormatCurrentUserKeyPath(&KeyName);
	  if (!NT_SUCCESS(Status))
	    return Status;
	  break;
     }

   if (Path[0] != L'\\')
     RtlAppendUnicodeToString(&KeyName,
			      L"\\");

   RtlAppendUnicodeToString(&KeyName,
			    Path);

   InitializeObjectAttributes(&ObjectAttributes,
			      &KeyName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);

   if (Create == TRUE)
     {
	Status = NtCreateKey(KeyHandle,
			     KEY_ALL_ACCESS,
			     &ObjectAttributes,
			     0,
			     NULL,
			     0,
			     NULL);
     }
   else
     {
	Status = NtOpenKey(KeyHandle,
			   KEY_ALL_ACCESS,
			   &ObjectAttributes);
     }

   return Status;
}
