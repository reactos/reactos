/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/misc/bcdopt.c
 * PURPOSE:         Boot Library BCD Option Parsing Routines
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
    ULONG StringLength;
    BcdElementType ElementType;
    //PGUID AppIdentifier;

    /* Make sure this is a BCD_STRING */
    ElementType.PackedValue = Type;
    if (ElementType.Format != BCD_TYPE_STRING)
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

    /* Make sure we have a valid, non-filtered string */
    if (NT_SUCCESS(Status))
    {
        /* Check if we have space for one more character */
        Status = RtlULongAdd(StringLength, 1, &StringLength);
        if (NT_SUCCESS(Status))
        {
            /* Check if it's safe to multiply by two */
            Status = RtlULongMult(StringLength, sizeof(WCHAR), &StringLength);
            if (NT_SUCCESS(Status))
            {
                /* Allocate a copy for the string */
                StringCopy = BlMmAllocateHeap(StringLength);
                if (StringCopy)
                {
                    /* NULL-terminate it */
                    RtlCopyMemory(StringCopy,
                                  String,
                                  StringLength - sizeof(UNICODE_NULL));
                    StringCopy[StringLength] = UNICODE_NULL;
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
    }

    /* All done */
    return Status;
}

NTSTATUS
BlGetBootOptionGuidList (
    _In_ PBL_BCD_OPTION List,
    _In_ ULONG Type,
    _Out_ PGUID *Value,
    _In_ PULONG Count
    )
{
    NTSTATUS Status;
    PBL_BCD_OPTION Option;
    PGUID GuidCopy, Guid;
    ULONG GuidCount;
    BcdElementType ElementType;

    /* Make sure this is a BCD_TYPE_OBJECT_LIST */
    ElementType.PackedValue = Type;
    if (ElementType.Format != BCD_TYPE_OBJECT_LIST)
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
        /* Get the GUIDs and allocate a copy for them */
        Guid = (PGUID)((ULONG_PTR)Option + Option->DataOffset);
        GuidCopy = BlMmAllocateHeap(Option->DataSize);
        if (GuidCopy)
        {
            /* Copy the GUIDs */
            RtlCopyMemory(GuidCopy, Guid, Option->DataSize);

            /* Return the number of GUIDs and the start of the array */
            GuidCount = Option->DataSize / sizeof(GUID);
            *Value = GuidCopy;
            *Count = GuidCount;
            Status = STATUS_SUCCESS;
        }
        else
        {
            /* No memory for the copy */
            Status = STATUS_NO_MEMORY;
        }
    }

    /* All good */
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
    BcdElementType ElementType;

    /* Make sure this is a BCD_TYPE_DEVICE */
    ElementType.PackedValue = Type;
    if (ElementType.Format != BCD_TYPE_DEVICE)
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
    BcdElementType ElementType;

    /* Make sure this is a BCD_TYPE_INTEGER */
    ElementType.PackedValue = Type;
    if (ElementType.Format != BCD_TYPE_INTEGER)
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
    BcdElementType ElementType;

    /* Make sure this is a BCD_TYPE_BOOLEAN */
    ElementType.PackedValue = Type;
    if (ElementType.Format != BCD_TYPE_BOOLEAN)
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

NTSTATUS
BlpGetBootOptionIntegerList (
    _In_ PBL_BCD_OPTION List,
    _In_ ULONG Type,
    _Out_ PULONGLONG* Value,
    _Out_ PULONGLONG Count,
    _In_ BOOLEAN NoCopy
    )
{
    PBL_BCD_OPTION Option;
    BcdElementType ElementType;
    PULONGLONG ValueCopy;

    /* Make sure this is a BCD_TYPE_INTEGER_LIST */
    ElementType.PackedValue = Type;
    if (ElementType.Format != BCD_TYPE_INTEGER_LIST)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Return the data */
    Option = MiscGetBootOption(List, Type);
    if (!Option)
    {
        return STATUS_NOT_FOUND;
    }

    /* Check if a copy should be made of it */
    if (NoCopy)
    {
        /* Nope, return the raw value */
        *Value = (PULONGLONG)((ULONG_PTR)Option + Option->DataOffset);
    }
    else
    {
        /* Allocate a buffer for the copy */
        ValueCopy = BlMmAllocateHeap(Option->DataSize);
        if (!ValueCopy)
        {
            return STATUS_NO_MEMORY;
        }

        /* Copy the data in */
        RtlCopyMemory(ValueCopy,
                      (PVOID)((ULONG_PTR)Option + Option->DataOffset),
                      Option->DataSize);

        /* Return our copy */
        *Value = ValueCopy;
    }

    /* Return count and success */
    *Count = Option->DataSize / sizeof(ULONGLONG);
    return STATUS_SUCCESS;
}

NTSTATUS
BlCopyBootOptions (
    _In_ PBL_BCD_OPTION OptionList,
    _Out_ PBL_BCD_OPTION *CopiedOptions
    )
{
    NTSTATUS Status;
    ULONG OptionSize;
    PBL_BCD_OPTION Options;

    /* Assume no options */
    Status = STATUS_SUCCESS;
    *CopiedOptions = NULL;

    /* Get the size of the list and allocate a copy for it */
    OptionSize = BlGetBootOptionListSize(OptionList);
    Options = BlMmAllocateHeap(OptionSize);
    if (!Options)
    {
        return STATUS_NO_MEMORY;
    }

    /* Make the copy and return it to the caller */
    RtlCopyMemory(Options, OptionList, OptionSize);
    *CopiedOptions = Options;
    return Status;
}

NTSTATUS
BlAppendBootOptions (
    _In_ PBL_LOADED_APPLICATION_ENTRY AppEntry,
    _In_ PBL_BCD_OPTION Options
    )
{
    ULONG OptionsSize, CurrentSize;
    PBL_BCD_OPTION NewOptions, CurrentOptions, NextOption;
    NTSTATUS Status;
    ULONG CurrentOffset;

    /* Get the current options */
    CurrentOptions = AppEntry->BcdData;

    /* Calculate the size of the current, and the appended options */
    CurrentSize = BlGetBootOptionListSize(CurrentOptions);
    OptionsSize = BlGetBootOptionListSize(Options);

    /* Allocate a buffer for the concatenated (new) options */
    NewOptions = BlMmAllocateHeap(CurrentSize + OptionsSize);
    if (!NewOptions)
    {
        return STATUS_NO_MEMORY;
    }

    /* Copy the old options, and the ones to be added */
    RtlCopyMemory(NewOptions, CurrentOptions, CurrentSize);
    RtlCopyMemory(&NewOptions[OptionsSize], Options, OptionsSize);

    /* We made it! */
    Status = STATUS_SUCCESS;

    /* Scan through to the last option in the list */
    CurrentOffset = 0;
    do
    {
        NextOption = (PBL_BCD_OPTION)((ULONG_PTR)NewOptions + CurrentOffset);
        CurrentOffset = NextOption->NextEntryOffset;
    } while (CurrentOffset);

    /* Every other option now has to have its offset adjusted */
    do
    {
        NextOption->NextEntryOffset += OptionsSize;
        NextOption = (PBL_BCD_OPTION)((ULONG_PTR)NewOptions + NextOption->NextEntryOffset);
    } while (NextOption->NextEntryOffset);

    /* If we already had internal options, free them */
    if (AppEntry->Flags & BL_APPLICATION_ENTRY_BCD_OPTIONS_INTERNAL)
    {
        BlMmFreeHeap(AppEntry->BcdData);
    }

    /* Write the new pointer */
    AppEntry->BcdData = NewOptions;

    /* Options are now internal, not external */
    AppEntry->Flags &= ~BL_APPLICATION_ENTRY_BCD_OPTIONS_EXTERNAL;
    AppEntry->Flags |= BL_APPLICATION_ENTRY_BCD_OPTIONS_INTERNAL;
    return Status;
}

