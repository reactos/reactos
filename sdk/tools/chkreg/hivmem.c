/*
 * PROJECT:     ReactOS Tools
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Check Registry Utility I/O hive memory operation routines
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "chkreg.h"

/* FUNCTIONS ****************************************************************/

PVOID
NTAPI
CmpAllocate(
    IN SIZE_T Size,
    IN BOOLEAN Paged,
    IN ULONG Tag)
{
    PVOID Buffer;

    Buffer = (PVOID)malloc((size_t)Size);
    if (Buffer == NULL)
    {
        return NULL;
    }

    return Buffer;
}

VOID
NTAPI
CmpFree(
    IN PVOID Ptr,
    IN ULONG Quota)
{
    if (Ptr != NULL)
    {
        free(Ptr);
    }
}

/* EOF */
