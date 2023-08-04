/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Power Manager miscellaneous utility routines
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS ***********************************************************/

PVOID
NTAPI
PopAllocatePool(
    _In_ SIZE_T PoolSize,
    _In_ BOOLEAN Paged,
    _In_ ULONG Tag)
{
    PVOID Buffer;
    BOOLEAN UseDefaultTag = FALSE;

    /* Avoid zero pool allocations */
    ASSERT(PoolSize != 0);

    /* Use the default tag if none was provided */
    if (Tag == 0)
    {
        UseDefaultTag = TRUE;
    }

    Buffer = ExAllocatePoolZero(Paged ? PagedPool : NonPagedPool,
                                PoolSize,
                                UseDefaultTag ? TAG_PO : Tag);
    if (Buffer == NULL)
    {
        return NULL;
    }

    return Buffer;
}

VOID
NTAPI
PopFreePool(
    _In_ _Post_invalid_ PVOID PoolBuffer,
    _In_ ULONG Tag)
{
    ASSERT(PoolBuffer != NULL);
    ExFreePoolWithTag(PoolBuffer, Tag);
}

VOID
NTAPI
PoShutdownBugCheck(
    _In_ BOOLEAN LogError,
    _In_ ULONG BugCheckCode,
    _In_ ULONG_PTR BugCheckParameter1,
    _In_ ULONG_PTR BugCheckParameter2,
    _In_ ULONG_PTR BugCheckParameter3,
    _In_ ULONG_PTR BugCheckParameter4)
{
    /* FIXME */
    UNIMPLEMENTED;
    NOTHING;
}

/* EOF */
