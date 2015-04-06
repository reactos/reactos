/*
 * PROJECT:         ReactOS Windows-Compatible Session Manager
 * LICENSE:         BSD 2-Clause License
 * FILE:            base/system/smss/smss.c
 * PURPOSE:         Main SMSS Code
 * PROGRAMMERS:     Alex Ionescu
 */

/* INCLUDES *******************************************************************/

#include "smss.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

UNICODE_STRING SmpSubsystemName, PosixName, Os2Name;
LIST_ENTRY SmpBootExecuteList, SmpSetupExecuteList, SmpPagingFileList;
LIST_ENTRY SmpDosDevicesList, SmpFileRenameList, SmpKnownDllsList;
LIST_ENTRY SmpExcludeKnownDllsList, SmpSubSystemList, SmpSubSystemsToLoad;
LIST_ENTRY SmpSubSystemsToDefer, SmpExecuteList, NativeProcessList;

PVOID SmpHeap;
ULONG SmBaseTag;
HANDLE SmpDebugPort, SmpDosDevicesObjectDirectory;
PWCHAR SmpDefaultEnvironment, SmpDefaultLibPathBuffer;
UNICODE_STRING SmpKnownDllPath, SmpDefaultLibPath;
ULONG SmpCalledConfigEnv;

ULONG SmpInitProgressByLine;
NTSTATUS SmpInitReturnStatus;
PVOID SmpInitLastCall;

SECURITY_DESCRIPTOR SmpPrimarySDBody, SmpLiberalSDBody, SmpKnownDllsSDBody;
SECURITY_DESCRIPTOR SmpApiPortSDBody;
PISECURITY_DESCRIPTOR SmpPrimarySecurityDescriptor, SmpLiberalSecurityDescriptor;
PISECURITY_DESCRIPTOR SmpKnownDllsSecurityDescriptor, SmpApiPortSecurityDescriptor;

ULONG SmpAllowProtectedRenames, SmpProtectionMode = 1;
BOOLEAN MiniNTBoot;

#define SMSS_CHECKPOINT(x, y)           \
{                                       \
    SmpInitProgressByLine = __LINE__;   \
    SmpInitReturnStatus = (y);          \
    SmpInitLastCall = (x);              \
}

/* REGISTRY CONFIGURATION *****************************************************/

NTSTATUS
NTAPI
SmpSaveRegistryValue(IN PLIST_ENTRY ListAddress,
                     IN PWSTR Name,
                     IN PWCHAR Value,
                     IN BOOLEAN Flags)
{
    PSMP_REGISTRY_VALUE RegEntry;
    UNICODE_STRING NameString, ValueString;
    ANSI_STRING AnsiValueString;
    PLIST_ENTRY NextEntry;

    /* Convert to unicode strings */
    RtlInitUnicodeString(&NameString, Name);
    RtlInitUnicodeString(&ValueString, Value);

    /* In case this is the first value, initialize a new list/structure */
    RegEntry = NULL;

    /* Check if we should do a duplicate check */
    if (Flags)
    {
        /* Loop the current list */
        NextEntry = ListAddress->Flink;
        while (NextEntry != ListAddress)
        {
            /* Get each entry */
            RegEntry = CONTAINING_RECORD(NextEntry, SMP_REGISTRY_VALUE, Entry);

            /* Check if the value name matches */
            if (!RtlCompareUnicodeString(&RegEntry->Name, &NameString, TRUE))
            {
                /* Check if the value is the exact same thing */
                if (!RtlCompareUnicodeString(&RegEntry->Value, &ValueString, TRUE))
                {
                    /* Fail -- the same setting is being set twice */
                    return STATUS_OBJECT_NAME_EXISTS;
                }

                /* We found the list, and this isn't a duplicate value */
                break;
            }

            /* This wasn't a match, keep going */
            NextEntry = NextEntry->Flink;
            RegEntry = NULL;
        }
    }

    /* Are we adding on, or creating a new entry */
    if (!RegEntry)
    {
        /* A new entry -- allocate it */
        RegEntry = RtlAllocateHeap(RtlGetProcessHeap(),
                                   SmBaseTag,
                                   sizeof(SMP_REGISTRY_VALUE) +
                                   NameString.MaximumLength);
        if (!RegEntry) return STATUS_NO_MEMORY;

        /* Initialize the list and set all values to NULL */
        InitializeListHead(&RegEntry->Entry);
        RegEntry->AnsiValue = NULL;
        RegEntry->Value.Buffer = NULL;

        /* Copy and initialize the value name */
        RegEntry->Name.Buffer = (PWCHAR)(RegEntry + 1);
        RegEntry->Name.Length = NameString.Length;
        RegEntry->Name.MaximumLength = NameString.MaximumLength;
        RtlCopyMemory(RegEntry->Name.Buffer,
                      NameString.Buffer,
                      NameString.MaximumLength);

        /* Add this entry into the list */
        InsertTailList(ListAddress, &RegEntry->Entry);
    }

    /* Did we have an old value buffer? */
    if (RegEntry->Value.Buffer)
    {
        /* Free it */
        ASSERT(RegEntry->Value.Length != 0);
        RtlFreeHeap(RtlGetProcessHeap(), 0, RegEntry->Value.Buffer);
    }

    /* Is there no value associated? */
    if (!Value)
    {
        /* We're done here */
        RtlInitUnicodeString(&RegEntry->Value, NULL);
        return STATUS_SUCCESS;
    }

    /* There is a value, so allocate a buffer for it */
    RegEntry->Value.Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                             SmBaseTag,
                                             ValueString.MaximumLength);
    if (!RegEntry->Value.Buffer)
    {
        /* Out of memory, undo */
        RemoveEntryList(&RegEntry->Entry);
        RtlFreeHeap(RtlGetProcessHeap(), 0, RegEntry);
        return STATUS_NO_MEMORY;
    }

    /* Copy the value into the entry */
    RegEntry->Value.Length = ValueString.Length;
    RegEntry->Value.MaximumLength = ValueString.MaximumLength;
    RtlCopyMemory(RegEntry->Value.Buffer,
                  ValueString.Buffer,
                  ValueString.MaximumLength);

    /* Now allocate memory for an ANSI copy of it */
    RegEntry->AnsiValue = RtlAllocateHeap(RtlGetProcessHeap(),
                                          SmBaseTag,
                                          (ValueString.Length / sizeof(WCHAR)) +
                                          sizeof(ANSI_NULL));
    if (!RegEntry->AnsiValue)
    {
        /* Out of memory, undo */
        RtlFreeHeap(RtlGetProcessHeap(), 0, RegEntry->Value.Buffer);
        RemoveEntryList(&RegEntry->Entry);
        RtlFreeHeap(RtlGetProcessHeap(), 0, RegEntry);
        return STATUS_NO_MEMORY;
    }

    /* Convert the Unicode value string and return success */
    RtlInitEmptyAnsiString(&AnsiValueString,
                           RegEntry->AnsiValue,
                           (ValueString.Length / sizeof(WCHAR)) +
                           sizeof(ANSI_NULL));
    RtlUnicodeStringToAnsiString(&AnsiValueString, &ValueString, FALSE);
    return STATUS_SUCCESS;
}

PSMP_REGISTRY_VALUE
NTAPI
SmpFindRegistryValue(IN PLIST_ENTRY List,
                     IN PWSTR ValueName)
{
    PSMP_REGISTRY_VALUE RegEntry;
    UNICODE_STRING ValueString;
    PLIST_ENTRY NextEntry;

    /* Initialize the value name sting */
    RtlInitUnicodeString(&ValueString, ValueName);

    /* Loop the list */
    NextEntry = List->Flink;
    while (NextEntry != List)
    {
        /* Get each entry */
        RegEntry = CONTAINING_RECORD(NextEntry, SMP_REGISTRY_VALUE, Entry);

        /* Check if the value name matches */
        if (!RtlCompareUnicodeString(&RegEntry->Name, &ValueString, TRUE)) break;

        /* It doesn't, move on */
        NextEntry = NextEntry->Flink;
    }

    /* If we looped back, return NULL, otherwise return the entry we found */
    if (NextEntry == List) RegEntry = NULL;
    return RegEntry;
}

NTSTATUS
NTAPI
SmpConfigureProtectionMode(IN PWSTR ValueName,
                           IN ULONG ValueType,
                           IN PVOID ValueData,
                           IN ULONG ValueLength,
                           IN PVOID Context,
                           IN PVOID EntryContext)
{
    /* Make sure the value is valid */
    if (ValueLength == sizeof(ULONG))
    {
        /* Read it */
        SmpProtectionMode = *(PULONG)ValueData;
    }
    else
    {
        /* Default is to protect stuff */
        SmpProtectionMode = 1;
    }

    /* Recreate the security descriptors to take into account security mode */
    SmpCreateSecurityDescriptors(FALSE);
    DPRINT("SmpProtectionMode: %lu\n", SmpProtectionMode);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
SmpConfigureAllowProtectedRenames(IN PWSTR ValueName,
                                  IN ULONG ValueType,
                                  IN PVOID ValueData,
                                  IN ULONG ValueLength,
                                  IN PVOID Context,
                                  IN PVOID EntryContext)
{
    /* Make sure the value is valid */
    if (ValueLength == sizeof(ULONG))
    {
        /* Read it */
        SmpAllowProtectedRenames = *(PULONG)ValueData;
    }
    else
    {
        /* Default is to not allow protected renames */
        SmpAllowProtectedRenames = 0;
    }

    DPRINT("SmpAllowProtectedRenames: %lu\n", SmpAllowProtectedRenames);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
SmpConfigureObjectDirectories(IN PWSTR ValueName,
                              IN ULONG ValueType,
                              IN PVOID ValueData,
                              IN ULONG ValueLength,
                              IN PVOID Context,
                              IN PVOID EntryContext)
{
    PISECURITY_DESCRIPTOR SecDescriptor;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE DirHandle;
    UNICODE_STRING RpcString, WindowsString, SearchString;
    PWCHAR SourceString = ValueData;

    /* Initialize the two strings we will be looking for */
    RtlInitUnicodeString(&RpcString, L"\\RPC Control");
    RtlInitUnicodeString(&WindowsString, L"\\Windows");

    /* Loop the registry data we received */
    while (*SourceString)
    {
        /* Assume primary SD for most objects */
        RtlInitUnicodeString(&SearchString, SourceString);
        SecDescriptor = SmpPrimarySecurityDescriptor;

        /* But for these two always set the liberal descriptor */
        if ((RtlEqualUnicodeString(&SearchString, &RpcString, TRUE)) ||
            (RtlEqualUnicodeString(&SearchString, &WindowsString, TRUE)))
        {
            SecDescriptor = SmpLiberalSecurityDescriptor;
        }

        /* Create the requested directory with the requested descriptor */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &SearchString,
                                   OBJ_CASE_INSENSITIVE |
                                   OBJ_OPENIF |
                                   OBJ_PERMANENT,
                                   NULL,
                                   SecDescriptor);
        DPRINT("Creating: %wZ directory\n", &SearchString);
        Status = NtCreateDirectoryObject(&DirHandle,
                                         DIRECTORY_ALL_ACCESS,
                                         &ObjectAttributes);
        if (!NT_SUCCESS(Status))
        {
            /* Failure case */
            DPRINT1("SMSS: Unable to create %wZ object directory - Status == %lx\n",
                    &SearchString, Status);
        }
        else
        {
            /* It worked, now close the handle */
            NtClose(DirHandle);
        }

        /* Move to the next requested object */
        while (*SourceString++);
    }

    /* All done */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
SmpConfigureMemoryMgmt(IN PWSTR ValueName,
                       IN ULONG ValueType,
                       IN PVOID ValueData,
                       IN ULONG ValueLength,
                       IN PVOID Context,
                       IN PVOID EntryContext)
{
    /* Save this is into a list */
    return SmpSaveRegistryValue(EntryContext, ValueData, NULL, TRUE);
}

NTSTATUS
NTAPI
SmpConfigureFileRenames(IN PWSTR ValueName,
                        IN ULONG ValueType,
                        IN PVOID ValueData,
                        IN ULONG ValueLength,
                        IN PVOID Context,
                        IN PVOID EntryContext)
{
    NTSTATUS Status;
    static PWCHAR Canary = NULL;

    /* Check if this is the second call */
    if (Canary)
    {
        /* Save the data into the list */
        DPRINT1("Renamed file: '%S' - '%S'\n", Canary, ValueData);
        Status = SmpSaveRegistryValue(EntryContext, Canary, ValueData, FALSE);
        Canary = NULL;
    }
    else
    {
        /* This it the first call, do nothing until we get the second call */
        Canary = ValueData;
        Status = STATUS_SUCCESS;
    }

    /* Return the status */
    return Status;
}

NTSTATUS
NTAPI
SmpConfigureExcludeKnownDlls(IN PWSTR ValueName,
                             IN ULONG ValueType,
                             IN PVOID ValueData,
                             IN ULONG ValueLength,
                             IN PVOID Context,
                             IN PVOID EntryContext)
{
    PWCHAR DllName;
    NTSTATUS Status;

    /* Make sure the value type is valid */
    if ((ValueType == REG_MULTI_SZ) || (ValueType == REG_SZ))
    {
        /* Keep going for each DLL in the list */
        DllName = ValueData;
        while (*DllName)
        {
            /* Add this to the linked list */
            DPRINT("Excluded DLL: %S\n", DllName);
            Status = SmpSaveRegistryValue(EntryContext, DllName, NULL, TRUE);

            /* Bail out on failure or if only one DLL name was present */
            if (!(NT_SUCCESS(Status)) || (ValueType == REG_SZ)) return Status;

            /* Otherwise, move to the next DLL name */
            while (*DllName++);
        }
    }

    /* All done */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
SmpConfigureDosDevices(IN PWSTR ValueName,
                       IN ULONG ValueType,
                       IN PVOID ValueData,
                       IN ULONG ValueLength,
                       IN PVOID Context,
                       IN PVOID EntryContext)
{
    /* Save into linked list */
    return SmpSaveRegistryValue(EntryContext, ValueName, ValueData, TRUE);
}

NTSTATUS
NTAPI
SmpInitializeKnownDllPath(IN PUNICODE_STRING DllPath,
                          IN PWCHAR Buffer,
                          IN ULONG Length)
{
    NTSTATUS Status;

    /* Allocate the buffer */
    DllPath->Buffer = RtlAllocateHeap(RtlGetProcessHeap(), SmBaseTag, Length);
    if (DllPath->Buffer)
    {
        /* Fill out the rest of the string */
        DllPath->MaximumLength = (USHORT)Length;
        DllPath->Length = (USHORT)Length - sizeof(UNICODE_NULL);

        /* Copy the actual path and return success */
        RtlCopyMemory(DllPath->Buffer, Buffer, Length);
        Status = STATUS_SUCCESS;
    }
    else
    {
        /* Fail with out of memory code */
        Status = STATUS_NO_MEMORY;
    }

    /* Return result */
    return Status;
}

NTSTATUS
NTAPI
SmpConfigureKnownDlls(IN PWSTR ValueName,
                      IN ULONG ValueType,
                      IN PVOID ValueData,
                      IN ULONG ValueLength,
                      IN PVOID Context,
                      IN PVOID EntryContext)
{
    /* Check which value is being set */
    if (_wcsicmp(ValueName, L"DllDirectory") == 0)
    {
        /* This is the directory, initialize it */
        DPRINT("KnownDll Path: %S\n", ValueData);
        return SmpInitializeKnownDllPath(&SmpKnownDllPath, ValueData, ValueLength);
    }
    else
    {
        /* Add to the linked list -- this is a file */
        return SmpSaveRegistryValue(EntryContext, ValueName, ValueData, TRUE);
    }
}

NTSTATUS
NTAPI
SmpConfigureEnvironment(IN PWSTR ValueName,
                        IN ULONG ValueType,
                        IN PVOID ValueData,
                        IN ULONG ValueLength,
                        IN PVOID Context,
                        IN PVOID EntryContext)
{
    NTSTATUS Status;
    UNICODE_STRING ValueString, DataString;

    /* Convert the strings into UNICODE_STRING and set the variable defined */
    RtlInitUnicodeString(&ValueString, ValueName);
    RtlInitUnicodeString(&DataString, ValueData);
    DPRINT("Setting %wZ = %wZ\n", &ValueString, &DataString);
    Status = RtlSetEnvironmentVariable(0, &ValueString, &DataString);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SMSS: 'SET %wZ = %wZ' failed - Status == %lx\n",
                &ValueString, &DataString, Status);
        return Status;
    }

    /* Check if the path is being set, and wait for the second instantiation */
    if ((_wcsicmp(ValueName, L"Path") == 0) && (++SmpCalledConfigEnv == 2))
    {
        /* Allocate the path buffer */
        SmpDefaultLibPathBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                                  SmBaseTag,
                                                  ValueLength);
        if (!SmpDefaultLibPathBuffer) return STATUS_NO_MEMORY;

        /* Copy the data into it and create the UNICODE_STRING to hold it */
        RtlCopyMemory(SmpDefaultLibPathBuffer, ValueData, ValueLength);
        RtlInitUnicodeString(&SmpDefaultLibPath, SmpDefaultLibPathBuffer);
    }

    /* All good */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
SmpConfigureSubSystems(IN PWSTR ValueName,
                       IN ULONG ValueType,
                       IN PVOID ValueData,
                       IN ULONG ValueLength,
                       IN PVOID Context,
                       IN PVOID EntryContext)
{
    PSMP_REGISTRY_VALUE RegEntry;
    PWCHAR SubsystemName;

    /* Is this a required or optional subsystem? */
    if ((_wcsicmp(ValueName, L"Required") != 0) &&
        (_wcsicmp(ValueName, L"Optional") != 0))
    {
        /* It isn't, is this the PSI flag? */
        if ((_wcsicmp(ValueName, L"PosixSingleInstance") != 0) ||
            (ValueType != REG_DWORD))
        {
            /* It isn't, must be a subsystem entry, add it to the list */
            DPRINT("Subsystem entry: %S-%S\n", ValueName, ValueData);
            return SmpSaveRegistryValue(EntryContext, ValueName, ValueData, TRUE);
        }

        /* This was the PSI flag, save it and exit */
        RegPosixSingleInstance = TRUE;
        return STATUS_SUCCESS;
    }

    /* This should be one of the required/optional lists. Is the type valid? */
    if (ValueType == REG_MULTI_SZ)
    {
        /* It is, get the first subsystem */
        SubsystemName = ValueData;
        while (*SubsystemName)
        {
            /* We should have already put it into the list when we found it */
            DPRINT("Found subsystem: %S\n", SubsystemName);
            RegEntry = SmpFindRegistryValue(EntryContext, SubsystemName);
            if (!RegEntry)
            {
                /* This subsystem doesn't exist, so skip it */
                DPRINT1("SMSS: Invalid subsystem name - %ws\n", SubsystemName);
            }
            else
            {
                /* Found it -- remove it from the main list */
                RemoveEntryList(&RegEntry->Entry);

                /* Figure out which list to put it in */
                if (_wcsicmp(ValueName, L"Required") == 0)
                {
                    /* Put it into the required list */
                    DPRINT("Required\n");
                    InsertTailList(&SmpSubSystemsToLoad, &RegEntry->Entry);
                }
                else
                {
                    /* Put it into the optional list */
                    DPRINT("Optional\n");
                    InsertTailList(&SmpSubSystemsToDefer, &RegEntry->Entry);
                }
            }

            /* Move to the next name */
            while (*SubsystemName++);
        }
    }

    /* All done! */
    return STATUS_SUCCESS;
}

RTL_QUERY_REGISTRY_TABLE
SmpRegistryConfigurationTable[] =
{
    {
        SmpConfigureProtectionMode,
        0,
        L"ProtectionMode",
        NULL,
        REG_DWORD,
        NULL,
        0
    },

    {
        SmpConfigureAllowProtectedRenames,
        0, //RTL_QUERY_REGISTRY_DELETE,
        L"AllowProtectedRenames",
        NULL,
        REG_DWORD,
        NULL,
        0
    },

    {
        SmpConfigureObjectDirectories,
        0,
        L"ObjectDirectories",
        NULL,
        REG_MULTI_SZ,
        L"\\Windows\0\\RPC Control\0",
        0
    },

    {
        SmpConfigureMemoryMgmt,
        0,
        L"BootExecute",
        &SmpBootExecuteList,
        REG_MULTI_SZ,
        L"autocheck AutoChk.exe *\0",
        0
    },

    {
        SmpConfigureMemoryMgmt,
        RTL_QUERY_REGISTRY_TOPKEY,
        L"SetupExecute",
        &SmpSetupExecuteList,
        REG_NONE,
        NULL,
        0
    },

    {
        SmpConfigureFileRenames,
        0, //RTL_QUERY_REGISTRY_DELETE,
        L"PendingFileRenameOperations",
        &SmpFileRenameList,
        REG_NONE,
        NULL,
        0
    },

    {
        SmpConfigureFileRenames,
        0, //RTL_QUERY_REGISTRY_DELETE,
        L"PendingFileRenameOperations2",
        &SmpFileRenameList,
        REG_NONE,
        NULL,
        0
    },

    {
        SmpConfigureExcludeKnownDlls,
        0,
        L"ExcludeFromKnownDlls",
        &SmpExcludeKnownDllsList,
        REG_MULTI_SZ,
        L"\0",
        0
    },

    {
        NULL,
        RTL_QUERY_REGISTRY_SUBKEY,
        L"Memory Management",
        NULL,
        REG_NONE,
        NULL,
        0
    },

    {
        SmpConfigureMemoryMgmt,
        0,
        L"PagingFiles",
        &SmpPagingFileList,
        REG_MULTI_SZ,
        L"?:\\pagefile.sys\0",
        0
    },

    {
        SmpConfigureDosDevices,
        RTL_QUERY_REGISTRY_SUBKEY,
        L"DOS Devices",
        &SmpDosDevicesList,
        REG_NONE,
        NULL,
        0
    },

    {
        SmpConfigureKnownDlls,
        RTL_QUERY_REGISTRY_SUBKEY,
        L"KnownDlls",
        &SmpKnownDllsList,
        REG_NONE,
        NULL,
        0
    },

    {
        SmpConfigureEnvironment,
        RTL_QUERY_REGISTRY_SUBKEY,
        L"Environment",
        NULL,
        REG_NONE,
        NULL,
        0
    },

    {
        SmpConfigureSubSystems,
        RTL_QUERY_REGISTRY_SUBKEY,
        L"SubSystems",
        &SmpSubSystemList,
        REG_NONE,
        NULL,
        0
    },

    {
        SmpConfigureSubSystems,
        RTL_QUERY_REGISTRY_NOEXPAND,
        L"Required",
        &SmpSubSystemList,
        REG_MULTI_SZ,
        L"Debug\0Windows\0",
        0
    },

    {
        SmpConfigureSubSystems,
        RTL_QUERY_REGISTRY_NOEXPAND,
        L"Optional",
        &SmpSubSystemList,
        REG_NONE,
        NULL,
        0
    },

    {
        SmpConfigureSubSystems,
        0,
        L"Kmode",
        &SmpSubSystemList,
        REG_NONE,
        NULL,
        0
    },

    {
        SmpConfigureMemoryMgmt,
        RTL_QUERY_REGISTRY_TOPKEY,
        L"Execute",
        &SmpExecuteList,
        REG_NONE,
        NULL,
        0
    },

    {0},
};

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
SmpTranslateSystemPartitionInformation(VOID)
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeString, LinkTarget, SearchString, SystemPartition;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle, LinkHandle;
    CHAR ValueBuffer[512 + sizeof(KEY_VALUE_PARTIAL_INFORMATION)];
    PKEY_VALUE_PARTIAL_INFORMATION PartialInfo = (PVOID)ValueBuffer;
    ULONG Length, Context;
    CHAR DirInfoBuffer[512 + sizeof(OBJECT_DIRECTORY_INFORMATION)];
    POBJECT_DIRECTORY_INFORMATION DirInfo = (PVOID)DirInfoBuffer;
    WCHAR LinkBuffer[MAX_PATH];

    /* Open the setup key */
    RtlInitUnicodeString(&UnicodeString, L"\\Registry\\Machine\\System\\Setup");
    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenKey(&KeyHandle, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SMSS: can't open system setup key for reading: 0x%x\n", Status);
        return;
    }

    /* Query the system partition */
    RtlInitUnicodeString(&UnicodeString, L"SystemPartition");
    Status = NtQueryValueKey(KeyHandle,
                             &UnicodeString,
                             KeyValuePartialInformation,
                             PartialInfo,
                             sizeof(ValueBuffer),
                             &Length);
    NtClose(KeyHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SMSS: can't query SystemPartition value: 0x%x\n", Status);
        return;
    }

    /* Initialize the system partition string string */
    RtlInitUnicodeString(&SystemPartition, (PWCHAR)PartialInfo->Data);

    /* Enumerate the directory looking for the symbolic link string */
    RtlInitUnicodeString(&SearchString, L"SymbolicLink");
    RtlInitEmptyUnicodeString(&LinkTarget, LinkBuffer, sizeof(LinkBuffer));
    Status = NtQueryDirectoryObject(SmpDosDevicesObjectDirectory,
                                    DirInfo,
                                    sizeof(DirInfoBuffer),
                                    TRUE,
                                    TRUE,
                                    &Context,
                                    NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SMSS: can't find drive letter for system partition\n");
        return;
    }

    /* Keep searching until we find it */
    do
    {
        /* Is this it? */
        if ((RtlEqualUnicodeString(&DirInfo->TypeName, &SearchString, TRUE)) &&
            (DirInfo->Name.Length == 2 * sizeof(WCHAR)) &&
            (DirInfo->Name.Buffer[1] == L':'))
        {
            /* Looks like we found it, open the link to get its target */
            InitializeObjectAttributes(&ObjectAttributes,
                                       &DirInfo->Name,
                                       OBJ_CASE_INSENSITIVE,
                                       SmpDosDevicesObjectDirectory,
                                       NULL);
            Status = NtOpenSymbolicLinkObject(&LinkHandle,
                                              SYMBOLIC_LINK_ALL_ACCESS,
                                              &ObjectAttributes);
            if (NT_SUCCESS(Status))
            {
                /* Open worked, query the target now */
                Status = NtQuerySymbolicLinkObject(LinkHandle,
                                                   &LinkTarget,
                                                   NULL);
                NtClose(LinkHandle);

                /* Check if it matches the string we had found earlier */
                if ((NT_SUCCESS(Status)) &&
                    ((RtlEqualUnicodeString(&SystemPartition,
                                           &LinkTarget,
                                           TRUE)) ||
                    ((RtlPrefixUnicodeString(&SystemPartition,
                                             &LinkTarget,
                                             TRUE)) &&
                     (LinkTarget.Buffer[SystemPartition.Length / sizeof(WCHAR)] == L'\\'))))
                {
                    /* All done */
                    break;
                }
            }
        }

        /* Couldn't find it, try again */
        Status = NtQueryDirectoryObject(SmpDosDevicesObjectDirectory,
                                        DirInfo,
                                        sizeof(DirInfoBuffer),
                                        TRUE,
                                        FALSE,
                                        &Context,
                                        NULL);
    } while (NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SMSS: can't find drive letter for system partition\n");
        return;
    }

    /* Open the setup key again, for full access this time */
    RtlInitUnicodeString(&UnicodeString,
                         L"\\Registry\\Machine\\Software\\Microsoft\\Windows\\CurrentVersion\\Setup");
    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenKey(&KeyHandle, KEY_ALL_ACCESS, &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SMSS: can't open software setup key for writing: 0x%x\n",
                Status);
        return;
    }

    /* Wrap up the end of the link buffer */
    wcsncpy(LinkBuffer, DirInfo->Name.Buffer, 2);
    LinkBuffer[2] = L'\\';
    LinkBuffer[3] = L'\0';

    /* Now set this as the "BootDir" */
    RtlInitUnicodeString(&UnicodeString, L"BootDir");
    Status = NtSetValueKey(KeyHandle,
                           &UnicodeString,
                           0,
                           REG_SZ,
                           LinkBuffer,
                           4 * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SMSS: couldn't write BootDir value: 0x%x\n", Status);
    }
    NtClose(KeyHandle);
}

NTSTATUS
NTAPI
SmpCreateSecurityDescriptors(IN BOOLEAN InitialCall)
{
    NTSTATUS Status;
    PSID WorldSid = NULL, AdminSid = NULL, SystemSid = NULL;
    PSID RestrictedSid = NULL, OwnerSid = NULL;
    SID_IDENTIFIER_AUTHORITY WorldAuthority = {SECURITY_WORLD_SID_AUTHORITY};
    SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};
    SID_IDENTIFIER_AUTHORITY CreatorAuthority = {SECURITY_CREATOR_SID_AUTHORITY};
    ULONG AclLength, SidLength;
    PACL Acl;
    PACE_HEADER Ace;
    BOOLEAN ProtectionRequired = FALSE;

    /* Check if this is the first call */
    if (InitialCall)
    {
        /* Create and set the primary descriptor */
        SmpPrimarySecurityDescriptor = &SmpPrimarySDBody;
        Status = RtlCreateSecurityDescriptor(SmpPrimarySecurityDescriptor,
                                             SECURITY_DESCRIPTOR_REVISION);
        ASSERT(NT_SUCCESS(Status));
        Status = RtlSetDaclSecurityDescriptor(SmpPrimarySecurityDescriptor,
                                              TRUE,
                                              NULL,
                                              FALSE);
        ASSERT(NT_SUCCESS(Status));

        /* Create and set the liberal descriptor */
        SmpLiberalSecurityDescriptor = &SmpLiberalSDBody;
        Status = RtlCreateSecurityDescriptor(SmpLiberalSecurityDescriptor,
                                             SECURITY_DESCRIPTOR_REVISION);
        ASSERT(NT_SUCCESS(Status));
        Status = RtlSetDaclSecurityDescriptor(SmpLiberalSecurityDescriptor,
                                              TRUE,
                                              NULL,
                                              FALSE);
        ASSERT(NT_SUCCESS(Status));

        /* Create and set the \KnownDlls descriptor */
        SmpKnownDllsSecurityDescriptor = &SmpKnownDllsSDBody;
        Status = RtlCreateSecurityDescriptor(SmpKnownDllsSecurityDescriptor,
                                             SECURITY_DESCRIPTOR_REVISION);
        ASSERT(NT_SUCCESS(Status));
        Status = RtlSetDaclSecurityDescriptor(SmpKnownDllsSecurityDescriptor,
                                              TRUE,
                                              NULL,
                                              FALSE);
        ASSERT(NT_SUCCESS(Status));

        /* Create and Set the \ApiPort descriptor */
        SmpApiPortSecurityDescriptor = &SmpApiPortSDBody;
        Status = RtlCreateSecurityDescriptor(SmpApiPortSecurityDescriptor,
                                             SECURITY_DESCRIPTOR_REVISION);
        ASSERT(NT_SUCCESS(Status));
        Status = RtlSetDaclSecurityDescriptor(SmpApiPortSecurityDescriptor,
                                              TRUE,
                                              NULL,
                                              FALSE);
        ASSERT(NT_SUCCESS(Status));
    }

    /* Check if protection was requested in the registry (on by default) */
    if (SmpProtectionMode & 1) ProtectionRequired = TRUE;

    /* Exit if there's nothing to do */
    if (!(InitialCall || ProtectionRequired)) return STATUS_SUCCESS;

    /* Build the world SID */
    Status = RtlAllocateAndInitializeSid(&WorldAuthority, 1,
                                         SECURITY_WORLD_RID,
                                         0, 0, 0, 0, 0, 0, 0,
                                         &WorldSid);
    if (!NT_SUCCESS(Status))
    {
        WorldSid = NULL;
        goto Quickie;
    }

    /* Build the admin SID */
    Status = RtlAllocateAndInitializeSid(&NtAuthority, 2,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_ADMINS,
                                         0, 0, 0, 0, 0, 0,
                                         &AdminSid);
    if (!NT_SUCCESS(Status))
    {
        AdminSid = NULL;
        goto Quickie;
    }

    /* Build the owner SID */
    Status = RtlAllocateAndInitializeSid(&CreatorAuthority, 1,
                                         SECURITY_CREATOR_OWNER_RID,
                                         0, 0, 0, 0, 0, 0, 0,
                                         &OwnerSid);
    if (!NT_SUCCESS(Status))
    {
        OwnerSid = NULL;
        goto Quickie;
    }

    /* Build the restricted SID */
    Status = RtlAllocateAndInitializeSid(&NtAuthority, 1,
                                         SECURITY_RESTRICTED_CODE_RID,
                                         0, 0, 0, 0, 0, 0, 0,
                                         &RestrictedSid);
    if (!NT_SUCCESS(Status))
    {
        RestrictedSid = NULL;
        goto Quickie;
    }

    /* Build the system SID */
    Status = RtlAllocateAndInitializeSid(&NtAuthority, 1,
                                         SECURITY_LOCAL_SYSTEM_RID,
                                         0, 0, 0, 0, 0, 0, 0,
                                         &SystemSid);
    if (!NT_SUCCESS(Status))
    {
        SystemSid = NULL;
        goto Quickie;
    }

    /* Now check if we're creating the core descriptors */
    if (!InitialCall)
    {
        /* We're skipping NextAcl so we have to do this here */
        SidLength = RtlLengthSid(WorldSid) + RtlLengthSid(RestrictedSid) + RtlLengthSid(AdminSid);
        SidLength *= 2;
        goto NotInitial;
    }

    /* Allocate an ACL with two ACEs with two SIDs each */
    SidLength = RtlLengthSid(SystemSid) + RtlLengthSid(AdminSid);
    AclLength = sizeof(ACL) + 2 * sizeof(ACCESS_ALLOWED_ACE) + SidLength;
    Acl = RtlAllocateHeap(RtlGetProcessHeap(), 0, AclLength);
    if (!Acl) Status = STATUS_NO_MEMORY;
    if (!NT_SUCCESS(Status)) goto NextAcl;

    /* Now build the ACL and add the two ACEs */
    Status = RtlCreateAcl(Acl, AclLength, ACL_REVISION2);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_ALL, AdminSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_ALL, SystemSid);
    ASSERT(NT_SUCCESS(Status));

    /* Set this as the DACL */
    Status = RtlSetDaclSecurityDescriptor(SmpApiPortSecurityDescriptor,
                                          TRUE,
                                          Acl,
                                          FALSE);
    ASSERT(NT_SUCCESS(Status));

NextAcl:
    /* Allocate an ACL with 6 ACEs, two ACEs per SID */
    SidLength = RtlLengthSid(WorldSid) + RtlLengthSid(RestrictedSid) + RtlLengthSid(AdminSid);
    SidLength *= 2;
    AclLength = sizeof(ACL) + 6 * sizeof(ACCESS_ALLOWED_ACE) + SidLength;
    Acl = RtlAllocateHeap(RtlGetProcessHeap(), 0, AclLength);
    if (!Acl) Status = STATUS_NO_MEMORY;
    if (!NT_SUCCESS(Status)) goto NotInitial;

    /* Now build the ACL and add the six ACEs */
    Status = RtlCreateAcl(Acl, AclLength, ACL_REVISION2);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_EXECUTE, WorldSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_EXECUTE, RestrictedSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_ALL, AdminSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_EXECUTE | GENERIC_READ | GENERIC_WRITE, WorldSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_EXECUTE | GENERIC_READ | GENERIC_WRITE, RestrictedSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_ALL, AdminSid);
    ASSERT(NT_SUCCESS(Status));

    /* Now edit the last three ACEs and make them inheritable */
    Status = RtlGetAce(Acl, 3, (PVOID)&Ace);
    ASSERT(NT_SUCCESS(Status));
    Ace->AceFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE;
    Status = RtlGetAce(Acl, 4, (PVOID)&Ace);
    ASSERT(NT_SUCCESS(Status));
    Ace->AceFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE;
    Status = RtlGetAce(Acl, 5, (PVOID)&Ace);
    ASSERT(NT_SUCCESS(Status));
    Ace->AceFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE;

    /* Set this as the DACL */
    Status = RtlSetDaclSecurityDescriptor(SmpKnownDllsSecurityDescriptor,
                                          TRUE,
                                          Acl,
                                          FALSE);
    ASSERT(NT_SUCCESS(Status));

NotInitial:
    /* The initial ACLs have been created, are we also protecting objects? */
    if (!ProtectionRequired) goto Quickie;

    /* Allocate an ACL with 7 ACEs, two ACEs per SID, and one final owner ACE */
    SidLength += RtlLengthSid(OwnerSid);
    AclLength = sizeof(ACL) + 7 * sizeof (ACCESS_ALLOWED_ACE) + 2 * SidLength;
    Acl = RtlAllocateHeap(RtlGetProcessHeap(), 0, AclLength);
    if (!Acl) Status = STATUS_NO_MEMORY;
    if (!NT_SUCCESS(Status)) goto Quickie;

    /* Build the ACL and add the seven ACEs */
    Status = RtlCreateAcl(Acl, AclLength, ACL_REVISION2);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_EXECUTE | GENERIC_READ, WorldSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_EXECUTE | GENERIC_READ, RestrictedSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_ALL, AdminSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_EXECUTE | GENERIC_READ, WorldSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_EXECUTE | GENERIC_READ, RestrictedSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_ALL, AdminSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_ALL, OwnerSid);
    ASSERT(NT_SUCCESS(Status));

    /* Edit the last 4 ACEs to make then inheritable */
    Status = RtlGetAce(Acl, 3, (PVOID)&Ace);
    ASSERT(NT_SUCCESS(Status));
    Ace->AceFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE;
    Status = RtlGetAce(Acl, 4, (PVOID)&Ace);
    ASSERT(NT_SUCCESS(Status));
    Ace->AceFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE;
    Status = RtlGetAce(Acl, 5, (PVOID)&Ace);
    ASSERT(NT_SUCCESS(Status));
    Ace->AceFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE;
    Status = RtlGetAce(Acl, 6, (PVOID)&Ace);
    ASSERT(NT_SUCCESS(Status));
    Ace->AceFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE;

    /* Set this as the DACL for the primary SD */
    Status = RtlSetDaclSecurityDescriptor(SmpPrimarySecurityDescriptor,
                                          TRUE,
                                          Acl,
                                          FALSE);
    ASSERT(NT_SUCCESS(Status));

    /* Allocate an ACL with 7 ACEs, two ACEs per SID, and one final owner ACE */
    AclLength = sizeof(ACL) + 7 * sizeof (ACCESS_ALLOWED_ACE) + 2 * SidLength;
    Acl = RtlAllocateHeap(RtlGetProcessHeap(), 0, AclLength);
    if (!Acl) Status = STATUS_NO_MEMORY;
    if (!NT_SUCCESS(Status)) goto Quickie;

    /* Build the ACL and add the seven ACEs */
    Status = RtlCreateAcl(Acl, AclLength, ACL_REVISION2);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_EXECUTE | GENERIC_READ | GENERIC_WRITE, WorldSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_EXECUTE | GENERIC_READ | GENERIC_WRITE, RestrictedSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_ALL, AdminSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_EXECUTE | GENERIC_READ | GENERIC_WRITE, WorldSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_EXECUTE | GENERIC_READ | GENERIC_WRITE, RestrictedSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_ALL, AdminSid);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlAddAccessAllowedAce(Acl, ACL_REVISION2, GENERIC_ALL, OwnerSid);
    ASSERT(NT_SUCCESS(Status));

    /* Edit the last 4 ACEs to make then inheritable */
    Status = RtlGetAce(Acl, 3, (PVOID)&Ace);
    ASSERT(NT_SUCCESS(Status));
    Ace->AceFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE;
    Status = RtlGetAce(Acl, 4, (PVOID)&Ace);
    ASSERT(NT_SUCCESS(Status));
    Ace->AceFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE;
    Status = RtlGetAce(Acl, 5, (PVOID)&Ace);
    ASSERT(NT_SUCCESS(Status));
    Ace->AceFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE;
    Status = RtlGetAce(Acl, 6, (PVOID)&Ace);
    ASSERT(NT_SUCCESS(Status));
    Ace->AceFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE;

    /* Now set this as the DACL for the liberal SD */
    Status = RtlSetDaclSecurityDescriptor(SmpLiberalSecurityDescriptor,
                                          TRUE,
                                          Acl,
                                          FALSE);
    ASSERT(NT_SUCCESS(Status));

Quickie:
    /* Cleanup the SIDs */
    if (OwnerSid) RtlFreeHeap(RtlGetProcessHeap(), 0, OwnerSid);
    if (AdminSid) RtlFreeHeap(RtlGetProcessHeap(), 0, AdminSid);
    if (WorldSid) RtlFreeHeap(RtlGetProcessHeap(), 0, WorldSid);
    if (SystemSid) RtlFreeHeap(RtlGetProcessHeap(), 0, SystemSid);
    if (RestrictedSid) RtlFreeHeap(RtlGetProcessHeap(), 0, RestrictedSid);
    return Status;
}

NTSTATUS
NTAPI
SmpInitializeDosDevices(VOID)
{
    NTSTATUS Status;
    PSMP_REGISTRY_VALUE RegEntry;
    SECURITY_DESCRIPTOR_CONTROL OldFlag = 0;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING DestinationString;
    HANDLE DirHandle;
    PLIST_ENTRY NextEntry, Head;

    /* Open the GLOBAL?? directory */
    RtlInitUnicodeString(&DestinationString, L"\\??");
    InitializeObjectAttributes(&ObjectAttributes,
                               &DestinationString,
                               OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_PERMANENT,
                               NULL,
                               NULL);
    Status = NtOpenDirectoryObject(&SmpDosDevicesObjectDirectory,
                                   DIRECTORY_ALL_ACCESS,
                                   &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SMSS: Unable to open %wZ directory - Status == %lx\n",
                &DestinationString, Status);
        return Status;
    }

    /* Loop the DOS devices */
    Head = &SmpDosDevicesList;
    while (!IsListEmpty(Head))
    {
        /* Get the entry and remove it */
        NextEntry = RemoveHeadList(Head);
        RegEntry = CONTAINING_RECORD(NextEntry, SMP_REGISTRY_VALUE, Entry);

        /* Initialize the attributes, and see which descriptor is being used */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &RegEntry->Name,
                                   OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_PERMANENT,
                                   SmpDosDevicesObjectDirectory,
                                   SmpPrimarySecurityDescriptor);
        if (SmpPrimarySecurityDescriptor)
        {
            /* Save the old flag and set it while we create this link */
            OldFlag = SmpPrimarySecurityDescriptor->Control;
            SmpPrimarySecurityDescriptor->Control |= SE_DACL_DEFAULTED;
        }

        /* Create the symbolic link */
        DPRINT("Creating symlink for %wZ to %wZ\n", &RegEntry->Name, &RegEntry->Value);
        Status = NtCreateSymbolicLinkObject(&DirHandle,
                                            SYMBOLIC_LINK_ALL_ACCESS,
                                            &ObjectAttributes,
                                            &RegEntry->Value);
        if (Status == STATUS_OBJECT_NAME_EXISTS)
        {
            /* Make it temporary and get rid of the handle */
            NtMakeTemporaryObject(DirHandle);
            NtClose(DirHandle);

            /* Treat this as success, and see if we got a name back */
            Status = STATUS_SUCCESS;
            if (RegEntry->Value.Length)
            {
                /* Create it now with this name */
                ObjectAttributes.Attributes &= ~OBJ_OPENIF;
                Status = NtCreateSymbolicLinkObject(&DirHandle,
                                                    SYMBOLIC_LINK_ALL_ACCESS,
                                                    &ObjectAttributes,
                                                    &RegEntry->Value);
            }
        }

        /* If we were using a security descriptor, restore the non-defaulted flag */
        if (ObjectAttributes.SecurityDescriptor)
        {
            SmpPrimarySecurityDescriptor->Control = OldFlag;
        }

        /* Print a failure if we failed to create the symbolic link */
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("SMSS: Unable to create %wZ => %wZ symbolic link object - Status == 0x%lx\n",
                    &RegEntry->Name,
                    &RegEntry->Value,
                    Status);
            break;
        }

        /* Close the handle */
        NtClose(DirHandle);

        /* Free this entry */
        if (RegEntry->AnsiValue) RtlFreeHeap(RtlGetProcessHeap(), 0, RegEntry->AnsiValue);
        if (RegEntry->Value.Buffer) RtlFreeHeap(RtlGetProcessHeap(), 0, RegEntry->Value.Buffer);
        RtlFreeHeap(RtlGetProcessHeap(), 0, RegEntry);
    }

    /* Return the status */
    return Status;
}

VOID
NTAPI
SmpProcessModuleImports(IN PVOID Unused,
                        IN PCHAR ImportName)
{
    ULONG Length = 0, Chars;
    WCHAR Buffer[MAX_PATH];
    PWCHAR DllName, DllValue;
    ANSI_STRING ImportString;
    UNICODE_STRING ImportUnicodeString;
    NTSTATUS Status;

    /* Skip NTDLL since it's already always mapped */
    if (!_stricmp(ImportName, "ntdll.dll")) return;

    /* Initialize our strings */
    RtlInitAnsiString(&ImportString, ImportName);
    RtlInitEmptyUnicodeString(&ImportUnicodeString, Buffer, sizeof(Buffer));
    Status = RtlAnsiStringToUnicodeString(&ImportUnicodeString, &ImportString, FALSE);
    if (!NT_SUCCESS(Status)) return;

    /* Loop in case we find a forwarder */
    ImportUnicodeString.MaximumLength = ImportUnicodeString.Length + sizeof(UNICODE_NULL);
    while (Length < ImportUnicodeString.Length)
    {
        if (ImportUnicodeString.Buffer[Length / sizeof(WCHAR)] == L'.') break;
        Length += sizeof(WCHAR);
    }

    /* Break up the values as needed */
    DllValue = ImportUnicodeString.Buffer;
    DllName = &ImportUnicodeString.Buffer[ImportUnicodeString.MaximumLength / sizeof(WCHAR)];
    Chars = Length >> 1;
    wcsncpy(DllName, ImportUnicodeString.Buffer, Chars);
    DllName[Chars] = 0;

    /* Add the DLL to the list */
    SmpSaveRegistryValue(&SmpKnownDllsList, DllName, DllValue, TRUE);
}

NTSTATUS
NTAPI
SmpInitializeKnownDllsInternal(IN PUNICODE_STRING Directory,
                               IN PUNICODE_STRING Path)
{
    HANDLE DirFileHandle, DirHandle, SectionHandle, FileHandle, LinkHandle;
    UNICODE_STRING NtPath, DestinationString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status, Status1;
    PLIST_ENTRY NextEntry;
    PSMP_REGISTRY_VALUE RegEntry;
    ULONG_PTR ErrorParameters[3];
    UNICODE_STRING ErrorResponse;
    IO_STATUS_BLOCK IoStatusBlock;
    SECURITY_DESCRIPTOR_CONTROL OldFlag = 0;
    USHORT ImageCharacteristics;

    /* Initialize to NULL */
    DirFileHandle = NULL;
    DirHandle = NULL;
    NtPath.Buffer = NULL;

    /* Create the \KnownDLLs directory */
    InitializeObjectAttributes(&ObjectAttributes,
                               Directory,
                               OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_PERMANENT,
                               NULL,
                               SmpKnownDllsSecurityDescriptor);
    Status = NtCreateDirectoryObject(&DirHandle,
                                     DIRECTORY_ALL_ACCESS,
                                     &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        /* Handle failure */
        DPRINT1("SMSS: Unable to create %wZ directory - Status == %lx\n",
                Directory, Status);
        return Status;
    }

    /* Convert the path to native format */
    if (!RtlDosPathNameToNtPathName_U(Path->Buffer, &NtPath, NULL, NULL))
    {
        /* Fail if this didn't work */
        DPRINT1("SMSS: Unable to to convert %wZ to an Nt path\n", Path);
        Status = STATUS_OBJECT_NAME_INVALID;
        goto Quickie;
    }

    /* Open the path that was specified, which should be a directory */
    InitializeObjectAttributes(&ObjectAttributes,
                               &NtPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenFile(&DirFileHandle,
                        FILE_LIST_DIRECTORY | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        /* Fail if we couldn't open it */
        DPRINT1("SMSS: Unable to open a handle to the KnownDll directory (%wZ)"
                "- Status == %lx\n",
                Path,
                Status);
        FileHandle = NULL;
        goto Quickie;
    }

    /* Temporarily hack the SD to use a default DACL for this symbolic link */
    if (SmpPrimarySecurityDescriptor)
    {
        OldFlag = SmpPrimarySecurityDescriptor->Control;
        SmpPrimarySecurityDescriptor->Control |= SE_DACL_DEFAULTED;
    }

    /* Create a symbolic link to the directory in the object manager */
    RtlInitUnicodeString(&DestinationString, L"KnownDllPath");
    InitializeObjectAttributes(&ObjectAttributes,
                               &DestinationString,
                               OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_PERMANENT,
                               DirHandle,
                               SmpPrimarySecurityDescriptor);
    Status = NtCreateSymbolicLinkObject(&LinkHandle,
                                        SYMBOLIC_LINK_ALL_ACCESS,
                                        &ObjectAttributes,
                                        Path);

    /* Undo the hack */
    if (SmpPrimarySecurityDescriptor) SmpPrimarySecurityDescriptor->Control = OldFlag;

    /* Check if the symlink was created */
    if (!NT_SUCCESS(Status))
    {
        /* It wasn't, so bail out since the OS needs it to exist */
        DPRINT1("SMSS: Unable to create %wZ symbolic link - Status == %lx\n",
                &DestinationString, Status);
        LinkHandle = NULL;
        goto Quickie;
    }

    /* We created it permanent, we can go ahead and close the handle now */
    Status1 = NtClose(LinkHandle);
    ASSERT(NT_SUCCESS(Status1));

    /* Now loop the known DLLs */
    NextEntry = SmpKnownDllsList.Flink;
    while (NextEntry != &SmpKnownDllsList)
    {
        /* Get the entry and skip it if it's in the exluded list */
        RegEntry = CONTAINING_RECORD(NextEntry, SMP_REGISTRY_VALUE, Entry);
        DPRINT("Processing known DLL: %wZ-%wZ\n", &RegEntry->Name, &RegEntry->Value);
        if ((SmpFindRegistryValue(&SmpExcludeKnownDllsList,
                                  RegEntry->Name.Buffer)) ||
            (SmpFindRegistryValue(&SmpExcludeKnownDllsList,
                                  RegEntry->Value.Buffer)))
        {
            continue;
        }

        /* Open the actual file */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &RegEntry->Value,
                                   OBJ_CASE_INSENSITIVE,
                                   DirFileHandle,
                                   NULL);
        Status = NtOpenFile(&FileHandle,
                            SYNCHRONIZE | FILE_EXECUTE,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            FILE_SHARE_READ | FILE_SHARE_DELETE,
                            FILE_NON_DIRECTORY_FILE |
                            FILE_SYNCHRONOUS_IO_NONALERT);
        if (!NT_SUCCESS(Status)) break;

        /* Checksum it */
        Status = LdrVerifyImageMatchesChecksum((HANDLE)((ULONG_PTR)FileHandle | 1),
                                               SmpProcessModuleImports,
                                               RegEntry,
                                               &ImageCharacteristics);
        if (!NT_SUCCESS(Status))
        {
            /* Checksum failed, so don't even try going further -- kill SMSS */
            RtlInitUnicodeString(&ErrorResponse,
                                 L"Verification of a KnownDLL failed.");
            ErrorParameters[0] = (ULONG)&ErrorResponse;
            ErrorParameters[1] = Status;
            ErrorParameters[2] = (ULONG)&RegEntry->Value;
            SmpTerminate(ErrorParameters, 5, RTL_NUMBER_OF(ErrorParameters));
        }
        else
        if (!(ImageCharacteristics & IMAGE_FILE_DLL))
        {
            /* An invalid known DLL entry will also kill SMSS */
            RtlInitUnicodeString(&ErrorResponse,
                                 L"Non-DLL file included in KnownDLL list.");
            ErrorParameters[0] = (ULONG)&ErrorResponse;
            ErrorParameters[1] = STATUS_INVALID_IMPORT_OF_NON_DLL;
            ErrorParameters[2] = (ULONG)&RegEntry->Value;
            SmpTerminate(ErrorParameters, 5, RTL_NUMBER_OF(ErrorParameters));
        }

        /* Temporarily hack the SD to use a default DACL for this section */
        if (SmpLiberalSecurityDescriptor)
        {
            OldFlag = SmpLiberalSecurityDescriptor->Control;
            SmpLiberalSecurityDescriptor->Control |= SE_DACL_DEFAULTED;
        }

        /* Create the section for this known DLL */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &RegEntry->Value,
                                   OBJ_PERMANENT,
                                   DirHandle,
                                   SmpLiberalSecurityDescriptor)
        Status = NtCreateSection(&SectionHandle,
                                 SECTION_ALL_ACCESS,
                                 &ObjectAttributes,
                                 0,
                                 PAGE_EXECUTE,
                                 SEC_IMAGE,
                                 FileHandle);

        /* Undo the hack */
        if (SmpLiberalSecurityDescriptor) SmpLiberalSecurityDescriptor->Control = OldFlag;

        /* Check if we created the section okay */
        if (NT_SUCCESS(Status))
        {
            /* We can close it now, since it's marked permanent */
            Status1 = NtClose(SectionHandle);
            ASSERT(NT_SUCCESS(Status1));
        }
        else
        {
            /* If we couldn't make it "known", that's fine and keep going */
            DPRINT1("SMSS: CreateSection for KnownDll %wZ failed - Status == %lx\n",
                    &RegEntry->Value, Status);
        }

        /* Close the file since we can move on to the next one */
        Status1 = NtClose(FileHandle);
        ASSERT(NT_SUCCESS(Status1));

        /* Go to the next entry */
        NextEntry = NextEntry->Flink;
    }

Quickie:
    /* Close both handles and free the NT path buffer */
    if (DirHandle)
    {
        Status1 = NtClose(DirHandle);
        ASSERT(NT_SUCCESS(Status1));
    }
    if (DirFileHandle)
    {
        Status1 = NtClose(DirFileHandle);
        ASSERT(NT_SUCCESS(Status1));
    }
    if (NtPath.Buffer) RtlFreeHeap(RtlGetProcessHeap(), 0, NtPath.Buffer);
    return Status;
}

NTSTATUS
NTAPI
SmpInitializeKnownDlls(VOID)
{
    NTSTATUS Status;
    PSMP_REGISTRY_VALUE RegEntry;
    UNICODE_STRING DestinationString;
    PLIST_ENTRY Head, NextEntry;

    /* Call the internal function */
    RtlInitUnicodeString(&DestinationString, L"\\KnownDlls");
    Status = SmpInitializeKnownDllsInternal(&DestinationString, &SmpKnownDllPath);

    /* Wipe out the list regardless of success */
    Head = &SmpKnownDllsList;
    while (!IsListEmpty(Head))
    {
        /* Remove this entry */
        NextEntry = RemoveHeadList(Head);

        /* Free it */
        RegEntry = CONTAINING_RECORD(NextEntry, SMP_REGISTRY_VALUE, Entry);
        RtlFreeHeap(RtlGetProcessHeap(), 0, RegEntry->AnsiValue);
        RtlFreeHeap(RtlGetProcessHeap(), 0, RegEntry->Value.Buffer);
        RtlFreeHeap(RtlGetProcessHeap(), 0, RegEntry);
    }

    /* All done */
    return Status;
}

NTSTATUS
NTAPI
SmpCreateDynamicEnvironmentVariables(VOID)
{
    NTSTATUS Status;
    SYSTEM_BASIC_INFORMATION BasicInfo;
    SYSTEM_PROCESSOR_INFORMATION ProcessorInfo;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING ValueName, DestinationString;
    HANDLE KeyHandle, KeyHandle2;
    ULONG ResultLength;
    PWCHAR ArchName;
    WCHAR ValueBuffer[512], ValueBuffer2[512];
    PKEY_VALUE_PARTIAL_INFORMATION PartialInfo = (PVOID)ValueBuffer;
    PKEY_VALUE_PARTIAL_INFORMATION PartialInfo2 = (PVOID)ValueBuffer2;

    /* Get system basic information -- we'll need the CPU count */
    Status = NtQuerySystemInformation(SystemBasicInformation,
                                      &BasicInfo,
                                      sizeof(BasicInfo),
                                      NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Bail out on failure */
        DPRINT1("SMSS: Unable to query system basic information - %x\n", Status);
        return Status;
    }

    /* Get the processor information, we'll query a bunch of revision info */
    Status = NtQuerySystemInformation(SystemProcessorInformation,
                                      &ProcessorInfo,
                                      sizeof(ProcessorInfo),
                                      NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Bail out on failure */
        DPRINT1("SMSS: Unable to query system processor information - %x\n", Status);
        return Status;
    }

    /* We'll be writing all these environment variables over here */
    RtlInitUnicodeString(&DestinationString,
                         L"\\Registry\\Machine\\System\\CurrentControlSet\\"
                         L"Control\\Session Manager\\Environment");
    InitializeObjectAttributes(&ObjectAttributes,
                               &DestinationString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenKey(&KeyHandle, GENERIC_WRITE, &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        /* Bail out on failure */
        DPRINT1("SMSS: Unable to open %wZ - %x\n", &DestinationString, Status);
        return Status;
    }

    /* First let's write the OS variable */
    RtlInitUnicodeString(&ValueName, L"OS");
    DPRINT("Setting %wZ to %S\n", &ValueName, L"Windows_NT");
    Status = NtSetValueKey(KeyHandle,
                           &ValueName,
                           0,
                           REG_SZ,
                           L"Windows_NT",
                           wcslen(L"Windows_NT") * sizeof(WCHAR) + sizeof(UNICODE_NULL));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SMSS: Failed writing %wZ environment variable - %x\n",
                &ValueName, Status);
        NtClose(KeyHandle);
        return Status;
    }

    /* Next, let's write the CPU architecture variable */
    RtlInitUnicodeString(&ValueName, L"PROCESSOR_ARCHITECTURE");
    switch (ProcessorInfo.ProcessorArchitecture)
    {
        /* Pick the correct string that matches the architecture */
        case PROCESSOR_ARCHITECTURE_INTEL:
            ArchName = L"x86";
            break;

        case PROCESSOR_ARCHITECTURE_AMD64:
            ArchName = L"AMD64";
            break;

        case PROCESSOR_ARCHITECTURE_IA64:
            ArchName = L"IA64";
            break;

        default:
            ArchName = L"Unknown";
            break;
    }

    /* Set it */
    DPRINT("Setting %wZ to %S\n", &ValueName, ArchName);
    Status = NtSetValueKey(KeyHandle,
                           &ValueName,
                           0,
                           REG_SZ,
                           ArchName,
                           wcslen(ArchName) * sizeof(WCHAR) + sizeof(UNICODE_NULL));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SMSS: Failed writing %wZ environment variable - %x\n",
                &ValueName, Status);
        NtClose(KeyHandle);
        return Status;
    }

    /* And now let's write the processor level */
    RtlInitUnicodeString(&ValueName, L"PROCESSOR_LEVEL");
    swprintf(ValueBuffer, L"%u", ProcessorInfo.ProcessorLevel);
    DPRINT("Setting %wZ to %S\n", &ValueName, ValueBuffer);
    Status = NtSetValueKey(KeyHandle,
                           &ValueName,
                           0,
                           REG_SZ,
                           ValueBuffer,
                           wcslen(ValueBuffer) * sizeof(WCHAR) + sizeof(UNICODE_NULL));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SMSS: Failed writing %wZ environment variable - %x\n",
                &ValueName, Status);
        NtClose(KeyHandle);
        return Status;
    }

    /* Now open the hardware CPU key */
    RtlInitUnicodeString(&DestinationString,
                         L"\\Registry\\Machine\\Hardware\\Description\\System\\"
                         L"CentralProcessor\\0");
    InitializeObjectAttributes(&ObjectAttributes,
                               &DestinationString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenKey(&KeyHandle2, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SMSS: Unable to open %wZ - %x\n", &DestinationString, Status);
        NtClose(KeyHandle);
        return Status;
    }

    /* So that we can read the identifier out of it... */
    RtlInitUnicodeString(&ValueName, L"Identifier");
    Status = NtQueryValueKey(KeyHandle2,
                             &ValueName,
                             KeyValuePartialInformation,
                             PartialInfo,
                             sizeof(ValueBuffer),
                             &ResultLength);
    if (!NT_SUCCESS(Status))
    {
        NtClose(KeyHandle2);
        NtClose(KeyHandle);
        DPRINT1("SMSS: Unable to read %wZ\\%wZ - %x\n",
                &DestinationString, &ValueName, Status);
        return Status;
    }

    /* As well as the vendor... */
    RtlInitUnicodeString(&ValueName, L"VendorIdentifier");
    Status = NtQueryValueKey(KeyHandle2,
                             &ValueName,
                             KeyValuePartialInformation,
                             PartialInfo2,
                             sizeof(ValueBuffer2),
                             &ResultLength);
    NtClose(KeyHandle2);
    if (NT_SUCCESS(Status))
    {
        /* To combine it into a single string */
        swprintf((PWCHAR)PartialInfo->Data + wcslen((PWCHAR)PartialInfo->Data),
                 L", %S",
                 PartialInfo2->Data);
    }

    /* So that we can set this as the PROCESSOR_IDENTIFIER variable */
    RtlInitUnicodeString(&ValueName, L"PROCESSOR_IDENTIFIER");
    DPRINT("Setting %wZ to %s\n", &ValueName, PartialInfo->Data);
    Status = NtSetValueKey(KeyHandle,
                           &ValueName,
                           0,
                           REG_SZ,
                           PartialInfo->Data,
                           wcslen((PWCHAR)PartialInfo->Data) * sizeof(WCHAR) + sizeof(UNICODE_NULL));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SMSS: Failed writing %wZ environment variable - %x\n",
                &ValueName, Status);
        NtClose(KeyHandle);
        return Status;
    }

    /* Now let's get the processor architecture */
    RtlInitUnicodeString(&ValueName, L"PROCESSOR_REVISION");
    switch (ProcessorInfo.ProcessorArchitecture)
    {
        /* Check if this is an older Intel CPU */
        case PROCESSOR_ARCHITECTURE_INTEL:
            if ((ProcessorInfo.ProcessorRevision >> 8) == 0xFF)
            {
                /* These guys used a revision + stepping, so get the rev only */
                swprintf(ValueBuffer, L"%02x", ProcessorInfo.ProcessorRevision & 0xFF);
                _wcsupr(ValueBuffer);
                break;
            }

        /* Modern Intel, as well as 64-bit CPUs use a revision without stepping */
        case PROCESSOR_ARCHITECTURE_IA64:
        case PROCESSOR_ARCHITECTURE_AMD64:
            swprintf(ValueBuffer, L"%04x", ProcessorInfo.ProcessorRevision);
            break;

        /* And anything else we'll just read the whole revision identifier */
        default:
            swprintf(ValueBuffer, L"%u", ProcessorInfo.ProcessorRevision);
            break;
    }

    /* Write the revision to the registry */
    DPRINT("Setting %wZ to %S\n", &ValueName, ValueBuffer);
    Status = NtSetValueKey(KeyHandle,
                           &ValueName,
                           0,
                           REG_SZ,
                           ValueBuffer,
                           wcslen(ValueBuffer) * sizeof(WCHAR) + sizeof(UNICODE_NULL));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SMSS: Failed writing %wZ environment variable - %x\n",
                &ValueName, Status);
        NtClose(KeyHandle);
        return Status;
    }

    /* And finally, write the number of CPUs */
    RtlInitUnicodeString(&ValueName, L"NUMBER_OF_PROCESSORS");
    swprintf(ValueBuffer, L"%d", BasicInfo.NumberOfProcessors);
    DPRINT("Setting %wZ to %S\n", &ValueName, ValueBuffer);
    Status = NtSetValueKey(KeyHandle,
                           &ValueName,
                           0,
                           REG_SZ,
                           ValueBuffer,
                           wcslen(ValueBuffer) * sizeof(WCHAR) + sizeof(UNICODE_NULL));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SMSS: Failed writing %wZ environment variable - %x\n",
                &ValueName, Status);
        NtClose(KeyHandle);
        return Status;
    }

    /* Now we need to write the safeboot option key in a different format */
    RtlInitUnicodeString(&DestinationString,
                         L"\\Registry\\Machine\\System\\CurrentControlSet\\"
                         L"Control\\Safeboot\\Option");
    InitializeObjectAttributes(&ObjectAttributes,
                               &DestinationString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenKey(&KeyHandle2, KEY_ALL_ACCESS, &ObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        /* This was indeed a safeboot, so check what kind of safeboot it was */
        RtlInitUnicodeString(&ValueName, L"OptionValue");
        Status = NtQueryValueKey(KeyHandle2,
                                 &ValueName,
                                 KeyValuePartialInformation,
                                 PartialInfo,
                                 sizeof(ValueBuffer),
                                 &ResultLength);
        NtClose(KeyHandle2);
        if (NT_SUCCESS(Status))
        {
            /* Convert from the integer value to the correct specifier */
            RtlInitUnicodeString(&ValueName, L"SAFEBOOT_OPTION");
            switch (*(PULONG)PartialInfo->Data)
            {
                case 1:
                    wcscpy(ValueBuffer, L"MINIMAL");
                    break;
                case 2:
                    wcscpy(ValueBuffer, L"NETWORK");
                    break;
                case 3:
                    wcscpy(ValueBuffer, L"DSREPAIR");
                    break;
            }

            /* And write it in the environment! */
            DPRINT("Setting %wZ to %S\n", &ValueName, ValueBuffer);
            Status = NtSetValueKey(KeyHandle,
                                   &ValueName,
                                   0,
                                   REG_SZ,
                                   ValueBuffer,
                                   wcslen(ValueBuffer) * sizeof(WCHAR) + sizeof(UNICODE_NULL));
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("SMSS: Failed writing %wZ environment variable - %x\n",
                        &ValueName, Status);
                NtClose(KeyHandle);
                return Status;
            }
        }
        else
        {
            DPRINT1("SMSS: Failed querying safeboot option = %x\n", Status);
        }
    }

    /* We are all done now */
    NtClose(KeyHandle);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
SmpProcessFileRenames(VOID)
{
    BOOLEAN OldState, HavePrivilege = FALSE;
    NTSTATUS Status;
    HANDLE FileHandle, OtherFileHandle;
    FILE_INFORMATION_CLASS InformationClass;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING FileString;
    FILE_BASIC_INFORMATION BasicInfo;
    FILE_DISPOSITION_INFORMATION DeleteInformation;
    PFILE_RENAME_INFORMATION Buffer;
    PLIST_ENTRY Head, NextEntry;
    PSMP_REGISTRY_VALUE RegEntry;
    PWCHAR FileName;
    ULONG ValueLength, Length;

    /* Give us access to restore any files we want */
    Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &OldState);
    if (NT_SUCCESS(Status)) HavePrivilege = TRUE;

    /* Process pending files to rename */
    Head = &SmpFileRenameList;
    while (!IsListEmpty(Head))
    {
        /* Get this entry */
        NextEntry = RemoveHeadList(Head);
        RegEntry = CONTAINING_RECORD(NextEntry, SMP_REGISTRY_VALUE, Entry);
        DPRINT1("Processing PFRO: '%wZ' / '%wZ'\n", &RegEntry->Value, &RegEntry->Name);

        /* Skip past the '@' marker */
        if (!(RegEntry->Value.Length) && (*RegEntry->Name.Buffer == L'@'))
        {
            RegEntry->Name.Length -= sizeof(UNICODE_NULL);
            RegEntry->Name.Buffer++;
        }

        /* Open the file for delete access */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &RegEntry->Name,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);
        Status = NtOpenFile(&OtherFileHandle,
                            DELETE | SYNCHRONIZE,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            FILE_SYNCHRONOUS_IO_NONALERT);
        if (!NT_SUCCESS(Status)) goto Quickie;

        /* Check if it's a rename or just a delete */
        ValueLength = RegEntry->Value.Length;
        if (!ValueLength)
        {
            /* Just a delete, set up the class, length and buffer */
            InformationClass = FileDispositionInformation;
            Length = sizeof(DeleteInformation);
            Buffer = (PFILE_RENAME_INFORMATION)&DeleteInformation;

            /* Set the delete disposition */
            DeleteInformation.DeleteFile = TRUE;
        }
        else
        {
            /* This is a rename, setup the class and length */
            InformationClass = FileRenameInformation;
            Length = ValueLength + sizeof(FILE_RENAME_INFORMATION);

            /* Skip past the special markers */
            FileName = RegEntry->Value.Buffer;
            if ((*FileName == L'!') || (*FileName == L'@'))
            {
                FileName++;
                Length -= sizeof(UNICODE_NULL);
            }

            /* Now allocate the buffer for the rename information */
            Buffer = RtlAllocateHeap(RtlGetProcessHeap(), SmBaseTag, Length);
            if (Buffer)
            {
                /* Setup the buffer to point to the filename, and copy it */
                Buffer->RootDirectory = NULL;
                Buffer->FileNameLength = Length - sizeof(FILE_RENAME_INFORMATION);
                Buffer->ReplaceIfExists = FileName != RegEntry->Value.Buffer;
                RtlCopyMemory(Buffer->FileName, FileName, Buffer->FileNameLength);
            }
            else
            {
                /* Fail */
                Status = STATUS_NO_MEMORY;
            }
        }

        /* Check if everything is okay till here */
        if (NT_SUCCESS(Status))
        {
            /* Now either rename or delete the file as requested */
            Status = NtSetInformationFile(OtherFileHandle,
                                          &IoStatusBlock,
                                          Buffer,
                                          Length,
                                          InformationClass);

            /* Check if we seem to have failed because the file was readonly */
            if ((!NT_SUCCESS(Status) &&
                (InformationClass == FileRenameInformation) &&
                (Status == STATUS_OBJECT_NAME_COLLISION) &&
                (Buffer->ReplaceIfExists)))
            {
                /* Open the file for write attribute access this time... */
                DPRINT1("\nSMSS: '%wZ' => '%wZ' failed - Status == %x, Possible readonly target\n",
                        &RegEntry->Name,
                        &RegEntry->Value,
                        STATUS_OBJECT_NAME_COLLISION);
                FileString.Length = RegEntry->Value.Length - sizeof(WCHAR);
                FileString.MaximumLength = RegEntry->Value.MaximumLength - sizeof(WCHAR);
                FileString.Buffer = FileName;
                InitializeObjectAttributes(&ObjectAttributes,
                                           &FileString,
                                           OBJ_CASE_INSENSITIVE,
                                           NULL,
                                           NULL);
                Status = NtOpenFile(&FileHandle,
                                    FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                                    &ObjectAttributes,
                                    &IoStatusBlock,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    FILE_SYNCHRONOUS_IO_NONALERT);
                if (!NT_SUCCESS(Status))
                {
                    /* That didn't work, so bail out */
                    DPRINT1("     SMSS: Open Existing file Failed - Status == %x\n",
                            Status);
                }
                else
                {
                    /* Now remove the read-only attribute from the file */
                    DPRINT1("     SMSS: Open Existing Success\n");
                    RtlZeroMemory(&BasicInfo, sizeof(BasicInfo));
                    BasicInfo.FileAttributes = FILE_ATTRIBUTE_NORMAL;
                    Status = NtSetInformationFile(FileHandle,
                                                  &IoStatusBlock,
                                                  &BasicInfo,
                                                  sizeof(BasicInfo),
                                                  FileBasicInformation);
                    NtClose(FileHandle);
                    if (!NT_SUCCESS(Status))
                    {
                        /* That didn't work, bail out */
                        DPRINT1("     SMSS: Set To NORMAL Failed - Status == %x\n",
                                Status);
                    }
                    else
                    {
                        /* Now that the file is no longer read-only, delete! */
                        DPRINT1("     SMSS: Set To NORMAL OK\n");
                        Status = NtSetInformationFile(OtherFileHandle,
                                                      &IoStatusBlock,
                                                      Buffer,
                                                      Length,
                                                      FileRenameInformation);
                        if (!NT_SUCCESS(Status))
                        {
                            /* That failed too! */
                            DPRINT1("     SMSS: Re-Rename Failed - Status == %x\n",
                                    Status);
                        }
                        else
                        {
                            /* Everything ok */
                            DPRINT1("     SMSS: Re-Rename Worked OK\n");
                        }
                    }
                }
            }
        }

        /* Close the file handle and check the operation result */
        NtClose(OtherFileHandle);
Quickie:
        if (!NT_SUCCESS(Status))
        {
            /* We totally failed */
            DPRINT1("SMSS: '%wZ' => '%wZ' failed - Status == %x\n",
                    &RegEntry->Name, &RegEntry->Value, Status);
        }
        else if (RegEntry->Value.Length)
        {
            /* We succeed with a rename */
            DPRINT1("SMSS: '%wZ' (renamed to) '%wZ'\n", &RegEntry->Name, &RegEntry->Value);
        }
        else
        {
            /* We suceeded with a delete */
            DPRINT1("SMSS: '%wZ' (deleted)\n", &RegEntry->Name);
        }

        /* Now free this entry and keep going */
        if (RegEntry->AnsiValue) RtlFreeHeap(RtlGetProcessHeap(), 0, RegEntry->AnsiValue);
        if (RegEntry->Value.Buffer) RtlFreeHeap(RtlGetProcessHeap(), 0, RegEntry->Value.Buffer);
        RtlFreeHeap(RtlGetProcessHeap(), 0, RegEntry);
    }

    /* Put back the restore privilege if we had requested it, and return */
    if (HavePrivilege) RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, FALSE, FALSE, &OldState);
    return Status;
}

NTSTATUS
NTAPI
SmpLoadDataFromRegistry(OUT PUNICODE_STRING InitialCommand)
{
    NTSTATUS Status;
    PLIST_ENTRY Head, NextEntry;
    PSMP_REGISTRY_VALUE RegEntry;
    PVOID OriginalEnvironment;
    ULONG MuSessionId = 0;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;
    UNICODE_STRING DestinationString;

    /* Initialize the keywords we'll be looking for */
    RtlInitUnicodeString(&SmpDebugKeyword, L"debug");
    RtlInitUnicodeString(&SmpASyncKeyword, L"async");
    RtlInitUnicodeString(&SmpAutoChkKeyword, L"autocheck");

    /* Initialize all the registry-associated list heads */
    InitializeListHead(&SmpBootExecuteList);
    InitializeListHead(&SmpSetupExecuteList);
    InitializeListHead(&SmpPagingFileList);
    InitializeListHead(&SmpDosDevicesList);
    InitializeListHead(&SmpFileRenameList);
    InitializeListHead(&SmpKnownDllsList);
    InitializeListHead(&SmpExcludeKnownDllsList);
    InitializeListHead(&SmpSubSystemList);
    InitializeListHead(&SmpSubSystemsToLoad);
    InitializeListHead(&SmpSubSystemsToDefer);
    InitializeListHead(&SmpExecuteList);
    SmpPagingFileInitialize();

    /* Initialize the SMSS environment */
    Status = RtlCreateEnvironment(TRUE, &SmpDefaultEnvironment);
    if (!NT_SUCCESS(Status))
    {
        /* Fail if there was a problem */
        DPRINT1("SMSS: Unable to allocate default environment - Status == %X\n",
                Status);
        SMSS_CHECKPOINT(RtlCreateEnvironment, Status);
        return Status;
    }

    /* Check if we were booted in PE mode (LiveCD should have this) */
    RtlInitUnicodeString(&DestinationString,
                         L"\\Registry\\Machine\\System\\CurrentControlSet\\"
                         L"Control\\MiniNT");
    InitializeObjectAttributes(&ObjectAttributes,
                               &DestinationString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenKey(&KeyHandle, KEY_ALL_ACCESS, &ObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        /* If the key exists, we were */
        NtClose(KeyHandle);
        MiniNTBoot = TRUE;
    }

    /* Print out if this is the case */
    if (MiniNTBoot) DPRINT1("SMSS: !!! MiniNT Boot !!!\n");

    /* Open the environment key to see if we are booted in safe mode */
    RtlInitUnicodeString(&DestinationString,
                         L"\\Registry\\Machine\\System\\CurrentControlSet\\"
                         L"Control\\Session Manager\\Environment");
    InitializeObjectAttributes(&ObjectAttributes,
                               &DestinationString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenKey(&KeyHandle, KEY_ALL_ACCESS, &ObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        /* Delete the value if we found it */
        RtlInitUnicodeString(&DestinationString, L"SAFEBOOT_OPTION");
        NtDeleteValueKey(KeyHandle, &DestinationString);
        NtClose(KeyHandle);
    }

    /* Switch environments, then query the registry for all needed settings */
    OriginalEnvironment = NtCurrentPeb()->ProcessParameters->Environment;
    NtCurrentPeb()->ProcessParameters->Environment = SmpDefaultEnvironment;
    Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
                                    L"Session Manager",
                                    SmpRegistryConfigurationTable,
                                    NULL,
                                    NULL);
    SmpDefaultEnvironment = NtCurrentPeb()->ProcessParameters->Environment;
    NtCurrentPeb()->ProcessParameters->Environment = OriginalEnvironment;
    if (!NT_SUCCESS(Status))
    {
        /* We failed somewhere in registry initialization, which is bad... */
        DPRINT1("SMSS: RtlQueryRegistryValues failed - Status == %lx\n", Status);
        SMSS_CHECKPOINT(RtlQueryRegistryValues, Status);
        return Status;
    }

    /* Now we can start acting on the registry settings. First to DOS devices */
    Status = SmpInitializeDosDevices();
    if (!NT_SUCCESS(Status))
    {
        /* Failed */
        DPRINT1("SMSS: Unable to initialize DosDevices configuration - Status == %lx\n",
                Status);
        SMSS_CHECKPOINT(SmpInitializeDosDevices, Status);
        return Status;
    }

    /* Next create the session directory... */
    RtlInitUnicodeString(&DestinationString, L"\\Sessions");
    InitializeObjectAttributes(&ObjectAttributes,
                               &DestinationString,
                               OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_PERMANENT,
                               NULL,
                               SmpPrimarySecurityDescriptor);
    Status = NtCreateDirectoryObject(&SmpSessionsObjectDirectory,
                                     DIRECTORY_ALL_ACCESS,
                                     &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        DPRINT1("SMSS: Unable to create %wZ object directory - Status == %lx\n",
                &DestinationString, Status);
        SMSS_CHECKPOINT(NtCreateDirectoryObject, Status);
        return Status;
    }

    /* Next loop all the boot execute binaries */
    Head = &SmpBootExecuteList;
    while (!IsListEmpty(Head))
    {
        /* Remove each one from the list */
        NextEntry = RemoveHeadList(Head);

        /* Execute it */
        RegEntry = CONTAINING_RECORD(NextEntry, SMP_REGISTRY_VALUE, Entry);
        SmpExecuteCommand(&RegEntry->Name, 0, NULL, 0);

        /* And free it */
        if (RegEntry->AnsiValue) RtlFreeHeap(RtlGetProcessHeap(), 0, RegEntry->AnsiValue);
        if (RegEntry->Value.Buffer) RtlFreeHeap(RtlGetProcessHeap(), 0, RegEntry->Value.Buffer);
        RtlFreeHeap(RtlGetProcessHeap(), 0, RegEntry);
    }

    /* Now do any pending file rename operations... */
    if (!MiniNTBoot) SmpProcessFileRenames();

    /* And initialize known DLLs... */
    Status = SmpInitializeKnownDlls();
    if (!NT_SUCCESS(Status))
    {
        /* Fail if that didn't work */
        DPRINT1("SMSS: Unable to initialize KnownDll configuration - Status == %lx\n",
                Status);
        SMSS_CHECKPOINT(SmpInitializeKnownDlls, Status);
        return Status;
    }

    /* Loop every page file */
    Head = &SmpPagingFileList;
    while (!IsListEmpty(Head))
    {
        /* Remove each one from the list */
        NextEntry = RemoveHeadList(Head);

        /* Create the descriptor for it */
        RegEntry = CONTAINING_RECORD(NextEntry, SMP_REGISTRY_VALUE, Entry);
        SmpCreatePagingFileDescriptor(&RegEntry->Name);

        /* And free it */
        if (RegEntry->AnsiValue) RtlFreeHeap(RtlGetProcessHeap(), 0, RegEntry->AnsiValue);
        if (RegEntry->Value.Buffer) RtlFreeHeap(RtlGetProcessHeap(), 0, RegEntry->Value.Buffer);
        RtlFreeHeap(RtlGetProcessHeap(), 0, RegEntry);
    }

    /* Now create all the paging files for the descriptors that we have */
    SmpCreatePagingFiles();

    /* Tell Cm it's now safe to fully enable write access to the registry */
    NtInitializeRegistry(CM_BOOT_FLAG_SMSS);

    /* Create all the system-based environment variables for later inheriting */
    Status = SmpCreateDynamicEnvironmentVariables();
    if (!NT_SUCCESS(Status))
    {
        /* Handle failure */
        SMSS_CHECKPOINT(SmpCreateDynamicEnvironmentVariables, Status);
        return Status;
    }

    /* And finally load all the subsytems for our first session! */
    Status = SmpLoadSubSystemsForMuSession(&MuSessionId,
                                           &SmpWindowsSubSysProcessId,
                                           InitialCommand);
    ASSERT(MuSessionId == 0);
    if (!NT_SUCCESS(Status)) SMSS_CHECKPOINT(SmpLoadSubSystemsForMuSession, Status);
    return Status;
}

NTSTATUS
NTAPI
SmpInit(IN PUNICODE_STRING InitialCommand,
        OUT PHANDLE ProcessHandle)
{
    NTSTATUS Status, Status2;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING PortName, EventName;
    HANDLE EventHandle, PortHandle;
    ULONG HardErrorMode;

    /* Create the SMSS Heap */
    SmBaseTag = RtlCreateTagHeap(RtlGetProcessHeap(),
                                 0,
                                 L"SMSS!",
                                 L"INIT");
    SmpHeap = RtlGetProcessHeap();

    /* Enable hard errors */
    HardErrorMode = TRUE;
    NtSetInformationProcess(NtCurrentProcess(),
                            ProcessDefaultHardErrorMode,
                            &HardErrorMode,
                            sizeof(HardErrorMode));

    /* Initialize the subsystem list and the session list, plus their locks */
    RtlInitializeCriticalSection(&SmpKnownSubSysLock);
    InitializeListHead(&SmpKnownSubSysHead);
    RtlInitializeCriticalSection(&SmpSessionListLock);
    InitializeListHead(&SmpSessionListHead);

    /* Initialize the process list */
    InitializeListHead(&NativeProcessList);

    /* Initialize session parameters */
    SmpNextSessionId = 1;
    SmpNextSessionIdScanMode = 0;
    SmpDbgSsLoaded = FALSE;

    /* Create the initial security descriptors */
    Status = SmpCreateSecurityDescriptors(TRUE);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        SMSS_CHECKPOINT(SmpCreateSecurityDescriptors, Status);
        return Status;
    }

    /* Initialize subsystem names */
    RtlInitUnicodeString(&SmpSubsystemName, L"NT-Session Manager");
    RtlInitUnicodeString(&PosixName, L"POSIX");
    RtlInitUnicodeString(&Os2Name, L"OS2");

    /* Create the SM API Port */
    RtlInitUnicodeString(&PortName, L"\\SmApiPort");
    InitializeObjectAttributes(&ObjectAttributes, &PortName, 0, NULL, NULL);
    Status = NtCreatePort(&PortHandle,
                          &ObjectAttributes,
                          sizeof(SB_CONNECTION_INFO),
                          sizeof(SM_API_MSG),
                          sizeof(SB_API_MSG) * 32);
    ASSERT(NT_SUCCESS(Status));
    SmpDebugPort = PortHandle;

    /* Create two SM API threads */
    Status = RtlCreateUserThread(NtCurrentProcess(),
                                 NULL,
                                 FALSE,
                                 0,
                                 0,
                                 0,
                                 SmpApiLoop,
                                 PortHandle,
                                 NULL,
                                 NULL);
    ASSERT(NT_SUCCESS(Status));
    Status = RtlCreateUserThread(NtCurrentProcess(),
                                 NULL,
                                 FALSE,
                                 0,
                                 0,
                                 0,
                                 SmpApiLoop,
                                 PortHandle,
                                 NULL,
                                 NULL);
    ASSERT(NT_SUCCESS(Status));

    /* Create the write event that autochk can set after running */
    RtlInitUnicodeString(&EventName, L"\\Device\\VolumesSafeForWriteAccess");
    InitializeObjectAttributes(&ObjectAttributes,
                               &EventName,
                               OBJ_PERMANENT,
                               NULL,
                               NULL);
    Status2 = NtCreateEvent(&EventHandle,
                            EVENT_ALL_ACCESS,
                            &ObjectAttributes,
                            0,
                            0);
    if (!NT_SUCCESS(Status2))
    {
        /* Should never really fail */
        DPRINT1("SMSS: Unable to create %wZ event - Status == %lx\n",
                &EventName, Status2);
        ASSERT(NT_SUCCESS(Status2));
    }

    /* Now initialize everything else based on the registry parameters */
    Status = SmpLoadDataFromRegistry(InitialCommand);
    if (NT_SUCCESS(Status))
    {
        /* Autochk should've run now. Set the event and save the CSRSS handle */
        *ProcessHandle = SmpWindowsSubSysProcess;
        NtSetEvent(EventHandle, 0);
        NtClose(EventHandle);
    }

    /* All done */
    return Status;
}
