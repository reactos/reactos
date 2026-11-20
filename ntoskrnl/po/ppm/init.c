/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Processor Power Management Initialization Code
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/

CODE_SEG("INIT")
NTSTATUS
NTAPI
PpmInitialize(
    _In_ BOOLEAN EarlyPhase)
{
    /* FIXME */
    UNREFERENCED_PARAMETER(EarlyPhase);
    return STATUS_SUCCESS;
}

/* EOF */
