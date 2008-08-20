/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Routines for IRQL-level support
 * PROGRAMMERS:     Timo Kreuzer
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#undef UNIMPLEMENTED

#define UNIMPLEMENTED \
 FrLdrDbgPrint("Sorry, %s is unimplemented!\n", __FUNCTION__)

/* FUNCTIONS ****************************************************************/

#undef KeGetCurrentIrql
NTKERNELAPI
KIRQL
KeGetCurrentIrql(VOID)
{
    UNIMPLEMENTED;
    return 0;
}


NTKERNELAPI
VOID
KfLowerIrql(IN KIRQL NewIrql)
{
    UNIMPLEMENTED;
}

NTKERNELAPI
KIRQL
KfRaiseIrql(IN KIRQL NewIrql)
{
    UNIMPLEMENTED;
    return 0;
}

NTKERNELAPI
KIRQL
KeRaiseIrqlToDpcLevel(VOID)
{
    UNIMPLEMENTED;
    return 0;
}

NTKERNELAPI
KIRQL
KeRaiseIrqlToSynchLevel(VOID)
{
    UNIMPLEMENTED;
    return 0;
}

NTKERNELAPI
VOID
KeLowerIrql(IN KIRQL NewIrql)
{
    UNIMPLEMENTED;
    return 0;
}

/* EOF */
