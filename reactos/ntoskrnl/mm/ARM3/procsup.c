/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/procsup.c
 * PURPOSE:         ARM Memory Manager Process Related Management
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#line 15 "ARM³::PROCSUP"
#define MODULE_INVOLVED_IN_ARM3
#include "../ARM3/miarm.h"

ULONG PagesForStacks = 0;

/* PRIVATE FUNCTIONS **********************************************************/

VOID
NTAPI
MmDeleteKernelStack(IN PVOID StackBase,
                    IN BOOLEAN GuiStack)
{
    PMMPTE PointerPte;
    PFN_NUMBER StackPages;
    ULONG i;
    
    //
    // This should be the guard page, so decrement by one
    //
    PointerPte = MiAddressToPte(StackBase);
    PointerPte--;
    
    //
    // Calculate pages used
    //
    StackPages = BYTES_TO_PAGES(GuiStack ?
                                KERNEL_LARGE_STACK_SIZE : KERNEL_STACK_SIZE);
    
    //
    // Loop them
    //
    for (i = 0; i < StackPages; i++)
    {
        //
        // Check if this is a valid PTE
        //
        if (PointerPte->u.Hard.Valid == 1)
        {
            //
            // Nuke it
            //
            MmReleasePageMemoryConsumer(MC_NPPOOL, PFN_FROM_PTE(PointerPte));
            PagesForStacks--;
        }
        
        //
        // Next one
        //
        PointerPte--;
    }
    
    //
    // We should be at the guard page now
    //
    ASSERT(PointerPte->u.Hard.Valid == 0);
    
    //
    // Release the PTEs
    //
    MiReleaseSystemPtes(PointerPte, StackPages + 1, SystemPteSpace);
}

PVOID
NTAPI
MmCreateKernelStack(IN BOOLEAN GuiStack,
                    IN UCHAR Node)
{
    PFN_NUMBER StackPtes, StackPages;
    PMMPTE PointerPte, StackPte;
    PVOID BaseAddress;
    MMPTE TempPte;
    KIRQL OldIrql;
    PFN_NUMBER PageFrameIndex;
    ULONG i;
    
    //
    // Calculate pages needed
    //
    if (GuiStack)
    {
        //
        // We'll allocate 64KB stack, but only commit 12K
        //
        StackPtes = BYTES_TO_PAGES(KERNEL_LARGE_STACK_SIZE);
        StackPages = BYTES_TO_PAGES(KERNEL_LARGE_STACK_COMMIT);
        
    }
    else
    {
        //
        // We'll allocate 12K and that's it
        //
        StackPtes = BYTES_TO_PAGES(KERNEL_STACK_SIZE);
        StackPages = StackPtes;
    }
    
    //
    // Reserve stack pages, plus a guard page
    //
    StackPte = MiReserveSystemPtes(StackPtes + 1, SystemPteSpace);
    if (!StackPte) return NULL;
    
    //
    // Get the stack address
    //
    BaseAddress = MiPteToAddress(StackPte + StackPtes + 1);
    
    //
    // Select the right PTE address where we actually start committing pages
    //
    PointerPte = StackPte;
    if (GuiStack) PointerPte += BYTES_TO_PAGES(KERNEL_LARGE_STACK_SIZE -
                                               KERNEL_LARGE_STACK_COMMIT);
    
    //
    // Setup the template stack PTE
    //
    TempPte = HyperTemplatePte;
    TempPte.u.Hard.Global = FALSE;
    TempPte.u.Hard.PageFrameNumber = 0;
    TempPte.u.Hard.Dirty = TRUE;
    
    //
    // Acquire the PFN DB lock
    //
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
    
    //
    // Loop each stack page
    //
    for (i = 0; i < StackPages; i++)
    {
        //
        // Next PTE
        //
        PointerPte++;
        ASSERT(PointerPte->u.Hard.Valid == 0);
        
        //
        // Get a page
        //
        PageFrameIndex = MmAllocPage(MC_NPPOOL, 0);
        TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
        
        //
        // Write it
        //
        *PointerPte = TempPte;
        PagesForStacks++;
    }
    
    //
    // Release the PFN lock
    //
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
    
    //
    // Return the stack address
    //
    return BaseAddress;
}

NTSTATUS
NTAPI
MmGrowKernelStackEx(IN PVOID StackPointer,
                    IN ULONG GrowSize)
{
    PKTHREAD Thread = KeGetCurrentThread();
    PMMPTE LimitPte, NewLimitPte, LastPte;
    PFN_NUMBER StackPages;
    KIRQL OldIrql;
    MMPTE TempPte;
    PFN_NUMBER PageFrameIndex;
    
    //
    // Make sure the stack did not overflow
    //
    ASSERT(((ULONG_PTR)Thread->StackBase - (ULONG_PTR)Thread->StackLimit) <=
           (KERNEL_LARGE_STACK_SIZE + PAGE_SIZE));
    
    //
    // Get the current stack limit
    //
    LimitPte = MiAddressToPte(Thread->StackLimit);
    ASSERT(LimitPte->u.Hard.Valid == 1);
    
    //
    // Get the new one and make sure this isn't a retarded request
    //
    NewLimitPte = MiAddressToPte((PVOID)((ULONG_PTR)StackPointer - GrowSize));
    if (NewLimitPte == LimitPte) return STATUS_SUCCESS;
    
    //
    // Now make sure you're not going past the reserved space
    //
    LastPte = MiAddressToPte((PVOID)((ULONG_PTR)Thread->StackBase -
                                     KERNEL_LARGE_STACK_SIZE));
    if (NewLimitPte < LastPte)
    {
        //
        // Sorry!
        //
        DPRINT1("Thread wants too much stack\n");
        return STATUS_STACK_OVERFLOW;
    }
    
    //
    // Calculate the number of new pages
    //
    LimitPte--;
    StackPages = (LimitPte - NewLimitPte + 1);
    
    //
    // Setup the template stack PTE
    //
    TempPte = HyperTemplatePte;
    TempPte.u.Hard.Global = FALSE;
    TempPte.u.Hard.PageFrameNumber = 0;
    TempPte.u.Hard.Dirty = TRUE;
    
    //
    // Acquire the PFN DB lock
    //
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
    
    //
    // Loop each stack page
    //
    while (LimitPte >= NewLimitPte)
    {
        //
        // Sanity check
        //
        ASSERT(LimitPte->u.Hard.Valid == 0);
        
        //
        // Get a page
        //
        PageFrameIndex = MmAllocPage(MC_NPPOOL, 0);
        TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
        
        //
        // Write it
        //
        *LimitPte-- = TempPte;
    }
    
    //
    // Release the PFN lock
    //
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
    
    //
    // Set the new limit
    //
    Thread->StackLimit = (ULONG_PTR)MiPteToAddress(NewLimitPte);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MmGrowKernelStack(IN PVOID StackPointer)
{
    //
    // Call the extended version
    //
    return MmGrowKernelStackEx(StackPointer, KERNEL_LARGE_STACK_COMMIT);
}

/* EOF */
