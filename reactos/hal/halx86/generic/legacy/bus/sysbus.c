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
HalpGetRootInterruptVector(IN ULONG BusInterruptLevel,
                           IN ULONG BusInterruptVector,
                           OUT PKIRQL Irql,
                           OUT PKAFFINITY Affinity)
{
    ULONG SystemVector;
    
    /* Get the system vector */
    SystemVector = PRIMARY_VECTOR_BASE + BusInterruptLevel;
    
    /* Validate it */
    if ((SystemVector < PRIMARY_VECTOR_BASE) || (SystemVector > PRIMARY_VECTOR_BASE + 27))
    {
        /* Invalid vector */
        DPRINT1("Vector %lx is too low or too high!\n", SystemVector);
        return 0;
    }
    
    /* Return the IRQL and affinity */
    *Irql = (PRIMARY_VECTOR_BASE + 27) - SystemVector;
    *Affinity = HalpDefaultInterruptAffinity;
    ASSERT(HalpDefaultInterruptAffinity);
    
    /* Return the vector */
    return SystemVector;
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
    ULONG Vector;
    
    /* Get the root vector */
    Vector = HalpGetRootInterruptVector(BusInterruptLevel,
                                        BusInterruptVector,
                                        Irql,
                                        Affinity);
    
    /* Check if the vector is owned by the HAL and fail if it is */
    if (HalpIDTUsageFlags[Vector].Flags & IDT_REGISTERED) DPRINT1("Vector %lx is ALREADY IN USE!\n", Vector);
    return (HalpIDTUsageFlags[Vector].Flags & IDT_REGISTERED) ? 0 : Vector;
}

/* EOF */
