/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/generic/bus/isabus.c
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
HalpTranslateIsaBusAddress(IN PBUS_HANDLER BusHandler,
                           IN PBUS_HANDLER RootHandler, 
                           IN PHYSICAL_ADDRESS BusAddress,
                           IN OUT PULONG AddressSpace,
                           OUT PPHYSICAL_ADDRESS TranslatedAddress)
{
    DPRINT1("ISA Translate\n");
    while (TRUE);
    return FALSE;
}

/* EOF */
