/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/generic/usage.c
 * PURPOSE:         HAL Resource Report Routines
 * PROGRAMMERS:     
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PUCHAR KdComPortInUse;

/* FUNCTIONS ******************************************************************/
#if 0
VOID
NTAPI
HalpReportResourceUsage(IN PUNICODE_STRING HalName,
                        IN INTERFACE_TYPE InterfaceType)
{
    DbgPrint("%wZ has been initialized\n", HalName);
}

VOID
NTAPI
HalpRegisterVector(IN UCHAR Flags,
                   IN ULONG BusVector,
                   IN ULONG SystemVector,
                   IN KIRQL Irql)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
HalpEnableInterruptHandler(IN UCHAR Flags,
                           IN ULONG BusVector,
                           IN ULONG SystemVector,
                           IN KIRQL Irql,
                           IN PVOID Handler,
                           IN KINTERRUPT_MODE Mode)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
NTAPI
HalReportResourceUsage(VOID)
{
    UNIMPLEMENTED;
}
#endif

