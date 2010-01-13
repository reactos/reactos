/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/config/arm/cmhardwr.c
 * PURPOSE:         Configuration Manager - ARM Specific Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
#define NDEBUG
#include "debug.h"

/* GLOBALS *******************************************************************/

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
CmpInitializeMachineDependentConfiguration(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    //
    // Nothing to do on ARM
    //
    return STATUS_SUCCESS;
}
