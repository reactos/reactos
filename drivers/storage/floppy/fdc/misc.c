/*
 * PROJECT:        ReactOS Floppy Disk Controller Driver
 * LICENSE:        GNU GPLv2 only as published by the Free Software Foundation
 * FILE:           drivers/storage/fdc/fdc/misc.c
 * PURPOSE:        Misc Routines
 * PROGRAMMERS:    Eric Kohl
 */

/* INCLUDES *******************************************************************/

#include "fdc.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

NTSTATUS
DuplicateUnicodeString(
    IN ULONG Flags,
    IN PCUNICODE_STRING SourceString,
    OUT PUNICODE_STRING DestinationString)
{
    USHORT DestMaxLength;

    if (SourceString == NULL ||
        DestinationString == NULL ||
        SourceString->Length > SourceString->MaximumLength ||
        (SourceString->Length == 0 && SourceString->MaximumLength > 0 && SourceString->Buffer == NULL) ||
        Flags == RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING ||
        Flags >= 4)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ((SourceString->Length == 0) &&
        (Flags != (RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE |
                   RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING)))
    {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
        DestinationString->Buffer = NULL;
    }
    else
    {
        DestMaxLength = SourceString->Length;

        if (Flags & RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE)
            DestMaxLength += sizeof(UNICODE_NULL);

        DestinationString->Buffer = ExAllocatePoolWithTag(PagedPool, DestMaxLength, FDC_TAG);
        if (DestinationString->Buffer == NULL)
            return STATUS_NO_MEMORY;

        RtlCopyMemory(DestinationString->Buffer, SourceString->Buffer, SourceString->Length);
        DestinationString->Length = SourceString->Length;
        DestinationString->MaximumLength = DestMaxLength;

        if (Flags & RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE)
            DestinationString->Buffer[DestinationString->Length / sizeof(WCHAR)] = 0;
    }

    return STATUS_SUCCESS;
}

/* EOF */
