/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Configuration Manager - Hives file list management
 * COPYRIGHT:   Copyright 2012-2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "ntoskrnl.h"
#define NDEBUG
#include "debug.h"

/* GLOBALS ********************************************************************/

static UNICODE_STRING HiveListKeyName =
    RTL_CONSTANT_STRING(L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Control\\hivelist");

/* FUNCTIONS ******************************************************************/

/* Note: the caller is expected to free the HiveName string buffer */
static
BOOLEAN
CmpGetHiveName(
    _In_ PCMHIVE Hive,
    _Out_ PUNICODE_STRING HiveName)
{
    HCELL_INDEX RootCell, LinkCell;
    PCELL_DATA RootData, LinkData, ParentData;
    ULONG ParentNameSize, LinkNameSize;
    SIZE_T NameSize;
    PWCHAR p;
    UNICODE_STRING RegistryName = RTL_CONSTANT_STRING(L"\\REGISTRY\\");

    /* Get the root cell of this hive */
    RootCell = Hive->Hive.BaseBlock->RootCell;
    RootData = HvGetCell(&Hive->Hive, RootCell);
    if (!RootData) return FALSE;

    /* Get the cell index at which this hive is linked to, and its parent */
    LinkCell = RootData->u.KeyNode.Parent;
    HvReleaseCell(&Hive->Hive, RootCell);

    /* Sanity check */
    ASSERT((&CmiVolatileHive->Hive)->ReleaseCellRoutine == NULL);

    /* Get the cell data for link and parent */
    LinkData = HvGetCell(&CmiVolatileHive->Hive, LinkCell);
    if (!LinkData) return FALSE;
    ParentData = HvGetCell(&CmiVolatileHive->Hive, LinkData->u.KeyNode.Parent);
    if (!ParentData) return FALSE;

    /* Get the size of the parent name */
    if (ParentData->u.KeyNode.Flags & KEY_COMP_NAME)
    {
        ParentNameSize = CmpCompressedNameSize(ParentData->u.KeyNode.Name,
                                               ParentData->u.KeyNode.NameLength);
    }
    else
    {
        ParentNameSize = ParentData->u.KeyNode.NameLength;
    }

    /* Get the size of the link name */
    if (LinkData->u.KeyNode.Flags & KEY_COMP_NAME)
    {
        LinkNameSize = CmpCompressedNameSize(LinkData->u.KeyNode.Name,
                                             LinkData->u.KeyNode.NameLength);
    }
    else
    {
        LinkNameSize = LinkData->u.KeyNode.NameLength;
    }

    /* No need to account for terminal NULL character since we deal with counted UNICODE strings */
    NameSize = RegistryName.Length + ParentNameSize + sizeof(WCHAR) + LinkNameSize;

    /* Allocate the memory */
    HiveName->Buffer = ExAllocatePoolWithTag(PagedPool, NameSize, TAG_CM);
    if (!HiveName->Buffer)
    {
        DPRINT1("CmpGetHiveName: Unable to allocate memory\n");
        return FALSE;
    }

    /* Build the string for it */
    HiveName->Length = HiveName->MaximumLength = (USHORT)NameSize;
    p = HiveName->Buffer;

    /* Copy the parent name */
    RtlCopyMemory(p, RegistryName.Buffer, RegistryName.Length);
    p += RegistryName.Length / sizeof(WCHAR);
    if (ParentData->u.KeyNode.Flags & KEY_COMP_NAME)
    {
        CmpCopyCompressedName(p,
                              ParentNameSize,
                              ParentData->u.KeyNode.Name,
                              ParentData->u.KeyNode.NameLength);
    }
    else
    {
        RtlCopyMemory(p, ParentData->u.KeyNode.Name, ParentNameSize);
    }

    /* Add a path separator between parent and link */
    p += ParentNameSize / sizeof(WCHAR);
    *p = OBJ_NAME_PATH_SEPARATOR;
    ++p;

    /* Now copy the link name */
    if (LinkData->u.KeyNode.Flags & KEY_COMP_NAME)
    {
        CmpCopyCompressedName(p,
                              LinkNameSize,
                              LinkData->u.KeyNode.Name,
                              LinkData->u.KeyNode.NameLength);

    }
    else
    {
        RtlCopyMemory(p, LinkData->u.KeyNode.Name, LinkNameSize);
    }

    /* All done */
    return TRUE;
}

NTSTATUS
NTAPI
CmpAddToHiveFileList(
    _Inout_ PCMHIVE Hive)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;
    UNICODE_STRING HivePath;
    PWSTR FilePath;
    ULONG Length;
    POBJECT_NAME_INFORMATION FileNameInfo;

    HivePath.Buffer = NULL;
    FileNameInfo = NULL;

    /* Create or open the hive list key */
    InitializeObjectAttributes(&ObjectAttributes,
                               &HiveListKeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwCreateKey(&KeyHandle,
                         KEY_SET_VALUE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CmpAddToHiveFileList: Failed to create or open the hive list (Status: 0x%08lx)\n", Status);
        return Status;
    }

    /* Retrieve the hive path.
     * NOTE: On Vista+, the hive path is constructed in CmpLinkHiveToMaster().
     * In ReactOS, its construction is instead deferred to this point, when
     * the hive is added to the hive file list. */
    if (!CmpGetHiveName(Hive, &HivePath))
    {
        DPRINT1("CmpAddToHiveFileList: Unable to retrieve the hive path\n");
        Status = STATUS_NO_MEMORY;
        goto Quit;
    }

    /* Get the name of the corresponding file */
    if (!(Hive->Hive.HiveFlags & HIVE_VOLATILE))
    {
        /* Determine the right buffer size and allocate */
        OBJECT_NAME_INFORMATION DummyNameInfo;
        Status = ZwQueryObject(Hive->FileHandles[HFILE_TYPE_PRIMARY],
                               ObjectNameInformation,
                               &DummyNameInfo,
                               sizeof(DummyNameInfo),
                               &Length);
        if (Status != STATUS_BUFFER_OVERFLOW)
        {
            DPRINT1("CmpAddToHiveFileList: Hive file name size query failed (Status: 0x%08lx)\n", Status);
            goto Quit;
        }

        FileNameInfo = ExAllocatePoolWithTag(PagedPool,
                                             Length + sizeof(UNICODE_NULL),
                                             TAG_CM);
        if (FileNameInfo == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Quit;
        }

        /* Try to get the value */
        Status = ZwQueryObject(Hive->FileHandles[HFILE_TYPE_PRIMARY],
                               ObjectNameInformation,
                               FileNameInfo,
                               Length,
                               &Length);
        if (NT_SUCCESS(Status))
        {
            /* Null-terminate and add the length of the terminator */
            Length -= sizeof(OBJECT_NAME_INFORMATION);
            FilePath = FileNameInfo->Name.Buffer;
            FilePath[Length / sizeof(WCHAR)] = UNICODE_NULL;
            Length += sizeof(UNICODE_NULL);
        }
        else
        {
            DPRINT1("CmpAddToHiveFileList: Hive file name query failed (Status: 0x%08lx)\n", Status);
            goto Quit;
        }
    }
    else
    {
        /* No name */
        FilePath = L"";
        Length = sizeof(UNICODE_NULL);
    }

    /* Set the entry in the hive list */
    Status = ZwSetValueKey(KeyHandle,
                           &HivePath,
                           0,
                           REG_SZ,
                           FilePath,
                           Length);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CmpAddToHiveFileList: Failed to add an entry in the hive list (Status: 0x%08lx)\n", Status);
    }

    /* Capture the hive path and reset the local buffer so it doesn't get freed */
    Hive->HiveRootPath = HivePath;
    HivePath.Buffer = NULL;

Quit:
    /* Cleanup and return status */
    if (HivePath.Buffer)
        ExFreePoolWithTag(HivePath.Buffer, TAG_CM);
    if (FileNameInfo)
        ExFreePoolWithTag(FileNameInfo, TAG_CM);

    ObCloseHandle(KeyHandle, KernelMode);
    return Status;
}

VOID
NTAPI
CmpRemoveFromHiveFileList(
    _Inout_ PCMHIVE Hive)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;

    /* Do nothing if no hive path was captured, e.g. CmpAddToHiveFileList() failed */
    if (Hive->HiveRootPath.Buffer == NULL)
        return;

    /* Open the hive list key */
    InitializeObjectAttributes(&ObjectAttributes,
                               &HiveListKeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwOpenKey(&KeyHandle,
                       KEY_SET_VALUE,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CmpRemoveFromHiveFileList: Failed to open the hive list (Status: 0x%08lx)\n", Status);
        return;
    }

    /* Delete the hive path from the list */
    ZwDeleteValueKey(KeyHandle, &Hive->HiveRootPath);

    /* Free the captured hive path */
    ExFreePoolWithTag(Hive->HiveRootPath.Buffer, TAG_CM);
    RtlInitEmptyUnicodeString(&Hive->HiveRootPath, NULL, 0);

    /* Close the key and exit */
    ObCloseHandle(KeyHandle, KernelMode);
}

/* EOF */
