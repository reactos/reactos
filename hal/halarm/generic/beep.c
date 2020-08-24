/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * PURPOSE:         Speaker support (beeping)
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOLEAN
NTAPI
HalMakeBeep(IN ULONG Frequency)
{
    UNIMPLEMENTED;
    return FALSE;
}
