/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/generic/bus/sysbus.c
 * PURPOSE:
 * PROGRAMMERS:     Stefan Ginsberg (stefan.ginsberg@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* PRIVATE FUNCTIONS **********************************************************/

BOOLEAN
NTAPI
HalpTranslateSystemBusAddress(IN PBUS_HANDLER BusHandler,
                              IN PBUS_HANDLER RootHandler, 
                              IN PHYSICAL_ADDRESS BusAddress,
                              IN OUT PULONG AddressSpace,
                              OUT PPHYSICAL_ADDRESS TranslatedAddress)
{
    DPRINT1("SYSTEM Translate\n");
    while (TRUE);
    return FALSE;
}

ULONG
NTAPI
HalpGetSystemInterruptVector(IN PBUS_HANDLER BusHandler,
                             IN PBUS_HANDLER RootHandler,
                             IN ULONG BusInterruptLevel,
                             IN ULONG BusInterruptVector,
                             OUT PKIRQL Irql,
                             OUT PKAFFINITY Affinity)
{
    /* Get the root vector */
    DPRINT1("SYSTEM GetVector\n");
    while (TRUE);
    return 0;
}

/* EOF */
