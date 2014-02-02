/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/ke/i386/mtrr.c
* PURPOSE:         Support for MTRR and AMD K6 MTRR
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
INIT_FUNCTION
KiInitializeMTRR(IN BOOLEAN FinalCpu)
{
    /* FIXME: Support this */
    DPRINT("MTRR support detected but not yet taken advantage of\n");
}

VOID
NTAPI
INIT_FUNCTION
KiAmdK6InitializeMTRR(VOID)
{
    /* FIXME: Support this */
    DPRINT("AMD MTRR support detected but not yet taken advantage of\n");
}
