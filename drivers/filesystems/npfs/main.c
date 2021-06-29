/*
 * PROJECT:     ReactOS Named Pipe FileSystem
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/filesystems/npfs/main.c
 * PURPOSE:     Named Pipe FileSystem Driver Initialization
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "npfs.h"

// File ID number for NPFS bugchecking support
#define NPFS_BUGCHECK_FILE_ID   (NPFS_BUGCHECK_MAIN)

/* GLOBALS ********************************************************************/

PDEVICE_OBJECT NpfsDeviceObject;
PVOID NpAliases;
PNPFS_ALIAS NpAliasList;
PNPFS_ALIAS NpAliasListByLength[MAX_INDEXED_LENGTH + 1 - MIN_INDEXED_LENGTH];

FAST_IO_DISPATCH NpFastIoDispatch =
{
    sizeof(FAST_IO_DISPATCH),
    NULL,
    NpFastRead,
    NpFastWrite,
};

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
NpReadAlias(
    PWSTR ValueName,
    ULONG ValueType,
    PVOID ValueData,
    ULONG ValueLength,
    PVOID Context,
    PVOID EntryContext)
{
    PNPFS_QUERY_VALUE_CONTEXT QueryContext = Context;
    PWSTR CurrentString;
    SIZE_T Length;
    PNPFS_ALIAS CurrentAlias;
    UNICODE_STRING TempString;
    PUNICODE_STRING CurrentTargetName;

    /* Check if we have the expected type */
    if (ValueType != REG_MULTI_SZ)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if only the size is requested */
    if (QueryContext->SizeOnly)
    {
        /* Count this entry */
        QueryContext->NumberOfEntries++;

        /* Get the length of the value name (i.e. the target name). */
        Length = wcslen(ValueName) * sizeof(WCHAR);

        /* Add the size of the name plus a '\' and a UNICODE_STRING structure */
        QueryContext->FullSize += Length + sizeof(UNICODE_NULL) +
                                  sizeof(OBJ_NAME_PATH_SEPARATOR) +
                                  sizeof(UNICODE_STRING);

        /* Loop while we have alias names */
        CurrentString = ValueData;
        while (*CurrentString != UNICODE_NULL)
        {
            /* Count this alias */
            QueryContext->NumberOfAliases++;

            /* Get the length of the current string (i.e. the alias name) */
            Length = wcslen(CurrentString) * sizeof(WCHAR);

            /* Count the length plus the size of an NPFS_ALIAS structure */
            QueryContext->FullSize += Length + sizeof(UNICODE_NULL) + sizeof(NPFS_ALIAS);

            /* Go to the next string */
            CurrentString += (Length + sizeof(UNICODE_NULL)) / sizeof(WCHAR);
        }
    }
    else
    {
        /* Get the next name string pointer */
        CurrentTargetName = QueryContext->CurrentTargetName++;

        /* Get the length of the value name (i.e. the target name). */
        Length = wcslen(ValueName) * sizeof(WCHAR);

        /* Initialize the current name string (one char more than the name) */
        CurrentTargetName->Buffer = QueryContext->CurrentStringPointer;
        CurrentTargetName->Length = Length + sizeof(OBJ_NAME_PATH_SEPARATOR);
        CurrentTargetName->MaximumLength = CurrentTargetName->Length + sizeof(UNICODE_NULL);

        /* Update the current string pointer */
        QueryContext->CurrentStringPointer +=
            CurrentTargetName->MaximumLength / sizeof(WCHAR);

        /* Prepend a '\' before the name */
        CurrentTargetName->Buffer[0] = OBJ_NAME_PATH_SEPARATOR;

        /* Append the value name (including the NULL termination) */
        RtlCopyMemory(&CurrentTargetName->Buffer[1],
                      ValueName,
                      Length + sizeof(UNICODE_NULL));

        /* Upcase the target name */
        RtlUpcaseUnicodeString(CurrentTargetName, CurrentTargetName, 0);

        /* Loop while we have alias names */
        CurrentString = ValueData;
        while (*CurrentString != UNICODE_NULL)
        {
            /* Get the next alias pointer */
            CurrentAlias = QueryContext->CurrentAlias++;

            /* Get the length of the current string (i.e. the alias name) */
            Length = wcslen(CurrentString) * sizeof(WCHAR);

            /* Setup the alias structure */
            CurrentAlias->TargetName = CurrentTargetName;
            CurrentAlias->Name.Buffer = QueryContext->CurrentStringPointer;
            CurrentAlias->Name.Length = Length;
            CurrentAlias->Name.MaximumLength = Length + sizeof(UNICODE_NULL);

            /* Upcase the alias name */
            TempString.Buffer = CurrentString;
            TempString.Length = Length;
            RtlUpcaseUnicodeString(&CurrentAlias->Name,
                                   &TempString,
                                   FALSE);

            /* Update the current string pointer */
            QueryContext->CurrentStringPointer +=
                CurrentAlias->Name.MaximumLength / sizeof(WCHAR);

            /* Go to the next string */
            CurrentString += (Length + sizeof(UNICODE_NULL)) / sizeof(WCHAR);
        }
    }

    return STATUS_SUCCESS;
}

LONG
NTAPI
NpCompareAliasNames(
    _In_ PCUNICODE_STRING String1,
    _In_ PCUNICODE_STRING String2)
{
    ULONG Count;
    PWCHAR P1, P2;

    /* First check if the string sizes match */
    if (String1->Length != String2->Length)
    {
        /* They don't, return positive if the first is longer, negative otherwise */
        return String1->Length - String2->Length;
    }

    /* Now loop all characters */
    Count = String1->Length / sizeof(WCHAR);
    P1 = String1->Buffer;
    P2 = String2->Buffer;
    while (Count)
    {
        /* Check if they don't match */
        if (*P1 != *P2)
        {
            /* Return positive if the first char is greater, negative otherwise */
            return *P1 - *P2;
        }

        /* Go to the next buffer position */
        P1++;
        P2++;
        Count--;
    }

    /* All characters matched, return 0 */
    return 0;
}

NTSTATUS
NTAPI
NpInitializeAliases(VOID)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    NPFS_QUERY_VALUE_CONTEXT Context;
    NTSTATUS Status;
    USHORT Length;
    ULONG i;
    PNPFS_ALIAS CurrentAlias, *AliasPointer;

    /* Initialize the query table */
    QueryTable[0].QueryRoutine = NpReadAlias;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_NOEXPAND;
    QueryTable[0].Name = NULL;
    QueryTable[0].EntryContext = NULL;
    QueryTable[0].DefaultType = REG_NONE;
    QueryTable[0].DefaultData = NULL;
    QueryTable[0].DefaultLength = 0;
    QueryTable[1].QueryRoutine = NULL;
    QueryTable[1].Flags = 0;
    QueryTable[1].Name = NULL;

    /* Setup the query context */
    Context.SizeOnly = 1;
    Context.FullSize = 0;
    Context.NumberOfAliases = 0;
    Context.NumberOfEntries = 0;

    /* Query the registry values (calculate length only) */
    Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES | RTL_REGISTRY_OPTIONAL,
                                    L"Npfs\\Aliases",
                                    QueryTable,
                                    &Context,
                                    NULL);
    if (!NT_SUCCESS(Status))
    {
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
            return STATUS_SUCCESS;

        return Status;
    }

    /* Check if there is anything */
    if (Context.FullSize == 0)
    {
        /* Nothing to do, return success */
        return STATUS_SUCCESS;
    }

    /* Allocate a structure large enough to hold all the data */
    NpAliases = ExAllocatePoolWithTag(NonPagedPool, Context.FullSize, 'sfpN');
    if (NpAliases == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Now setup the actual pointers in the context */
    Context.CurrentTargetName = NpAliases;
    CurrentAlias = (PNPFS_ALIAS)&Context.CurrentTargetName[Context.NumberOfEntries];
    Context.CurrentAlias = CurrentAlias;
    Context.CurrentStringPointer = (PWCHAR)&CurrentAlias[Context.NumberOfAliases];

    /* This time query the real data */
    Context.SizeOnly = FALSE;
    Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES | RTL_REGISTRY_OPTIONAL,
                                    L"Npfs\\Aliases",
                                    QueryTable,
                                    &Context,
                                    NULL);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(NpAliases, 0);
        NpAliases = NULL;
        return Status;
    }

    /* Make sure we didn't go past the end of the allocation! */
    NT_ASSERT((PUCHAR)Context.CurrentStringPointer <=
              ((PUCHAR)NpAliases + Context.FullSize));

    /* Loop all aliases we got */
    for (i = 0; i < Context.NumberOfAliases; i++)
    {
        /* Get the length and check what list to use */
        Length = CurrentAlias->Name.Length;
        if ((Length >= MIN_INDEXED_LENGTH * sizeof(WCHAR)) &&
            (Length <= MAX_INDEXED_LENGTH * sizeof(WCHAR)))
        {
            /* For this length range, we use an indexed list */
            AliasPointer = &NpAliasListByLength[(Length / sizeof(WCHAR)) - 5];
        }
        else
        {
            /* Length is outside of the range, use the default list */
            AliasPointer = &NpAliasList;
        }

        /* Loop through all aliases already in the list until we find one that
           is greater than our current alias */
        while ((*AliasPointer != NULL) &&
               (NpCompareAliasNames(&CurrentAlias->Name,
                                    &(*AliasPointer)->Name) > 0))
        {
            /* Go to the next alias */
            AliasPointer = &(*AliasPointer)->Next;
        }

        /* Insert the alias in the list */
        CurrentAlias->Next = *AliasPointer;
        *AliasPointer = CurrentAlias;

        /* Go to the next alias in the array */
        CurrentAlias++;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
NpFsdDirectoryControl(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp)
{
    TRACE("Entered\n");
    UNIMPLEMENTED;

    Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING DeviceName;
    NTSTATUS Status;
    UNREFERENCED_PARAMETER(RegistryPath);

    DPRINT("Next-Generation NPFS-Advanced\n");

    Status = NpInitializeAliases();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to initialize aliases!\n");
        return Status;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE] = NpFsdCreate;
    DriverObject->MajorFunction[IRP_MJ_CREATE_NAMED_PIPE] = NpFsdCreateNamedPipe;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = NpFsdClose;
    DriverObject->MajorFunction[IRP_MJ_READ] = NpFsdRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = NpFsdWrite;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = NpFsdQueryInformation;
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] = NpFsdSetInformation;
    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = NpFsdQueryVolumeInformation;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = NpFsdCleanup;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = NpFsdFlushBuffers;
    DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] = NpFsdDirectoryControl;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = NpFsdFileSystemControl;
    DriverObject->MajorFunction[IRP_MJ_QUERY_SECURITY] = NpFsdQuerySecurityInfo;
    DriverObject->MajorFunction[IRP_MJ_SET_SECURITY] = NpFsdSetSecurityInfo;

    DriverObject->DriverUnload = NULL;

    DriverObject->FastIoDispatch = &NpFastIoDispatch;

    RtlInitUnicodeString(&DeviceName, L"\\Device\\NamedPipe");
    Status = IoCreateDevice(DriverObject,
                            sizeof(NP_VCB),
                            &DeviceName,
                            FILE_DEVICE_NAMED_PIPE,
                            0,
                            FALSE,
                            &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create named pipe device! (Status %lx)\n", Status);
        return Status;
    }

    /* Initialize the device object */
    NpfsDeviceObject = DeviceObject;
    DeviceObject->Flags |= DO_LONG_TERM_REQUESTS;

    /* Initialize the Volume Control Block (VCB) */
    NpVcb = DeviceObject->DeviceExtension;
    NpInitializeVcb();
    Status = NpCreateRootDcb();
    ASSERT(Status == STATUS_SUCCESS);
    return STATUS_SUCCESS;
}

/* EOF */
