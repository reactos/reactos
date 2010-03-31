/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            hal/halx86/generic/acpi/halpcat.c
 * PURPOSE:         HAL Legacy Support Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* PRIVATE FUNCTIONS **********************************************************/

NTSTATUS
NTAPI
HalpSetupAcpiPhase0(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* There is no ACPI on these HALs */
    return STATUS_NO_SUCH_DEVICE;
}

/* EOF */
