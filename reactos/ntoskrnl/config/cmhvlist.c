/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmhvlist.c
 * PURPOSE:         Configuration Manager - Hives file list management
 * PROGRAMMERS:     Hermes BELUSCA - MAITO
 */

/* INCLUDES *******************************************************************/

#include "ntoskrnl.h"
#define NDEBUG
#include "debug.h"

/* GLOBALS ********************************************************************/

UNICODE_STRING HiveListValueName = RTL_CONSTANT_STRING(L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Control\\hivelist");

/* FUNCTIONS ******************************************************************/

/* Note: the caller is expected to free the HiveName string buffer */
BOOLEAN
NTAPI
CmpGetHiveName(IN PCMHIVE Hive,
               OUT PUNICODE_STRING HiveName)
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
        /* Fail */
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
CmpAddToHiveFileList(IN PCMHIVE Hive)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;
    UNICODE_STRING HivePath;
    PWCHAR FilePath;
    UCHAR Buffer[sizeof(OBJECT_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR)];
    ULONG Length = sizeof(Buffer);
    POBJECT_NAME_INFORMATION FileNameInfo = (POBJECT_NAME_INFORMATION)&Buffer;
    HivePath.Buffer = NULL;

    /* Create or open the hive list key */
    InitializeObjectAttributes(&ObjectAttributes,
                               &HiveListValueName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwCreateKey(&KeyHandle,
                         KEY_READ | KEY_WRITE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        DPRINT1("CmpAddToHiveFileList: Creation or opening of the hive list failed, status = 0x%08lx\n", Status);
        return Status;
    }

    /* Retrieve the name of the hive */
    if (!CmpGetHiveName(Hive, &HivePath))
    {
        /* Fail */
        DPRINT1("CmpAddToHiveFileList: Unable to retrieve the hive name\n");
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* Get the name of the corresponding file */
    if (!(Hive->Hive.HiveFlags & HIVE_VOLATILE))
    {
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
            /* Fail */
            DPRINT1("CmpAddToHiveFileList: Hive file name query failed, status = 0x%08lx\n", Status);
            goto Quickie;
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
        /* Fail */
        DPRINT1("CmpAddToHiveFileList: Setting of entry in the hive list failed, status = 0x%08lx\n", Status);
    }

Quickie:
    /* Cleanup and return status */
    if (HivePath.Buffer) ExFreePoolWithTag(HivePath.Buffer, TAG_CM);
    ObCloseHandle(KeyHandle, KernelMode);
    return Status;
}

VOID
NTAPI
CmpRemoveFromHiveFileList(IN PCMHIVE Hive)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;
    UNICODE_STRING HivePath;

    /* Open the hive list key */
    InitializeObjectAttributes(&ObjectAttributes,
                               &HiveListValueName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwOpenKey(&KeyHandle,
                       KEY_READ | KEY_WRITE,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        DPRINT1("CmpRemoveFromHiveFileList: Opening of the hive list failed, status = 0x%08lx\n", Status);
        return;
    }

    /* Get the hive path name */
    CmpGetHiveName(Hive, &HivePath);

    /* Delete the hive path name from the list */
    ZwDeleteValueKey(KeyHandle, &HivePath);

    /* Cleanup allocation and handle */
    ExFreePoolWithTag(HivePath.Buffer, TAG_CM);
    ObCloseHandle(KeyHandle, KernelMode);
}

/* EOF */
