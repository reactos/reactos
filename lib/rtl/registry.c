/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Rtl registry functions
 * FILE:              lib/rtl/registry.c
 * PROGRAMER:         Alex Ionescu (alex.ionescu@reactos.org)
 *                    Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>
#define NDEBUG
#include <debug.h>

#define TAG_RTLREGISTRY TAG('R', 'q', 'r', 'v')

extern SIZE_T RtlpAllocDeallocQueryBufferSize;

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
RtlpQueryRegistryDirect(IN ULONG ValueType,
                        IN PVOID ValueData,
                        IN ULONG ValueLength,
                        IN PVOID Buffer)
{
    USHORT ActualLength = (USHORT)ValueLength;
    PUNICODE_STRING ReturnString = Buffer;
    PULONG Length = Buffer;
    ULONG RealLength;

    /* Check if this is a string */
    if ((ValueType == REG_SZ) ||
        (ValueType == REG_EXPAND_SZ) ||
        (ValueType == REG_MULTI_SZ))
    {
        /* Normalize the length */
        if (ValueLength > MAXUSHORT) ValueLength = MAXUSHORT;

        /* Check if the return string has been allocated */
        if (!ReturnString->Buffer)
        {
            /* Allocate it */
            ReturnString->Buffer = RtlpAllocateMemory(ActualLength, TAG_RTLREGISTRY);
            if (!ReturnString->Buffer) return STATUS_NO_MEMORY;
            ReturnString->MaximumLength = ActualLength;
        }
        else if (ActualLength > ReturnString->MaximumLength)
        {
            /* The string the caller allocated is too small */
            return STATUS_BUFFER_TOO_SMALL;
        }

        /* Copy the data */
        RtlCopyMemory(ReturnString->Buffer, ValueData, ActualLength);
        ReturnString->Length = ActualLength - sizeof(UNICODE_NULL);
    }
    else if (ValueLength <= sizeof(ULONG))
    {
        /* Check if we can just copy the data */
        if ((Buffer != ValueData) && (ValueLength))
        {
            /* Copy it */
            RtlCopyMemory(Buffer, ValueData, ValueLength);
        }
    }
    else
    {
        /* Check if the length is negative */
        if ((LONG)*Length < 0)
        {
            /* Get the real length and copy the buffer */
            RealLength = -(LONG)*Length;
            if (RealLength < ValueLength) return STATUS_BUFFER_TOO_SMALL;
            RtlCopyMemory(Buffer, ValueData, ValueLength);
        }
        else
        {
            /* Check if there's space for the length and type, plus data */
            if (*Length < (2 * sizeof(ULONG) + ValueLength))
            {
                /* Nope, fail */
                return STATUS_BUFFER_TOO_SMALL;
            }

            /* Return the data */
            *Length++ = ValueLength;
            *Length++ = ValueType;
            RtlCopyMemory(Length, ValueData, ValueLength);
        }
    }

    /* All done */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlpCallQueryRegistryRoutine(IN PRTL_QUERY_REGISTRY_TABLE QueryTable,
                             IN PKEY_VALUE_FULL_INFORMATION KeyValueInfo,
                             IN OUT PULONG InfoSize,
                             IN PVOID Context,
                             IN PVOID Environment)
{
    ULONG InfoLength, Length, c;
    LONG RequiredLength, SpareLength;
    PCHAR SpareData, DataEnd;
    ULONG Type;
    PWCHAR Name, p, ValueEnd;
    PVOID Data;
    NTSTATUS Status;
    BOOLEAN FoundExpander = FALSE;
    UNICODE_STRING Source, Destination;

    /* Setup defaults */
    InfoLength = *InfoSize;
    *InfoSize = 0;

    /* Check if there's no data */
    if (KeyValueInfo->DataOffset == (ULONG)-1)
    {
        /* Return proper status code */
        return (QueryTable->Flags & RTL_QUERY_REGISTRY_REQUIRED) ?
                STATUS_OBJECT_NAME_NOT_FOUND : STATUS_SUCCESS;
    }

    /* Setup spare data pointers */
    SpareData = (PCHAR)KeyValueInfo;
    SpareLength = InfoLength;
    DataEnd = SpareData + SpareLength;

    /* Check if there's no value or data */
    if ((KeyValueInfo->Type == REG_NONE) ||
        (!(KeyValueInfo->DataLength) &&
          (KeyValueInfo->Type == QueryTable->DefaultType)))
    {
        /* Check if there's no value */
        if (QueryTable->DefaultType == REG_NONE)
        {
            /* Return proper status code */
            return (QueryTable->Flags & RTL_QUERY_REGISTRY_REQUIRED) ?
                    STATUS_OBJECT_NAME_NOT_FOUND : STATUS_SUCCESS;
        }

        /* We can setup a default value... capture the defaults */
        Name = (PWCHAR)QueryTable->Name;
        Type = QueryTable->DefaultType;
        Data = QueryTable->DefaultData;
        Length = QueryTable->DefaultLength;
        if (!Length)
        {
            /* No default length given, try to calculate it */
            p = Data;
            if ((Type == REG_SZ) || (Type == REG_EXPAND_SZ))
            {
                /* This is a string, count the characters */
                while (*p++);
                Length = (ULONG_PTR)p - (ULONG_PTR)Data;
            }
            else if (Type == REG_MULTI_SZ)
            {
                /* This is a multi-string, calculate all characters */
                while (*p) while (*p++);
                Length = (ULONG_PTR)p - (ULONG_PTR)Data + sizeof(UNICODE_NULL);
            }
        }
    }
    else
    {
        /* Check if this isn't a direct return */
        if (!(QueryTable->Flags & RTL_QUERY_REGISTRY_DIRECT))
        {
            /* Check if we have length */
            if (KeyValueInfo->DataLength)
            {
                /* Increase the spare data */
                SpareData += KeyValueInfo->DataOffset +
                             KeyValueInfo->DataLength;
            }
            else
            {
                /* Otherwise, the spare data only has the name data */
                SpareData += FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name) +
                             KeyValueInfo->NameLength;
            }

            /* Align the pointer and get new size of spare data */
            SpareData = (PVOID)(((ULONG_PTR)SpareData + 7) & ~7);
            SpareLength = DataEnd - SpareData;

            /* Check if we have space to copy the data */
            RequiredLength = KeyValueInfo->NameLength + sizeof(UNICODE_NULL);
            if (SpareLength < RequiredLength)
            {
                /* Fail and return the missing length */
                *InfoSize = SpareData - (PCHAR)KeyValueInfo + RequiredLength;
                return STATUS_BUFFER_TOO_SMALL;
            }

            /* Copy the data and null-terminate it */
            Name = (PWCHAR)SpareData;
            RtlCopyMemory(Name, KeyValueInfo->Name, KeyValueInfo->NameLength);
            Name[KeyValueInfo->NameLength / sizeof(WCHAR)] = UNICODE_NULL;

            /* Update the spare data information */
            SpareData += RequiredLength;
            SpareData = (PVOID)(((ULONG_PTR)SpareData + 7) & ~7);
            SpareLength = DataEnd - SpareData;
        }
        else
        {
            /* Just return the name */
            Name = (PWCHAR)QueryTable->Name;
        }

        /* Capture key data */
        Type = KeyValueInfo->Type;
        Data = (PVOID)((ULONG_PTR)KeyValueInfo + KeyValueInfo->DataOffset);
        Length = KeyValueInfo->DataLength;
    }

    /* Check if we're expanding */
    if (!(QueryTable->Flags & RTL_QUERY_REGISTRY_NOEXPAND))
    {
        /* Check if it's a multi-string */
        if (Type == REG_MULTI_SZ)
        {
            /* Prepare defaults */
            Status = STATUS_SUCCESS;
            ValueEnd = (PWSTR)((ULONG_PTR)Data + Length) - sizeof(UNICODE_NULL);
            p = Data;

            /* Loop all strings */
            while (p < ValueEnd)
            {
                /* Go to the next string */
                while (*p++);

                /* Get the length and check if this is direct */
                Length = (ULONG_PTR)p - (ULONG_PTR)Data;
                if (QueryTable->Flags & RTL_QUERY_REGISTRY_DIRECT)
                {
                    /* Do the query */
                    Status = RtlpQueryRegistryDirect(REG_SZ,
                                                     Data,
                                                     Length,
                                                     QueryTable->EntryContext);
                    QueryTable->EntryContext = (PVOID)((ULONG_PTR)QueryTable->
                                                       EntryContext +
                                                       sizeof(UNICODE_STRING));
                }
                else
                {
                    /* Call the custom routine */
                    Status = QueryTable->QueryRoutine(Name,
                                                      REG_SZ,
                                                      Data,
                                                      Length,
                                                      Context,
                                                      QueryTable->EntryContext);
                }

                /* Normalize status */
                if (Status == STATUS_BUFFER_TOO_SMALL) Status = STATUS_SUCCESS;
                if (!NT_SUCCESS(Status)) break;

                /* Update data pointer */
                Data = p;
            }

            /* Return */
            return Status;
        }

        /* Check if this is an expand string */
        if ((Type == REG_EXPAND_SZ) && (Length >= sizeof(WCHAR)))
        {
            /* Try to find the expander */
            c = Length - sizeof(UNICODE_NULL);
            p = (PWCHAR)Data;
            while (c)
            {
                /* Check if this is one */
                if (*p == L'%')
                {
                    /* Yup! */
                    FoundExpander = TRUE;
                    break;
                }

                /* Continue in the buffer */
                p++;
                c -= sizeof(WCHAR);
            }

            /* So check if we have one */
            if (FoundExpander)
            {
                /* Setup the source string */
                RtlInitEmptyUnicodeString(&Source, Data, Length);
                Source.Length = Source.MaximumLength - sizeof(UNICODE_NULL);

                /* Setup the desination string */
                RtlInitEmptyUnicodeString(&Destination, (PWCHAR)SpareData, 0);

                /* Check if we're out of space */
                if (SpareLength <= 0)
                {
                    /* Then we don't have any space in our string */
                    Destination.MaximumLength = 0;
                }
                else if (SpareLength <= MAXUSHORT)
                {
                    /* This is the good case, where we fit into a string */
                    Destination.MaximumLength = SpareLength;
                    Destination.Buffer[SpareLength / 2 - 1] = UNICODE_NULL;
                }
                else
                {
                    /* We can't fit into a string, so truncate */
                    Destination.MaximumLength = MAXUSHORT;
                    Destination.Buffer[MAXUSHORT / 2 - 1] = UNICODE_NULL;
                }

                /* Expand the strings and set our type as one string */
                Status = RtlExpandEnvironmentStrings_U(Environment,
                                                       &Source,
                                                       &Destination,
                                                       (PULONG)&RequiredLength);
                Type = REG_SZ;

                /* Check for success */
                if (NT_SUCCESS(Status))
                {
                    /* Set the value name and length to our string */
                    Data = Destination.Buffer;
                    Length = Destination.Length + sizeof(UNICODE_NULL);
                }
                else
                {
                    /* Check if our buffer is too small */
                    if (Status == STATUS_BUFFER_TOO_SMALL)
                    {
                        /* Set the required missing length */
                        *InfoSize = SpareData -
                                    (PCHAR)KeyValueInfo +
                                    RequiredLength;

                        /* Notify debugger */
                        DPRINT1("RTL: Expand variables for %wZ failed - "
                                "Status == %lx Size %x > %x <%x>\n",
                                &Source,
                                Status,
                                *InfoSize,
                                InfoLength,
                                Destination.MaximumLength);
                    }
                    else
                    {
                        /* Notify debugger */
                        DPRINT1("RTL: Expand variables for %wZ failed - "
                                "Status == %lx\n",
                                &Source,
                                Status);
                    }

                    /* Return the status */
                    return Status;
                }
            }
        }
    }

    /* Check if this is a direct query */
    if (QueryTable->Flags & RTL_QUERY_REGISTRY_DIRECT)
    {
        /* Return the data */
        Status = RtlpQueryRegistryDirect(Type,
                                         Data,
                                         Length,
                                         QueryTable->EntryContext);
    }
    else
    {
        /* Call the query routine */
        Status = QueryTable->QueryRoutine(Name,
                                          Type,
                                          Data,
                                          Length,
                                          Context,
                                          QueryTable->EntryContext);
    }

    /* Normalize and return status */
    return (Status == STATUS_BUFFER_TOO_SMALL) ? STATUS_SUCCESS : Status;
}

PVOID
NTAPI
RtlpAllocDeallocQueryBuffer(IN OUT PSIZE_T BufferSize,
                            IN PVOID OldBuffer,
                            IN SIZE_T OldBufferSize,
                            OUT PNTSTATUS Status)
{
    PVOID Buffer = NULL;

    /* Assume success */
    if (Status) *Status = STATUS_SUCCESS;

    /* Free the old buffer */
    if (OldBuffer) RtlpFreeMemory(OldBuffer, TAG_RTLREGISTRY);

    /* Check if we need to allocate a new one */
    if (BufferSize)
    {
        /* Allocate */
        Buffer = RtlpAllocateMemory(*BufferSize, TAG_RTLREGISTRY);
        if (!(Buffer) && (Status)) *Status = STATUS_NO_MEMORY;
    }

    /* Return the pointer */
    return Buffer;
}

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
 * @implemented
 */
NTSTATUS
NTAPI
RtlQueryRegistryValues(IN ULONG RelativeTo,
                       IN PCWSTR Path,
                       IN PRTL_QUERY_REGISTRY_TABLE QueryTable,
                       IN PVOID Context,
                       IN PVOID Environment OPTIONAL)
{
    NTSTATUS Status;
    PKEY_VALUE_FULL_INFORMATION KeyValueInfo = NULL;
    HANDLE KeyHandle, CurrentKey;
    SIZE_T BufferSize, InfoSize;
    UNICODE_STRING KeyPath, KeyValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG i, Value;
    ULONG ResultLength;

    /* Get the registry handle */
    Status = RtlpGetRegistryHandle(RelativeTo, Path, FALSE, &KeyHandle);
    if (!NT_SUCCESS(Status)) return Status;

    /* Initialize the path */
    RtlInitUnicodeString(&KeyPath,
                         (RelativeTo & RTL_REGISTRY_HANDLE) ? NULL : Path);

    /* Allocate a query buffer */
    BufferSize = RtlpAllocDeallocQueryBufferSize;
    KeyValueInfo = RtlpAllocDeallocQueryBuffer(&BufferSize, NULL, 0, &Status);
    if (!KeyValueInfo)
    {
        /* Close the handle if we have one and fail */
        if (!(RelativeTo & RTL_REGISTRY_HANDLE)) ZwClose(KeyHandle);
        return Status;
    }

    /* Set defaults */
    KeyValueInfo->DataOffset = 0;
    InfoSize = BufferSize - sizeof(UNICODE_NULL);
    CurrentKey = KeyHandle;

    /* Loop the query table */
    while ((QueryTable->QueryRoutine) ||
           (QueryTable->Flags & (RTL_QUERY_REGISTRY_SUBKEY |
                                 RTL_QUERY_REGISTRY_DIRECT)))
    {
        /* Check if the request is invalid */
        if ((QueryTable->Flags & RTL_QUERY_REGISTRY_DIRECT) &&
            (!(QueryTable->Name) ||
             (QueryTable->Flags & RTL_QUERY_REGISTRY_SUBKEY) ||
             (QueryTable->QueryRoutine)))
        {
            /* Fail */
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        /* Check if we want a specific key */
        if (QueryTable->Flags & (RTL_QUERY_REGISTRY_TOPKEY |
                                 RTL_QUERY_REGISTRY_SUBKEY))
        {
            /* Check if we're working with another handle */
            if (CurrentKey != KeyHandle)
            {
                /* Close our current key and use the top */
                NtClose(CurrentKey);
                CurrentKey = KeyHandle;
            }
        }

        /* Check if we're querying the subkey */
        if (QueryTable->Flags & RTL_QUERY_REGISTRY_SUBKEY)
        {
            /* Make sure we have a name */
            if (!QueryTable->Name)
            {
                /* Fail */
                Status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                /* Initialize the name */
                RtlInitUnicodeString(&KeyPath, QueryTable->Name);

                /* Get the key handle */
                InitializeObjectAttributes(&ObjectAttributes,
                                           &KeyPath,
                                           OBJ_CASE_INSENSITIVE |
                                           OBJ_KERNEL_HANDLE,
                                           KeyHandle,
                                           NULL);
                Status = ZwOpenKey(&CurrentKey,
                                   MAXIMUM_ALLOWED,
                                   &ObjectAttributes);
                if (NT_SUCCESS(Status))
                {
                    /* If we have a query routine, go enumerate values */
                    if (QueryTable->QueryRoutine) goto ProcessValues;
                }
            }
        }
        else if (QueryTable->Name)
        {
            /* Initialize the path */
            RtlInitUnicodeString(&KeyValueName, QueryTable->Name);

            /* Start query loop */
            i = 0;
            while (TRUE)
            {
                /* Make sure we didn't retry too many times */
                if (i++ > 4)
                {
                    /* Fail */
                    DPRINT1("RtlQueryRegistryValues: Miscomputed buffer size "
                            "at line %d\n", __LINE__);
                    break;
                }

                /* Query key information */
                Status = ZwQueryValueKey(CurrentKey,
                                         &KeyValueName,
                                         KeyValueFullInformation,
                                         KeyValueInfo,
                                         InfoSize,
                                         &ResultLength);
                if (Status == STATUS_BUFFER_OVERFLOW)
                {
                    /* Normalize status code */
                    Status = STATUS_BUFFER_TOO_SMALL;
                }

                /* Check for failure */
                if (!NT_SUCCESS(Status))
                {
                    /* Check if we didn't find it */
                    if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
                    {
                        /* Setup a default */
                        KeyValueInfo->Type = REG_NONE;
                        KeyValueInfo->DataLength = 0;
                        ResultLength = InfoSize;

                        /* Call the query routine */
                        Status = RtlpCallQueryRegistryRoutine(QueryTable,
                                                              KeyValueInfo,
                                                              &ResultLength,
                                                              Context,
                                                              Environment);
                    }

                    /* Check for buffer being too small */
                    if (Status == STATUS_BUFFER_TOO_SMALL)
                    {
                        /* Increase allocation size */
                        BufferSize = ResultLength +
                                     sizeof(ULONG_PTR) +
                                     sizeof(UNICODE_NULL);
                        KeyValueInfo = RtlpAllocDeallocQueryBuffer(&BufferSize,
                                                                   KeyValueInfo,
                                                                   BufferSize,
                                                                   &Status);
                        if (!KeyValueInfo) break;

                        /* Update the data */
                        KeyValueInfo->DataOffset = 0;
                        InfoSize = BufferSize - sizeof(UNICODE_NULL);
                        continue;
                    }
                }
                else
                {
                    /* Check if this is a multi-string */
                    if (KeyValueInfo->Type == REG_MULTI_SZ)
                    {
                        /* Add a null-char */
                        ((PWCHAR)KeyValueInfo)[ResultLength / 2] = UNICODE_NULL;
                        KeyValueInfo->DataLength += sizeof(UNICODE_NULL);
                    }

                    /* Call the query routine */
                    ResultLength = InfoSize;
                    Status = RtlpCallQueryRegistryRoutine(QueryTable,
                                                          KeyValueInfo,
                                                          &ResultLength,
                                                          Context,
                                                          Environment);

                    /* Check for buffer being too small */
                    if (Status == STATUS_BUFFER_TOO_SMALL)
                    {
                        /* Increase allocation size */
                        BufferSize = ResultLength +
                                     sizeof(ULONG_PTR) +
                                     sizeof(UNICODE_NULL);
                        KeyValueInfo = RtlpAllocDeallocQueryBuffer(&BufferSize,
                                                                   KeyValueInfo,
                                                                   BufferSize,
                                                                   &Status);
                        if (!KeyValueInfo) break;

                        /* Update the data */
                        KeyValueInfo->DataOffset = 0;
                        InfoSize = BufferSize - sizeof(UNICODE_NULL);
                        continue;
                    }

                    /* Check if we need to delete the key */
                    if ((NT_SUCCESS(Status)) &&
                        (QueryTable->Flags & RTL_QUERY_REGISTRY_DELETE))
                    {
                        /* Delete it */
                        ZwDeleteValueKey(CurrentKey, &KeyValueName);
                    }
                }

                /* We're done, break out */
                break;
            }
        }
        else if (QueryTable->Flags & RTL_QUERY_REGISTRY_NOVALUE)
        {
            /* Just call the query routine */
            Status = QueryTable->QueryRoutine(NULL,
                                              REG_NONE,
                                              NULL,
                                              0,
                                              Context,
                                              QueryTable->EntryContext);
        }
        else
        {
ProcessValues:
            /* Loop every value */
            i = Value = 0;
            while (TRUE)
            {
                /* Enumerate the keys */
                Status = ZwEnumerateValueKey(CurrentKey,
                                             Value,
                                             KeyValueFullInformation,
                                             KeyValueInfo,
                                             InfoSize,
                                             &ResultLength);
                if (Status == STATUS_BUFFER_OVERFLOW)
                {
                    /* Normalize the status */
                    Status = STATUS_BUFFER_TOO_SMALL;
                }

                /* Check if we found all the entries */
                if (Status == STATUS_NO_MORE_ENTRIES)
                {
                    /* Check if this was the first entry and caller needs it */
                    if (!(Value) &&
                         (QueryTable->Flags & RTL_QUERY_REGISTRY_REQUIRED))
                    {
                        /* Fail */
                        Status = STATUS_OBJECT_NAME_NOT_FOUND;
                    }
                    else
                    {
                        /* Otherwise, it's ok */
                        Status = STATUS_SUCCESS;
                    }
                    break;
                }

                /* Check if enumeration worked */
                if (NT_SUCCESS(Status))
                {
                    /* Call the query routine */
                    ResultLength = InfoSize;
                    Status = RtlpCallQueryRegistryRoutine(QueryTable,
                                                          KeyValueInfo,
                                                          &ResultLength,
                                                          Context,
                                                          Environment);
                }

                /* Check if the query failed */
                if (Status == STATUS_BUFFER_TOO_SMALL)
                {
                    /* Increase allocation size */
                    BufferSize = ResultLength +
                                 sizeof(ULONG_PTR) +
                                 sizeof(UNICODE_NULL);
                    KeyValueInfo = RtlpAllocDeallocQueryBuffer(&BufferSize,
                                                               KeyValueInfo,
                                                               BufferSize,
                                                               &Status);
                    if (!KeyValueInfo) break;

                    /* Update the data */
                    KeyValueInfo->DataOffset = 0;
                    InfoSize = BufferSize - sizeof(UNICODE_NULL);

                    /* Try the value again unless it's been too many times */
                    if (i++ <= 4) continue;
                    break;
                }

                /* Break out if we failed */
                if (!NT_SUCCESS(Status)) break;

                /* Reset the number of retries and check if we need to delete */
                i = 0;
                if (QueryTable->Flags & RTL_QUERY_REGISTRY_DELETE)
                {
                    /* Build the name */
                    RtlInitEmptyUnicodeString(&KeyValueName,
                                              KeyValueInfo->Name,
                                              (USHORT)KeyValueInfo->NameLength);
                    KeyValueName.Length = KeyValueName.MaximumLength;

                    /* Delete the key */
                    Status = ZwDeleteValueKey(CurrentKey, &KeyValueName);
                    if (NT_SUCCESS(Status)) Value--;
                }

                /* Go to the next value */
                Value++;
            }
        }

        /* Check if we failed anywhere along the road */
        if (!NT_SUCCESS(Status)) break;

        /* Continue */
        QueryTable++;
    }

    /* Check if we need to close our handle */
    if ((KeyHandle) && !(RelativeTo & RTL_REGISTRY_HANDLE)) ZwClose(KeyHandle);
    if ((CurrentKey) && (CurrentKey != KeyHandle)) ZwClose(CurrentKey);

    /* Free our buffer and return status */
    RtlpAllocDeallocQueryBuffer(NULL, KeyValueInfo, BufferSize, NULL);
    return Status;
}

/* EOF */
