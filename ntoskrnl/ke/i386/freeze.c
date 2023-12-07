/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Processor freeze support for i386
 * COPYRIGHT:
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

VOID
NTAPI
KxFreezeExecution(
    VOID)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
KxThawExecution(
    VOID)
{
    UNIMPLEMENTED;
}

KCONTINUE_STATUS
NTAPI
KxSwitchKdProcessor(
    _In_ ULONG ProcessorIndex)
{
    UNIMPLEMENTED;
    return ContinueError;
}
