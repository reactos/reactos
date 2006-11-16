/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Rtl registry functions
 * FILE:              lib/rtl/registry.c
 * PROGRAMER:         Eric Kohl
 */

/*
 * TODO:
 *   - finish RtlQueryRegistryValues()
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

#define TAG_RTLREGISTRY TAG('R', 't', 'l', 'R')

/* DATA **********************************************************************/

PCWSTR RtlpRegPaths[RTL_REGISTRY_MAXIMUM] =
{
    NULL,
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services",
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control",
    L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion",
    L"\\Registry\\Machine\\Hardware\\DeviceMap",
    L"\\Registry\\User\\.Default",
};

/* PRIVATE FUNCTIONS *********************************************************/

NTSTATUS
NTAPI
RtlpGetRegistryHandle(IN ULONG RelativeTo,
                      IN PCWSTR Path,
                      IN BOOLEAN Create,
                      IN PHANDLE KeyHandle)
{
    UNICODE_STRING KeyPath, KeyName;
    WCHAR KeyBuffer[MAX_PATH];
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;

    /* Check if we just want the handle */
    if (RelativeTo & RTL_REGISTRY_HANDLE)
    {
        *KeyHandle = (HANDLE)Path;
        return STATUS_SUCCESS;
    }

    /* Check for optional flag */
    if (RelativeTo & RTL_REGISTRY_OPTIONAL)
    {
        /* Mask it out */
        RelativeTo &= ~RTL_REGISTRY_OPTIONAL;
    }

    /* Fail on invalid parameter */
    if (RelativeTo >= RTL_REGISTRY_MAXIMUM) return STATUS_INVALID_PARAMETER;

    /* Initialize the key name */
    RtlInitEmptyUnicodeString(&KeyName, KeyBuffer, sizeof(KeyBuffer));

    /* Check if we have to lookup a path to prefix */
    if (RelativeTo != RTL_REGISTRY_ABSOLUTE)
    {
        /* Check if we need the current user key */
        if (RelativeTo == RTL_REGISTRY_USER)
        {
            /* Get the path */
            Status = RtlFormatCurrentUserKeyPath(&KeyPath);
            if (!NT_SUCCESS(Status)) return(Status);

            /* Append it */
            Status = RtlAppendUnicodeStringToString(&KeyName, &KeyPath);
            RtlFreeUnicodeString (&KeyPath);
        }
        else
        {
            /* Get one of the prefixes */
            Status = RtlAppendUnicodeToString(&KeyName,
                                              RtlpRegPaths[RelativeTo]);
        }

        /* Check for failure, otherwise, append the path separator */
        if (!NT_SUCCESS(Status)) return Status;
        Status = RtlAppendUnicodeToString(&KeyName, L"\\");
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* And now append the path */
    DPRINT1("I'm about to crash due to an overwrite problem, Alex thinks\n");
    DPRINT1("I'm about to crash due to a overwrite problem, Alex thinks\n");
    DPRINT1("I'm about to crash due to a overwrite problem, Alex thinks\n");
    DPRINT1("I'm about to crash due to a overwrite problem, Alex thinks\n");
    DPRINT1("I'm about to crash due to a overwrite problem, Alex thinks\n");
    if (Path[0] == L'\\' && RelativeTo != RTL_REGISTRY_ABSOLUTE) Path++; // HACK!
    Status = RtlAppendUnicodeToString(&KeyName, Path);
    if (!NT_SUCCESS(Status)) return Status;

    /* Initialize the object attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Check if we want to create it */
    if (Create)
    {
        /* Create the key with write privileges */
        Status = ZwCreateKey(KeyHandle,
                             GENERIC_WRITE,
                             &ObjectAttributes,
                             0,
                             NULL,
                             0,
                             NULL);
    }
    else
    {
        /* Otherwise, just open it with read access */
        Status = ZwOpenKey(KeyHandle,
                           MAXIMUM_ALLOWED | GENERIC_READ,
                           &ObjectAttributes);
    }

    /* Return status */
    return Status;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlCheckRegistryKey(IN ULONG RelativeTo,
                    IN PWSTR Path)
{
    HANDLE KeyHandle;
    NTSTATUS Status;
    PAGED_CODE_RTL();

    /* Call the helper */
    Status = RtlpGetRegistryHandle(RelativeTo,
                                   Path,
                                   FALSE,
                                   &KeyHandle);
    if (!NT_SUCCESS(Status)) return Status;

    /* All went well, close the handle and return success */
    ZwClose(KeyHandle);
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlCreateRegistryKey(IN ULONG RelativeTo,
                     IN PWSTR Path)
{
    HANDLE KeyHandle;
    NTSTATUS Status;
    PAGED_CODE_RTL();

    /* Call the helper */
    Status = RtlpGetRegistryHandle(RelativeTo,
                                   Path,
                                   TRUE,
                                   &KeyHandle);
    if (!NT_SUCCESS(Status)) return Status;

    /* All went well, close the handle and return success */
    ZwClose(KeyHandle);
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlDeleteRegistryValue(IN ULONG RelativeTo,
                       IN PCWSTR Path,
                       IN PCWSTR ValueName)
{
    HANDLE KeyHandle;
    NTSTATUS Status;
    UNICODE_STRING Name;
    PAGED_CODE_RTL();

    /* Call the helper */
    Status = RtlpGetRegistryHandle(RelativeTo,
                                   Path,
                                   TRUE,
                                   &KeyHandle);
    if (!NT_SUCCESS(Status)) return Status;

    /* Initialize the key name and delete it */
    RtlInitUnicodeString(&Name, ValueName);
    Status = ZwDeleteValueKey(KeyHandle, &Name);

    /* All went well, close the handle and return status */
    ZwClose(KeyHandle);
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
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

    /* Call the helper */
    Status = RtlpGetRegistryHandle(RelativeTo,
                                   Path,
                                   TRUE,
                                   &KeyHandle);
    if (!NT_SUCCESS(Status)) return Status;

    /* Initialize the key name and set it */
    RtlInitUnicodeString(&Name, ValueName);
    Status = ZwSetValueKey(KeyHandle,
                           &Name,
                           0,
                           ValueType,
                           ValueData,
                           ValueLength);

    /* All went well, close the handle and return status */
    ZwClose(KeyHandle);
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlOpenCurrentUser(IN ACCESS_MASK DesiredAccess,
                   OUT PHANDLE KeyHandle)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyPath;
    NTSTATUS Status;
    PAGED_CODE_RTL();

    /* Get the user key */
    Status = RtlFormatCurrentUserKeyPath(&KeyPath);
    if (NT_SUCCESS(Status))
    {
        /* Initialize the attributes and open it */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyPath,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);
        Status = ZwOpenKey(KeyHandle, DesiredAccess, &ObjectAttributes);

        /* Free the path and return success if it worked */
        RtlFreeUnicodeString(&KeyPath);
        if (NT_SUCCESS(Status)) return STATUS_SUCCESS;
    }

    /* It didn't work, so use the default key */
    RtlInitUnicodeString(&KeyPath, RtlpRegPaths[RTL_REGISTRY_USER]);
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = ZwOpenKey(KeyHandle, DesiredAccess, &ObjectAttributes);

    /* Return status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlFormatCurrentUserKeyPath(OUT PUNICODE_STRING KeyPath)
{
    HANDLE TokenHandle;
    UCHAR Buffer[256];
    PSID_AND_ATTRIBUTES SidBuffer;
    ULONG Length;
    UNICODE_STRING SidString;
    NTSTATUS Status;
    PAGED_CODE_RTL();

    /* Open the thread token */
    Status = ZwOpenThreadToken(NtCurrentThread(),
                               TOKEN_QUERY,
                               TRUE,
                               &TokenHandle);
    if (!NT_SUCCESS(Status))
    {
        /* We failed, is it because we don't have a thread token? */
        if (Status != STATUS_NO_TOKEN) return Status;

        /* It is, so use the process token */
        Status = ZwOpenProcessToken(NtCurrentProcess(),
                                    TOKEN_QUERY,
                                    &TokenHandle);
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Now query the token information */
    SidBuffer = (PSID_AND_ATTRIBUTES)Buffer;
    Status = ZwQueryInformationToken(TokenHandle,
                                     TokenUser,
                                     (PVOID)SidBuffer,
                                     sizeof(Buffer),
                                     &Length);

    /* Close the handle and handle failure */
    ZwClose(TokenHandle);
    if (!NT_SUCCESS(Status)) return Status;

    /* Convert the SID */
    Status = RtlConvertSidToUnicodeString(&SidString, SidBuffer[0].Sid, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    /* Add the length of the prefix */
    Length = SidString.Length + sizeof(L"\\REGISTRY\\USER\\");

    /* Initialize a string */
    RtlInitEmptyUnicodeString(KeyPath,
                              RtlpAllocateStringMemory(Length, TAG_USTR),
                              Length);
    if (!KeyPath->Buffer)
    {
        /* Free the string and fail */
        RtlFreeUnicodeString(&SidString);
        return STATUS_NO_MEMORY;
    }

    /* Append the prefix and SID */
    RtlAppendUnicodeToString(KeyPath, L"\\REGISTRY\\USER\\");
    RtlAppendUnicodeStringToString(KeyPath, &SidString);

    /* Free the temporary string and return success */
    RtlFreeUnicodeString(&SidString);
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlpNtCreateKey(OUT HANDLE KeyHandle,
                IN ACCESS_MASK DesiredAccess,
                IN POBJECT_ATTRIBUTES ObjectAttributes,
                IN ULONG TitleIndex,
                IN PUNICODE_STRING Class,
                OUT PULONG Disposition)
{
    /* Check if we have object attributes */
    if (ObjectAttributes)
    {
        /* Mask out the unsupported flags */
        ObjectAttributes->Attributes &= ~(OBJ_PERMANENT | OBJ_EXCLUSIVE);
    }

    /* Create the key */
    return ZwCreateKey(KeyHandle,
                       DesiredAccess,
                       ObjectAttributes,
                       0,
                       NULL,
                       0,
                       Disposition);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlpNtEnumerateSubKey(IN HANDLE KeyHandle,
                      OUT PUNICODE_STRING SubKeyName,
                      IN ULONG Index,
                      IN ULONG Unused)
{
    PKEY_BASIC_INFORMATION KeyInfo = NULL;
    ULONG BufferLength = 0;
    ULONG ReturnedLength;
    NTSTATUS Status;

    /* Check if we have a name */
    if (SubKeyName->MaximumLength)
    {
        /* Allocate a buffer for it */
        BufferLength = SubKeyName->MaximumLength +
                       sizeof(KEY_BASIC_INFORMATION);
        KeyInfo = RtlAllocateHeap(RtlGetProcessHeap(), 0, BufferLength);
        if (!KeyInfo) return STATUS_NO_MEMORY;
    }

    /* Enumerate the key */
    Status = ZwEnumerateKey(KeyHandle,
                            Index,
                            KeyBasicInformation,
                            KeyInfo,
                            BufferLength,
                            &ReturnedLength);
    if (NT_SUCCESS(Status))
    {
        /* Check if the name fits */
        if (KeyInfo->NameLength <= SubKeyName->MaximumLength)
        {
            /* Set the length */
            SubKeyName->Length = KeyInfo->NameLength;

            /* Copy it */
            RtlMoveMemory(SubKeyName->Buffer,
                          KeyInfo->Name,
                          SubKeyName->Length);
        }
        else
        {
            /* Otherwise, we ran out of buffer space */
            Status = STATUS_BUFFER_OVERFLOW;
        }
    }

    /* Free the buffer and return status */
    if (KeyInfo) RtlFreeHeap(RtlGetProcessHeap(), 0, KeyInfo);
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlpNtMakeTemporaryKey(IN HANDLE KeyHandle)
{
    /* This just deletes the key */
    return ZwDeleteKey(KeyHandle);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlpNtOpenKey(OUT HANDLE KeyHandle,
              IN ACCESS_MASK DesiredAccess,
              IN POBJECT_ATTRIBUTES ObjectAttributes,
              IN ULONG Unused)
{
    /* Check if we have object attributes */
    if (ObjectAttributes)
    {
        /* Mask out the unsupported flags */
        ObjectAttributes->Attributes &= ~(OBJ_PERMANENT | OBJ_EXCLUSIVE);
    }

    /* Open the key */
    return ZwOpenKey(KeyHandle, DesiredAccess, ObjectAttributes);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlpNtQueryValueKey(IN HANDLE KeyHandle,
                    OUT PULONG Type OPTIONAL,
                    OUT PVOID Data OPTIONAL,
                    IN OUT PULONG DataLength OPTIONAL,
                    IN ULONG Unused)
{
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
    UNICODE_STRING ValueName;
    ULONG BufferLength = 0;
    NTSTATUS Status;

    /* Clear the value name */
    RtlInitEmptyUnicodeString(&ValueName, NULL, 0);

    /* Check if we were already given a length */
    if (DataLength) BufferLength = *DataLength;

    /* Add the size of the structure */
    BufferLength += FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);

    /* Allocate memory for the value */
    ValueInfo = RtlAllocateHeap(RtlGetProcessHeap(), 0, BufferLength);
    if (!ValueInfo) return STATUS_NO_MEMORY;

    /* Query the value */
    Status = ZwQueryValueKey(KeyHandle,
                             &ValueName,
                             KeyValuePartialInformation,
                             ValueInfo,
                             BufferLength,
                             &BufferLength);
    if ((NT_SUCCESS(Status)) || (Status == STATUS_BUFFER_OVERFLOW))
    {
        /* Return the length and type */
        if (DataLength) *DataLength = ValueInfo->DataLength;
        if (Type) *Type = ValueInfo->Type;
    }

    /* Check if the caller wanted data back, and we got it */
    if ((NT_SUCCESS(Status)) && (Data))
    {
        /* Copy it */
        RtlMoveMemory(Data, ValueInfo->Data, ValueInfo->DataLength);
    }

    /* Free the memory and return status */
    RtlFreeHeap(RtlGetProcessHeap(), 0, ValueInfo);
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlpNtSetValueKey(IN HANDLE KeyHandle,
                  IN ULONG Type,
                  IN PVOID Data,
                  IN ULONG DataLength)
{
    UNICODE_STRING ValueName;

    /* Set the value */
    RtlInitEmptyUnicodeString(&ValueName, NULL, 0);
    return ZwSetValueKey(KeyHandle,
                         &ValueName,
                         0,
                         Type,
                         Data,
                         DataLength);
}

/*
 * @unimplemented
 */
NTSTATUS NTAPI
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
  ULONG DataSize = 0;

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
		  Status = ZwDeleteValueKey(CurrentKeyHandle, &KeyName);

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
		  while (DataSize < (ValueInfo->DataLength-2))
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
			  DataSize += StringLen;
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
			Status = ZwDeleteValueKey(CurrentKeyHandle, &KeyName);
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

/* EOF */
