/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/misc/bcd.c
 * PURPOSE:         Boot Library BCD Routines
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"

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
