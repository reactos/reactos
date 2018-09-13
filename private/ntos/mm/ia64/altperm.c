/*++

Module Name:

    altperm.c

Abstract:

    This module contains the routines needed to implement 4K pages on IA64

    The idea is for an alternate set of permissions be kept that are on
    4K boundaries. Permissions are kept for all memory, not just split pages
    and the information is updated on any call to NtVirtualProtect()
    and NtVirtualAllocate().


Author:

    ky 18-Aug-98

Revision History:


--*/

#include "mi.h"

extern ULONG MMVADKEY;

#if defined(_MIALT4K_)

VOID
MiCheckPoint(ULONG Number);

VOID
MiCheckPointBreak(VOID);


ULONG MmCheckPointNumber = 100;
PVOID MmAddressBreak = (PVOID)0x7ecd0000;
LOGICAL _MiMakeRtlBoot = FALSE;

ULONG
MiFindProtectionForNativePte( 
    PVOID VirtualAddress
    );

VOID
MiFillZeroFor4kPage (
    IN PVOID BaseAddress,
    IN PEPROCESS Process
    );

MiCheckPointBreakVirtualAddress (
    PVOID VirtualAddress
);

VOID
MiResetAccessBitForNativePtes(
    IN PVOID StartVirtual,
    IN PVOID EndVirtual,
    IN PEPROCESS Process
    );

NTSTATUS
MiMapViewOfDataSection (
    IN PCONTROL_AREA ControlArea,
    IN PEPROCESS Process,
    IN PVOID *CapturedBase,
    IN PLARGE_INTEGER SectionOffset,
    IN PSIZE_T CapturedViewSize,
    IN PSECTION Section,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG ProtectionMask,
    IN SIZE_T CommitSize,
    IN ULONG_PTR ZeroBits,
    IN ULONG AllocationType,
    OUT PBOOLEAN ReleasedWsMutex
    );

BOOLEAN
MiIsSplitPage(
    IN PVOID Virtual
    );

VOID
MiCopyOnWriteFor4kPage(
    PVOID VirtualAddress,
    PEPROCESS Process
    
    );

VOID
MiCheckDemandZeroCopyOnWriteFor4kPage(
    PVOID VirtualAddress,
    PEPROCESS Process
    );

VOID
MiCheckVirtualAddressFor4kPage(
    PVOID VirtualAddress,
    PEPROCESS Process
    );

MmX86Fault (
    IN BOOLEAN StoreInstruction,
    IN PVOID VirtualAddress, 
    IN KPROCESSOR_MODE PreviousMode,
    IN PVOID TrapInformation
    )

/*++

Routine Description:

    This function is called by the kernel on data or instruction
    access faults if CurrentProcess->Vm.u.Flags.AltPerm is set. 

    This routine determines what type of fault by checking the alternate
    4Kb granular page table and calls MmAccessFault() if necessary to 
    handle the page fault or the write fault.

Arguments:

    StoreInstruction - Supplies TRUE (1) if the operation causes a write into
                       memory.  Note this value must be 1 or 0.

    VirtualAddress - Supplies the virtual address which caused the fault.

    PreviousMode - Supplies the mode (kernel or user) in which the fault
                   occurred.

    TrapInformation - Opaque information about the trap, interpreted by the
                      kernel, not Mm.  Needed to allow fast interlocked access
                      to operate correctly.

Return Value:

    Returns the status of the fault handling operation.  Can be one of:
        - Success.
        - Access Violation.
        - Guard Page Violation.
        - In-page Error.

Environment:

    Kernel mode, APCs disabled.

--*/

{
    PMMPTE PointerAltPte;
    MMPTE AltPteContents;
    MMPTE PteContents;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    PMMPTE PointerProtoPte = (PMMPTE)NULL;
    ULONG ProtectCode;
    MMPTE TempPte;
    ULONG NewPteProtection = 0;
    ULONG AteProtection;
    LOGICAL ExecutionFault = FALSE;
    LOGICAL FillZero = FALSE;
    LOGICAL SetNewProtection = FALSE;
    LOGICAL PageIsSplit = FALSE;
    PEPROCESS CurrentProcess;
    PWOW64_PROCESS Wow64Process;
    KIRQL PreviousIrql;
    KIRQL OldIrql;
    NTSTATUS status;
    PMMINPAGE_SUPPORT ReadBlock;
    ULONG Waited;
    ULONG OriginalProtection;
    ULONGLONG ProtectionMaskOriginal;
    PMMPTE ProtoPte;
    PMMPFN Pfn1;
    ULONG i;

    //
    // debug checking
    //

    MiCheckPointBreakVirtualAddress (VirtualAddress);

    PreviousIrql = KeGetCurrentIrql ();
    
    ASSERT (PreviousIrql <= APC_LEVEL);

    CurrentProcess = PsGetCurrentProcess ();

    Wow64Process = CurrentProcess->Wow64Process;

    ASSERT (VirtualAddress < (PVOID)_MAX_WOW64_ADDRESS);

    if (StoreInstruction == 2) { 
        ExecutionFault = TRUE;
        StoreInstruction = FALSE;
    }

    //
    // lock the alternate table and this also blocks APCs.
    //

    LOCK_ALTERNATE_TABLE (Wow64Process);

    //
    // check to see if the protection is registered in the alternate entry
    //

    if (MI_CHECK_BIT(Wow64Process->AltPermBitmap, 
                     MI_VA_TO_VPN(VirtualAddress)) == 0) { 

        MiCheckVirtualAddressFor4kPage(VirtualAddress, CurrentProcess);

    }

    //
    // 
    //

    PointerPte = MiGetPteAddress(VirtualAddress);
    PointerAltPte = MiGetAltPteAddress(VirtualAddress);

    //
    // read alternate PTE contents
    //

    AltPteContents = *PointerAltPte;

    //
    // check to see if alternate entry is empty
    //

    if (AltPteContents.u.Long == 0) {

        MiCheckPoint(10);

        //
        // if empty, get the protection info from OS and fill the entry
        //

        LOCK_WS (CurrentProcess);

        ProtoPte = MiCheckVirtualAddress (VirtualAddress, &OriginalProtection);

        if (OriginalProtection == MM_UNKNOWN_PROTECTION) {

            if (!MI_IS_PHYSICAL_ADDRESS(ProtoPte)) {
                PointerPde = MiGetPteAddress (ProtoPte);
                LOCK_PFN (OldIrql);
                if (PointerPde->u.Hard.Valid == 0) {
                    MiMakeSystemAddressValidPfn (ProtoPte);
                }
                Pfn1 = MI_PFN_ELEMENT (PointerPde->u.Hard.PageFrameNumber);
                Pfn1->u3.e2.ReferenceCount += 1;
                ASSERT (Pfn1->u3.e2.ReferenceCount > 1);
                UNLOCK_PFN (OldIrql);
            }

            OriginalProtection = 
                MiMakeProtectionMask(MiGetPageProtection(ProtoPte, CurrentProcess));

            //
            // Unlock page containing prototype PTEs.
            //

            if (!MI_IS_PHYSICAL_ADDRESS(ProtoPte)) {
                LOCK_PFN (OldIrql);
                ASSERT (Pfn1->u3.e2.ReferenceCount > 1);
                Pfn1->u3.e2.ReferenceCount -= 1;
                UNLOCK_PFN (OldIrql);
            }
        }
        
        UNLOCK_WS (CurrentProcess);

        if (OriginalProtection != MM_NOACCESS) {

            ProtectionMaskOriginal = MiMakeProtectionAteMask (OriginalProtection);
            ProtectionMaskOriginal |= MM_ATE_COMMIT;

            AltPteContents.u.Long = ProtectionMaskOriginal;
            AltPteContents.u.Alt.Protection = OriginalProtection;

            //
            // atomic PTE update
            //

            PointerAltPte->u.Long = AltPteContents.u.Long;
        }
    } 

    if (AltPteContents.u.Alt.NoAccess != 0) {

        //
        // this 4KB page is no access
        //

        status = STATUS_ACCESS_VIOLATION;
        
        goto return_status;

    }
    
    if (AltPteContents.u.Alt.PteIndirect != 0) {

        MiCheckPoint(3);

        //
        // make PPE and PDE exist and valid
        //

        PointerPde = MiGetPdeAddress(VirtualAddress);
        PointerPpe = MiGetPpeAddress(VirtualAddress);

        //
        // make the page table for the original PTE exit to satisfy 
        // the TLB forward progress for the TLB indirect fault
        // 

        LOCK_WS (CurrentProcess);

        (VOID)MiMakePpeExistAndMakeValid (PointerPpe,
                                          CurrentProcess,
                                          FALSE);
    
        (VOID)MiMakePdeExistAndMakeValid (PointerPde,
                                          CurrentProcess,
                                          FALSE);

        UNLOCK_WS (CurrentProcess);

        PointerPte = (PMMPTE)(AltPteContents.u.Alt.PteOffset + PTE_UBASE);

        VirtualAddress = MiGetVirtualAddressMappedByPte(PointerPte);

        goto Check_Pte;

    }
        
    if ((_MiMakeRtlBoot == TRUE) && (AltPteContents.u.Alt.Commit == 0)) {
      
        //
        // This is supposed to be an access to an uncommmitted page and should 
        // result in STATUS_ACCESS_VIOLATION.  As long as we test in IA64, 
        // we cannot make a fault here. Some code assume PAGE_SIZE is given at least 
        // for commitment.
        // 

        MiCheckPoint(0);

        PointerAltPte->u.Alt.Commit = 1;

        AltPteContents = *PointerAltPte;

    }

    if (AltPteContents.u.Alt.Commit == 0) {
        
        //
        // if the page is no commit, return as STATUS_ACCESS_VIOLATION.
        //

        status = STATUS_ACCESS_VIOLATION;

        goto return_status;

    }

    //
    // check to see if the faulting page is split to 4k pages
    //

    PageIsSplit = MiIsSplitPage(VirtualAddress);

    //
    // get a real protection for the native PTE
    //

    NewPteProtection = MiFindProtectionForNativePte (VirtualAddress);

Check_Pte:

    //
    // Block APCs and acquire the working set lock.
    //

    LOCK_WS (CurrentProcess);

    //
    // make PPE and PDE exist and valid
    //

    PointerPde = MiGetPdeAddress(VirtualAddress);
    PointerPpe = MiGetPpeAddress(VirtualAddress);

    if (MiDoesPpeExistAndMakeValid (PointerPpe,
                                    CurrentProcess,
                                    FALSE,
                                    &Waited) == FALSE) {
        PteContents.u.Long = 0;
    
    } else if (MiDoesPdeExistAndMakeValid (PointerPde,
                                           CurrentProcess,
                                           FALSE,
                                           &Waited) == FALSE) {

        PteContents.u.Long = 0;

    } else {

        //
        // if it is safe to read PointerPte
        //

        PteContents = *PointerPte;

    }

    //
    // Check to see if the protection for the native page should be set
    // and if the access-bit of the PTE should be set.
    //

    if (PteContents.u.Hard.Valid != 0) { 

        TempPte = PteContents;

        //
        // perfom PTE protection mask corrections
        //

        TempPte.u.Long |= NewPteProtection;

        if (PteContents.u.Hard.Accessed == 0) {

            TempPte.u.Hard.Accessed = 1;

            if (PageIsSplit == TRUE) {

                TempPte.u.Hard.Cache = MM_PTE_CACHE_RESERVED;

            } 
        }

        //
        // if the private page has been assigned to COW, remove the COW bit
        //

        if ((TempPte.u.Hard.CopyOnWrite != 0) && (PteContents.u.Hard.Write != 0)) { 

            TempPte.u.Hard.CopyOnWrite = 0;
            
        }

        MI_WRITE_VALID_PTE_NEW_PROTECTION(PointerPte, TempPte); 
    }

    UNLOCK_WS (CurrentProcess);
    
    //
    // faulting 4kb page must be a valid page, but we need to resolve it 
    // case by case.
    //

    ASSERT (AltPteContents.u.Long != 0);
    ASSERT (AltPteContents.u.Alt.Commit != 0);
        
    if (AltPteContents.u.Alt.Accessed == 0) {

        //
        // When PointerAte->u.Hard.Accessed is zero, there are the following 4 cases:
        // 
        //  1. Lowest Protection 
        //  2. 4kb Demand Zero 
        //  3. GUARD page fault
        //  4. this 4kb page is no access, but the other 4K page(s) within a native page 
        //     has accessible permission.
        //

        if (AltPteContents.u.Alt.FillZero != 0) {

            //
            // schedule it later
            //

            FillZero = TRUE;
            
        } 

        if ((AltPteContents.u.Alt.Protection & MM_GUARD_PAGE) != 0) {
        
            MiCheckPoint(5);

            //
            // if 4k page is guard page then call MmAccessFault() to handle the faults
            //
        
            if (PteContents.u.Hard.Valid == 0) {

                UNLOCK_ALTERNATE_TABLE (Wow64Process);

                //
                // Let MmAccessFault() perform a page-in, dirty-bit setting, etc.
                //

                status = MmAccessFault (StoreInstruction,
                                        VirtualAddress,
                                        PreviousMode,
                                        TrapInformation);

                LOCK_ALTERNATE_TABLE (Wow64Process);

                if (status == STATUS_PAGE_FAULT_GUARD_PAGE) {
                    
                    PointerAltPte = MiGetAltPteAddress(PAGE_ALIGN(VirtualAddress));
                
                    for (i = 0; i < SPLITS_PER_PAGE; i += 1) {
                    
                        AltPteContents.u.Long = PointerAltPte->u.Long;

                        if ((AltPteContents.u.Alt.Protection & MM_GUARD_PAGE) != 0) {

                            AltPteContents.u.Alt.Protection &= ~MM_GUARD_PAGE;
                            AltPteContents.u.Alt.Accessed = 1;
                            PointerAltPte->u.Long = AltPteContents.u.Long;

                        }

                        PointerAltPte += 1;

                    } 
                    
                    goto return_status;
                }

            }

            AltPteContents.u.Alt.Protection &= ~MM_GUARD_PAGE;
            AltPteContents.u.Alt.Accessed = 1;

            PointerAltPte->u.Long = AltPteContents.u.Long;

            status =  STATUS_GUARD_PAGE_VIOLATION;           

            goto return_status;

        } else if (FillZero == FALSE) {

            //
            // this 4kb page has no access permission
            //

            status = STATUS_ACCESS_VIOLATION;
            
            goto return_status;

        }
    }

    if (ExecutionFault == TRUE) {
        
        //
        // As execute permission is already given to IA32 by setting it in 
        // MI_MAKE_VALID_PTE().
        //

    } else if (StoreInstruction == TRUE) {
        
        //
        // Check to see if this is the copy-on-write page.
        //

        if (AltPteContents.u.Alt.CopyOnWrite != 0) {

#if 0
            MiCheckDemandZeroCopyOnWriteFor4kPage(VirtualAddress, CurrentProcess);
#endif
            //
            // let MmAccessFault() perform a copy-on-write
            //

            status = MmAccessFault (StoreInstruction,
                                    VirtualAddress,
                                    PreviousMode,
                                    TrapInformation);

#if 0
            if (PteContents.u.Hard.Valid != 0) {
                DbgPrint("copyonwrite original page = %p, %p\n", VirtualAddress, PteContents.u.Long);
            }
#endif

            if (!NT_SUCCESS(status)) {

                MiCheckPointBreak();
                goto return_status;
                
            }

            if (status == STATUS_PAGE_FAULT_COPY_ON_WRITE) {
                MiCopyOnWriteFor4kPage(VirtualAddress, CurrentProcess);
            } else {
                MiCheckPointBreak();
            }
            
            //
            // write debug code to check the consistency
            //

            status = STATUS_SUCCESS;

            goto return_status;
        }
            
        if (AltPteContents.u.Hard.Write == 0) {

            status = STATUS_ACCESS_VIOLATION;
            
            goto return_status;
        }

    }

    //
    // Let MmAccessFault() perform a page-in, dirty-bit setting, etc.
    //

    UNLOCK_ALTERNATE_TABLE (Wow64Process);
    
    status = MmAccessFault (StoreInstruction,
                            VirtualAddress,
                            PreviousMode,
                            TrapInformation);

    if ((status == STATUS_PAGE_FAULT_GUARD_PAGE) ||
        (status == STATUS_GUARD_PAGE_VIOLATION)) {

        LOCK_ALTERNATE_TABLE (Wow64Process);

        AltPteContents = *PointerAltPte;

        if ((AltPteContents.u.Alt.Protection & MM_GUARD_PAGE) != 0) {
        
            AltPteContents = *PointerAltPte;
            AltPteContents.u.Alt.Protection &= ~MM_GUARD_PAGE;
            AltPteContents.u.Alt.Accessed = 1;
        
            PointerAltPte->u.Long = AltPteContents.u.Long;

        }

        UNLOCK_ALTERNATE_TABLE (Wow64Process);

        if (status == STATUS_GUARD_PAGE_VIOLATION) {

            status = STATUS_SUCCESS;

        }

    }

    KiFlushSingleTb(TRUE, VirtualAddress);

    if (FillZero == TRUE) {

        MiFillZeroFor4kPage (VirtualAddress, CurrentProcess);

    }

    if (!NT_SUCCESS(status)) {

        MiCheckPointBreak();

    } 

    return status;

 return_status:

    KiFlushSingleTb(TRUE, VirtualAddress);

    UNLOCK_ALTERNATE_TABLE (Wow64Process);

    if (FillZero == TRUE) {

        MiFillZeroFor4kPage (VirtualAddress, CurrentProcess);

    }

    if (!NT_SUCCESS(status)) {

        MiCheckPointBreak();

    } 

    return status;
}

ULONG
MiFindProtectionForNativePte( 
    PVOID VirtualAddress
    )

/*++

Routine Description:

    This function finds the protection for the native PTE

Arguments:

    VirtualAddress - Supplies a virtual address to be examined for the protection 
         of the PTE.

Return Value:

    none

Environment:


--*/

{
    PMMPTE PointerAltPte;
    ULONG i;
    ULONG ProtectionCode = 0;

    PointerAltPte = MiGetAltPteAddress(PAGE_ALIGN(VirtualAddress));
    
    for (i = 0; i < SPLITS_PER_PAGE; i++) {

        ProtectionCode |= (PointerAltPte->u.Long & ALT_PROTECTION_MASK);

        if (PointerAltPte->u.Alt.CopyOnWrite != 0) {
            ProtectionCode |= MM_PTE_COPY_ON_WRITE_MASK;
        }

        PointerAltPte += 1;
    }

    return ProtectionCode;

}


//
// Define and initialize the protection convertion table for 
// Alternate Permision Table Entries.
//

ULONGLONG MmProtectToAteMask[32] = {
                       MM_PTE_NOACCESS | MM_ATE_NOACCESS,
                       MM_PTE_EXECUTE_READ | MM_PTE_ACCESS_MASK,
                       MM_PTE_EXECUTE_READ | MM_PTE_ACCESS_MASK,
                       MM_PTE_EXECUTE_READ | MM_PTE_ACCESS_MASK,
                       MM_PTE_EXECUTE_READWRITE | MM_PTE_ACCESS_MASK,
                       MM_PTE_EXECUTE_READ | MM_PTE_ACCESS_MASK | MM_ATE_COPY_ON_WRITE,
                       MM_PTE_EXECUTE_READWRITE | MM_PTE_ACCESS_MASK,
                       MM_PTE_EXECUTE_READ | MM_PTE_ACCESS_MASK | MM_ATE_COPY_ON_WRITE,
                       MM_PTE_NOACCESS | MM_ATE_NOACCESS,
                       MM_PTE_EXECUTE_READ | MM_PTE_ACCESS_MASK,
                       MM_PTE_EXECUTE_READ | MM_PTE_ACCESS_MASK,
                       MM_PTE_EXECUTE_READ | MM_PTE_ACCESS_MASK,
                       MM_PTE_EXECUTE_READWRITE | MM_PTE_ACCESS_MASK,
                       MM_PTE_EXECUTE_READ | MM_PTE_ACCESS_MASK | MM_ATE_COPY_ON_WRITE,
                       MM_PTE_EXECUTE_READWRITE | MM_PTE_ACCESS_MASK,
                       MM_PTE_EXECUTE_READ | MM_PTE_ACCESS_MASK | MM_ATE_COPY_ON_WRITE,
                       MM_PTE_NOACCESS | MM_ATE_NOACCESS,
                       MM_PTE_EXECUTE_READ,
                       MM_PTE_EXECUTE_READ,
                       MM_PTE_EXECUTE_READ,
                       MM_PTE_EXECUTE_READWRITE,
                       MM_PTE_EXECUTE_READ | MM_ATE_COPY_ON_WRITE,
                       MM_PTE_EXECUTE_READWRITE,
                       MM_PTE_EXECUTE_READ | MM_ATE_COPY_ON_WRITE,
                       MM_PTE_NOACCESS | MM_ATE_NOACCESS,
                       MM_PTE_EXECUTE_READ,
                       MM_PTE_EXECUTE_READ,
                       MM_PTE_EXECUTE_READ,
                       MM_PTE_EXECUTE_READWRITE,
                       MM_PTE_EXECUTE_READ | MM_ATE_COPY_ON_WRITE,
                       MM_PTE_EXECUTE_READWRITE,
                       MM_PTE_EXECUTE_READ | MM_ATE_COPY_ON_WRITE
                    };

#define MiMakeProtectionAteMask(NewProtect) MmProtectToAteMask[NewProtect]

VOID
MiProtectFor4kPage(
    IN PVOID Base,
    IN SIZE_T Size,
    IN ULONG NewProtect,
    IN ULONG Flags,
    IN PEPROCESS Process
    )
/*++

Routine Description:

    This routine sets the permissions on the alternate bitmap (based on
    4K page sizes). The base and size are assumed to be aligned for
    4K pages already.

Arguments:

    Base - The base address (assumed to be 4K aligned already)

    Size - The size to be protected (assumed to be 4K aligned already)

    NewProtect - The protection for the new pages

    Flags - The alternate table entry request flags

    Process - Supplies a pointer to the process in which to create the 
            protections on the alternate table

    ChangeProtection - if the protection on the existing entries to be 
             changed. FALSE if new alternate entries are allocated.

Return Value:
 
    None

Environment:

    Kernel mode.  All arguments are assumed to be in kernel space.

--*/

{
    PVOID Starting4KAddress;
    PVOID Ending4KAddress;
    PVOID VirtualAddress;
    ULONG NewProtectNotCopy;
    ULONGLONG ProtectionMask;
    ULONGLONG ProtectionMaskNotCopy;
    ULONGLONG HardwareProtectionMask;
    PMMPTE StartAltPte, EndAltPte;
    PMMPTE StartAltPte0, EndAltPte0;
    PWOW64_PROCESS Wow64Process = Process->Wow64Process;
    PVOID Virtual[MM_MAXIMUM_FLUSH_COUNT];
    ULONG FlushCount;
    MMPTE AltPteContents;
    MMPTE TempAltPte;


    Starting4KAddress = Base;
    Ending4KAddress = (PCHAR)Base + Size - 1;

    //
    // Make sure we don't over run the table
    //
    ASSERT( ((ULONG_PTR) Starting4KAddress) < _MAX_WOW64_ADDRESS);
    ASSERT( ((ULONG_PTR) Ending4KAddress) < _MAX_WOW64_ADDRESS);

    //
    // for free builds, until this is more tested
    //
    if ((((UINT_PTR) Starting4KAddress) >= _MAX_WOW64_ADDRESS)
             || (((UINT_PTR) Ending4KAddress) >= _MAX_WOW64_ADDRESS)) {
        return;
    }

    //
    // Set up the protection to be used for this range of addresses
    //

    if ((NewProtect & MM_COPY_ON_WRITE_MASK) == MM_COPY_ON_WRITE_MASK) {
        NewProtectNotCopy = NewProtect & ~MM_PROTECTION_COPY_MASK;
    } else {
        NewProtectNotCopy = NewProtect;
    }    

    ProtectionMask = MiMakeProtectionAteMask (NewProtect);
    ProtectionMaskNotCopy = MiMakeProtectionAteMask (NewProtectNotCopy);

    if (Flags & ALT_COMMIT) {

        ProtectionMask |= MM_ATE_COMMIT;
        ProtectionMaskNotCopy |= MM_ATE_COMMIT; 

    }

    //
    // Get the entry in the table for each of these addresses
    //

    StartAltPte = MiGetAltPteAddress (Starting4KAddress);
    EndAltPte = MiGetAltPteAddress (Ending4KAddress);

    StartAltPte0 = MiGetAltPteAddress(PAGE_ALIGN(Starting4KAddress) );
    EndAltPte0 = MiGetAltPteAddress((ULONG_PTR)PAGE_ALIGN(Ending4KAddress)+PAGE_SIZE-1);

    //
    // lock the alternate page table
    //

    LOCK_ALTERNATE_TABLE (Wow64Process);

    if (!(Flags & ALT_ALLOCATE) && 
        (MI_CHECK_BIT(Wow64Process->AltPermBitmap, MI_VA_TO_VPN(Starting4KAddress)) == 0)) {

        UNLOCK_ALTERNATE_TABLE (Wow64Process);
        return;

    }

    //
    // And then change all of the protections
    //

    VirtualAddress = Starting4KAddress;

    while (StartAltPte <= EndAltPte) {

        AltPteContents.u.Long = StartAltPte->u.Long;

        if (!(Flags & ALT_ALLOCATE) && (AltPteContents.u.Alt.Private != 0)) {


            //
            // if it is already private, don't make it writecopy
            //
                
            TempAltPte.u.Long = ProtectionMaskNotCopy;
            TempAltPte.u.Alt.Protection = NewProtectNotCopy;

            //
            // Private is sticky bit
            //

            TempAltPte.u.Alt.Private = 1;

        } else {             
                
            TempAltPte.u.Long = ProtectionMask;
            TempAltPte.u.Alt.Protection = NewProtect;

        } 

        if (Flags & ALT_CHANGE) {

            //
            // if it is a change request, make Commit sticky
            //

            TempAltPte.u.Alt.Commit = AltPteContents.u.Alt.Commit;
            
        }

        if (!(Flags & ALT_ALLOCATE) && (AltPteContents.u.Alt.FillZero != 0)) {

            TempAltPte.u.Alt.Accessed = 0;
            TempAltPte.u.Alt.FillZero = 1;

        }
            
        //
        // atomic PTE update
        //

        StartAltPte->u.Long = TempAltPte.u.Long;

        StartAltPte++;
        VirtualAddress = (PVOID)((ULONG_PTR)VirtualAddress + PAGE_4K);

    }

    if (Flags & ALT_ALLOCATE) {

        //
        // fill the empty Alt Pte as NoAccess ATE at the end
        //

        while (EndAltPte <= EndAltPte0) {

            if (EndAltPte->u.Long == 0) {

                TempAltPte.u.Long = EndAltPte->u.Long;
                TempAltPte.u.Alt.NoAccess = 1;

                //
                // atomic PTE update
                //

                EndAltPte->u.Long = TempAltPte.u.Long;

            }

            EndAltPte++;
        }

        //
        // update the permission bitmap
        //

        MiMarkSplitPages(Base, 
                         (PVOID)((ULONG_PTR)Base + Size - 1), 
                         Wow64Process->AltPermBitmap,
                         TRUE);
    }

    // 
    // As OS always perform MI_MAKE_VLID_PTE to change a valid PTE
    // and the macro always reset the access-bit, we don't need to 
    // call MiResetAccessBitForNativePtes.
    // 
    // MiResetAccessBitForNativePtes(Starting4KAddress, 
    //                              Ending4KAddress, 
    //                              Process);



    //
    // flush the TB since the page protections has been changed.
    //

    VirtualAddress = PAGE_ALIGN(Starting4KAddress);

    FlushCount = 0;
    while (VirtualAddress <= Ending4KAddress) {
        if (FlushCount != MM_MAXIMUM_FLUSH_COUNT) {
            Virtual[FlushCount] = VirtualAddress;
            FlushCount += 1;
        } 
        VirtualAddress = (PVOID)((ULONG_PTR)VirtualAddress + PAGE_SIZE);
    }
    
    if (FlushCount != 0) {
        
        if (FlushCount <= MM_MAXIMUM_FLUSH_COUNT) {

            KeFlushMultipleTb(FlushCount,
                              &Virtual[0],
                              TRUE,
                              TRUE,
                              NULL,
                              ZeroPte.u.Flush);                          
        } else {

            KeFlushEntireTb(TRUE, TRUE);

        }
    }

    UNLOCK_ALTERNATE_TABLE (Wow64Process);
}

VOID
MiProtectMapFileFor4kPage(
    IN PVOID Base,
    IN SIZE_T Size,
    IN ULONG NewProtect,
    IN PMMPTE PointerPte,
    IN PEPROCESS Process
    )
/*++

Routine Description:

    This routine sets the permissions on the alternate bitmap (based on
    4K page sizes). The base and size are assumed to be aligned for
    4K pages already.

Arguments:

    Base - The base address (assumed to be 4K aligned already)

    Size - The size to be protected (assumed to be 4K aligned already)

    NewProtect - The protection for the new pages

    Commit - True if the page is commited, false otherwise

    Process - Supplies a pointer to the process in which to create the 
            protections on the alternate table

Return Value:
 
    None

Environment:

    Kernel mode.  All arguments are assumed to be in kernel space.

--*/

{
    PVOID Starting4KAddress;
    PVOID Ending4KAddress;
    ULONGLONG ProtectionMask;
    ULONGLONG HardwareProtectionMask;
    PMMPTE StartAltPte, EndAltPte;
    PMMPTE StartAltPte0, EndAltPte0;
    PWOW64_PROCESS Wow64Process = Process->Wow64Process;
    MMPTE TempAltPte;

    Starting4KAddress = Base;
    Ending4KAddress = (PCHAR)Base + Size - 1;

    //
    // Make sure we don't over run the table
    //
    ASSERT( ((ULONG_PTR) Starting4KAddress) < _MAX_WOW64_ADDRESS);
    ASSERT( ((ULONG_PTR) Ending4KAddress) < _MAX_WOW64_ADDRESS);

    //
    // for free builds, until this is more tested
    //
    if ((((UINT_PTR) Starting4KAddress) >= _MAX_WOW64_ADDRESS)
             || (((UINT_PTR) Ending4KAddress) >= _MAX_WOW64_ADDRESS)) {
        return;
    }

    //
    // Set up the protection to be used for this range of addresses
    //

    ProtectionMask = MiMakeProtectionAteMask (NewProtect);

    //
    // Get the entry in the table for each of these addresses
    //

    StartAltPte = MiGetAltPteAddress (Starting4KAddress);
    EndAltPte = MiGetAltPteAddress (Ending4KAddress);
    EndAltPte0 = MiGetAltPteAddress((ULONG_PTR)PAGE_ALIGN(Ending4KAddress)+PAGE_SIZE-1);

    LOCK_ALTERNATE_TABLE (Wow64Process);

    ExAcquireFastMutexUnsafe (&MmSectionCommitMutex);

    //
    // And then change all of the protections
    //

    while (StartAltPte <= EndAltPte) {

        TempAltPte.u.Long = ProtectionMask;
        TempAltPte.u.Alt.Protection = NewProtect;

        if (PointerPte->u.Long != 0) {

            TempAltPte.u.Alt.Commit = 1;

        } else {

            TempAltPte.u.Alt.Commit = 0;

        }

        //
        // atomic PTE update
        //

        StartAltPte->u.Long = TempAltPte.u.Long;

        StartAltPte++;

        if (((ULONG_PTR)StartAltPte & ((SPLITS_PER_PAGE * sizeof(MMPTE))-1)) == 0) {

            PointerPte++;

        }
    }

    ExReleaseFastMutexUnsafe (&MmSectionCommitMutex);

    //
    // fill the empty Alt Pte as NoAccess ATE at the end
    //

    while (EndAltPte <= EndAltPte0) {

        if (EndAltPte->u.Long == 0) {

            TempAltPte.u.Long = EndAltPte->u.Long;
            TempAltPte.u.Alt.NoAccess = 1;

            //
            // atomic PTE size update
            //

            EndAltPte->u.Long = TempAltPte.u.Long;

        }

        EndAltPte++;
    }
    
    MiMarkSplitPages(Base, 
                     (PVOID)((ULONG_PTR)Base + Size - 1), 
                     Wow64Process->AltPermBitmap,
                     TRUE);

    UNLOCK_ALTERNATE_TABLE (Wow64Process);
}

VOID
MiProtectImageFileFor4kPage(
    IN PVOID Base,
    IN SIZE_T Size,
    IN PMMPTE PointerPte,
    IN PEPROCESS Process
    )
{
    PVOID Starting4KAddress;
    PVOID Ending4KAddress;
    ULONGLONG ProtectionMask;
    ULONGLONG HardwareProtectionMask;
    PMMPTE StartAltPte;
    PMMPTE EndAltPte;
    PMMPTE EndAltPte0;
    PWOW64_PROCESS Wow64Process = Process->Wow64Process;
    MMPTE TempAltPte;
    MMPTE TempPte;
    ULONG NewProtect;
    KIRQL OldIrql;
    ULONG i;

    Starting4KAddress = Base;
    Ending4KAddress = (PCHAR)Base + Size - 1;

    //
    // Make sure we don't over run the table
    //
    ASSERT( ((ULONG_PTR) Starting4KAddress) < _MAX_WOW64_ADDRESS);
    ASSERT( ((ULONG_PTR) Ending4KAddress) < _MAX_WOW64_ADDRESS);

    //
    // for free builds, until this is more tested
    //
    if ((((UINT_PTR) Starting4KAddress) >= _MAX_WOW64_ADDRESS)
             || (((UINT_PTR) Ending4KAddress) >= _MAX_WOW64_ADDRESS)) {
        return;
    }

    //
    // Get the entry in the table for each of these addresses
    //

    StartAltPte = MiGetAltPteAddress (Starting4KAddress);
    EndAltPte = MiGetAltPteAddress (Ending4KAddress);
    EndAltPte0 = MiGetAltPteAddress((ULONG_PTR)PAGE_ALIGN(Ending4KAddress)+PAGE_SIZE-1);

    LOCK_ALTERNATE_TABLE (Wow64Process);

    //
    // And then change all of the protections
    //

    while (StartAltPte <= EndAltPte) {

        //
        // Get the original protection information from the prototype PTEs
        //

        LOCK_WS_UNSAFE (Process);

        LOCK_PFN (OldIrql);
        MiMakeSystemAddressValidPfnWs (PointerPte, Process);
        TempPte = *PointerPte;
        UNLOCK_PFN (OldIrql);

        NewProtect = 
            MiMakeProtectionMask(MiGetPageProtection(&TempPte, Process));

        UNLOCK_WS_UNSAFE (Process);

        //
        // if demand-zero and copy-on-write, remove copy-on-write
        //

        if ((!IS_PTE_NOT_DEMAND_ZERO(TempPte)) && 
            (TempPte.u.Soft.Protection & MM_COPY_ON_WRITE_MASK)) {
            NewProtect = NewProtect & ~MM_PROTECTION_COPY_MASK;
        }

        ProtectionMask = MiMakeProtectionAteMask (NewProtect);
        ProtectionMask |= MM_ATE_COMMIT;

        TempAltPte.u.Long = ProtectionMask;
        TempAltPte.u.Alt.Protection = NewProtect;

        if ((NewProtect & MM_PROTECTION_COPY_MASK) == 0) {

            //
            // if the copy-on-write is removed, make it private
            //

            TempAltPte.u.Alt.Private = 1;

        }

        //
        // atomic PTE update
        //

        for (i = 0; i < SPLITS_PER_PAGE; i += 1) { 

            StartAltPte->u.Long = TempAltPte.u.Long;
            StartAltPte++;

        }

        PointerPte++;
    }

    //
    // fill the empty Alt Pte as NoAccess ATE at the end
    //

    while (EndAltPte <= EndAltPte0) {

        if (EndAltPte->u.Long == 0) {

            TempAltPte.u.Long = EndAltPte->u.Long;
            TempAltPte.u.Alt.NoAccess = 1;

            //
            // atomic PTE size update
            //

            EndAltPte->u.Long = TempAltPte.u.Long;

        }

        EndAltPte++;
    }
    
    MiMarkSplitPages(Base, 
                     (PVOID)((ULONG_PTR)Base + Size - 1), 
                     Wow64Process->AltPermBitmap,
                     TRUE);

    UNLOCK_ALTERNATE_TABLE (Wow64Process);
}

VOID
MiReleaseFor4kPage(
    IN PVOID StartVirtual,
    IN PVOID EndVirtual,
    IN PEPROCESS Process
    )
/*++

Routine Description:

    This function releases a region of pages within the virtual address
    space of a subject process.

Arguments:


   StartVirtual - the start address of the region of pages
        to be released.

   EndVirtual - the end address of the region of pages 
        to be released.

    Process - Supplies a pointer to the process in which to release a 
            region of pages.

Return Value:

    None


--*/

{
    PMMPTE StartAltPte;
    PMMPTE EndAltPte;
    MMPTE TempAltPte;
    ULONG_PTR VirtualAddress;
    ULONG i;
    PWOW64_PROCESS Wow64Process = Process->Wow64Process;
    PVOID Virtual[MM_MAXIMUM_FLUSH_COUNT];
    ULONG FlushCount = 0;

    ASSERT(StartVirtual <= EndVirtual);

    StartAltPte = MiGetAltPteAddress (StartVirtual);
    EndAltPte = MiGetAltPteAddress (EndVirtual);

    LOCK_ALTERNATE_TABLE (Wow64Process);

    VirtualAddress = (ULONG_PTR)StartVirtual;

    while (StartAltPte <= EndAltPte) {

        StartAltPte->u.Long = 0;
        StartAltPte->u.Alt.NoAccess = 1;
        StartAltPte->u.Alt.FillZero = 1;
        StartAltPte++;

    }

    StartVirtual = PAGE_ALIGN(StartVirtual);

    VirtualAddress = (ULONG_PTR)StartVirtual;

    while (VirtualAddress <= (ULONG_PTR)EndVirtual) {

        StartAltPte = MiGetAltPteAddress(VirtualAddress);
        TempAltPte = *StartAltPte;

        i = 0;

        while (TempAltPte.u.Long == StartAltPte->u.Long) {
            i += 1;
            StartAltPte++;
        }

        if (i == SPLITS_PER_PAGE) {

            StartAltPte = MiGetAltPteAddress(VirtualAddress);
            
            for (i = 0; i < SPLITS_PER_PAGE; i += 1) {
                StartAltPte->u.Long = 0;
                StartAltPte++;
            }
            
        }
        
        VirtualAddress  += PAGE_SIZE;
        
        if (FlushCount != MM_MAXIMUM_FLUSH_COUNT) {
            Virtual[FlushCount] = (PVOID)VirtualAddress;
            FlushCount += 1;
        } 
    }

    MiResetAccessBitForNativePtes(StartVirtual, EndVirtual, Process);

    if (FlushCount != 0) {
        
        if (FlushCount <= MM_MAXIMUM_FLUSH_COUNT) {

            KeFlushMultipleTb(FlushCount,
                              &Virtual[0],
                              TRUE,
                              TRUE,
                              NULL,
                              ZeroPte.u.Flush);                          
        } else {

            KeFlushEntireTb(TRUE, TRUE);

        }
    }

    UNLOCK_ALTERNATE_TABLE (Wow64Process);
}

VOID
MiDecommitFor4kPage(
    IN PVOID StartVirtual,
    IN PVOID EndVirtual,
    IN PEPROCESS Process
    )
/*++

Routine Description:

    This function decommits a region of pages within the virtual address
    space of a subject process.

Arguments:


   StartVirtual - the start address of the region of pages
        to be decommitted.

   EndVirtual - the end address of the region of the pages 
        to be decommitted.

    Process - Supplies a pointer to the process in which to decommit a
            a region of pages.

Return Value:

    None


--*/

{
    PMMPTE StartAltPte;
    PMMPTE EndAltPte;
    MMPTE TempAltPte;
    ULONG_PTR VirtualAddress;
    PWOW64_PROCESS Wow64Process = Process->Wow64Process;
    PVOID Virtual[MM_MAXIMUM_FLUSH_COUNT];
    ULONG FlushCount = 0;

    ASSERT(StartVirtual <= EndVirtual);

    StartAltPte = MiGetAltPteAddress (StartVirtual);
    EndAltPte = MiGetAltPteAddress (EndVirtual);

    LOCK_ALTERNATE_TABLE (Wow64Process);

    while (StartAltPte <= EndAltPte) {

        TempAltPte.u.Long = StartAltPte->u.Long;
        TempAltPte.u.Alt.Commit = 0;
        TempAltPte.u.Alt.FillZero = 1;

        //
        // atomic PTE update
        //

        StartAltPte->u.Long = TempAltPte.u.Long;

        StartAltPte++;
    }

    //
    // Flush the tb for virtual addreses
    //

    VirtualAddress = (ULONG_PTR)StartVirtual;

    while ((PVOID)VirtualAddress <= EndVirtual) {

        if (FlushCount != MM_MAXIMUM_FLUSH_COUNT) {
            Virtual[FlushCount] = (PVOID)VirtualAddress;
            FlushCount += 1;
        } 
        VirtualAddress += PAGE_SIZE;

    }

    MiResetAccessBitForNativePtes(StartVirtual, EndVirtual, Process);

    if (FlushCount != 0) {
        
        if (FlushCount <= MM_MAXIMUM_FLUSH_COUNT) {

            KeFlushMultipleTb(FlushCount,
                              &Virtual[0],
                              TRUE,
                              TRUE,
                              NULL,
                              ZeroPte.u.Flush);                          
        } else {

            KeFlushEntireTb(TRUE, TRUE);

        }
    }

    UNLOCK_ALTERNATE_TABLE (Wow64Process);
}


VOID
MiDeleteFor4kPage(
    IN PVOID StartVirtual,
    IN PVOID EndVirtual,
    IN PEPROCESS Process
    )
/*++

Routine Description:

    This function deletes a region of pages within the virtual address
    space of a subject process.

Arguments:


   StartVirtual - the start address of the region of pages
        to be deleted

   EndVirtual - the end address of the region of the pages 
        to be deleted.

    Process - Supplies a pointer to the process in which to delete a
            a region of pages.

Return Value:

    None


--*/

{
    PMMPTE StartAltPte;
    PMMPTE EndAltPte;
    ULONG_PTR VirtualAddress;
    PWOW64_PROCESS Wow64Process = Process->Wow64Process;
    PVOID Virtual[MM_MAXIMUM_FLUSH_COUNT];
    ULONG FlushCount = 0;

    ASSERT(StartVirtual <= EndVirtual);

    StartAltPte = MiGetAltPteAddress (StartVirtual);
    EndAltPte = MiGetAltPteAddress (EndVirtual);

    LOCK_ALTERNATE_TABLE (Wow64Process);

    VirtualAddress = (ULONG_PTR)StartVirtual;

    while (StartAltPte <= EndAltPte) {

        StartAltPte->u.Long= 0;
        StartAltPte++;
    }


    //
    // Flush the tb for virtual addreses
    //

    VirtualAddress = (ULONG_PTR)StartVirtual;

    while ((PVOID)VirtualAddress <= EndVirtual) {

        if (FlushCount != MM_MAXIMUM_FLUSH_COUNT) {
            Virtual[FlushCount] = (PVOID)VirtualAddress;
            FlushCount += 1;
        } 
        VirtualAddress += PAGE_SIZE;

    }

    MiMarkSplitPages(StartVirtual, 
                     EndVirtual, 
                     Wow64Process->AltPermBitmap,
                     TRUE);

    MiResetAccessBitForNativePtes(StartVirtual, EndVirtual, Process);

    if (FlushCount != 0) {
        
        if (FlushCount <= MM_MAXIMUM_FLUSH_COUNT) {

            KeFlushMultipleTb(FlushCount,
                              &Virtual[0],
                              TRUE,
                              TRUE,
                              NULL,
                              ZeroPte.u.Flush);                          
        } else {

            KeFlushEntireTb(TRUE, TRUE);

        }
    }

    UNLOCK_ALTERNATE_TABLE (Wow64Process);
}

BOOLEAN
MiIsSplitPage(
    IN PVOID Virtual
    )
{
    PMMPTE AltPte;
    MMPTE PteContents;
    BOOLEAN IsSplit;
    ULONG i;

    Virtual = PAGE_ALIGN(Virtual);
    AltPte = MiGetAltPteAddress(Virtual);
    PteContents = *AltPte;

    for (i = 0; i < SPLITS_PER_PAGE; i++) {

        if ((AltPte->u.Long != 0) && 
            ((AltPte->u.Alt.Commit == 0) || 
             (AltPte->u.Alt.Accessed == 0) ||
             (AltPte->u.Alt.CopyOnWrite != 0) || 
             (AltPte->u.Alt.FillZero != 0))) {

            //
            // if it is a NoAccess, FillZero or Guard page, CopyONWrite,
            // mark it as a split page
            //

            return TRUE;

        } else if (PteContents.u.Long != AltPte->u.Long) {

            //
            // if the next 4kb page is different from the 1st 4k page
            // the page is split
            //

            return TRUE;

        }

        AltPte++;

    }

    return FALSE;
}


VOID
MiMarkSplitPages(
    IN PVOID StartVirtual,
    IN PVOID EndVirtual,
    IN PULONG Bitmap,
    IN BOOLEAN SetBit
    )
/*++

Routine Description:

    This function sets the corresponding bit on the bitmap table if a split 
    page condition within a native page is detected. 

Arguments:


   StartVirtual - the start address of the region of pages
        to be inspected. 

   EndVirtual - the end address of the region of the pages 
        to be inspected.

   Bitmap - Supplies a pointer to the alternate bitmap table.

   SetBit - if TRUE, set the bit on the bitmap table. Otherwise, the bit 
        is set when only when a split contidition is found.

Return Value:

    None

--*/

{
    ULONG i;
    
    ASSERT(StartVirtual <= EndVirtual);
    ASSERT((ULONG_PTR)StartVirtual < _MAX_WOW64_ADDRESS);
    ASSERT((ULONG_PTR)EndVirtual < _MAX_WOW64_ADDRESS);

    StartVirtual = PAGE_ALIGN(StartVirtual);

    while (StartVirtual <= EndVirtual) {
        
        if (SetBit == TRUE) {

            //
            // set the bit, marking it as a split page
            //

            MI_SET_BIT(Bitmap, MI_VA_TO_VPN(StartVirtual));

        } else {

            //
            // clear the bit, marking it as a non split page
            //

            MI_CLEAR_BIT(Bitmap, MI_VA_TO_VPN(StartVirtual)); 
                
        }
            
        StartVirtual = (PVOID)((ULONG_PTR)StartVirtual + PAGE_SIZE);
    }
}

VOID
MiResetAccessBitForNativePtes(
    IN PVOID StartVirtual,
    IN PVOID EndVirtual,
    IN PEPROCESS Process
    )
/*++

Routine Description:

    This function resets the access bit of the native PTEs if the bitmap 
    indicates it is a split page.

Arguments:


   StartVirtual - the start address of the region of pages
        to be inspected. 

   EndVirtual - the end address of the region of the pages 
        to be inspected.

   Bitmap - Supplies a pointer to the process.

Return Value:

    None

--*/
{
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    BOOLEAN FirstTime;
    ULONG Waited;
    PWOW64_PROCESS Wow64Process = Process->Wow64Process;

    PointerPte = MiGetPteAddress (StartVirtual);

    LOCK_WS_UNSAFE (Process);

    FirstTime = TRUE;

    while (StartVirtual <= EndVirtual) {

        if ((FirstTime == TRUE) || MiIsPteOnPdeBoundary (PointerPte)) {

            PointerPde = MiGetPteAddress (PointerPte);
            PointerPpe = MiGetPdeAddress (PointerPte);

            if (MiDoesPpeExistAndMakeValid (PointerPpe, 
                                            Process, 
                                            FALSE,
                                             &Waited) == FALSE) {

                //
                // This page directory parent entry is empty,
                // go to the next one.
                //

                PointerPpe += 1;
                PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
                PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
                StartVirtual = MiGetVirtualAddressMappedByPte (PointerPte);
                continue;
            }

            if (MiDoesPdeExistAndMakeValid (PointerPde, 
                                            Process,
                                            FALSE,
                                            &Waited) == FALSE) {


                //
                // This page directory entry is empty,
                // go to the next one.
                //

                PointerPde++;
                PointerPte = MiGetVirtualAddressMappedByPte(PointerPde);
                StartVirtual = MiGetVirtualAddressMappedByPte(PointerPte);
                continue;
            }
                    
            FirstTime = FALSE;

        }
            
        if ((MI_CHECK_BIT(Wow64Process->AltPermBitmap, MI_VA_TO_VPN(StartVirtual))) &&
            ((PointerPte->u.Hard.Valid != 0) && (PointerPte->u.Hard.Accessed != 0))) {

            PointerPte->u.Hard.Accessed = 0;

        }

        PointerPte += 1;
        StartVirtual = (PVOID)((ULONG_PTR)StartVirtual + PAGE_SIZE); 
        
    }

    UNLOCK_WS_UNSAFE (Process);
}

VOID
MiQueryRegionFor4kPage(
    IN PVOID BaseAddress,
    IN PVOID EndAddress,
    IN OUT PSIZE_T RegionSize,
    IN OUT PULONG RegionState,
    IN OUT PULONG RegionProtect,
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This function checks the size of region which has the same memory 
    state.

Arguments:

    BaseAddress - The base address of the region of pages to be
        queried.
 
    EndAddress - The end of address of the region of pages to be queried.

    RegionSize - Original region size. Returns a region size for 4k pages 
                 if different.

    RegionState - Original region state. Returns a region state for 4k pages 
                 if different.

    RegionProtect - Original protection. Returns a protection for 4k pages 
                 if different.

    Process - Supplies a pointer to the process to be queried.

Return Value:

    Returns the size of the region

Environment:


--*/

{
   PMMPTE AltPte;
   MMPTE AltContents;
   PVOID Va;
   PWOW64_PROCESS Wow64Process = Process->Wow64Process;

   AltPte = MiGetAltPteAddress(BaseAddress);

   LOCK_ALTERNATE_TABLE (Wow64Process);

   if (MI_CHECK_BIT(Wow64Process->AltPermBitmap, 
                     MI_VA_TO_VPN(BaseAddress)) == 0) {

       UNLOCK_ALTERNATE_TABLE (Wow64Process);
       return;
       
   }


   AltContents.u.Long = AltPte->u.Long;

   if (AltContents.u.Long == 0) {

       UNLOCK_ALTERNATE_TABLE (Wow64Process);

       return;

   }

   if (AltContents.u.Alt.Commit != 0) {

       *RegionState = MEM_COMMIT;

       *RegionProtect = 
           MI_CONVERT_FROM_PTE_PROTECTION(AltContents.u.Alt.Protection);

   } else {

       *RegionState = MEM_RESERVE;
       
       *RegionProtect = 0;

   }

   Va = BaseAddress;

   while ((ULONG_PTR)Va < (ULONG_PTR)EndAddress) {

       Va = (PVOID)((ULONG_PTR)Va + PAGE_4K);
       AltPte++;

       if ((AltPte->u.Alt.Protection != AltContents.u.Alt.Protection) ||
           (AltPte->u.Alt.Commit != AltContents.u.Alt.Commit)) {

            //
            // The state for this address does not match, calculate
            // size and return.
            //

            break;
       }
       
   } // end while

   UNLOCK_ALTERNATE_TABLE (Wow64Process);

   *RegionSize = (SIZE_T)((ULONG_PTR)Va - (ULONG_PTR)BaseAddress);
}

ULONG
MiQueryProtectionFor4kPage (
    IN PVOID BaseAddress,
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This function queries the protection for a specified 4k page.

Arguments:

    BaseAddress - Supplies a base address of the 4k page. 

    Process - Supplies a pointer to the process to query the 4k page.

Return Value:

    Returns the protection of the 4k page.

Environment:


--*/

{

    ULONG Protection;
    PMMPTE PointerAltPte;
    PWOW64_PROCESS Wow64Process = Process->Wow64Process;

    PointerAltPte = MiGetAltPteAddress(BaseAddress);

    LOCK_ALTERNATE_TABLE (Process->Wow64Process);
    
   if (MI_CHECK_BIT(Wow64Process->AltPermBitmap, 
                     MI_VA_TO_VPN(BaseAddress)) == 0) {

       UNLOCK_ALTERNATE_TABLE (Wow64Process);
       return 0;
       
   }

    Protection = (ULONG)PointerAltPte->u.Alt.Protection;

    UNLOCK_ALTERNATE_TABLE (Process->Wow64Process);
    
    return Protection;
    
}

VOID
MiCheckPointBreak(VOID)
{
}

VOID
MiCheckPoint(
    ULONG Number
    )
{
    if (Number == MmCheckPointNumber) {
        
        MiCheckPointBreak();

    }
}


MiCheckPointBreakVirtualAddress (
    PVOID VirtualAddress
)
{
    ULONG Value;

    if (PAGE_4K_ALIGN(VirtualAddress) == MmAddressBreak) {

        MiCheckPointBreak();

    }
}


NTSTATUS
MiInitializeAlternateTable(
    PEPROCESS Process
    )
/*++

Routine Description:

    This function initializes the alternate table for the specified process.

Arguments:

    Process - Supplies a pointer to the process to initialize the alternate 
         table.

Return Value:

    STATUS_SUCCESS is returned if the alternate table was successfully initialized.

    STATUS_NO_MEMORY is returned if there was not enough physical memory in the 
    system.

Environment:


--*/
{
    PULONG AltTablePointer; 
    PWOW64_PROCESS Wow64Process = Process->Wow64Process;

    AltTablePointer = (PULONG)ExAllocatePoolWithTag (NonPagedPool,
                                                    (_MAX_WOW64_ADDRESS >> PTI_SHIFT)/8,
                                                    'AlmM');

    if (AltTablePointer == (PULONG) NULL) {
        return  STATUS_NO_MEMORY;
    }

    Wow64Process->AltPermBitmap = AltTablePointer;

    RtlZeroMemory(AltTablePointer, (_MAX_WOW64_ADDRESS >> PTI_SHIFT)/8);

    ExInitializeFastMutex(&Wow64Process->AlternateTableLock);

    return STATUS_SUCCESS;
}


VOID
MiDeleteAlternateTable(
    PEPROCESS Process
    )
/*++

Routine Description:

    This function deletes the alternate table for the specified process.

Arguments:

    Process - Supplies a pointer to the process to delete the alternate 
         table.

Return Value:

    none

Environment:

    Kernel mode, APCs disabled, working set mutex held.

--*/

{

    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    ULONG_PTR Va;
    ULONG_PTR TempVa;
    ULONG i;
    ULONG Waited;
    MMPTE_FLUSH_LIST PteFlushList;
    PWOW64_PROCESS Wow64Process = Process->Wow64Process;
    KIRQL OldIrql;

    ASSERT (Wow64Process->AltPermBitmap != NULL);
    
    //
    // Since Ppe for Alternate Table is shared with the hyper space,
    // we can assume it is always present without performing 
    // MiDoesPpeExistAndMakeValid().
    //

    PointerPpe = MiGetPpeAddress (ALT4KB_PERMISSION_TABLE_START);
    PointerPde = MiGetPdeAddress (ALT4KB_PERMISSION_TABLE_START);
    PointerPte = MiGetPteAddress (ALT4KB_PERMISSION_TABLE_START);

    Va = ALT4KB_PERMISSION_TABLE_START;

    LOCK_PFN(OldIrql);

    while (Va < ALT4KB_PERMISSION_TABLE_END) {

        while (MiDoesPdeExistAndMakeValid (PointerPde,
                                           Process,
                                           TRUE,
                                           &Waited) == FALSE) {

            //
            // this page directory entry is empty, go to the next one.
            //

            PointerPde += 1;
            PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
            Va = (ULONG_PTR)MiGetVirtualAddressMappedByPte (PointerPte);

            if (Va > ALT4KB_PERMISSION_TABLE_START) {
                
                goto delete_end;

            }

        }
    
        //
        // delete PTE entries for Altnerate Table
        //

        TempVa = Va;
        for (i = 0; i < PTE_PER_PAGE; i++) {

            if (PointerPte->u.Long != 0) {

                if (IS_PTE_NOT_DEMAND_ZERO (*PointerPte)) {

                    MiDeletePte (PointerPte,
                                 (PVOID)TempVa,
                                 TRUE,
                                 Process,
                                 NULL,
                                 &PteFlushList);
                } else {

                    *PointerPte = ZeroPte;
                }
                                    
            }
            
            TempVa = PAGE_4K;
            PointerPte += 1;
        }

        //
        // delete PDE entries for Alternate Table
        //

        TempVa = (ULONG_PTR)MiGetVirtualAddressMappedByPte(PointerPde);
        MiDeletePte (PointerPde,
                     (PVOID)TempVa,
                     TRUE,
                     Process,
                     NULL,
                     &PteFlushList);
       
        
        MiFlushPteList (&PteFlushList, FALSE, ZeroPte);

        PointerPde += 1;
        PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
        Va = (ULONG_PTR)MiGetVirtualAddressMappedByPte (PointerPte);
        
    }

 delete_end:

    MiFlushPteList (&PteFlushList, FALSE, ZeroPte);

    UNLOCK_PFN(OldIrql);
    
    ExFreePool (Wow64Process->AltPermBitmap);

    Wow64Process->AltPermBitmap = NULL;
}

VOID
MiFillZeroFor4kPage (
    IN PVOID VirtualAddress,
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This function performs bzero for a specified 4k page.

Arguments:

    BaseAddress - Supplies a base address of the 4k page. 

    Process - Supplies a pointer to the process to perform bzero.

Return Value:

    None.

Environment:


--*/

{
    SIZE_T RegionSize = PAGE_4K;
    PVOID BaseAddress;
    ULONG OldProtect;
    NTSTATUS Status;
    PMMPTE PointerAltPte;
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    PMMPTE PointerPte;
    MMPTE TempAltContents;
    MMPTE PteContents;
    ULONG Waited;
    PWOW64_PROCESS Wow64Process = Process->Wow64Process;

    PointerAltPte = MiGetAltPteAddress (VirtualAddress);

    LOCK_ALTERNATE_TABLE (Wow64Process);

    if (PointerAltPte->u.Alt.FillZero == 0) {

        //
        // Some one has already completed the bzero operations.
        //

        UNLOCK_ALTERNATE_TABLE (Wow64Process);

        return;

    }

    //
    // make PPE and PDE exist and valid
    //

    PointerPte = MiGetPteAddress(VirtualAddress);
    PointerPde = MiGetPdeAddress(VirtualAddress);
    PointerPpe = MiGetPpeAddress(VirtualAddress);

    //
    // make the page table for the original PTE exit to satisfy 
    // the TLB forward progress for the TLB indirect fault
    // 

    LOCK_WS (Process);

    if (MiDoesPpeExistAndMakeValid (PointerPpe,
                                    Process,
                                    FALSE,
                                    &Waited) == FALSE) {
        PteContents.u.Long = 0;
    
    } else if (MiDoesPdeExistAndMakeValid (PointerPde,
                                           Process,
                                           FALSE,
                                           &Waited) == FALSE) {

        PteContents.u.Long = 0;

    } else {

        //
        // if it is safe to read PointerPte
        //

        PteContents = *PointerPte;

    }

    TempAltContents.u.Long = PointerAltPte->u.Long;

    if (PteContents.u.Hard.Valid != 0) { 

        BaseAddress = KSEG_ADDRESS(PteContents.u.Hard.PageFrameNumber);

        BaseAddress = 
            (PVOID)((ULONG_PTR)BaseAddress + 
                    ((ULONG_PTR)PAGE_4K_ALIGN(VirtualAddress) & (PAGE_SIZE-1)));

        RtlZeroMemory(BaseAddress, PAGE_4K);

        UNLOCK_WS (Process);

        TempAltContents.u.Alt.FillZero = 0;
        TempAltContents.u.Alt.Accessed = 1;

    } else {

        UNLOCK_WS (Process);

        TempAltContents.u.Alt.Accessed = 0;

    }

    PointerAltPte->u.Long = TempAltContents.u.Long;

    UNLOCK_ALTERNATE_TABLE (Wow64Process);
}

#define USE_MAPVIEW 1

#if !USE_MAPVIEW
NTSTATUS
MiCreateAliasOfDataSection(
    IN PCONTROL_AREA ControlArea,
    IN PEPROCESS Process,
    IN PVOID *CapturedBase,
    IN PLARGE_INTEGER SectionOffset,
    IN PSIZE_T CapturedViewSize,
    IN ULONG ProtectionMask,
    OUT PMMVAD *Vad
    )
/*++

Routine Description:

    This is a simplified version of MiMapViewOfSection and creates an alias  
    map view for the exsiting map view space. 

Arguments:

    See MmMapViewOfSection.

    ControlArea - Supplies the control area for the section.

    Process - Supplies the process pointer which is receiving the section.

    ProtectionMask - Supplies the initial page protection-mask.

    Vad - Returns a pointer to the pointer to the VAD containing the new alias
          map view.
   
Return Value:

    Returns a NT status code.

Environment:

    Kernel mode, address creating mutex held.
    APCs disabled.

--*/

{
    ULONG_PTR Alignment;
    KIRQL OldIrql;
    PSUBSECTION Subsection;
    ULONG PteOffset;
    PVOID NewStartingAddress;
    PVOID NewEndingAddress;
    PMMVAD NewVad;
    PMMPTE TheFirstPrototypePte;

    //
    // Calculate the first prototype PTE field in the Vad.
    //

    Subsection = (PSUBSECTION)(ControlArea + 1);

    Alignment = X64K;

    SectionOffset->LowPart = SectionOffset->LowPart & ~((ULONG)Alignment - 1);
    PteOffset = (ULONG)(SectionOffset->QuadPart >> PAGE_SHIFT);

    //
    // Make sure the PTEs are not in the extended part of the
    // segment.
    //

    if (PteOffset >= ControlArea->Segment->TotalNumberOfPtes) {
        LOCK_PFN (OldIrql);
        ControlArea->NumberOfMappedViews -= 1;
        ControlArea->NumberOfUserReferences -= 1;
        UNLOCK_PFN (OldIrql);
        return STATUS_INVALID_VIEW_SIZE;
    }

    while (PteOffset >= Subsection->PtesInSubsection) {
        PteOffset -= Subsection->PtesInSubsection;
        Subsection = Subsection->NextSubsection;
        ASSERT (Subsection != NULL);
    }

    TheFirstPrototypePte = &Subsection->SubsectionBase[PteOffset];

    LOCK_WS_UNSAFE (Process);

    try {

        //
        // Find a starting address on a 64k boundary.
        //

        NewStartingAddress = MiFindEmptyAddressRange (*CapturedViewSize,
                                                      X64K,
                                                      (ULONG)0);
    } except (EXCEPTION_EXECUTE_HANDLER) {

        LOCK_PFN (OldIrql);
        ControlArea->NumberOfMappedViews -= 1;
        ControlArea->NumberOfUserReferences -= 1;
        UNLOCK_PFN (OldIrql);

        UNLOCK_WS_UNSAFE(Process);

        return GetExceptionCode();
    }

    NewEndingAddress = (PVOID)(((ULONG_PTR)NewStartingAddress +
                                *CapturedViewSize - 1L) | (PAGE_SIZE - 1L));

    //
    // An unoccupied address range has been found, build the virtual
    // address descriptor to describe this range.
    //

    try  {

        NewVad = ExAllocatePoolWithTag (NonPagedPool,
                                        sizeof(MMVAD),
                                        MMVADKEY);
        if (NewVad == NULL) {
            ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
        }
        RtlZeroMemory (NewVad, sizeof(MMVAD));

        NewVad->StartingVpn = MI_VA_TO_VPN (NewStartingAddress);
        NewVad->EndingVpn = MI_VA_TO_VPN (NewEndingAddress);
        NewVad->FirstPrototypePte = TheFirstPrototypePte;

        //
        // Set the protection in the PTE template field of the VAD.
        //

        NewVad->ControlArea = ControlArea;

        NewVad->u2.VadFlags2.Inherit = 1;
        NewVad->u.VadFlags.Protection = ProtectionMask;
        NewVad->u2.VadFlags2.CopyOnWrite = 0;

        //
        // Note that for MEM_DOS_LIM significance is lost here, but those
        // files are not mapped MEM_RESERVE.
        //

        NewVad->u2.VadFlags2.FileOffset = (ULONG)(SectionOffset->QuadPart >> 16);

        //
        // If this is a page file backed section, charge the process's page
        // file quota as if all the pages have been committed.  This solves
        // the problem when other processes commit all the pages and leave
        // only one process around who may not have been charged the proper
        // quota.  This is solved by charging everyone the maximum quota.
        //

        PteOffset += (ULONG)(NewVad->EndingVpn - NewVad->StartingVpn);

        if (PteOffset < Subsection->PtesInSubsection ) {
            NewVad->LastContiguousPte = &Subsection->SubsectionBase[PteOffset];

        } else {
            NewVad->LastContiguousPte = &Subsection->SubsectionBase[
                                        (Subsection->PtesInSubsection - 1) +
                                        Subsection->UnusedPtes];
        }

        ASSERT (NewVad->FirstPrototypePte <= NewVad->LastContiguousPte);
        MiInsertVad (NewVad);

    } except (EXCEPTION_EXECUTE_HANDLER) {

        LOCK_PFN (OldIrql);
        ControlArea->NumberOfMappedViews -= 1;
        ControlArea->NumberOfUserReferences -= 1;
        UNLOCK_PFN (OldIrql);

        if (NewVad != (PMMVAD)NULL) {

            //
            // The pool allocation succeeded, but the quota charge
            // in InsertVad failed, deallocate the pool and return
            // an error.
            //

            ExFreePool (NewVad);

            UNLOCK_WS_UNSAFE (Process);

            return GetExceptionCode();
        }
        
        UNLOCK_WS_UNSAFE (Process);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    UNLOCK_WS_UNSAFE (Process);

    //
    // Update the current virtual size in the process header.
    //

    *CapturedViewSize = (PCHAR)NewEndingAddress - (PCHAR)NewStartingAddress + 1L;

    Process->VirtualSize += *CapturedViewSize;

    if (Process->VirtualSize > Process->PeakVirtualSize) {
        Process->PeakVirtualSize = Process->VirtualSize;
    }

    *CapturedBase = NewStartingAddress;

    *Vad = NewVad;

    return STATUS_SUCCESS;
}
#endif

NTSTATUS
MiSetCopyPagesFor4kPage(
    IN PEPROCESS Process,
    IN OUT PMMVAD *Vad,
    IN OUT PVOID *StartingAddress,
    IN OUT PVOID *EndingAddress,
    IN ULONG NewProtection
    )
/*++

Routine Description:

    This function creates another map for the existing mapped view space then give 
    copy-on-write protection. This function is called when SetProtectionOnSection() 
    tries to change the protection from non copy-on-write to copy-on-write. Since a 
    large native page cannot be broken to shared and copy-on-written 4kb pages, 
    references to the copy-on-written page(s) needs to fixed to reference to 
    the new mapped view space and this should be done through the smart tlb handler
    and the alternate page table entries.

Arguments:

    Process - Supplies a EPROCESS pointer of the current process.

    Vad - Supplies a pointer to the pointer to the VAD containing the range to 
          protect.
   
    StartingAddress - Supplies a pointer to the starting address to protect.

    EndingAddress - Supplies a pointer to the ending address to the protect.

    NewProtect - Supplies the new protection to set.

Return Value:

    Returns a NT status code.

Environment:

    Kernel mode, working set mutex held, address creating mutex held
    APCs disabled.

--*/
{
    LARGE_INTEGER SectionOffset;
    SIZE_T CapturedViewSize;
    PVOID CapturedBase;
    PVOID Va;
    PVOID VaEnd;
    PVOID Alias;
    PMMVAD NewVad;
    PMMPTE PointerPte;
    PMMPTE AltPte;
#if USE_MAPVIEW
    BOOLEAN ReleasedWsMutex;
    SECTION Section;
    PCONTROL_AREA ControlArea;
#endif
    NTSTATUS status;

    SectionOffset.QuadPart = (ULONG_PTR)*StartingAddress - 
        (ULONG_PTR)((*Vad)->StartingVpn << PAGE_SHIFT);

    CapturedBase = (PVOID)NULL;
    CapturedViewSize = (ULONG_PTR)*EndingAddress - (ULONG_PTR)*StartingAddress + 1;

#if USE_MAPVIEW

    ControlArea = (*Vad)->ControlArea;

    RtlZeroMemory((PVOID)&Section, sizeof(Section));

    ReleasedWsMutex = FALSE;

    UNLOCK_WS_UNSAFE (Process);

    status = MiMapViewOfDataSection (ControlArea,
                                     Process,
                                     &CapturedBase,
                                     &SectionOffset,
                                     &CapturedViewSize,
                                     &Section,
                                     ViewShare,
                                     (ULONG)(*Vad)->u.VadFlags.Protection,
                                     0,
                                     0,
                                     0,
                                     &ReleasedWsMutex);
        
    if (!ReleasedWsMutex) {
        UNLOCK_WS_UNSAFE (Process);
    }

    if (status != STATUS_SUCCESS) {

        return status;
    }    

    NewVad = MiLocateAddress(CapturedBase);

    ASSERT(NewVad != (PMMVAD)NULL);

#else

    UNLOCK_WS_UNSAFE (Process);

    status = MiCreateAliasOfDataSection((*Vad)->ControlArea,
                                        Process,
                                        &CapturedBase,
                                        &SectionOffset,
                                        &CapturedViewSize,
                                        (ULONG)(*Vad)->u.VadFlags.Protection,
                                        &NewVad);
    if (status != STATUS_SUCCESS) {

        LOCK_WS_UNSAFE (Process);
        return status;
    }    


#endif

    LOCK_ALTERNATE_TABLE (Process->Wow64Process);

    Va = *StartingAddress;
    VaEnd = *EndingAddress;
    Alias = CapturedBase;

    while (Va <= VaEnd) {

        AltPte = MiGetAltPteAddress (Va);
        PointerPte = MiGetPteAddress (Alias);
        if (AltPte->u.Alt.CopyOnWrite == 1) {
            AltPte->u.Alt.PteOffset = (ULONG_PTR)PointerPte - PTE_UBASE;
            AltPte->u.Alt.PteIndirect = 1;
        }

        Va = (PVOID)((ULONG_PTR)Va + PAGE_4K);
        Alias = (PVOID)((ULONG_PTR)Alias + PAGE_4K);
    }
        
    MiMarkSplitPages(*StartingAddress, 
                     *EndingAddress,
                     Process->Wow64Process->AltPermBitmap,
                     TRUE);

    UNLOCK_ALTERNATE_TABLE (Process->Wow64Process);

    LOCK_WS_UNSAFE (Process);

    Process->Wow64Process->AltFlags |= MI_ALTFLG_FLUSH2G;

    *Vad = NewVad;
    *StartingAddress = CapturedBase;
    *EndingAddress = (PVOID)((ULONG_PTR)CapturedBase + CapturedViewSize - 1L);

    return STATUS_SUCCESS;
}    

VOID
MiLockFor4kPage(
    PVOID CapturedBase,
    SIZE_T CapturedRegionSize,
    PEPROCESS Process
    )
/*++

Routine Description:

    This function adds the page locked attributes to the alternate table entries.

Arguments:

    CapturedBase - Supplies the base address to be locked.

    CapturedREgionSize - Supplies the size of the region to be locked.

    Process - Supplies a pointer to the process object.
   
    
Return Value:

    None.

Environment:

    Kernel mode, the address creation mutex is held.

--*/
{
    PWOW64_PROCESS Wow64Process = Process->Wow64Process;
    PVOID EndingAddress;
    PMMPTE StartAltPte;
    PMMPTE EndAltPte;

    EndingAddress = (PVOID)((ULONG_PTR)CapturedBase + CapturedRegionSize - 1);

    StartAltPte = MiGetAltPteAddress(CapturedBase);
    EndAltPte = MiGetAltPteAddress(EndingAddress);

    LOCK_ALTERNATE_TABLE (Wow64Process);
    
    while (StartAltPte <= EndAltPte) {

        StartAltPte->u.Alt.Lock = 1;
        StartAltPte++;

    }

    UNLOCK_ALTERNATE_TABLE (Wow64Process);

}

NTSTATUS
MiUnlockFor4kPage(
    PVOID CapturedBase,
    SIZE_T CapturedRegionSize,
    PEPROCESS Process
    )
/*++

Routine Description:

    This function removes the page locked attributes from the alternate table entries.

Arguments:

    CapturedBase - Supplies the base address to be unlocked.

    CapturedREgionSize - Supplies the size of the region to be unlocked.

    Process - Supplies a pointer to the process object.
   
    
Return Value:

    None.

Environment:

    Kernel mode, the address creation mutex is held.

--*/
{
    PMMPTE StartAltPte;
    PMMPTE EndAltPte;
    PWOW64_PROCESS Wow64Process = Process->Wow64Process;
    PVOID EndingAddress;
    NTSTATUS Status = STATUS_SUCCESS;

    EndingAddress = (PVOID)((ULONG_PTR)CapturedBase + CapturedRegionSize - 1);

    StartAltPte = MiGetAltPteAddress(CapturedBase);
    EndAltPte = MiGetAltPteAddress(EndingAddress);

    //
    // unlock the working set mutex
    //

    UNLOCK_WS_UNSAFE (Process);

    LOCK_ALTERNATE_TABLE (Wow64Process);
    
    while (StartAltPte <= EndAltPte) {

        if (StartAltPte->u.Alt.Lock == 0) {

            Status = STATUS_NOT_LOCKED;
            goto StatusReturn;

        }

        StartAltPte++;
    }

    StartAltPte = MiGetAltPteAddress(CapturedBase);

    while (StartAltPte <= EndAltPte) {

        StartAltPte->u.Alt.Lock = 0;

        StartAltPte++;
    }

StatusReturn:

    UNLOCK_ALTERNATE_TABLE (Wow64Process);

    LOCK_WS_UNSAFE (Process);

    return (Status);
}

BOOLEAN
MiShouldBeUnlockedFor4kPage(
    PVOID VirtualAddress,
    PEPROCESS Process
    )
/*++

Routine Description:

    This function examines whether the pape should be unlocked.

Arguments:

    VirtualAddress - Supplies the virtual address to be examined.

    Process - Supplies a pointer to the process object.
   
    
Return Value:

    None.

Environment:

    Kernel mode, the working set mutex is held and the address 
    creation mutex is held.

--*/
{
    PMMPTE PointerAltPte;
    PMMPTE StartAltPte;
    PMMPTE EndAltPte;
    PWOW64_PROCESS Wow64Process = Process->Wow64Process;
    PVOID VirtualAligned;
    PVOID EndingAddress;
    BOOLEAN PageUnlocked = TRUE;

    VirtualAligned = PAGE_ALIGN(VirtualAddress);
    EndingAddress = (PVOID)((ULONG_PTR)VirtualAligned + PAGE_SIZE - 1);

    StartAltPte = MiGetAltPteAddress(VirtualAligned);
    EndAltPte = MiGetAltPteAddress(EndingAddress);

    //
    // unlock the working set mutex
    //

    UNLOCK_WS_UNSAFE (Process);

    LOCK_ALTERNATE_TABLE (Wow64Process);

    while (StartAltPte <= EndAltPte) {

        if (StartAltPte->u.Alt.Lock != 0) {
            PageUnlocked = FALSE;
        }

        StartAltPte++;
    }

    UNLOCK_ALTERNATE_TABLE (Wow64Process);

    LOCK_WS_UNSAFE (Process);

    return (PageUnlocked);
}

VOID
MiCopyOnWriteFor4kPage(
    PVOID VirtualAddress,
    PEPROCESS Process
    )
/*++

Routine Description:

    This function changes the protection of the alt pages for a copy on 
    written native page. 

Arguments:

    VirtualAddress - Supplies the virtual address which caused the 
                     copy-on-written.

    Process - Supplies a pointer to the process object.

Return Value: 

    None.

Environment:

    Kernel mode, alternate table lock is held.

--**/
{
    PMMPTE PointerAltPte;
    MMPTE TempAltPte;
    ULONG i;
    
    PointerAltPte = MiGetAltPteAddress(PAGE_ALIGN(VirtualAddress));
    
    for (i = 0; i < SPLITS_PER_PAGE; i++) {
     
        TempAltPte.u.Long = PointerAltPte->u.Long;

        if ((TempAltPte.u.Alt.Commit != 0) && 
            (TempAltPte.u.Alt.CopyOnWrite != 0)) {

            TempAltPte.u.Alt.CopyOnWrite = 0;
            TempAltPte.u.Alt.Private = 1;
            TempAltPte.u.Hard.Write = 1;

            TempAltPte.u.Alt.Protection = 
                    MI_MAKE_PROTECT_NOT_WRITE_COPY(PointerAltPte->u.Alt.Protection);

        } else {

            //
            // make IA64 binary work
            //

            TempAltPte.u.Alt.Private = 1;

            MiCheckPointBreak();

        }
        
        //
        // atomic PTE update
        //

        PointerAltPte->u.Long = TempAltPte.u.Long;
        PointerAltPte++;
    }
}

VOID
MiCheckDemandZeroCopyOnWriteFor4kPage(
    PVOID VirtualAddress,
    PEPROCESS Process
    )
/*++

Routine Description:

    This function changes the protection of the alt pages for a copy on 
    written native page. 

Arguments:

    VirtualAddress - Supplies the virtual address which caused the 
                     copy-on-written.

    Process - Supplies a pointer to the process object.

Return Value: 

    None.

Environment:

    Kernel mode, alternate table lock is held.

--**/
{
    PMMPTE PointerPpe;
    PMMPTE PointerPde;
    PMMPTE PointerPte;
    MMPTE PteContents;
    PMMPTE ProtoPte;
    PMMPFN Pfn1;
    KIRQL OldIrql;
    ULONG OriginalProtection;
    ULONGLONG ProtectionMaskOriginal;
    ULONG Waited;

    //
    // get the original protection from the OS
    //

    LOCK_WS (Process);

    ProtoPte = MiCheckVirtualAddress (VirtualAddress, &OriginalProtection);

    if (OriginalProtection == MM_UNKNOWN_PROTECTION) {

        if (!MI_IS_PHYSICAL_ADDRESS(ProtoPte)) {
            PointerPde = MiGetPteAddress (ProtoPte);
            LOCK_PFN (OldIrql);
            if (PointerPde->u.Hard.Valid == 0) {
                MiMakeSystemAddressValidPfn (ProtoPte);
            }
            Pfn1 = MI_PFN_ELEMENT (PointerPde->u.Hard.PageFrameNumber);
            Pfn1->u3.e2.ReferenceCount += 1;
            ASSERT (Pfn1->u3.e2.ReferenceCount > 1);
            UNLOCK_PFN (OldIrql);
        }

        if (!IS_PTE_NOT_DEMAND_ZERO(*ProtoPte)) {

            PointerPde = MiGetPdeAddress(VirtualAddress);
            PointerPpe = MiGetPpeAddress(VirtualAddress);
            PointerPte = MiGetPteAddress(VirtualAddress);

            if (MiDoesPpeExistAndMakeValid (PointerPpe,
                                            Process,
                                            FALSE,
                                            &Waited) == FALSE) {
                PteContents.u.Long = 0;
    
            } else if (MiDoesPdeExistAndMakeValid (PointerPde,
                                                   Process,
                                                   FALSE,
                                                   &Waited) == FALSE) {

                PteContents.u.Long = 0;

            } else {

                //
                // if it is safe to read PointerPte
                //

                PteContents = *PointerPte;

            }

            if (PteContents.u.Hard.Valid != 0) {

                PteContents.u.Hard.CopyOnWrite = 0;
                PteContents.u.Hard.Write = 1;
                PointerPte->u.Long = PteContents.u.Long;

            }
        }

        //
        // Unlock page containing prototype PTEs.
        //

        if (!MI_IS_PHYSICAL_ADDRESS(ProtoPte)) {
            LOCK_PFN (OldIrql);
            ASSERT (Pfn1->u3.e2.ReferenceCount > 1);
            Pfn1->u3.e2.ReferenceCount -= 1;
            UNLOCK_PFN (OldIrql);
        }
    }
        
    UNLOCK_WS (Process);
}

ULONG
MiMakeProtectForNativePage(
    IN PVOID VirtualAddress,
    IN ULONG NewProtect,
    IN PEPROCESS Process
    )
/*++

Routine Description:

    This function makes a PAGE protection mask for native pages. 

Arguments:

    VirtualAddress - Supplies the virtual address to make the PAGE
             protection mask.

    NewProtect - Supplies the protection originally given.

    Process - Supplies a pointer to the process object.

Return Value: 

    None.

Environment:

    Kernel mode.

--**/
{
    PWOW64_PROCESS Wow64Process = Process->Wow64Process;

#if 0
    LOCK_ALTERNATE_TABLE (Wow64Process);
#endif 
    
    if (MI_CHECK_BIT(Wow64Process->AltPermBitmap, 
                     MI_VA_TO_VPN(VirtualAddress)) != 0) {

        if (NewProtect & PAGE_NOACCESS) {
            NewProtect &= ~PAGE_NOACCESS;
            NewProtect |= PAGE_EXECUTE_READWRITE;
        }

        if (NewProtect & PAGE_READONLY) {
            NewProtect &= ~PAGE_READONLY;
            NewProtect |= PAGE_EXECUTE_READWRITE;
        }

        if (NewProtect & PAGE_EXECUTE) {
            NewProtect &= ~PAGE_EXECUTE;
            NewProtect |= PAGE_EXECUTE_READWRITE;
        }

        if (NewProtect & PAGE_EXECUTE_READ) {
            NewProtect &= ~PAGE_EXECUTE_READ;
            NewProtect |= PAGE_EXECUTE_READWRITE;
        }

#if 0            
        //
        // Remove PAGE_GUARD as it is emulated by Altenate Table.
        //

        if (NewProtect & PAGE_GUARD) {
            NewProtect &= ~PAGE_GUARD;
        }
#endif
    }

#if 0
    UNLOCK_ALTERNATE_TABLE (Wow64Process);
#endif

    return NewProtect;
}

VOID
MiCheckVirtualAddressFor4kPage(
    PVOID VirtualAddress,
    PEPROCESS Process
    )
{
    PMMPTE ProtoPte;
    PMMPTE PointerAltPte;
    PMMPTE PointerPde;
    MMPTE AltPteContents;
    MMPTE TempPte;
    ULONG OriginalProtection;
    ULONGLONG ProtectionMaskOriginal;
    PWOW64_PROCESS Wow64Process;
    KIRQL OldIrql;
    PMMPFN Pfn1;
    ULONG i;

    Wow64Process = Process->Wow64Process;

    LOCK_WS (Process);

    ProtoPte = MiCheckVirtualAddress (VirtualAddress, &OriginalProtection);

    if (OriginalProtection == MM_UNKNOWN_PROTECTION) {

        if (!MI_IS_PHYSICAL_ADDRESS(ProtoPte)) {
            PointerPde = MiGetPteAddress (ProtoPte);
            LOCK_PFN (OldIrql);
            if (PointerPde->u.Hard.Valid == 0) {
                MiMakeSystemAddressValidPfn (ProtoPte);
            }
            Pfn1 = MI_PFN_ELEMENT (PointerPde->u.Hard.PageFrameNumber);
            Pfn1->u3.e2.ReferenceCount += 1;
            ASSERT (Pfn1->u3.e2.ReferenceCount > 1);
            UNLOCK_PFN (OldIrql);
        }

        OriginalProtection = 
            MiMakeProtectionMask(MiGetPageProtection(ProtoPte, Process));

        //
        // Unlock page containing prototype PTEs.
        //

        if (!MI_IS_PHYSICAL_ADDRESS(ProtoPte)) {
            LOCK_PFN (OldIrql);
            ASSERT (Pfn1->u3.e2.ReferenceCount > 1);
            Pfn1->u3.e2.ReferenceCount -= 1;
            UNLOCK_PFN (OldIrql);
        }

    }
        
    UNLOCK_WS (Process);

    ProtectionMaskOriginal = MiMakeProtectionAteMask (OriginalProtection);
    ProtectionMaskOriginal |= MM_ATE_COMMIT;

    PointerAltPte = MiGetAltPteAddress(PAGE_ALIGN(VirtualAddress));

    AltPteContents.u.Long = ProtectionMaskOriginal;
    AltPteContents.u.Alt.Protection = OriginalProtection;
        
    for (i = 0; i < SPLITS_PER_PAGE; i += 1) {

        //
        // atomic PTE update
        //

        PointerAltPte->u.Long = AltPteContents.u.Long;
        PointerAltPte += 1;
    }

    //
    // Update the bitmap
    //

    MiMarkSplitPages(
        VirtualAddress,
        VirtualAddress,
        Wow64Process->AltPermBitmap,
        TRUE);
}

MiIsEntireRangeCommittedFor4kPage(
    IN PVOID StartVirtual,
    IN PVOID EndVirtual,
    IN PEPROCESS Process
    )
{
    PWOW64_PROCESS Wow64Process = Process->Wow64Process;

    LOCK_ALTERNATE_TABLE (Wow64Process);

    UNLOCK_ALTERNATE_TABLE (Wow64Process);
}

#endif


