/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/legacy/bus/sysbus.c
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
    PSUPPORTED_RANGE Range = NULL;
    
    /* Check what kind of address space this is */
    switch (*AddressSpace)
    {
        /* Memory address */
        case 0:
            
            /* Loop all prefetch memory */
            for (Range = &BusHandler->BusAddresses->PrefetchMemory;
                 Range;
                 Range = Range->Next)
            {
                /* Check if it's in a valid range */
                if ((BusAddress.QuadPart >= Range->Base) &&
                    (BusAddress.QuadPart <= Range->Limit))
                {
                    /* Get out */
                    break;
                }
            }
            
            /* Check if we haven't found anything yet */
            if (!Range)
            {
                /* Loop all bus memory */
                for (Range = &BusHandler->BusAddresses->Memory;
                     Range;
                     Range = Range->Next)
                {
                    /* Check if it's in a valid range */
                    if ((BusAddress.QuadPart >= Range->Base) &&
                        (BusAddress.QuadPart <= Range->Limit))
                    {
                        /* Get out */
                        break;
                    }
                }
            }
            
            /* Done */
            break;
            
        /* I/O Space */
        case 1:

            /* Loop all bus I/O memory */
            for (Range = &BusHandler->BusAddresses->IO;
                 Range;
                 Range = Range->Next)
            {
                /* Check if it's in a valid range */
                if ((BusAddress.QuadPart >= Range->Base) &&
                    (BusAddress.QuadPart <= Range->Limit))
                {
                    /* Get out */
                    break;
                }
            }
            
            /* Done */
            break;
    }
    
    /* Check if we found a range */
    if (Range)
    {
        /* Do the translation and return the kind of address space this is */
        TranslatedAddress->QuadPart = BusAddress.QuadPart + Range->SystemBase;
        if ((TranslatedAddress->QuadPart != BusAddress.QuadPart) ||
            (*AddressSpace != Range->SystemAddressSpace))
        {
            /* Different than what the old HAL would do */
            DPRINT1("Translation of %I64x is %I64x %s\n",
                    BusAddress.QuadPart, TranslatedAddress->QuadPart,
                    Range->SystemAddressSpace ? "In I/O Space" : "In RAM");
        }
        *AddressSpace = Range->SystemAddressSpace;
        return TRUE;
    }
    
    /* Nothing found */
    DPRINT1("Translation of %I64x failed!\n", BusAddress.QuadPart);
    return FALSE;
}

ULONG
NTAPI
HalpGetRootInterruptVector(IN ULONG BusInterruptLevel,
                           IN ULONG BusInterruptVector,
                           OUT PKIRQL Irql,
                           OUT PKAFFINITY Affinity)
{
    UCHAR SystemVector;

    /* Validate the IRQ */
    if (BusInterruptLevel > 23)
    {
        /* Invalid vector */
        DPRINT1("IRQ %lx is too high!\n", BusInterruptLevel);
        return 0;
    }

    /* Get the system vector */
    SystemVector = HalpIrqToVector((UCHAR)BusInterruptLevel);

    /* Return the IRQL and affinity */
    *Irql = HalpVectorToIrql(SystemVector);
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
