/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Source file for Inter-Processor Interrupts management
 * COPYRIGHT:   Copyright 2023 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#include <smp.h>

/* GLOBALS *******************************************************************/

VOID
NTAPI
HalRequestIpi(
    _In_ KAFFINITY TargetProcessors)
{
    HalpRequestIpi(TargetProcessors);
}
