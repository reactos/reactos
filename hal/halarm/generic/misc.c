/*
 * PROJECT:         ReactOS Hardware Abstraction Layer (HAL)
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            hal/halarm/generic/misc.c
 * PURPOSE:         Misc functions to move
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS  *******************************************************************/

/* FUNCTIONS ******************************************************************/

/*
 * @implemented
 */
UCHAR
FASTCALL
HalSystemVectorDispatchEntry(IN ULONG Vector,
                             OUT PKINTERRUPT_ROUTINE **FlatDispatch,
                             OUT PKINTERRUPT_ROUTINE *NoConnection)
{
    /* Not implemented */
    UNIMPLEMENTED;
    while (TRUE);
    return 0;
}

/*
 * @implemented
 */
VOID
NTAPI
KeFlushWriteBuffer(VOID)
{
    /* Not implemented */
    UNIMPLEMENTED;
    while (TRUE);
    return;
}

/* EOF */
