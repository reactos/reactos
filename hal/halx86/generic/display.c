/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/generic/display.c
 * PURPOSE:         Screen Display Routines, now useless since NT 5.1+
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

#include <hal.h>
#define NDEBUG
#include <debug.h>
#include <ndk/inbvfuncs.h>

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
VOID
NTAPI
HalAcquireDisplayOwnership(IN PHAL_RESET_DISPLAY_PARAMETERS ResetDisplayParameters)
{
    /* Stub since Windows XP implemented Inbv */
    return;
}

/*
 * @implemented
 */
VOID
NTAPI
HalDisplayString(IN PCH String)
{
    /* Call the Inbv driver */
    InbvDisplayString(String);
}

/*
 * @implemented
 */
VOID
NTAPI
HalQueryDisplayParameters(OUT PULONG DispSizeX,
                          OUT PULONG DispSizeY,
                          OUT PULONG CursorPosX,
                          OUT PULONG CursorPosY)
{
    /* Stub since Windows XP implemented Inbv */
    return;
}

/*
 * @implemented
 */
VOID
NTAPI
HalSetDisplayParameters(IN ULONG CursorPosX,
                        IN ULONG CursorPosY)
{
    /* Stub since Windows XP implemented Inbv */
    return;
}

/* EOF */
