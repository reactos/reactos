/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            hal/halarm/generic/usage.c
 * PURPOSE:         Resource Usage Management Routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PUCHAR KdComPortInUse;

IDTUsageFlags HalpIDTUsageFlags[256];
IDTUsage HalpIDTUsage[256];

/* PRIVATE FUNCTIONS **********************************************************/

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
    /* Save the vector flags */
    HalpIDTUsageFlags[SystemVector].Flags = Flags;

    /* Save the vector data */
    HalpIDTUsage[SystemVector].Irql  = Irql;
    HalpIDTUsage[SystemVector].BusReleativeVector = BusVector;
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
    /* Register the routine */
    KeGetPcr()->InterruptRoutine[Irql] = Handler;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @unimplemented
 */
VOID
NTAPI
HalReportResourceUsage(VOID)
{
    UNICODE_STRING HalString;

    /* Build HAL usage */
    RtlInitUnicodeString(&HalString, L"ARM Versatile HAL");
    HalpReportResourceUsage(&HalString, Internal);
}

/* EOF */
