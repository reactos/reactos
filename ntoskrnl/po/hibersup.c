/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Power system hibernation infrastructure support
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/

VOID
NTAPI
PoSetHiberRange(
    _In_ PVOID HiberContext,
    _In_ ULONG Flags,
    _In_ OUT PVOID StartPage,
    _In_ ULONG Length,
    _In_ ULONG PageTag)
{
    /* FIXME */
    UNIMPLEMENTED;
    NOTHING;
}

/* EOF */
