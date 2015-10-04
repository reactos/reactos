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
    PBL_BCD_OPTION Option;

    /* No options, bail out */
    if (!List)
    {
        return NULL;
    }

    /* Loop while we find an option */
    while (TRUE)
    {
        /* Get the next option and see if it matches the type */
        Option = (PBL_BCD_OPTION)((ULONG_PTR)List + NextOption);
        if ((Option->Type == Type) && !(Option->Empty))
        {
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

            /* Found one, return it */
            if (Option)
            {
                return Option;
            }
        }
    }

    /* We found the option, return it */
    return Option;
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
    } while (NextOffset != 0);

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
    if (BcdOption->DataOffset != 0)
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
    if (Offset != 0)
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
    }
    else
    {
        /* No string is present */
        String = NULL;
    }

    /* Compute the data size */
    StringLength = Option->DataSize / sizeof(WCHAR);

#ifdef _SECURE_BOOT_
    /* Filter out SecureBoot Options */
    AppIdentifier = BlGetApplicationIdentifier();
    Status = BlpBootOptionCallbackString(AppIdentifier, Type, String, StringLength, &String, &StringLength);
#else
    Status = STATUS_SUCCESS;
#endif

    /* Check if we have space for one more character */
    CopyLength = StringLength + 1;
    if (CopyLength < StringLength)
    {
        /* Nope, we'll overflow */
        CopyLength = -1;
        Status = STATUS_INTEGER_OVERFLOW;
    }
    else
    {
        /* Go ahead */
        Status = STATUS_SUCCESS;
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
    Status = STATUS_SUCCESS;
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

#define BI_FLUSH_HIVE   0x01

typedef struct _BI_KEY_HIVE
{
    PVOID ImageBase;
    PBL_FILE_PATH_DESCRIPTOR FilePath;
    CMHIVE Hive;
    LONG ReferenceCount;
    ULONG Flags;
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
        ParentNode = (PCM_KEY_NODE)Hive->GetCellRoutine(Hive, KeyCell);
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
BiLoadHive (
    _In_ PBL_FILE_PATH_DESCRIPTOR FilePath,
    _Out_ PHANDLE HiveHandle
    )
{
    /* This is TODO */
    EfiPrintf(L"Loading a hive is not yet implemented\r\n");
    *HiveHandle = NULL;
    return STATUS_NOT_IMPLEMENTED;
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

