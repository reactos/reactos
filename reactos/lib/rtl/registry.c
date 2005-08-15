/* $Id$
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Rtl registry functions
 * FILE:              lib/rtl/registry.c
 * PROGRAMER:         Eric Kohl
 * REVISION HISTORY:
 *                    2000/08/11: Created
 */

/*
 * TODO:
 *   - finish RtlQueryRegistryValues()
 *	- support RTL_QUERY_REGISTRY_DELETE
 */

/* INCLUDES ****************************************************************/

#define __NTDRIVER__
#include <rtl.h>

#define NDEBUG
#include <debug.h>

#define TAG_RTLREGISTRY TAG('R', 't', 'l', 'R')


/* FUNCTIONS ***************************************************************/

static NTSTATUS
RtlpGetRegistryHandle(ULONG RelativeTo,
		      PWSTR Path,
		      BOOLEAN Create,
		      PHANDLE KeyHandle)
{
  UNICODE_STRING KeyPath;
  UNICODE_STRING KeyName;
  WCHAR KeyBuffer[MAX_PATH];
  OBJECT_ATTRIBUTES ObjectAttributes;
  NTSTATUS Status;

  DPRINT("RtlpGetRegistryHandle()\n");

  if (RelativeTo & RTL_REGISTRY_HANDLE)
    {
      Status = ZwDuplicateObject(NtCurrentProcess(),
				 (HANDLE)Path,
				 NtCurrentProcess(),
				 KeyHandle,
				 0,
				 FALSE,
				 DUPLICATE_SAME_ACCESS);
#ifndef NDEBUG
      if(!NT_SUCCESS(Status))
      {
        DPRINT("ZwDuplicateObject() failed! Status: 0x%x\n", Status);
      }
#endif

      return(Status);
    }

  if (RelativeTo & RTL_REGISTRY_OPTIONAL)
    RelativeTo &= ~RTL_REGISTRY_OPTIONAL;

  if (RelativeTo >= RTL_REGISTRY_MAXIMUM)
  {
    DPRINT("Invalid relative flag, parameter invalid!\n");
    return(STATUS_INVALID_PARAMETER);
  }

  KeyName.Length = 0;
  KeyName.MaximumLength = sizeof(KeyBuffer);
  KeyName.Buffer = KeyBuffer;
  KeyBuffer[0] = 0;

  switch (RelativeTo)
    {
      case RTL_REGISTRY_ABSOLUTE:
        /* nothing to prefix! */
	break;

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
	Status = RtlFormatCurrentUserKeyPath (&KeyPath);
	if (!NT_SUCCESS(Status))
	  return(Status);
	RtlAppendUnicodeStringToString (&KeyName,
					&KeyPath);
	RtlFreeUnicodeString (&KeyPath);
	RtlAppendUnicodeToString (&KeyName,
				  L"\\");
	break;
    }

  if (Path[0] == L'\\' && RelativeTo != RTL_REGISTRY_ABSOLUTE)
    {
      Path++;
    }
  RtlAppendUnicodeToString(&KeyName,
			   Path);

  DPRINT("KeyName %wZ\n", &KeyName);

  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
			     NULL,
			     NULL);

  if (Create)
    {
      Status = ZwCreateKey(KeyHandle,
			   KEY_ALL_ACCESS,
			   &ObjectAttributes,
			   0,
			   NULL,
			   0,
			   NULL);
    }
  else
    {
      Status = ZwOpenKey(KeyHandle,
			 KEY_ALL_ACCESS,
			 &ObjectAttributes);
    }

#ifndef NDEBUG
  if(!NT_SUCCESS(Status))
  {
    DPRINT("%s failed! Status: 0x%x\n", (Create ? "ZwCreateKey" : "ZwOpenKey"), Status);
  }
#endif

  return(Status);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlCheckRegistryKey(IN ULONG RelativeTo,
		    IN PWSTR Path)
{
  HANDLE KeyHandle;
  NTSTATUS Status;

  PAGED_CODE_RTL();

  Status = RtlpGetRegistryHandle(RelativeTo,
				 Path,
				 FALSE,
				 &KeyHandle);
  if (!NT_SUCCESS(Status))
    return(Status);

  ZwClose(KeyHandle);

  return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlCreateRegistryKey(IN ULONG RelativeTo,
		     IN PWSTR Path)
{
  HANDLE KeyHandle;
  NTSTATUS Status;

  PAGED_CODE_RTL();

  Status = RtlpGetRegistryHandle(RelativeTo,
				 Path,
				 TRUE,
				 &KeyHandle);
  if (!NT_SUCCESS(Status))
    return(Status);

  ZwClose(KeyHandle);

  return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlDeleteRegistryValue(IN ULONG RelativeTo,
		       IN PCWSTR Path,
		       IN PCWSTR ValueName)
{
  HANDLE KeyHandle;
  NTSTATUS Status;
  UNICODE_STRING Name;

  PAGED_CODE_RTL();

  Status = RtlpGetRegistryHandle(RelativeTo,
				 (PWSTR)Path,
				 FALSE,
				 &KeyHandle);
  if (!NT_SUCCESS(Status))
    return(Status);

  RtlInitUnicodeString(&Name,
		       ValueName);

  Status = ZwDeleteValueKey(KeyHandle,
			    &Name);

  ZwClose(KeyHandle);

  return(Status);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlFormatCurrentUserKeyPath (OUT PUNICODE_STRING KeyPath)
{
  HANDLE TokenHandle;
  UCHAR Buffer[256];
  PSID_AND_ATTRIBUTES SidBuffer;
  ULONG Length;
  UNICODE_STRING SidString;
  NTSTATUS Status;

  PAGED_CODE_RTL();

  DPRINT ("RtlFormatCurrentUserKeyPath() called\n");

  Status = ZwOpenThreadToken (NtCurrentThread (),
			      TOKEN_QUERY,
			      TRUE,
			      &TokenHandle);
  if (!NT_SUCCESS (Status))
    {
      if (Status != STATUS_NO_TOKEN)
	{
	  DPRINT1 ("ZwOpenThreadToken() failed (Status %lx)\n", Status);
	  return Status;
	}

      Status = ZwOpenProcessToken (NtCurrentProcess (),
				   TOKEN_QUERY,
				   &TokenHandle);
      if (!NT_SUCCESS (Status))
	{
	  DPRINT1 ("ZwOpenProcessToken() failed (Status %lx)\n", Status);
	  return Status;
	}
    }

  SidBuffer = (PSID_AND_ATTRIBUTES)Buffer;
  Status = ZwQueryInformationToken (TokenHandle,
				    TokenUser,
				    (PVOID)SidBuffer,
				    256,
				    &Length);
  ZwClose (TokenHandle);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("ZwQueryInformationToken() failed (Status %lx)\n", Status);
      return Status;
    }

  Status = RtlConvertSidToUnicodeString (&SidString,
					 SidBuffer[0].Sid,
					 TRUE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("RtlConvertSidToUnicodeString() failed (Status %lx)\n", Status);
      return Status;
    }

  DPRINT ("SidString: '%wZ'\n", &SidString);

  Length = SidString.Length + sizeof(L"\\Registry\\User\\");
  DPRINT ("Length: %lu\n", Length);

  KeyPath->Length = 0;
  KeyPath->MaximumLength = Length;
  KeyPath->Buffer = RtlpAllocateStringMemory(KeyPath->MaximumLength, TAG_USTR);
  if (KeyPath->Buffer == NULL)
    {
      DPRINT1 ("RtlpAllocateMemory() failed\n");
      RtlFreeUnicodeString (&SidString);
      return STATUS_NO_TOKEN;
    }

  RtlAppendUnicodeToString (KeyPath,
			    L"\\Registry\\User\\");
  RtlAppendUnicodeStringToString (KeyPath,
				  &SidString);
  RtlFreeUnicodeString (&SidString);

  return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlOpenCurrentUser(IN ACCESS_MASK DesiredAccess,
		   OUT PHANDLE KeyHandle)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyPath;
  NTSTATUS Status;

  PAGED_CODE_RTL();

  Status = RtlFormatCurrentUserKeyPath(&KeyPath);
  if (NT_SUCCESS(Status))
    {
      InitializeObjectAttributes(&ObjectAttributes,
				 &KeyPath,
				 OBJ_CASE_INSENSITIVE,
				 NULL,
				 NULL);
      Status = ZwOpenKey(KeyHandle,
			 DesiredAccess,
			 &ObjectAttributes);
      RtlFreeUnicodeString(&KeyPath);
      if (NT_SUCCESS(Status))
	{
	  return STATUS_SUCCESS;
	}
    }

  RtlInitUnicodeString (&KeyPath,
			L"\\Registry\\User\\.Default");
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyPath,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);
  Status = ZwOpenKey(KeyHandle,
		     DesiredAccess,
		     &ObjectAttributes);

  return Status;
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
RtlQueryRegistryValues(IN ULONG RelativeTo,
		       IN PCWSTR Path,
		       IN PRTL_QUERY_REGISTRY_TABLE QueryTable,
		       IN PVOID Context,
		       IN PVOID Environment OPTIONAL)
{
  NTSTATUS Status;
  HANDLE BaseKeyHandle;
  HANDLE CurrentKeyHandle;
  PRTL_QUERY_REGISTRY_TABLE QueryEntry;
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyName;
  PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
  PKEY_VALUE_FULL_INFORMATION FullValueInfo;
  ULONG BufferSize;
  ULONG ResultSize;
  ULONG Index;
  ULONG StringLen;
  ULONG ValueNameSize;
  PWSTR StringPtr;
  PWSTR ExpandBuffer;
  PWSTR ValueName;
  UNICODE_STRING EnvValue;
  UNICODE_STRING EnvExpandedValue;

  PAGED_CODE_RTL();

  DPRINT("RtlQueryRegistryValues() called\n");

  Status = RtlpGetRegistryHandle(RelativeTo,
				 (PWSTR)Path,
				 FALSE,
				 &BaseKeyHandle);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("RtlpGetRegistryHandle() failed (Status %lx)\n", Status);
      return(Status);
    }

  CurrentKeyHandle = BaseKeyHandle;
  QueryEntry = QueryTable;
  while ((QueryEntry->QueryRoutine != NULL) ||
	 (QueryEntry->Name != NULL))
    {
      if (((QueryEntry->Flags & (RTL_QUERY_REGISTRY_SUBKEY | RTL_QUERY_REGISTRY_TOPKEY)) != 0) &&
	  (BaseKeyHandle != CurrentKeyHandle))
	{
	  ZwClose(CurrentKeyHandle);
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
	  Status = ZwOpenKey(&CurrentKeyHandle,
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
	  ValueInfo = RtlpAllocateMemory(BufferSize, TAG_RTLREGISTRY);
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
	      if (QueryEntry->Flags & RTL_QUERY_REGISTRY_REQUIRED)
		{
		  RtlpFreeMemory(ValueInfo, TAG_RTLREGISTRY);
		  Status = STATUS_OBJECT_NAME_NOT_FOUND;
		  break;
		}

	      if (QueryEntry->DefaultType == REG_SZ)
		{
		  PUNICODE_STRING ValueString;
		  PUNICODE_STRING SourceString;

		  SourceString = (PUNICODE_STRING)QueryEntry->DefaultData;
		  ValueString = (PUNICODE_STRING)QueryEntry->EntryContext;
		  if (ValueString->Buffer == NULL)
		    {
		      ValueString->Length = SourceString->Length;
		      ValueString->MaximumLength = SourceString->MaximumLength;
		      ValueString->Buffer = RtlpAllocateMemory(BufferSize, TAG_RTLREGISTRY);
		      if (!ValueString->Buffer)
			break;
		      ValueString->Buffer[0] = 0;
		      memcpy(ValueString->Buffer,
			     SourceString->Buffer,
			     SourceString->MaximumLength);
		    }
		  else
		    {
		      ValueString->Length = min(SourceString->Length,
						ValueString->MaximumLength - sizeof(WCHAR));
		      memcpy(ValueString->Buffer,
			     SourceString->Buffer,
			     ValueString->Length);
		      ((PWSTR)ValueString->Buffer)[ValueString->Length / sizeof(WCHAR)] = 0;
		    }
		}
	      else
		{
		  memcpy(QueryEntry->EntryContext,
			 QueryEntry->DefaultData,
			 QueryEntry->DefaultLength);
		}
	      Status = STATUS_SUCCESS;
	    }
	  else
	    {
	      if ((ValueInfo->Type == REG_SZ) ||
		  (ValueInfo->Type == REG_MULTI_SZ) ||
		  (ValueInfo->Type == REG_EXPAND_SZ && (QueryEntry->Flags & RTL_QUERY_REGISTRY_NOEXPAND)))
		{
		  PUNICODE_STRING ValueString;

		  ValueString = (PUNICODE_STRING)QueryEntry->EntryContext;
		  if (ValueString->Buffer == NULL)
		    {
		      ValueString->MaximumLength = ValueInfo->DataLength;
		      ValueString->Buffer = RtlpAllocateMemory(ValueString->MaximumLength, TAG_RTLREGISTRY);
		      if (ValueString->Buffer == NULL)
			{
			  Status = STATUS_INSUFFICIENT_RESOURCES;
			  break;
			}
		      ValueString->Buffer[0] = 0;
		     }
		  ValueString->Length = min(ValueInfo->DataLength,
					    ValueString->MaximumLength) - sizeof(WCHAR);
		  memcpy(ValueString->Buffer,
			 ValueInfo->Data,
			 ValueString->Length);
		  ((PWSTR)ValueString->Buffer)[ValueString->Length / sizeof(WCHAR)] = 0;
		}
	      else if (ValueInfo->Type == REG_EXPAND_SZ)
		{
		  PUNICODE_STRING ValueString;

		  DPRINT("Expand REG_EXPAND_SZ type\n");

		  ValueString = (PUNICODE_STRING)QueryEntry->EntryContext;

		  ExpandBuffer = RtlpAllocateMemory(ValueInfo->DataLength * 2, TAG_RTLREGISTRY);
		  if (ExpandBuffer == NULL)
		    {
		      Status = STATUS_NO_MEMORY;
		      break;
		    }

		  RtlInitUnicodeString(&EnvValue,
				       (PWSTR)ValueInfo->Data);
		  EnvExpandedValue.Length = 0;
		  EnvExpandedValue.MaximumLength = ValueInfo->DataLength * 2;
		  EnvExpandedValue.Buffer = ExpandBuffer;
		  *ExpandBuffer = 0;

		  RtlExpandEnvironmentStrings_U(Environment,
						&EnvValue,
						&EnvExpandedValue,
						&StringLen);

		  if (ValueString->Buffer == NULL)
		    {
		      ValueString->MaximumLength = EnvExpandedValue.Length + sizeof(WCHAR);
		      ValueString->Length = EnvExpandedValue.Length;
		      ValueString->Buffer = RtlpAllocateMemory(ValueString->MaximumLength, TAG_RTLREGISTRY);
		      if (ValueString->Buffer == NULL)
			{
			  Status = STATUS_INSUFFICIENT_RESOURCES;
			  break;
			}
		    }
		  else
		    {
		      ValueString->Length = min(EnvExpandedValue.Length,
						ValueString->MaximumLength - sizeof(WCHAR));
		    }

		  memcpy(ValueString->Buffer,
			 EnvExpandedValue.Buffer,
			 ValueString->Length);
		  ((PWSTR)ValueString->Buffer)[ValueString->Length / sizeof(WCHAR)] = 0;

		  RtlpFreeMemory(ExpandBuffer, TAG_RTLREGISTRY);
		}
	      else
		{
		  memcpy(QueryEntry->EntryContext,
			 ValueInfo->Data,
			 ValueInfo->DataLength);
		}
	    }

	  if (QueryEntry->Flags & RTL_QUERY_REGISTRY_DELETE)
	    {
	      DPRINT1("FIXME: Delete value: %S\n", QueryEntry->Name);

	    }

	  RtlpFreeMemory(ValueInfo, TAG_RTLREGISTRY);
	}
      else
	{
	  DPRINT("Query value via query routine: %S\n", QueryEntry->Name);
	  if (QueryEntry->Name != NULL)
	    {
	      RtlInitUnicodeString(&KeyName,
				   QueryEntry->Name);

	      BufferSize = sizeof (KEY_VALUE_PARTIAL_INFORMATION) + 4096;
	      ValueInfo = RtlpAllocateMemory(BufferSize, TAG_RTLREGISTRY);
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
		  if (!(QueryEntry->Flags & RTL_QUERY_REGISTRY_REQUIRED))
		    {
		      Status = QueryEntry->QueryRoutine(QueryEntry->Name,
							QueryEntry->DefaultType,
							QueryEntry->DefaultData,
							QueryEntry->DefaultLength,
							Context,
							QueryEntry->EntryContext);
		    }
		}
	      else if ((ValueInfo->Type == REG_MULTI_SZ) &&
		       !(QueryEntry->Flags & RTL_QUERY_REGISTRY_NOEXPAND))
		{
		  DPRINT("Expand REG_MULTI_SZ type\n");
		  StringPtr = (PWSTR)ValueInfo->Data;
		  while (*StringPtr != 0)
		    {
		      StringLen = (wcslen(StringPtr) + 1) * sizeof(WCHAR);
		      Status = QueryEntry->QueryRoutine(QueryEntry->Name,
							REG_SZ,
							(PVOID)StringPtr,
							StringLen,
							Context,
							QueryEntry->EntryContext);
		      if(!NT_SUCCESS(Status))
			break;
		      StringPtr = (PWSTR)((PUCHAR)StringPtr + StringLen);
		    }
		}
	      else if ((ValueInfo->Type == REG_EXPAND_SZ) &&
		       !(QueryEntry->Flags & RTL_QUERY_REGISTRY_NOEXPAND))
		{
		  DPRINT("Expand REG_EXPAND_SZ type\n");

		  ExpandBuffer = RtlpAllocateMemory(ValueInfo->DataLength * 2, TAG_RTLREGISTRY);
		  if (ExpandBuffer == NULL)
		    {
		      Status = STATUS_NO_MEMORY;
		      break;
		    }

		  RtlInitUnicodeString(&EnvValue,
				       (PWSTR)ValueInfo->Data);
		  EnvExpandedValue.Length = 0;
		  EnvExpandedValue.MaximumLength = ValueInfo->DataLength * 2 * sizeof(WCHAR);
		  EnvExpandedValue.Buffer = ExpandBuffer;
		  *ExpandBuffer = 0;

		  RtlExpandEnvironmentStrings_U(Environment,
						&EnvValue,
						&EnvExpandedValue,
						&StringLen);

		  StringLen = (wcslen(ExpandBuffer) + 1) * sizeof(WCHAR);
		  Status = QueryEntry->QueryRoutine(QueryEntry->Name,
						    REG_SZ,
						    (PVOID)ExpandBuffer,
						    StringLen,
						    Context,
						    QueryEntry->EntryContext);

		  RtlpFreeMemory(ExpandBuffer, TAG_RTLREGISTRY);
		}
	      else
		{
		  Status = QueryEntry->QueryRoutine(QueryEntry->Name,
						    ValueInfo->Type,
						    ValueInfo->Data,
						    ValueInfo->DataLength,
						    Context,
						    QueryEntry->EntryContext);
		}

	      if (QueryEntry->Flags & RTL_QUERY_REGISTRY_DELETE)
		{
		  DPRINT1("FIXME: Delete value: %S\n", QueryEntry->Name);

		}

	      RtlpFreeMemory(ValueInfo, TAG_RTLREGISTRY);
	      if (!NT_SUCCESS(Status))
		break;
	    }
	  else if (QueryEntry->Flags & RTL_QUERY_REGISTRY_NOVALUE)
	    {
	      DPRINT("Simple callback\n");
	      Status = QueryEntry->QueryRoutine(NULL,
						REG_NONE,
						NULL,
						0,
						Context,
						QueryEntry->EntryContext);
	      if (!NT_SUCCESS(Status))
		break;
	    }
	  else
	    {
	      DPRINT("Enumerate values\n");

	      BufferSize = sizeof(KEY_VALUE_FULL_INFORMATION) + 4096;
	      FullValueInfo = RtlpAllocateMemory(BufferSize, TAG_RTLREGISTRY);
	      if (FullValueInfo == NULL)
		{
		  Status = STATUS_NO_MEMORY;
		  break;
		}
	      ValueNameSize = 256 * sizeof(WCHAR);
	      ValueName = RtlpAllocateMemory(ValueNameSize, TAG_RTLREGISTRY);
	      if (ValueName == NULL)
	        {
		  Status = STATUS_NO_MEMORY;
		  break;
		}
	      Index = 0;
	      while (TRUE)
		{
		  Status = ZwEnumerateValueKey(CurrentKeyHandle,
					       Index,
					       KeyValueFullInformation,
					       FullValueInfo,
					       BufferSize,
					       &ResultSize);
		  if (!NT_SUCCESS(Status))
		    {
		      if ((Status == STATUS_NO_MORE_ENTRIES) &&
			  (Index == 0) &&
			  (QueryEntry->Flags & RTL_QUERY_REGISTRY_REQUIRED))
			{
			  Status = STATUS_OBJECT_NAME_NOT_FOUND;
			}
		      else if (Status == STATUS_NO_MORE_ENTRIES)
			{
			  Status = STATUS_SUCCESS;
			}
		      break;
		    }

		  if (FullValueInfo->NameLength > ValueNameSize - sizeof(WCHAR))
		    {
		      /* Should not happen, because the name length is limited to 255 characters */
		      RtlpFreeMemory(ValueName, TAG_RTLREGISTRY);
		      ValueNameSize = FullValueInfo->NameLength + sizeof(WCHAR);
		      ValueName = RtlpAllocateMemory(ValueNameSize, TAG_RTLREGISTRY);
		      if (ValueName == NULL)
		        {
		          Status = STATUS_NO_MEMORY;
		          break;
		        }
		    }

		  memcpy(ValueName,
			 FullValueInfo->Name,
			 FullValueInfo->NameLength);
		  ValueName[FullValueInfo->NameLength / sizeof(WCHAR)] = 0;

		  DPRINT("FullValueInfo->Type: %lu\n", FullValueInfo->Type);
		  if ((FullValueInfo->Type == REG_MULTI_SZ) &&
		      !(QueryEntry->Flags & RTL_QUERY_REGISTRY_NOEXPAND))
		    {
		      DPRINT("Expand REG_MULTI_SZ type\n");
		      StringPtr = (PWSTR)((ULONG_PTR)FullValueInfo + FullValueInfo->DataOffset);
		      while (*StringPtr != 0)
			{
			  StringLen = (wcslen(StringPtr) + 1) * sizeof(WCHAR);
			  Status = QueryEntry->QueryRoutine(ValueName,
							    REG_SZ,
							    (PVOID)StringPtr,
							    StringLen,
							    Context,
							    QueryEntry->EntryContext);
			  if(!NT_SUCCESS(Status))
			    break;
			  StringPtr = (PWSTR)((PUCHAR)StringPtr + StringLen);
			}
		    }
		  else if ((FullValueInfo->Type == REG_EXPAND_SZ) &&
			   !(QueryEntry->Flags & RTL_QUERY_REGISTRY_NOEXPAND))
		    {
		      DPRINT("Expand REG_EXPAND_SZ type\n");

		      StringPtr = (PWSTR)((ULONG_PTR)FullValueInfo + FullValueInfo->DataOffset);
		      ExpandBuffer = RtlpAllocateMemory(FullValueInfo->DataLength * 2, TAG_RTLREGISTRY);
		      if (ExpandBuffer == NULL)
			{
			  Status = STATUS_NO_MEMORY;
			  break;
			}

		      RtlInitUnicodeString(&EnvValue,
					   StringPtr);
		      EnvExpandedValue.Length = 0;
		      EnvExpandedValue.MaximumLength = FullValueInfo->DataLength * 2;
		      EnvExpandedValue.Buffer = ExpandBuffer;
		      *ExpandBuffer = 0;

		      RtlExpandEnvironmentStrings_U(Environment,
						    &EnvValue,
						    &EnvExpandedValue,
						    &StringLen);

		      StringLen = (wcslen(ExpandBuffer) + 1) * sizeof(WCHAR);
		      Status = QueryEntry->QueryRoutine(ValueName,
							REG_SZ,
							(PVOID)ExpandBuffer,
							StringLen,
							Context,
							QueryEntry->EntryContext);

		       RtlpFreeMemory(ExpandBuffer, TAG_RTLREGISTRY);
		    }
		  else
		    {
		      Status = QueryEntry->QueryRoutine(ValueName,
							FullValueInfo->Type,
							(PVOID)((ULONG_PTR)FullValueInfo + FullValueInfo->DataOffset),
							FullValueInfo->DataLength,
							Context,
							QueryEntry->EntryContext);
		    }

		  if (!NT_SUCCESS(Status))
		    break;

		  /* FIXME: How will these be deleted? */

		  Index++;
		}

	      RtlpFreeMemory(FullValueInfo, TAG_RTLREGISTRY);
	      RtlpFreeMemory(ValueName, TAG_RTLREGISTRY);
	      if (!NT_SUCCESS(Status))
		break;
	    }
	}

      QueryEntry++;
    }

  if (CurrentKeyHandle != BaseKeyHandle)
    ZwClose(CurrentKeyHandle);

  ZwClose(BaseKeyHandle);

  return(Status);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlWriteRegistryValue(IN ULONG RelativeTo,
		      IN PCWSTR Path,
		      IN PCWSTR ValueName,
		      IN ULONG ValueType,
		      IN PVOID ValueData,
		      IN ULONG ValueLength)
{
  HANDLE KeyHandle;
  NTSTATUS Status;
  UNICODE_STRING Name;

  PAGED_CODE_RTL();

  Status = RtlpGetRegistryHandle(RelativeTo,
				 (PWSTR)Path,
				 TRUE,
				 &KeyHandle);
  if (!NT_SUCCESS(Status))
  {
    DPRINT("RtlpGetRegistryHandle() failed! Status: 0x%x\n", Status);
    return(Status);
  }

  RtlInitUnicodeString(&Name,
		       ValueName);

  Status = ZwSetValueKey(KeyHandle,
			 &Name,
			 0,
			 ValueType,
			 ValueData,
			 ValueLength);
  if (!NT_SUCCESS(Status))
  {
    DPRINT1("ZwSetValueKey() failed! Status: 0x%x\n", Status);
  }

  ZwClose(KeyHandle);

  return(Status);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlpNtCreateKey(OUT HANDLE KeyHandle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes,
		IN ULONG Unused1,
		OUT PULONG Disposition,
		IN ULONG Unused2)
{
  if (ObjectAttributes != NULL)
    ObjectAttributes->Attributes &= ~(OBJ_PERMANENT | OBJ_EXCLUSIVE);

  return(ZwCreateKey(KeyHandle,
		     DesiredAccess,
		     ObjectAttributes,
		     0,
		     NULL,
		     0,
		     Disposition));
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlpNtEnumerateSubKey(IN HANDLE KeyHandle,
		      OUT PUNICODE_STRING SubKeyName,
		      IN ULONG Index,
		      IN ULONG Unused)
{
  PKEY_BASIC_INFORMATION KeyInfo = NULL;
  ULONG BufferLength = 0;
  ULONG ReturnedLength;
  NTSTATUS Status;

  if (SubKeyName->MaximumLength != 0)
    {
      BufferLength = SubKeyName->MaximumLength +
		     sizeof(KEY_BASIC_INFORMATION);
      KeyInfo = RtlpAllocateMemory(BufferLength, TAG_RTLREGISTRY);
      if (KeyInfo == NULL)
	return(STATUS_NO_MEMORY);
    }

  Status = ZwEnumerateKey(KeyHandle,
			  Index,
			  KeyBasicInformation,
			  KeyInfo,
			  BufferLength,
			  &ReturnedLength);
  if (NT_SUCCESS(Status))
    {
      if (KeyInfo->NameLength + sizeof(WCHAR) <= SubKeyName->MaximumLength)
	{
	  memmove(SubKeyName->Buffer,
		  KeyInfo->Name,
		  KeyInfo->NameLength);
	  SubKeyName->Buffer[KeyInfo->NameLength / sizeof(WCHAR)] = 0;
	  SubKeyName->Length = KeyInfo->NameLength;
	}
      else
	{
	  Status = STATUS_BUFFER_OVERFLOW;
	}
    }

  if (KeyInfo != NULL)
    {
      RtlpFreeMemory(KeyInfo, TAG_RTLREGISTRY);
    }

  return(Status);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlpNtMakeTemporaryKey(IN HANDLE KeyHandle)
{
  return(ZwDeleteKey(KeyHandle));
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlpNtOpenKey(OUT HANDLE KeyHandle,
	      IN ACCESS_MASK DesiredAccess,
	      IN POBJECT_ATTRIBUTES ObjectAttributes,
	      IN ULONG Unused)
{
  if (ObjectAttributes != NULL)
    ObjectAttributes->Attributes &= ~(OBJ_PERMANENT | OBJ_EXCLUSIVE);

  return(ZwOpenKey(KeyHandle,
		   DesiredAccess,
		   ObjectAttributes));
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlpNtQueryValueKey(IN HANDLE KeyHandle,
		    OUT PULONG Type OPTIONAL,
		    OUT PVOID Data OPTIONAL,
		    IN OUT PULONG DataLength OPTIONAL,
		    IN ULONG Unused)
{
  PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
  UNICODE_STRING ValueName;
  ULONG BufferLength;
  ULONG ReturnedLength;
  NTSTATUS Status;

  RtlInitUnicodeString(&ValueName,
		       NULL);

  BufferLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION);
  if (DataLength != NULL)
    BufferLength = *DataLength;

  ValueInfo = RtlpAllocateMemory(BufferLength, TAG_RTLREGISTRY);
  if (ValueInfo == NULL)
    return(STATUS_NO_MEMORY);

  Status = ZwQueryValueKey(KeyHandle,
			   &ValueName,
			   KeyValuePartialInformation,
			   ValueInfo,
			   BufferLength,
			   &ReturnedLength);
  if (NT_SUCCESS(Status))
    {
      if (DataLength != NULL)
	*DataLength = ValueInfo->DataLength;

      if (Type != NULL)
	*Type = ValueInfo->Type;

      if (Data != NULL)
	{
	  memmove(Data,
		  ValueInfo->Data,
		  ValueInfo->DataLength);
	}
    }

  RtlpFreeMemory(ValueInfo, TAG_RTLREGISTRY);

  return(Status);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlpNtSetValueKey(IN HANDLE KeyHandle,
		  IN ULONG Type,
		  IN PVOID Data,
		  IN ULONG DataLength)
{
  UNICODE_STRING ValueName;

  RtlInitUnicodeString(&ValueName,
		       NULL);
  return(ZwSetValueKey(KeyHandle,
		       &ValueName,
		       0,
		       Type,
		       Data,
		       DataLength));
}

/* EOF */
