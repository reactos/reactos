/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/ke/i386/patpge.c
* PURPOSE:         Support for PAT and PGE (Large Pages)
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

ULONG Ke386GlobalPagesEnabled;

/* FUNCTIONS *****************************************************************/

ULONG_PTR
NTAPI
Ki386EnableGlobalPage(IN volatile ULONG_PTR Context)
{
    volatile PLONG Count = (PLONG)Context;
    ULONG Cr4, Cr3;

    /* Disable interrupts */
    _disable();

    /* Decrease CPU Count and loop until it's reached 0 */
    do {InterlockedDecrement(Count);} while (!*Count);

    /* Now check if this is the Boot CPU */
    if (!KeGetPcr()->Number)
    {
        /* It is.FIXME: Patch KeFlushCurrentTb */
    }

    /* Now get CR4 and make sure PGE is masked out */
    Cr4 = Ke386GetCr4();
    Ke386SetCr4(Cr4 & ~CR4_PGE);

    /* Flush the TLB */
    Ke386GetPageTableDirectory(Cr3);
    Ke386SetPageTableDirectory(Cr3);

    /* Now enable PGE */
    Ke386SetCr4(Cr4 | CR4_PGE);
    Ke386GlobalPagesEnabled = TRUE;

    /* Restore interrupts */
    _enable();
    return 0;
}

VOID
NTAPI
KiInitializePAT(VOID)
{
    /* FIXME: Support this */
    DPRINT1("Your machine supports PAT but ReactOS doesn't yet.\n");
}
