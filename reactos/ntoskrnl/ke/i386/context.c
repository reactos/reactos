/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/ke/i386/context.c
 * PURPOSE:         Context Switching Related Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
KiSwapProcess(IN PKPROCESS NewProcess,
              IN PKPROCESS OldProcess)
{
    PKIPCR Pcr = (PKIPCR)KeGetPcr();
#ifdef CONFIG_SMP
    ULONG SetMember;
    
    /* Update active processor mask */
    SetMember = Pcr->SetMember;
    InterlockedXor(NewProcess->ActiveProcessors, SetMember);
    InterlockedXor(OldProcess->ActiveProcessors, SetMember);
#endif

    /* Check for new LDT */
    if (NewProcess->LdtDescriptor.LimitLow != OldProcess->LdtDescriptor.LimitLow)
    {
        /* Not handled yet */
        UNIMPLEMENTED;
        while (TRUE);
    }
    
    /* Update CR3 */
    __writecr3(NewProcess->DirectoryTableBase[0]);
    
    /* Clear GS */
    Ke386SetGs(0);
    
    /* Update IOPM offset */
    Pcr->TSS->IoMapBase = NewProcess->IopmOffset;
}

