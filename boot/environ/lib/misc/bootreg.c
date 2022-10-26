/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/misc/bootreg.c
 * PURPOSE:         Boot Library Boot Registry Wrapper for CMLIB
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"
#include <bcd.h>

/* DEFINITIONS ***************************************************************/

#define BI_FLUSH_HIVE       0x01
#define BI_HIVE_WRITEABLE   0x02

/* DATA STRUCTURES ***********************************************************/

typedef struct _BI_KEY_HIVE
{
    PHBASE_BLOCK BaseBlock;
    ULONG HiveSize;
    PBL_FILE_PATH_DESCRIPTOR FilePath;
    CMHIVE Hive;
    LONG ReferenceCount;
    ULONG Flags;
    PCM_KEY_NODE RootNode;
} BI_KEY_HIVE, *PBI_KEY_HIVE;

typedef struct _BI_KEY_OBJECT
{
    PBI_KEY_HIVE KeyHive;
    PCM_KEY_NODE KeyNode;
    HCELL_INDEX KeyCell;
    PWCHAR KeyName;
} BI_KEY_OBJECT, *PBI_KEY_OBJECT;

/* GLOBALS *******************************************************************/

BOOLEAN BiHiveHashLibraryInitialized;
ULONGLONG HvSymcryptSeed;

/* FUNCTIONS *****************************************************************/

BOOLEAN
HvIsInPlaceBaseBlockValid (
    _In_ PHBASE_BLOCK BaseBlock
    )
{
    ULONG HiveLength, HeaderSum;
    BOOLEAN Valid;

    /* Assume failure */
    Valid = FALSE;

    /* Check for incorrect signature, type, version, or format */
    if ((BaseBlock->Signature == 'fger') &&
        (BaseBlock->Type == 0) &&
        (BaseBlock->Major <= 1) &&
        (BaseBlock->Minor <= 5) &&
        (BaseBlock->Minor >= 3) &&
        (BaseBlock->Format == 1))
    {
        /* Check for invalid hive size */
        HiveLength = BaseBlock->Length;
        if (HiveLength)
        {
            /* Check for misaligned or too large hive size */
            if (!(HiveLength & 0xFFF) && HiveLength <= 0x7FFFE000)
            {
                /* Check for invalid header checksum */
                HeaderSum = HvpHiveHeaderChecksum(BaseBlock);
                if (HeaderSum == BaseBlock->CheckSum)
                {
                    /* All good */
                    Valid = TRUE;
                }
            }
        }
    }

    /* Return validity */
    return Valid;
}

PVOID
NTAPI
CmpAllocate (
    _In_ SIZE_T Size,
    _In_ BOOLEAN Paged,
    _In_ ULONG Tag
    )
{
    UNREFERENCED_PARAMETER(Paged);
    UNREFERENCED_PARAMETER(Tag);

    /* Call the heap allocator */
    return BlMmAllocateHeap(Size);
}

VOID
NTAPI
CmpFree (
    _In_ PVOID Ptr,
    _In_ ULONG Quota
    )
{
    UNREFERENCED_PARAMETER(Quota);

    /* Call the heap allocator */
    BlMmFreeHeap(Ptr);
}

VOID
BiDereferenceHive (
    _In_ HANDLE KeyHandle
    )
{
    PBI_KEY_OBJECT KeyObject;

    /* Get the key object */
    KeyObject = (PBI_KEY_OBJECT)KeyHandle;

    /* Drop a reference on the parent hive */
    --KeyObject->KeyHive->ReferenceCount;
}

VOID
BiFlushHive (
    _In_ HANDLE KeyHandle
    )
{
    /* Not yet implemented */
    EfiPrintf(L"NO reg flush\r\n");
    return;
}

VOID
BiCloseKey (
    _In_ HANDLE KeyHandle
    )
{
    PBI_KEY_HIVE KeyHive;
    PBI_KEY_OBJECT KeyObject;

    /* Get the key object and hive */
    KeyObject = (PBI_KEY_OBJECT)KeyHandle;
    KeyHive = KeyObject->KeyHive;

    /* Check if we have a hive, or name, or key node */
    if ((KeyHive) || (KeyObject->KeyNode) || (KeyObject->KeyName))
    {
        /* Drop a reference, see if it's the last one */
        BiDereferenceHive(KeyHandle);
        if (!KeyHive->ReferenceCount)
        {
            /* Check if we should flush it */
            if (KeyHive->Flags & BI_FLUSH_HIVE)
            {
                BiFlushHive(KeyHandle);
            }

            /* Unmap the hive */
            MmPapFreePages(KeyHive->BaseBlock, BL_MM_INCLUDE_MAPPED_ALLOCATED);

            /* Free the hive and hive path */
            BlMmFreeHeap(KeyHive->FilePath);
            BlMmFreeHeap(KeyHive);
        }

        /* Check if a key name is present */
        if (KeyObject->KeyName)
        {
            /* Free it */
            BlMmFreeHeap(KeyObject->KeyName);
        }
    }

    /* Free the object */
    BlMmFreeHeap(KeyObject);
}

NTSTATUS
BiOpenKey(
    _In_ HANDLE ParentHandle,
    _In_ PWCHAR KeyName,
    _Out_ PHANDLE Handle
    )
{
    PBI_KEY_OBJECT ParentKey, NewKey;
    PBI_KEY_HIVE ParentHive;
    NTSTATUS Status;
    SIZE_T NameLength, SubNameLength, NameBytes;
    PWCHAR NameStart, NameBuffer;
    UNICODE_STRING KeyString;
    HCELL_INDEX KeyCell;
    PHHIVE Hive;
    PCM_KEY_NODE ParentNode;

    /* Convert from a handle to our key object */
    ParentKey = (PBI_KEY_OBJECT)ParentHandle;

    /* Extract the hive and node information */
    ParentHive = ParentKey->KeyHive;
    ParentNode = ParentKey->KeyNode;
    Hive = &ParentKey->KeyHive->Hive.Hive;

    /* Initialize variables */
    KeyCell = HCELL_NIL;
    Status = STATUS_SUCCESS;
    NameBuffer = NULL;

    /* Loop as long as there's still portions of the key name in play */
    NameLength = wcslen(KeyName);
    while (NameLength)
    {
        /* Find the first path separator */
        NameStart = wcschr(KeyName, OBJ_NAME_PATH_SEPARATOR);
        if (NameStart)
        {
            /* Look only at the key before the separator */
            SubNameLength = NameStart - KeyName;
            ++NameStart;
        }
        else
        {
            /* No path separator, this is the final leaf key */
            SubNameLength = NameLength;
        }

        /* Free the name buffer from the previous pass if needed */
        if (NameBuffer)
        {
            BlMmFreeHeap(NameBuffer);
        }

        /* Allocate a buffer to hold the name of this specific subkey only */
        NameBytes = SubNameLength * sizeof(WCHAR);
        NameBuffer = BlMmAllocateHeap(NameBytes + sizeof(UNICODE_NULL));
        if (!NameBuffer)
        {
            Status = STATUS_NO_MEMORY;
            goto Quickie;
        }

        /* Copy and null-terminate the name of the subkey */
        RtlCopyMemory(NameBuffer, KeyName, NameBytes);
        NameBuffer[SubNameLength] = UNICODE_NULL;

        /* Convert it into a UNICODE_STRING and try to find it */
        RtlInitUnicodeString(&KeyString, NameBuffer);
        KeyCell = CmpFindSubKeyByName(Hive, ParentNode, &KeyString);
        if (KeyCell == HCELL_NIL)
        {
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
            goto Quickie;
        }

        /* We found it -- get the key node out of it */
        ParentNode = (PCM_KEY_NODE)HvGetCell(Hive, KeyCell);
        if (!ParentNode)
        {
            Status = STATUS_REGISTRY_CORRUPT;
            goto Quickie;
        }

        /* Update the key name to the next remaining path element */
        KeyName = NameStart;
        if (NameStart)
        {
            /* Update the length to the remainder of the path */
            NameLength += -1 - SubNameLength;
        }
        else
        {
            /* There's nothing left, this was the leaf key */
            NameLength = 0;
        }
    }

    /* Allocate a key object */
    NewKey = BlMmAllocateHeap(sizeof(*NewKey));
    if (!NewKey)
    {
        /* Bail out if we had no memory for it */
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* Fill out the key object data */
    NewKey->KeyNode = ParentNode;
    NewKey->KeyHive = ParentHive;
    NewKey->KeyName = NameBuffer;
    NewKey->KeyCell = KeyCell;

    /* Add a reference to the hive */
    ++ParentHive->ReferenceCount;

    /* Return the object back to the caller */
    *Handle = NewKey;

Quickie:
    /* If we had a name buffer, free it */
    if (NameBuffer)
    {
        BlMmFreeHeap(NameBuffer);
    }

    /* Return status of the open operation */
    return Status;
}

NTSTATUS
BiInitializeAndValidateHive (
    _In_ PBI_KEY_HIVE Hive
    )
{
    ULONG HiveSize;
    CM_CHECK_REGISTRY_STATUS CmStatusCode;
    NTSTATUS Status;

    /* Make sure the hive is at least the size of a base block */
    if (Hive->HiveSize < sizeof(HBASE_BLOCK))
    {
        return STATUS_REGISTRY_CORRUPT;
    }

    /* Make sure that the base block accurately describes the size of the hive */
    HiveSize = Hive->BaseBlock->Length + sizeof(HBASE_BLOCK);
    if ((HiveSize < sizeof(HBASE_BLOCK)) || (HiveSize > Hive->HiveSize))
    {
        return STATUS_REGISTRY_CORRUPT;
    }

    /* Initialize a flat memory hive */
    RtlZeroMemory(&Hive->Hive, sizeof(Hive->Hive));
    Status = HvInitialize(&Hive->Hive.Hive,
                          HINIT_FLAT,
                          0,
                          0,
                          Hive->BaseBlock,
                          CmpAllocate,
                          CmpFree,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          0,
                          NULL);
    if (NT_SUCCESS(Status))
    {
        /* Cleanup volatile/old data */
        CmStatusCode = CmCheckRegistry(Hive->Hive, CM_CHECK_REGISTRY_BOOTLOADER_PURGE_VOLATILES | CM_CHECK_REGISTRY_VALIDATE_HIVE);
        if (!CM_CHECK_REGISTRY_SUCCESS(CmStatusCode))
        {
            return STATUS_REGISTRY_CORRUPT;
        }

        Status = STATUS_SUCCESS;
    }

    /* Return the final status */
    return Status;
}

NTSTATUS
BiLoadHive (
    _In_ PBL_FILE_PATH_DESCRIPTOR FilePath,
    _Out_ PHANDLE HiveHandle
    )
{
    ULONG DeviceId;
    PHBASE_BLOCK BaseBlock, NewBaseBlock;
    PBI_KEY_OBJECT KeyObject;
    PBI_KEY_HIVE BcdHive;
    PBL_DEVICE_DESCRIPTOR BcdDevice;
    ULONG PathLength, DeviceLength, HiveSize, HiveLength, NewHiveSize;
    PWCHAR HiveName, LogName;
    BOOLEAN HaveWriteAccess;
    NTSTATUS Status;
    PVOID LogData;
    PHHIVE Hive;
    UNICODE_STRING KeyString;
    PCM_KEY_NODE RootNode;
    HCELL_INDEX CellIndex;

    /* Initialize variables */
    DeviceId = -1;
    BaseBlock = NULL;
    BcdHive = NULL;
    KeyObject = NULL;
    LogData = NULL;
    LogName = NULL;

    /* Initialize the crypto seed */
    if (!BiHiveHashLibraryInitialized)
    {
        HvSymcryptSeed = 0x82EF4D887A4E55C5;
        BiHiveHashLibraryInitialized = TRUE;
    }

    /* Extract and validate the input path */
    BcdDevice = (PBL_DEVICE_DESCRIPTOR)&FilePath->Path;
    PathLength = FilePath->Length;
    DeviceLength = BcdDevice->Size;
    HiveName = (PWCHAR)((ULONG_PTR)BcdDevice + BcdDevice->Size);
    if (PathLength <= DeviceLength)
    {
        /* Doesn't make sense, bail out */
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    /* Attempt to open the underlying device for RW access */
    HaveWriteAccess = TRUE;
    Status = BlpDeviceOpen(BcdDevice,
                           BL_DEVICE_READ_ACCESS | BL_DEVICE_WRITE_ACCESS,
                           0,
                           &DeviceId);
    if (!NT_SUCCESS(Status))
    {
        /* Try for RO access instead */
        HaveWriteAccess = FALSE;
        Status = BlpDeviceOpen(BcdDevice, BL_DEVICE_READ_ACCESS, 0, &DeviceId);
        if (!NT_SUCCESS(Status))
        {
            /* No access at all -- bail out */
            goto Quickie;
        }
    }

    /* Now try to load the hive on disk */
    Status = BlImgLoadImageWithProgress2(DeviceId,
                                         BlLoaderRegistry,
                                         HiveName,
                                         (PVOID*)&BaseBlock,
                                         &HiveSize,
                                         0,
                                         FALSE,
                                         NULL,
                                         NULL);
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"Hive read failure: % lx\r\n", Status);
        goto Quickie;
    }

    /* Allocate a hive structure */
    BcdHive = BlMmAllocateHeap(sizeof(*BcdHive));
    if (!BcdHive)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* Initialize it */
    RtlZeroMemory(BcdHive, sizeof(*BcdHive));
    BcdHive->BaseBlock = BaseBlock;
    BcdHive->HiveSize = HiveSize;
    if (HaveWriteAccess)
    {
        BcdHive->Flags |= BI_HIVE_WRITEABLE;
    }

    /* Make sure the hive was at least one bin long */
    if (HiveSize < sizeof(*BaseBlock))
    {
        Status = STATUS_REGISTRY_CORRUPT;
        goto Quickie;
    }

    /* Make sure the hive contents are at least one bin long */
    HiveLength = BaseBlock->Length;
    if (BaseBlock->Length < sizeof(*BaseBlock))
    {
        Status = STATUS_REGISTRY_CORRUPT;
        goto Quickie;
    }

    /* Validate the initial bin (the base block) */
    if (!HvIsInPlaceBaseBlockValid(BaseBlock))
    {
        EfiPrintf(L"Recovery not implemented\r\n");
        Status = STATUS_REGISTRY_CORRUPT;
        goto Quickie;
    }

    /* Check if there's log recovery that needs to happen */
    if (BaseBlock->Sequence1 != BaseBlock->Sequence2)
    {
        EfiPrintf(L"Log fix not implemented: %lx %lx\r\n");
        Status = STATUS_REGISTRY_CORRUPT;
        goto Quickie;
    }

    /*
     * Check if the whole hive doesn't fit in the buffer.
     * Note: HiveLength does not include the size of the baseblock itself
     */
    if (HiveSize < (HiveLength + sizeof(*BaseBlock)))
    {
        EfiPrintf(L"Need bigger hive buffer path\r\n");

        /* Allocate a slightly bigger buffer */
        NewHiveSize = HiveLength + sizeof(*BaseBlock);
        Status = MmPapAllocatePagesInRange((PVOID*)&NewBaseBlock,
                                           BlLoaderRegistry,
                                           NewHiveSize >> PAGE_SHIFT,
                                           0,
                                           0,
                                           NULL,
                                           0);
        if (!NT_SUCCESS(Status))
        {
            goto Quickie;
        }

        /* Copy the current data in there */
        RtlCopyMemory(NewBaseBlock, BaseBlock, HiveSize);

        /* Free the old data */
        MmPapFreePages(BaseBlock, BL_MM_INCLUDE_MAPPED_ALLOCATED);

        /* Update our pointers */
        BaseBlock = NewBaseBlock;
        HiveSize = NewHiveSize;
        BcdHive->BaseBlock = BaseBlock;
        BcdHive->HiveSize = HiveSize;
    }

    /* Check if any log stuff needs to happen */
    if (LogData)
    {
        EfiPrintf(L"Log fix not implemented: %lx %lx\r\n");
        Status = STATUS_REGISTRY_CORRUPT;
        goto Quickie;
    }

    /* Call Hv to setup the hive library */
    Status = BiInitializeAndValidateHive(BcdHive);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Now get the root node */
    Hive = &BcdHive->Hive.Hive;
    RootNode = (PCM_KEY_NODE)HvGetCell(Hive, Hive->BaseBlock->RootCell);
    if (!RootNode)
    {
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto Quickie;
    }

    /* Find the Objects subkey under it to see if it's a real BCD hive */
    RtlInitUnicodeString(&KeyString, L"Objects");
    CellIndex = CmpFindSubKeyByName(Hive, RootNode, &KeyString);
    if (CellIndex == HCELL_NIL)
    {
        EfiPrintf(L"No OBJECTS subkey found!\r\n");
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto Quickie;
    }

    /* This is a valid BCD hive, store its root node here */
    BcdHive->RootNode = RootNode;

    /* Allocate a copy of the file path */
    BcdHive->FilePath = BlMmAllocateHeap(FilePath->Length);
    if (!BcdHive->FilePath)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* Make a copy of it */
    RtlCopyMemory(BcdHive->FilePath, FilePath, FilePath->Length);

    /* Create a key object to describe the rot */
    KeyObject = BlMmAllocateHeap(sizeof(*KeyObject));
    if (!KeyObject)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* Fill out the details */
    KeyObject->KeyNode = RootNode;
    KeyObject->KeyHive = BcdHive;
    KeyObject->KeyName = NULL;
    KeyObject->KeyCell = Hive->BaseBlock->RootCell;

    /* One reference for the key object, plus one lifetime reference */
    BcdHive->ReferenceCount = 2;

    /* This is the hive handle */
    *HiveHandle  = KeyObject;

    /* We're all good */
    Status = STATUS_SUCCESS;

Quickie:
    /* If we had a log name, free it */
    if (LogName)
    {
        BlMmFreeHeap(LogName);
    }

    /* If we had logging data, free it */
    if (LogData)
    {
        MmPapFreePages(LogData, BL_MM_INCLUDE_MAPPED_ALLOCATED);
    }

    /* Check if this is the failure path */
    if (!NT_SUCCESS(Status))
    {
        /* If we mapped the hive, free it */
        if (BaseBlock)
        {
            MmPapFreePages(BaseBlock, BL_MM_INCLUDE_MAPPED_ALLOCATED);
        }

        /* If we opened the device, close it */
        if (DeviceId != -1)
        {
            BlDeviceClose(DeviceId);
        }

        /* Did we create a hive object? */
        if (BcdHive)
        {
            /* Free the file path if we made a copy of it */
            if (BcdHive->FilePath)
            {
                BlMmFreeHeap(BcdHive->FilePath);
            }

            /* Free the hive itself */
            BlMmFreeHeap(BcdHive);
        }

        /* Finally, free the root key object if we created one */
        if (KeyObject)
        {
            BlMmFreeHeap(KeyObject);
        }
    }

    /* Return the final status */
    return Status;
}

NTSTATUS
BiGetRegistryValue (
    _In_ HANDLE KeyHandle,
    _In_ PWCHAR ValueName,
    _In_ ULONG Type,
    _Out_ PVOID* Buffer,
    _Out_ PULONG ValueLength
    )
{
    PCM_KEY_NODE KeyNode;
    PHHIVE KeyHive;
    UNICODE_STRING ValueString;
    PBI_KEY_OBJECT KeyObject;
    PCM_KEY_VALUE KeyValue;
    PVOID ValueCopy;
    ULONG Size;
    HCELL_INDEX CellIndex;
    PCELL_DATA ValueData;

    /* Get the key object, node,and hive */
    KeyObject = (PBI_KEY_OBJECT)KeyHandle;
    KeyNode = KeyObject->KeyNode;
    KeyHive = &KeyObject->KeyHive->Hive.Hive;

    /* Find the value cell index in the list of values */
    RtlInitUnicodeString(&ValueString, ValueName);
    CmpFindNameInList(KeyHive,
                      &KeyNode->ValueList,
                      &ValueString,
                      NULL,
                      &CellIndex);
    if (CellIndex == HCELL_NIL)
    {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    /* Get the cell data for it */
    KeyValue = (PCM_KEY_VALUE)HvGetCell(KeyHive, CellIndex);
    if (!KeyValue)
    {
        return STATUS_REGISTRY_CORRUPT;
    }

    /* Make sure the type matches */
    if (KeyValue->Type != Type)
    {
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    /* Now get the data cell */
    ValueData = CmpValueToData(KeyHive, KeyValue, &Size);

    /* Make a copy of it */
    ValueCopy = BlMmAllocateHeap(Size);
    if (!ValueCopy)
    {
        return STATUS_NO_MEMORY;
    }

    /* Copy it in the buffer, and return it and its size */
    RtlCopyMemory(ValueCopy, ValueData, Size);
    *Buffer = ValueCopy;
    *ValueLength = Size;
    return STATUS_SUCCESS;
}

NTSTATUS
BiEnumerateSubKeys (
    _In_ HANDLE KeyHandle,
    _Out_ PWCHAR** SubKeyList,
    _Out_ PULONG SubKeyCount
    )
{
    PCM_KEY_NODE KeyNode, Node;
    PBI_KEY_OBJECT KeyObject;
    ULONG KeyCount;
    ULONG NameLength, NewTotalNameLength, FinalLength, TotalNameLength;
    PHHIVE Hive;
    PWCHAR KeyName, NameEnd;
    HCELL_INDEX CellIndex;
    PWCHAR* SubKeys;
    NTSTATUS Status;
    ULONG i;

    /* Get the key object, node, and hive */
    KeyObject = (PBI_KEY_OBJECT)KeyHandle;
    KeyNode = KeyObject->KeyNode;
    Hive = &KeyObject->KeyHive->Hive.Hive;

    /* Assume it's empty */
    *SubKeyList = 0;
    *SubKeyCount = 0;

    /* Initialize locals */
    KeyCount = 0;
    SubKeys = 0;
    TotalNameLength = 0;

    /* Find the first subkey cell index */
    CellIndex = CmpFindSubKeyByNumber(Hive, KeyNode, KeyCount);
    while (CellIndex != HCELL_NIL)
    {
        /* Move to the next one */
        KeyCount++;

        /* Get the cell data for it */
        Node = (PCM_KEY_NODE)HvGetCell(Hive, CellIndex);
        if (!Node)
        {
            return STATUS_REGISTRY_CORRUPT;
        }

        /* Check if the value is compressed */
        if (Node->Flags & KEY_COMP_NAME)
        {
            /* Get the compressed name size */
            NameLength = CmpCompressedNameSize(Node->Name, Node->NameLength);
        }
        else
        {
            /* Get the real size */
            NameLength = Node->NameLength;
        }

        /* Add up the new length, protecting against overflow */
        NewTotalNameLength = TotalNameLength + NameLength + sizeof(UNICODE_NULL);
        if (NewTotalNameLength < TotalNameLength)
        {
            Status = STATUS_NAME_TOO_LONG;
            goto Quickie;
        }

        /* We're good, use the new length */
        TotalNameLength = NewTotalNameLength;

        /* Find the next subkey cell index */
        CellIndex = CmpFindSubKeyByNumber(Hive, KeyNode, KeyCount);
    }

    /* Were there no keys? We're done, if so */
    if (!KeyCount)
    {
        return STATUS_SUCCESS;
    }

    /* Safely compute the size of the array needed */
    Status = RtlULongLongToULong(sizeof(PWCHAR) * KeyCount, &FinalLength);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Safely add that to the name length */
    Status = RtlULongAdd(TotalNameLength, FinalLength, &FinalLength);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Allocate an array big enough for the names and pointers */
    SubKeys = BlMmAllocateHeap(FinalLength);
    if (!SubKeys)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* Go over each key again */
    NameEnd = (PWCHAR)&SubKeys[KeyCount];
    for (i = 0; i < KeyCount; i++)
    {
        /* Get the cell index for this subkey */
        CellIndex = CmpFindSubKeyByNumber(Hive, KeyNode, i);
        if (CellIndex == HCELL_NIL)
        {
            break;
        }

        /* Get the cell data for it */
        Node = (PCM_KEY_NODE)HvGetCell(Hive, CellIndex);
        if (!Node)
        {
            Status = STATUS_REGISTRY_CORRUPT;
            goto Quickie;
        }

        /* Check if the value is compressed */
        KeyName = Node->Name;
        if (Node->Flags & KEY_COMP_NAME)
        {
            /* Get the compressed name size */
            NameLength = CmpCompressedNameSize(KeyName, Node->NameLength);
            CmpCopyCompressedName(NameEnd, NameLength, KeyName, Node->NameLength);
        }
        else
        {
            /* Get the real size */
            NameLength = Node->NameLength;
            RtlCopyMemory(NameEnd, KeyName, NameLength);
        }

        /* Move the name buffer to the next spot, and NULL-terminate */
        SubKeys[i] = NameEnd;
        NameEnd += (NameLength / sizeof(WCHAR));
        *NameEnd = UNICODE_NULL;

        /* Keep going */
        NameEnd++;
    }

    /* Check if the subkeys were empty */
    if (i == 0)
    {
        /* They disappeared in the middle of enumeration */
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto Quickie;
    }

    /* Return the count and the array of names */
    *SubKeyList = SubKeys;
    *SubKeyCount = i;
    SubKeys = NULL;
    Status = STATUS_SUCCESS;

Quickie:
    /* On the failure path, free the subkeys if any exist */
    if (SubKeys)
    {
        BlMmFreeHeap(SubKeys);
    }

    /* All done, return the result */
    return Status;
}

NTSTATUS
BiDeleteKey (
    _In_ HANDLE KeyHandle
    )
{
    NTSTATUS Status;
    PBI_KEY_OBJECT KeyObject;
    PHHIVE Hive;
    ULONG SubKeyCount, i;
    PWCHAR* SubKeyList;
    HANDLE SubKeyHandle;

    /* Get the key object and hive */
    KeyObject = (PBI_KEY_OBJECT)KeyHandle;
    Hive = &KeyObject->KeyHive->Hive.Hive;

    /* Make sure the hive is writeable */
    if (!(KeyObject->KeyHive->Flags & BI_HIVE_WRITEABLE))
    {
        return STATUS_MEDIA_WRITE_PROTECTED;
    }

    /* Enumerate all of the subkeys */
    Status = BiEnumerateSubKeys(KeyHandle, &SubKeyList, &SubKeyCount);
    if ((NT_SUCCESS(Status)) && (SubKeyCount > 0))
    {
        /* Loop through each one */
        for (i = 0; i < SubKeyCount; i++)
        {
            /* Open a handle to it */
            Status = BiOpenKey(KeyHandle, SubKeyList[i], &SubKeyHandle);
            if (NT_SUCCESS(Status))
            {
                /* Recursively call us to delete it */
                Status = BiDeleteKey(SubKeyHandle);
                if (Status != STATUS_SUCCESS)
                {
                    /* Close the key on failure */
                    BiCloseKey(SubKeyHandle);
                }
            }
        }
    }

    /* Check if we had a list of subkeys */
    if (SubKeyList)
    {
        /* Free it */
        BlMmFreeHeap(SubKeyList);
    }

    /* Delete this key cell */
    Status = CmpFreeKeyByCell(Hive, KeyObject->KeyCell, TRUE);
    if (NT_SUCCESS(Status))
    {
        /* Mark the hive as requiring a flush */
        KeyObject->KeyHive->Flags |= BI_FLUSH_HIVE;
        BiCloseKey(KeyHandle);
    }

    /* All done */
    return Status;
}