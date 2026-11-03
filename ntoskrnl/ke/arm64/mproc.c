/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 multiprocessor startup
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/* Secondary processors come up through PSCI CPU_ON, the HAL does not offer it yet */
CODE_SEG("INIT")
VOID
NTAPI
KeStartAllProcessors(VOID)
{
    UNIMPLEMENTED;
}
