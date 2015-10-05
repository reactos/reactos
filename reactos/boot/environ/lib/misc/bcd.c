/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/misc/bcd.c
 * PURPOSE:         Boot Library BCD Routines
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"
#include <bcd.h>

/* FUNCTIONS *****************************************************************/

PBL_BCD_OPTION
MiscGetBootOption (
    _In_ PBL_BCD_OPTION List,
    _In_ ULONG Type
    )
{
    ULONG_PTR NextOption = 0, ListOption;
    PBL_BCD_OPTION Option, FoundOption;

    /* No options, bail out */
    if (!List)
    {
        return NULL;
    }

    /* Loop while we find an option */
    FoundOption = NULL;
    do
    {
        /* Get the next option and see if it matches the type */
        Option = (PBL_BCD_OPTION)((ULONG_PTR)List + NextOption);
        if ((Option->Type == Type) && !(Option->Empty))
        {
            FoundOption = Option;
            break;
        }

        /* Store the offset of the next option */
        NextOption = Option->NextEntryOffset;

        /* Failed to match. Check for list options */
        ListOption = Option->ListOffset;
        if (ListOption)
        {
            /* Try to get a match in the associated option */
            Option = MiscGetBootOption((PBL_BCD_OPTION)((ULONG_PTR)Option +
                                       ListOption),
                                       Type);
            if (Option)
            {
                /* Return it */
                FoundOption = Option;
                break;
            }
        }
    } while (NextOption);

    /* Return the option that was found, if any */
    return FoundOption;
}

/*++
 * @name BlGetBootOptionListSize
 *
 *     The BlGetBootOptionListSize routine
 *
 * @param  BcdOption
 *         UEFI Image Handle for the current loaded application.
 *
 * @return Size of the BCD option
 *
 *--*/
ULONG
BlGetBootOptionListSize (
    _In_ PBL_BCD_OPTION BcdOption
    )
{
    ULONG Size = 0, NextOffset = 0;
    PBL_BCD_OPTION NextOption;

    /* Loop all the options*/
    do
    {
        /* Move to the next one */
        NextOption = (PBL_BCD_OPTION)((ULONG_PTR)BcdOption + NextOffset);

        /* Compute the size of the next one */
        Size += BlGetBootOptionSize(NextOption);

        /* Update the offset */
        NextOffset = NextOption->NextEntryOffset;
    } while (NextOffset);

    /* Return final computed size */
    return Size;
}

/*++
 * @name BlGetBootOptionSize
 *
 *     The BlGetBootOptionSize routine
 *
 * @param  BcdOption
 *         UEFI Image Handle for the current loaded application.
 *
 * @return Size of the BCD option
 *
 *--*/
ULONG
BlGetBootOptionSize (
    _In_ PBL_BCD_OPTION BcdOption
    )
{
    ULONG Size, Offset;

    /* Check if there's any data */
    if (BcdOption->DataOffset)
    {
        /* Add the size of the data */
        Size = BcdOption->DataOffset + BcdOption->DataSize;
    }
    else
    {
        /* No data, just the structure itself */
        Size = sizeof(*BcdOption);
    }

    /* Any associated options? */
    Offset = BcdOption->ListOffset;
    if (Offset)
    {
        /* Go get those too */
        Size += BlGetBootOptionListSize((PVOID)((ULONG_PTR)BcdOption + Offset));
    }

    /* Return the final size */
    return Size;
}

NTSTATUS
BlGetBootOptionString (
    _In_ PBL_BCD_OPTION List,
    _In_ ULONG Type,
    _Out_ PWCHAR* Value
    )
{
    NTSTATUS Status;
    PBL_BCD_OPTION Option;
    PWCHAR String, StringCopy;
    ULONG StringLength, CopyLength;
    //PGUID AppIdentifier;

    /* Make sure this is a BCD_STRING */
    if ((Type & 0xF000000) != 0x2000000)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Return the data */
    Option = MiscGetBootOption(List, Type);
    if (Option)
    {
        /* Extract the string */
        String = (PWCHAR)((ULONG_PTR)Option + Option->DataOffset);
        Status = STATUS_SUCCESS;
    }
    else
    {
        /* No string is present */
        String = NULL;
        Status = STATUS_NOT_FOUND;
    }

    /* Compute the data size */
    StringLength = Option->DataSize / sizeof(WCHAR);

#ifdef _SECURE_BOOT_
    /* Filter out SecureBoot Options */
    AppIdentifier = BlGetApplicationIdentifier();
    Status = BlpBootOptionCallbackString(AppIdentifier, Type, String, StringLength, &String, &StringLength);
#else
#endif
    /* Check if we have space for one more character */
    CopyLength = StringLength + 1;
    if (CopyLength < StringLength)
    {
        /* Nope, we'll overflow */
        CopyLength = -1;
        Status = STATUS_INTEGER_OVERFLOW;
    }

    /* No overflow? */
    if (NT_SUCCESS(Status))
    {
        /* Check if it's safe to multiply by two */
        if ((CopyLength * sizeof(WCHAR)) > 0xFFFFFFFF)
        {
            /* Nope */
            CopyLength = -1;
            Status = STATUS_INTEGER_OVERFLOW;
        }
        else
        {
            /* We're good, do the multiplication */
            Status = STATUS_SUCCESS;
            CopyLength *= sizeof(WCHAR);
        }

        /* Allocate a copy for the string */
        if (NT_SUCCESS(Status))
        {
            StringCopy = BlMmAllocateHeap(CopyLength);
            if (StringCopy)
            {
                /* NULL-terminate it */
                RtlCopyMemory(StringCopy,
                              String,
                              CopyLength - sizeof(UNICODE_NULL));
                StringCopy[CopyLength] = UNICODE_NULL;
                *Value = StringCopy;
                Status = STATUS_SUCCESS;
            }
            else
            {
                /* No memory, fail */
                Status = STATUS_NO_MEMORY;
            }
        }
    }

    /* All done */
    return Status;
}

NTSTATUS
BlGetBootOptionDevice (
    _In_ PBL_BCD_OPTION List,
    _In_ ULONG Type,
    _Out_ PBL_DEVICE_DESCRIPTOR* Value,
    _In_opt_ PBL_BCD_OPTION* ExtraOptions
    )
{
    NTSTATUS Status;
    PBL_BCD_OPTION Option, ListData, ListCopy, SecureListData;
    PBCD_DEVICE_OPTION BcdDevice;
    ULONG DeviceSize, ListOffset, ListSize;
    PBL_DEVICE_DESCRIPTOR DeviceDescriptor, SecureDescriptor;
    //PGUID AppIdentifier;

    /* Make sure this is a BCD_DEVICE */
    if ((Type & 0xF000000) != 0x1000000)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Return the data */
    Option = MiscGetBootOption(List, Type);
    if (!Option)
    {
        /* Set failure if no data exists */
        Status = STATUS_NOT_FOUND;
    }
    else
    {
        /* Otherwise, read the size of the BCD device encoded */
        BcdDevice = (PBCD_DEVICE_OPTION)((ULONG_PTR)Option + Option->DataOffset);
        DeviceSize = BcdDevice->DeviceDescriptor.Size;

        /* Allocate a buffer to copy it into */
        DeviceDescriptor = BlMmAllocateHeap(DeviceSize);
        if (!DeviceDescriptor)
        {
            return STATUS_NO_MEMORY;
        }

        /* Copy it into that buffer */
        RtlCopyMemory(DeviceDescriptor, &BcdDevice->DeviceDescriptor, DeviceSize);
        Status = STATUS_SUCCESS;
    }

    /* Check if extra options were requested */
    if (ExtraOptions)
    {
        /* See where they are */
        ListOffset = Option->ListOffset;
        if (ListOffset)
        {
            /* See how big they are */
            ListData = (PBL_BCD_OPTION)((ULONG_PTR)Option + ListOffset);
            ListSize = BlGetBootOptionListSize(ListData);

            /* Allocate a buffer to hold them into */
            ListCopy = BlMmAllocateHeap(ListSize);
            if (!ListCopy)
            {
                Status = STATUS_NO_MEMORY;
                goto Quickie;
            }

            /* Copy them in there */
            RtlCopyMemory(ListCopy, ListData, ListSize);
        }
    }

#ifdef _SECURE_BOOT_
    /* Filter out SecureBoot Options */
    AppIdentifier = BlGetApplicationIdentifier();
    if (BlpBootOptionCallbacks)
    {
        DeviceCallback = BlpBootOptionCallbacks->Device;
        if (DeviceCallback)
        {
            Status = DeviceCallback(BlpBootOptionCallbackCookie,
                                    Status,
                                    0,
                                    AppIdentifier,
                                    Type,
                                    &SecureDescriptor,
                                    PtrOptionData);
        }
    }
#else
    /* No secure boot, so the secure descriptors are the standard ones */
    SecureDescriptor = DeviceDescriptor;
    SecureListData = ListCopy;
#endif

    /* Check if the data was read correctly */
    if (NT_SUCCESS(Status))
    {
        /* Check if we had a new descriptor after filtering */
        if (SecureDescriptor != DeviceDescriptor)
        {
            /* Yep -- if we had an old one, free it */
            if (DeviceDescriptor)
            {
                BlMmFreeHeap(DeviceDescriptor);
            }
        }

        /* Check if we had a new list after filtering */
        if (SecureListData != ListCopy)
        {
            /* Yep -- if we had an old list, free it */
            if (ListCopy)
            {
                BlMmFreeHeap(ListCopy);
            }
        }

        /* Finally, check if the caller wanted extra options */
        if (ExtraOptions)
        {
            /* Yep -- so pass the caller our copy */
            *ExtraOptions = ListCopy;
            ListCopy = NULL;
        }

        /* Caller always wants data back, so pass them our copy */
        *Value = DeviceDescriptor;
        DeviceDescriptor = NULL;
    }

Quickie:
    /* On the failure path, if these buffers are active, we should free them */
    if (ListCopy)
    {
        BlMmFreeHeap(ListCopy);
    }
    if (DeviceDescriptor)
    {
        BlMmFreeHeap(DeviceDescriptor);
    }

    /* All done */
    return Status;
}

NTSTATUS
BlGetBootOptionInteger (
    _In_ PBL_BCD_OPTION List,
    _In_ ULONG Type,
    _Out_ PULONGLONG Value
    )
{
    NTSTATUS Status;
    PBL_BCD_OPTION Option;
    //PGUID AppIdentifier;

    /* Make sure this is a BCD_INTEGER */
    if ((Type & 0xF000000) != 0x5000000)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Return the data */
    Option = MiscGetBootOption(List, Type);
    if (Option)
    {
        *Value = *(PULONGLONG)((ULONG_PTR)Option + Option->DataOffset);
    }

#ifdef _SECURE_BOOT_
    /* Filter out SecureBoot Options */
    AppIdentifier = BlGetApplicationIdentifier();
    Status = BlpBootOptionCallbackULongLong(AppIdentifier, Type, Value);
#else
    /* Option found */
    Status = Option ? STATUS_SUCCESS : STATUS_NOT_FOUND;
#endif
    return Status;
}

NTSTATUS
BlGetBootOptionBoolean (
    _In_ PBL_BCD_OPTION List,
    _In_ ULONG Type,
    _Out_ PBOOLEAN Value
    )
{
    NTSTATUS Status;
    PBL_BCD_OPTION Option;
    //PGUID AppIdentifier;

    /* Make sure this is a BCD_BOOLEAN */
    if ((Type & 0xF000000) != 0x6000000)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Return the data */
    Option = MiscGetBootOption(List, Type);
    if (Option)
    {
        *Value = *(PBOOLEAN)((ULONG_PTR)Option + Option->DataOffset);
    }

#ifdef _SECURE_BOOT_
    /* Filter out SecureBoot Options */
    AppIdentifier = BlGetApplicationIdentifier();
    Status = BlpBootOptionCallbackBoolean(AppIdentifier, Type, Value);
#else
    /* Option found */
    Status = Option ? STATUS_SUCCESS : STATUS_NOT_FOUND;
#endif
    return Status;
}

#define BI_FLUSH_HIVE       0x01
#define BI_HIVE_WRITEABLE   0x02

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

FORCEINLINE
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
            //MmPapFreePages(KeyHive->ImageBase, 1);
            EfiPrintf(L"Leaking hive memory\r\n");

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
    ULONG NameLength, SubNameLength, NameBytes;
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

BOOLEAN BiHiveHashLibraryInitialized;
ULONGLONG HvSymcryptSeed;

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

NTSTATUS
BiInitializeAndValidateHive (
    _In_ PBI_KEY_HIVE Hive
    )
{
    ULONG HiveSize;
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
        CmPrepareHive(&Hive->Hive.Hive); // CmCheckRegistry 
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
        EfiPrintf(L"Leaking old hive buffer\r\n");
        //MmPapFreePages(BaseBlock, 1);

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
        EfiPrintf(L"Leaking log buffer\r\n");
        //MmPapFreePages(LogData, 1);
    }

    /* Check if this is the failure path */
    if (!NT_SUCCESS(Status))
    {
        /* If we mapped the hive, free it */
        if (BaseBlock)
        {
            EfiPrintf(L"Leaking base block on failure\r\n");
            //MmPapFreePages(BaseBlock, 1u);
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
BiAddStoreFromFile (
    _In_ PBL_FILE_PATH_DESCRIPTOR FilePath,
    _Out_ PHANDLE StoreHandle
    )
{
    NTSTATUS Status;
    HANDLE HiveHandle, KeyHandle;

    /* Load the specified hive */
    Status = BiLoadHive(FilePath, &HiveHandle);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Open the description key to make sure this is really a BCD */
    Status = BiOpenKey(HiveHandle, L"Description", &KeyHandle);
    if (NT_SUCCESS(Status))
    {
        /* It is -- close the key as we don't need it */
        BiCloseKey(KeyHandle);
        *StoreHandle = HiveHandle;
    }
    else
    {
        /* Failure, drop a reference on the hive and close the key */
        BiDereferenceHive(HiveHandle);
        BiCloseKey(HiveHandle);
    }

    /* Return the status */
    return Status;
}

NTSTATUS
BcdOpenStoreFromFile (
    _In_ PUNICODE_STRING FileName,
    _In_ PHANDLE StoreHandle
    )
{
    ULONG Length;
    PBL_FILE_PATH_DESCRIPTOR FilePath;
    NTSTATUS Status;
    HANDLE LocalHandle;

    /* Assume failure */
    LocalHandle = NULL;

    /* Allocate a path descriptor */
    Length = FileName->Length + sizeof(*FilePath);
    FilePath = BlMmAllocateHeap(Length);
    if (!FilePath)
    {
        return STATUS_NO_MEMORY;
    }

    /* Initialize it */
    FilePath->Version = 1;
    FilePath->PathType = InternalPath;
    FilePath->Length = Length;

    /* Copy the name and NULL-terminate it */
    RtlCopyMemory(FilePath->Path, FileName->Buffer, Length);
    FilePath->Path[Length / sizeof(WCHAR)] = UNICODE_NULL;

    /* Open the BCD */
    Status = BiAddStoreFromFile(FilePath, &LocalHandle);
    if (NT_SUCCESS(Status))
    {
        /* Return the handle on success */
        *StoreHandle = LocalHandle;
    }

    /* Free the descriptor and return the status */
    BlMmFreeHeap(FilePath);
    return Status;
}

