/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Source file for Inter-Processor Interrupts management
 * COPYRIGHT:   Copyright 2023 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#include <smp.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

VOID
NTAPI
HalRequestIpi(
    _In_ KAFFINITY TargetProcessors)
{
    HalpRequestIpi(TargetProcessors);
}

#ifdef _M_AMD64

VOID
NTAPI
HalSendNMI(
    _In_ KAFFINITY TargetSet)
{
    HalpSendNMI(TargetSet);
}

// See:
// - https://www.virtualbox.org/browser/vbox/trunk/src/VBox/Runtime/r0drv/nt/internal-r0drv-nt.h#L53
// - https://github.com/mirror/vbox/blob/b9657cd5351cf17432b664009cc25bb480dc64c1/src/VBox/Runtime/r0drv/nt/mp-r0drv-nt.cpp#L683
VOID
NTAPI
HalSendSoftwareInterrupt(
    _In_ KAFFINITY TargetSet,
    _In_ KIRQL Irql)
{
    HalpSendSoftwareInterrupt(TargetSet, Irql);
}

#endif // _M_AMD64
