/*
* PROJECT:         Filesystem Filter Manager
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            drivers/filters/fltmgr/Lib.c
* PURPOSE:         Miscellaneous library functions
* PROGRAMMERS:     Ged Murphy (gedmurphy@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include "fltmgr.h"

#define NDEBUG
#include <debug.h>


/* DATA *********************************************************************/



/* FUNCTIONS **********************************************/

VOID
FltpFreeUnicodeString(_In_ PUNICODE_STRING String)
{
    /* Free up any existing buffer */
    if (String->Buffer)
    {
        ExFreePoolWithTag(String->Buffer, FM_TAG_UNICODE_STRING);
    }

    /* Empty the string */
    String->Buffer = NULL;
    String->Length = 0;
    String->MaximumLength = 0;
}

NTSTATUS
FltpReallocateUnicodeString(_In_ PUNICODE_STRING String,
                            _In_ SIZE_T NewLength,
                            _In_ BOOLEAN CopyExisting)
{
    PWCH NewBuffer;

    /* Don't bother reallocating if the buffer is smaller */
    if (NewLength <= String->MaximumLength)
    {
        String->Length = 0;
        return STATUS_SUCCESS;
    }

    /* Allocate a new buffer at the size requested */
    NewBuffer = ExAllocatePoolWithTag(PagedPool, NewLength, FM_TAG_UNICODE_STRING);
    if (NewBuffer == NULL) return STATUS_INSUFFICIENT_RESOURCES;

    if (CopyExisting)
    {
        /* Copy the old data across */
        RtlCopyMemory(NewBuffer, String->Buffer, String->Length);
    }
    else
    {
        /* Reset the length */
        String->Length = 0;
    }

    /* Free any old buffer */
    if (String->Buffer)
        ExFreePoolWithTag(String->Buffer, FM_TAG_UNICODE_STRING);

    /* Update the lengths */
    String->Buffer = NewBuffer;
    String->MaximumLength = NewLength;

    return STATUS_SUCCESS;
}

NTSTATUS
FltpCopyUnicodeString(_In_ PUNICODE_STRING StringOne,
                      _In_ PUNICODE_STRING StringTwo)
{
    return 0;
}
