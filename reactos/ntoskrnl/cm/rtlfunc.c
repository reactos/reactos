/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             ntoskrnl/cm/rtlfunc.c
 * PURPOSE:          Rtlxxx function for registry access
 * UPDATE HISTORY:
*/

#include <ddk/ntddk.h>
#include <roscfg.h>
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
   NTSTATUS Status;
   HANDLE BaseKeyHandle;
   HANDLE CurrentKeyHandle;
   PRTL_QUERY_REGISTRY_TABLE QueryEntry;
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING KeyName;
   PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
   ULONG BufferSize;
   ULONG ResultSize;
   
   DPRINT("RtlQueryRegistryValues() called\n");
   
   Status = RtlpGetRegistryHandle(RelativeTo,
				  Path,
				  FALSE,
				  &BaseKeyHandle);
   if (!NT_SUCCESS(Status))
    {
      DPRINT("RtlpGetRegistryHandle() failed with status %x\n", Status);
      return Status;
    }

   CurrentKeyHandle = BaseKeyHandle;
   QueryEntry = QueryTable;
   while ((QueryEntry->QueryRoutine != NULL) ||
	  (QueryEntry->Name != NULL))
     {
    //CSH: Was:
    //if ((QueryEntry->QueryRoutine == NULL) &&
	  //  ((QueryEntry->Flags & (RTL_QUERY_REGISTRY_SUBKEY | RTL_QUERY_REGISTRY_DIRECT)) != 0))
    // Which is more correct?
	if ((QueryEntry->QueryRoutine == NULL) &&
	    ((QueryEntry->Flags & RTL_QUERY_REGISTRY_SUBKEY) != 0))
	  {
       DPRINT("Bad parameters\n");
	     Status = STATUS_INVALID_PARAMETER;
	     break;
	  }

	DPRINT("Name: %S\n", QueryEntry->Name);

	if (((QueryEntry->Flags & (RTL_QUERY_REGISTRY_SUBKEY | RTL_QUERY_REGISTRY_TOPKEY)) != 0) &&
	    (BaseKeyHandle != CurrentKeyHandle))
	  {
	     NtClose(CurrentKeyHandle);
	     CurrentKeyHandle = BaseKeyHandle;
	  }

	if (QueryEntry->Flags & RTL_QUERY_REGISTRY_SUBKEY)
	  {
	     DPRINT("Open new subkey: %S\n", QueryEntry->Name);
	  
	     RtlInitUnicodeString(&KeyName,
				  QueryEntry->Name);
	     InitializeObjectAttributes(&ObjectAttributes,
					&KeyName,
					OBJ_CASE_INSENSITIVE,
					BaseKeyHandle,
					NULL);
	     Status = NtOpenKey(&CurrentKeyHandle,
				KEY_ALL_ACCESS,
				&ObjectAttributes);
	     if (!NT_SUCCESS(Status))
	       break;
	  }
	else if (QueryEntry->Flags & RTL_QUERY_REGISTRY_DIRECT)
	  {
	     DPRINT("Query value directly: %S\n", QueryEntry->Name);
	  
	     RtlInitUnicodeString(&KeyName,
				  QueryEntry->Name);
	  
	     BufferSize = sizeof (KEY_VALUE_PARTIAL_INFORMATION) + 4096;
	     ValueInfo = ExAllocatePool(PagedPool, BufferSize);
	     if (ValueInfo == NULL)
	       {
		  Status = STATUS_NO_MEMORY;
		  break;
	       }

	     Status = ZwQueryValueKey(CurrentKeyHandle,
				      &KeyName,
				      KeyValuePartialInformation,
				      ValueInfo,
				      BufferSize,
				      &ResultSize);
	     if (!NT_SUCCESS(Status))
	       {
      DPRINT("ZwQueryValueKey() failed with status %x\n", Status);
		  ExFreePool(ValueInfo);
		  break;
	       }
	     else
	       {
		  if (ValueInfo->Type == REG_SZ)
		    {
		       PUNICODE_STRING ValueString;
		       ValueString = (PUNICODE_STRING)QueryEntry->EntryContext;
		       if (ValueString->Buffer == 0)
			 {
          RtlInitUnicodeString(ValueString, NULL);
			    ValueString->MaximumLength = 256 * sizeof(WCHAR);
			    ValueString->Buffer = ExAllocatePool(PagedPool, ValueString->MaximumLength);
          if (!ValueString->Buffer)
            break;
          ValueString->Buffer[0] = 0;
			 }
		       ValueString->Length = RtlMin(ValueInfo->DataLength,
						 ValueString->MaximumLength - sizeof(WCHAR));
		       memcpy(ValueString->Buffer,
			      ValueInfo->Data,
			      ValueInfo->DataLength);
			((PWSTR)ValueString->Buffer)[ValueString->Length / sizeof(WCHAR)] = 0;
		    }
		  else
		    {
		       memcpy(QueryEntry->EntryContext,
			      ValueInfo->Data,
			      ValueInfo->DataLength);
		    }
	       }

	     ExFreePool (ValueInfo);
	  }
	else
	  {
	     DPRINT("Query value via query routine: %S\n", QueryEntry->Name);
	     
	  }

	if (QueryEntry->Flags & RTL_QUERY_REGISTRY_DELETE)
	  {
	     DPRINT("Delete value: %S\n", QueryEntry->Name);
	     
	  }

	QueryEntry++;
     }

   if (CurrentKeyHandle != BaseKeyHandle)
     NtClose(CurrentKeyHandle);

   NtClose(BaseKeyHandle);

   return Status;
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
   Status = NtDuplicateObject(
      PsGetCurrentProcessId(),
      (PHANDLE)&Path,
      PsGetCurrentProcessId(),
      KeyHandle,
      0,
      FALSE,
      DUPLICATE_SAME_ACCESS);
   return Status;
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

  /* ReactOS specific */
  case RTL_REGISTRY_ENUM:
	  RtlAppendUnicodeToString(&KeyName,
		       L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum\\");
    break;
     }

   if (Path[0] != L'\\')
     {
	RtlAppendUnicodeToString(&KeyName,
				 Path);
     }
   else
     {
	Path++;
	RtlAppendUnicodeToString(&KeyName,
				 Path);
     }

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


NTSTATUS
RtlpCreateRegistryKeyPath(PWSTR Path)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  WCHAR KeyBuffer[MAX_PATH];
  UNICODE_STRING KeyName;
  HANDLE KeyHandle;
  NTSTATUS Status;
  PWCHAR Current;
  PWCHAR Next;

  if (_wcsnicmp(Path, L"\\Registry\\", 10) != 0)
    {
   return STATUS_INVALID_PARAMETER;
    }

  wcsncpy(KeyBuffer, Path, MAX_PATH-1);
  RtlInitUnicodeString(&KeyName, KeyBuffer);

  /* Skip \\Registry\\ */
  Current = KeyName.Buffer;
  Current = wcschr(Current, '\\') + 1;
  Current = wcschr(Current, '\\') + 1;

  do {
    Next = wcschr(Current, '\\');
    if (Next == NULL)
      {
    /* The end */
      }
    else
      {
    *Next = 0;
      }

    InitializeObjectAttributes(
      &ObjectAttributes,
	    &KeyName,
	    OBJ_CASE_INSENSITIVE,
	    NULL,
	    NULL);

    DPRINT("Create '%S'\n", KeyName.Buffer);

    Status = NtCreateKey(
      &KeyHandle,
      KEY_ALL_ACCESS,
	    &ObjectAttributes,
	    0,
	    NULL,
	    0,
	    NULL);
    if (!NT_SUCCESS(Status))
      {
        DPRINT("NtCreateKey() failed with status %x\n", Status);
        return Status;
      }

    NtClose(KeyHandle);

    if (Next != NULL)
      {
    *Next = L'\\';
      }

    Current = Next + 1;
  } while (Next != NULL);

  return STATUS_SUCCESS;
}

/* EOF */
