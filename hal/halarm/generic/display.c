/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            hal/halarm/generic/display.c
 * PURPOSE:         Screen Display Routines, now useless since NT 5.1+
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include <hal.h>
#include <ndk/inbvfuncs.h>

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
VOID
NTAPI
HalAcquireDisplayOwnership(
    _In_ PHAL_RESET_DISPLAY_PARAMETERS ResetDisplayParameters)
{
    /* Stub since Windows XP implemented Inbv */
    return;
}

/*
 * @implemented
 */
VOID
NTAPI
HalDisplayString(
    _In_ PCSTR String)
{
    /* Call the Inbv driver */
    InbvDisplayString(String);
}

/*
 * @implemented
 */
VOID
NTAPI
HalQueryDisplayParameters(
    _Out_ PULONG DispSizeX,
    _Out_ PULONG DispSizeY,
    _Out_ PULONG CursorPosX,
    _Out_ PULONG CursorPosY)
{
    /* Stub since Windows XP implemented Inbv */
    return;
}

/*
 * @implemented
 */
VOID
NTAPI
HalSetDisplayParameters(
    _In_ ULONG CursorPosX,
    _In_ ULONG CursorPosY)
{
    /* Stub since Windows XP implemented Inbv */
    return;
}

/* EOF */
