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

ULONG
NTAPI
HalpcGetCmosData(IN PBUS_HANDLER BusHandler,
                 IN PBUS_HANDLER RootHandler,
                 IN ULONG SlotNumber,
                 IN PVOID Buffer,
                 IN ULONG Offset,
                 IN ULONG Length)
{
    DPRINT1("CMOS GetData\n");
    while (TRUE);
    return 0;
}

ULONG
NTAPI
HalpcSetCmosData(IN PBUS_HANDLER BusHandler,
                 IN PBUS_HANDLER RootHandler,
                 IN ULONG SlotNumber,
                 IN PVOID Buffer,
                 IN ULONG Offset,
                 IN ULONG Length)
{
    DPRINT1("CMOS SetData\n");
    while (TRUE);
    return 0;
}

/* EOF */
