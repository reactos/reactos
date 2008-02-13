/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/mm/arm/stubs.c
 * PURPOSE:         ARM Memory Manager
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>
/* GLOBALS ********************************************************************/

//
// METAFIXME: We need to stop using 1MB Section Entry TTEs!
//

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
MmUpdatePageDir(IN PEPROCESS Process,
                IN PVOID Address,
                IN ULONG Size)
{
    //
    // Nothing to do
    //
    UNIMPLEMENTED;
    return;
}
