/*
 * Copyright (C) 2002-2005 ReactOS Team (and the authors from the programmers section)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/anonmem.c
 * PURPOSE:         Implementing anonymous memory.
 *
 * PROGRAMMERS:     David Welch
 *                  Casper Hornstrup
 *                  KJK::Hyperion
 *                  Ge van Geldorp
 *                  Eric Kohl
 *                  Royce Mitchell III
 *                  Aleksey Bragin
 *                  Jason Filby
 *                  Art Yerkes
 *                  Gunnar Andre' Dalsnes
 *                  Filip Navara
 *                  Thomas Weidenmueller
 *                  Alex Ionescu
 *                  Trevor McCort
 *                  Steven Edwards
 */

/* INCLUDE *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include "ARM3/miarm.h"

/* FUNCTIONS *****************************************************************/

VOID
MmModifyAttributes(IN PMMSUPPORT AddressSpace,
                   IN PVOID BaseAddress,
                   IN SIZE_T RegionSize,
                   IN ULONG OldType,
                   IN ULONG OldProtect,
                   IN ULONG NewType,
                   IN ULONG NewProtect)
{
    //
    // This function is deprecated but remains in order to support VirtualAlloc
    // calls with MEM_COMMIT on top of MapViewOfFile calls with SEC_RESERVE.
    //
    // Win32k's shared user heap, for example, uses that mechanism. The two
    // conditions when this function needs to do something are ASSERTed for,
    // because they should not arise.
    //
    if (NewType == MEM_RESERVE && OldType == MEM_COMMIT)
    {
        ASSERT(FALSE);
    }

    if ((NewType == MEM_COMMIT) && (OldType == MEM_COMMIT))
    {
        ASSERT(OldProtect == NewProtect);
    }
}

NTSTATUS
NTAPI
MiRosAllocateVirtualMemory(IN HANDLE ProcessHandle,
                           IN PEPROCESS Process,
                           IN PMEMORY_AREA MemoryArea,
                           IN PMMSUPPORT AddressSpace,
                           IN OUT PVOID* UBaseAddress,
                           IN BOOLEAN Attached,
                           IN OUT PSIZE_T URegionSize,
                           IN ULONG AllocationType,
                           IN ULONG Protect)
{
    ULONG_PTR PRegionSize;
    ULONG Type, RegionSize;
    NTSTATUS Status;
    PVOID PBaseAddress, BaseAddress;
    KAPC_STATE ApcState;

    PBaseAddress = *UBaseAddress;
    PRegionSize = *URegionSize;

    BaseAddress = (PVOID)PAGE_ROUND_DOWN(PBaseAddress);
    RegionSize = PAGE_ROUND_UP((ULONG_PTR)PBaseAddress + PRegionSize) -
    PAGE_ROUND_DOWN(PBaseAddress);
    Type = (AllocationType & MEM_COMMIT) ? MEM_COMMIT : MEM_RESERVE;

    ASSERT(PBaseAddress != 0);
    ASSERT(Type == MEM_COMMIT);
    ASSERT(MemoryArea->Type == MEMORY_AREA_SECTION_VIEW);
    ASSERT(((ULONG_PTR)BaseAddress + RegionSize) <= (ULONG_PTR)MemoryArea->EndingAddress);
    ASSERT(((ULONG_PTR)MemoryArea->EndingAddress - (ULONG_PTR)MemoryArea->StartingAddress) >= RegionSize);
    ASSERT(MemoryArea->Data.SectionData.RegionListHead.Flink);

    Status = MmAlterRegion(AddressSpace,
                           MemoryArea->StartingAddress,
                           &MemoryArea->Data.SectionData.RegionListHead,
                           BaseAddress,
                           RegionSize,
                           Type,
                           Protect,
                           MmModifyAttributes);

    MmUnlockAddressSpace(AddressSpace);
    if (Attached) KeUnstackDetachProcess(&ApcState);
    if (ProcessHandle != NtCurrentProcess()) ObDereferenceObject(Process);
    if (NT_SUCCESS(Status))
    {
        *UBaseAddress = BaseAddress;
        *URegionSize = RegionSize;
    }

    return Status;
}

NTSTATUS NTAPI
MiProtectVirtualMemory(IN PEPROCESS Process,
    IN OUT PVOID *BaseAddress,
    IN OUT PSIZE_T NumberOfBytesToProtect,
    IN ULONG NewAccessProtection,
    OUT PULONG OldAccessProtection  OPTIONAL)
{
    PMEMORY_AREA MemoryArea;
    PMMSUPPORT AddressSpace;
    ULONG OldAccessProtection_;
    NTSTATUS Status;

    *NumberOfBytesToProtect =
    PAGE_ROUND_UP((ULONG_PTR)(*BaseAddress) + (*NumberOfBytesToProtect)) -
    PAGE_ROUND_DOWN(*BaseAddress);
    *BaseAddress = (PVOID)PAGE_ROUND_DOWN(*BaseAddress);

    AddressSpace = &Process->Vm;

    MmLockAddressSpace(AddressSpace);
    MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, *BaseAddress);
    if (MemoryArea == NULL || MemoryArea->DeleteInProgress)
    {
        MmUnlockAddressSpace(AddressSpace);
        return STATUS_UNSUCCESSFUL;
    }

    if (OldAccessProtection == NULL)
    OldAccessProtection = &OldAccessProtection_;

    if (MemoryArea->Type == MEMORY_AREA_SECTION_VIEW)
    {
        Status = MmProtectSectionView(AddressSpace, MemoryArea, *BaseAddress,
            *NumberOfBytesToProtect,
            NewAccessProtection,
            OldAccessProtection);
    }
    else
    {
        /* FIXME: Should we return failure or success in this case? */
        Status = STATUS_CONFLICTING_ADDRESSES;
    }

    MmUnlockAddressSpace(AddressSpace);

    return Status;
}

VOID
NTAPI
MiMakePdeExistAndMakeValid(IN PMMPTE PointerPde,
                           IN PEPROCESS TargetProcess,
                           IN KIRQL OldIrql)
{
   PMMPTE PointerPte, PointerPpe, PointerPxe;

   //
   // Sanity checks. The latter is because we only use this function with the
   // PFN lock not held, so it may go away in the future.
   //
   ASSERT(KeAreAllApcsDisabled() == TRUE);
   ASSERT(OldIrql == MM_NOIRQL);

   //
   // Also get the PPE and PXE. This is okay not to #ifdef because they will
   // return the same address as the PDE on 2-level page table systems.
   //
   // If everything is already valid, there is nothing to do.
   //
   PointerPpe = MiAddressToPte(PointerPde);
   PointerPxe = MiAddressToPde(PointerPde);
   if ((PointerPxe->u.Hard.Valid) &&
       (PointerPpe->u.Hard.Valid) &&
       (PointerPde->u.Hard.Valid))
   {
       return;
   }

   //
   // At least something is invalid, so begin by getting the PTE for the PDE itself
   // and then lookup each additional level. We must do it in this precise order
   // because the pagfault.c code (as well as in Windows) depends that the next
   // level up (higher) must be valid when faulting a lower level
   //
   PointerPte = MiPteToAddress(PointerPde);
   do
   {
       //
       // Make sure APCs continued to be disabled
       //
       ASSERT(KeAreAllApcsDisabled() == TRUE);

       //
       // First, make the PXE valid if needed
       //
       if (!PointerPxe->u.Hard.Valid)
       {
           MiMakeSystemAddressValid(PointerPpe, TargetProcess);
           ASSERT(PointerPxe->u.Hard.Valid == 1);
       }

       //
       // Next, the PPE
       //
       if (!PointerPpe->u.Hard.Valid)
       {
           MiMakeSystemAddressValid(PointerPde, TargetProcess);
           ASSERT(PointerPpe->u.Hard.Valid == 1);
       }

       //
       // And finally, make the PDE itself valid.
       //
       MiMakeSystemAddressValid(PointerPte, TargetProcess);

       //
       // This should've worked the first time so the loop is really just for
       // show -- ASSERT that we're actually NOT going to be looping.
       //
       ASSERT(PointerPxe->u.Hard.Valid == 1);
       ASSERT(PointerPpe->u.Hard.Valid == 1);
       ASSERT(PointerPde->u.Hard.Valid == 1);
   } while (!(PointerPxe->u.Hard.Valid) ||
            !(PointerPpe->u.Hard.Valid) ||
            !(PointerPde->u.Hard.Valid));
}

/*
* @implemented
*/
NTSTATUS
NTAPI
NtAllocateVirtualMemory(IN HANDLE ProcessHandle,
                        IN OUT PVOID* UBaseAddress,
                        IN ULONG_PTR ZeroBits,
                        IN OUT PSIZE_T URegionSize,
                        IN ULONG AllocationType,
                        IN ULONG Protect)
{
    PEPROCESS Process;
    PMEMORY_AREA MemoryArea;
    PFN_NUMBER PageCount;
    PMMVAD Vad, FoundVad;
    PUSHORT UsedPageTableEntries;
    NTSTATUS Status;
    PMMSUPPORT AddressSpace;
    PVOID PBaseAddress;
    ULONG_PTR PRegionSize, StartingAddress, EndingAddress;
    PEPROCESS CurrentProcess = PsGetCurrentProcess();
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PETHREAD CurrentThread = PsGetCurrentThread();
    KAPC_STATE ApcState;
    ULONG ProtectionMask;
    BOOLEAN Attached = FALSE, ChangeProtection = FALSE;
    MMPTE TempPte;
    PMMPTE PointerPte, PointerPde, LastPte;
    PAGED_CODE();

    /* Check for valid Zero bits */
    if (ZeroBits > 21)
    {
        DPRINT1("Too many zero bits\n");
        return STATUS_INVALID_PARAMETER_3;
    }

    /* Check for valid Allocation Types */
    if ((AllocationType & ~(MEM_COMMIT | MEM_RESERVE | MEM_RESET | MEM_PHYSICAL |
                    MEM_TOP_DOWN | MEM_WRITE_WATCH)))
    {
        DPRINT1("Invalid Allocation Type\n");
        return STATUS_INVALID_PARAMETER_5;
    }

    /* Check for at least one of these Allocation Types to be set */
    if (!(AllocationType & (MEM_COMMIT | MEM_RESERVE | MEM_RESET)))
    {
        DPRINT1("No memory allocation base type\n");
        return STATUS_INVALID_PARAMETER_5;
    }

    /* MEM_RESET is an exclusive flag, make sure that is valid too */
    if ((AllocationType & MEM_RESET) && (AllocationType != MEM_RESET))
    {
        DPRINT1("Invalid use of MEM_RESET\n");
        return STATUS_INVALID_PARAMETER_5;
    }

    /* Check if large pages are being used */
    if (AllocationType & MEM_LARGE_PAGES)
    {
        /* Large page allocations MUST be committed */
        if (!(AllocationType & MEM_COMMIT))
        {
            DPRINT1("Must supply MEM_COMMIT with MEM_LARGE_PAGES\n");
            return STATUS_INVALID_PARAMETER_5;
        }

        /* These flags are not allowed with large page allocations */
        if (AllocationType & (MEM_PHYSICAL | MEM_RESET | MEM_WRITE_WATCH))
        {
            DPRINT1("Using illegal flags with MEM_LARGE_PAGES\n");
            return STATUS_INVALID_PARAMETER_5;
        }
    }

    /* MEM_WRITE_WATCH can only be used if MEM_RESERVE is also used */
    if ((AllocationType & MEM_WRITE_WATCH) && !(AllocationType & MEM_RESERVE))
    {
        DPRINT1("MEM_WRITE_WATCH used without MEM_RESERVE\n");
        return STATUS_INVALID_PARAMETER_5;
    }

    /* MEM_PHYSICAL can only be used if MEM_RESERVE is also used */
    if ((AllocationType & MEM_PHYSICAL) && !(AllocationType & MEM_RESERVE))
    {
        DPRINT1("MEM_WRITE_WATCH used without MEM_RESERVE\n");
        return STATUS_INVALID_PARAMETER_5;
    }

    /* Check for valid MEM_PHYSICAL usage */
    if (AllocationType & MEM_PHYSICAL)
    {
        /* Only these flags are allowed with MEM_PHYSIAL */
        if (AllocationType & ~(MEM_RESERVE | MEM_TOP_DOWN | MEM_PHYSICAL))
        {
            DPRINT1("Using illegal flags with MEM_PHYSICAL\n");
            return STATUS_INVALID_PARAMETER_5;
        }

        /* Then make sure PAGE_READWRITE is used */
        if (Protect != PAGE_READWRITE)
        {
            DPRINT1("MEM_PHYSICAL used without PAGE_READWRITE\n");
            return STATUS_INVALID_PARAMETER_6;
        }
    }

    //
    // Force PAGE_READWRITE for everything, for now
    //
    Protect = PAGE_READWRITE;

    /* Calculate the protection mask and make sure it's valid */
    ProtectionMask = MiMakeProtectionMask(Protect);
    if (ProtectionMask == MM_INVALID_PROTECTION)
    {
        DPRINT1("Invalid protection mask\n");
        return STATUS_INVALID_PAGE_PROTECTION;
    }

    /* Enter SEH */
    _SEH2_TRY
    {
        /* Check for user-mode parameters */
        if (PreviousMode != KernelMode)
        {
            /* Make sure they are writable */
            ProbeForWritePointer(UBaseAddress);
            ProbeForWriteUlong(URegionSize);
        }

        /* Capture their values */
        PBaseAddress = *UBaseAddress;
        PRegionSize = *URegionSize;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Return the exception code */
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* Make sure the allocation isn't past the VAD area */
    if (PBaseAddress >= MM_HIGHEST_VAD_ADDRESS)
    {
        DPRINT1("Virtual allocation base above User Space\n");
        return STATUS_INVALID_PARAMETER_2;
    }

    /* Make sure the allocation wouldn't overflow past the VAD area */
    if ((((ULONG_PTR)MM_HIGHEST_VAD_ADDRESS + 1) - (ULONG_PTR)PBaseAddress) < PRegionSize)
    {
        DPRINT1("Region size would overflow into kernel-memory\n");
        return STATUS_INVALID_PARAMETER_4;
    }

    /* Make sure there's a size specified */
    if (!PRegionSize)
    {
        DPRINT1("Region size is invalid (zero)\n");
        return STATUS_INVALID_PARAMETER_4;
    }

    //
    // If this is for the current process, just use PsGetCurrentProcess
    //
    if (ProcessHandle == NtCurrentProcess())
    {
        Process = CurrentProcess;
    }
    else
    {
        //
        // Otherwise, reference the process with VM rights and attach to it if
        // this isn't the current process. We must attach because we'll be touching
        // PTEs and PDEs that belong to user-mode memory, and also touching the
        // Working Set which is stored in Hyperspace.
        //
        Status = ObReferenceObjectByHandle(ProcessHandle,
                                           PROCESS_VM_OPERATION,
                                           PsProcessType,
                                           PreviousMode,
                                           (PVOID*)&Process,
                                           NULL);
        if (!NT_SUCCESS(Status)) return Status;
        if (CurrentProcess != Process)
        {
            KeStackAttachProcess(&Process->Pcb, &ApcState);
            Attached = TRUE;
        }
    }

    //
    // Check for large page allocations and make sure that the required privilege
    // is being held, before attempting to handle them.
    //
    if ((AllocationType & MEM_LARGE_PAGES) &&
        !(SeSinglePrivilegeCheck(SeLockMemoryPrivilege, PreviousMode)))
    {
        /* Fail without it */
        DPRINT1("Privilege not held for MEM_LARGE_PAGES\n");
        Status = STATUS_PRIVILEGE_NOT_HELD;
        goto FailPathNoLock;
    }

    //
    // Assert on the things we don't yet support
    //
    ASSERT(ZeroBits == 0);
    ASSERT((AllocationType & MEM_LARGE_PAGES) == 0);
    ASSERT((AllocationType & MEM_PHYSICAL) == 0);
    ASSERT((AllocationType & MEM_WRITE_WATCH) == 0);
    ASSERT((AllocationType & MEM_TOP_DOWN) == 0);
    ASSERT((AllocationType & MEM_RESET) == 0);
    ASSERT(Process->VmTopDown == 0);

    //
    // Check if the caller is reserving memory, or committing memory and letting
    // us pick the base address
    //
    if (!(PBaseAddress) || (AllocationType & MEM_RESERVE))
    {
        //
        //  Do not allow COPY_ON_WRITE through this API
        //
        if ((Protect & PAGE_WRITECOPY) || (Protect & PAGE_EXECUTE_WRITECOPY))
        {
            DPRINT1("Copy on write not allowed through this path\n");
            Status = STATUS_INVALID_PAGE_PROTECTION;
            goto FailPathNoLock;
        }

        //
        // Does the caller have an address in mind, or is this a blind commit?
        //
        if (!PBaseAddress)
        {
            //
            // This is a blind commit, all we need is the region size
            //
            PRegionSize = ROUND_TO_PAGES(PRegionSize);
            PageCount = BYTES_TO_PAGES(PRegionSize);
        }
        else
        {
            //
            // This is a reservation, so compute the starting address on the
            // expected 64KB granularity, and see where the ending address will
            // fall based on the aligned address and the passed in region size
            //
            StartingAddress = ROUND_DOWN((ULONG_PTR)PBaseAddress, _64K);
            EndingAddress = ((ULONG_PTR)PBaseAddress + PRegionSize - 1) | (PAGE_SIZE - 1);
            PageCount = BYTES_TO_PAGES(EndingAddress - StartingAddress);
        }

        //
        // Allocate and initialize the VAD
        //
        Vad = ExAllocatePoolWithTag(NonPagedPool, sizeof(MMVAD_LONG), 'SdaV');
        ASSERT(Vad != NULL);
        Vad->u.LongFlags = 0;
        if (AllocationType & MEM_COMMIT) Vad->u.VadFlags.MemCommit = 1;
        Vad->u.VadFlags.Protection = ProtectionMask;
        Vad->u.VadFlags.PrivateMemory = 1;
        Vad->u.VadFlags.CommitCharge = AllocationType & MEM_COMMIT ? PageCount : 0;

        //
        // Lock the address space and make sure the process isn't already dead
        //
        AddressSpace = MmGetCurrentAddressSpace();
        MmLockAddressSpace(AddressSpace);
        if (Process->VmDeleted)
        {
            Status = STATUS_PROCESS_IS_TERMINATING;
            goto FailPath;
        }

        //
        // Did we have a base address? If no, find a valid address that is 64KB
        // aligned in the VAD tree. Otherwise, make sure that the address range
        // which was passed in isn't already conflicting with an existing address
        // range.
        //
        EndingAddress = 0;
        if (!PBaseAddress)
        {
            Status = MiFindEmptyAddressRangeInTree(PRegionSize,
                                                   _64K,
                                                   &Process->VadRoot,
                                                   (PMMADDRESS_NODE*)&Process->VadFreeHint,
                                                   &StartingAddress);
            ASSERT(NT_SUCCESS(Status));
        }
        else if (MiCheckForConflictingNode(StartingAddress >> PAGE_SHIFT,
                                           EndingAddress >> PAGE_SHIFT,
                                           &Process->VadRoot))
        {
            //
            // The address specified is in conflict!
            //
            Status = STATUS_CONFLICTING_ADDRESSES;
            goto FailPath;
        }

        //
        // Now we know where the allocation ends. Make sure it doesn't end up
        // somewhere in kernel mode.
        //
        EndingAddress = ((ULONG_PTR)StartingAddress + PRegionSize - 1) | (PAGE_SIZE - 1);
        if ((PVOID)EndingAddress > MM_HIGHEST_VAD_ADDRESS)
        {
            Status = STATUS_NO_MEMORY;
            goto FailPath;
        }

        //
        // Write out the VAD fields for this allocation
        //
        Vad->StartingVpn = (ULONG_PTR)StartingAddress >> PAGE_SHIFT;
        Vad->EndingVpn = (ULONG_PTR)EndingAddress >> PAGE_SHIFT;

        //
        // FIXME: Should setup VAD bitmap
        //
        Status = STATUS_SUCCESS;

        //
        // Lock the working set and insert the VAD into the process VAD tree
        //
        MiLockProcessWorkingSet(Process, CurrentThread);
        Vad->ControlArea = NULL; // For Memory-Area hack
        MiInsertVad(Vad, Process);
        MiUnlockProcessWorkingSet(Process, CurrentThread);

        //
        // Update the virtual size of the process, and if this is now the highest
        // virtual size we have ever seen, update the peak virtual size to reflect
        // this.
        //
        Process->VirtualSize += PRegionSize;
        if (Process->VirtualSize > Process->PeakVirtualSize)
        {
            Process->PeakVirtualSize = Process->VirtualSize;
        }

        //
        // Release address space and detach and dereference the target process if
        // it was different from the current process
        //
        MmUnlockAddressSpace(AddressSpace);
        if (Attached) KeUnstackDetachProcess(&ApcState);
        if (ProcessHandle != NtCurrentProcess()) ObDereferenceObject(Process);

        //
        // Use SEH to write back the base address and the region size. In the case
        // of an exception, we do not return back the exception code, as the memory
        // *has* been allocated. The caller would now have to call VirtualQuery
        // or do some other similar trick to actually find out where its memory
        // allocation ended up
        //
        _SEH2_TRY
        {
            *URegionSize = PRegionSize;
            *UBaseAddress = (PVOID)StartingAddress;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
        }
        _SEH2_END;
        return STATUS_SUCCESS;
    }

    //
    // This is a MEM_COMMIT on top of an existing address which must have been
    // MEM_RESERVED already. Compute the start and ending base addresses based
    // on the user input, and then compute the actual region size once all the
    // alignments have been done.
    //
    StartingAddress = (ULONG_PTR)PAGE_ALIGN(PBaseAddress);
    EndingAddress = (((ULONG_PTR)PBaseAddress + PRegionSize - 1) | (PAGE_SIZE - 1));
    PRegionSize = EndingAddress - StartingAddress + 1;

    //
    // Lock the address space and make sure the process isn't already dead
    //
    AddressSpace = MmGetCurrentAddressSpace();
    MmLockAddressSpace(AddressSpace);
    if (Process->VmDeleted)
    {
        DPRINT1("Process is dying\n");
        Status = STATUS_PROCESS_IS_TERMINATING;
        goto FailPath;
    }

    //
    // Get the VAD for this address range, and make sure it exists
    //
    FoundVad = (PMMVAD)MiCheckForConflictingNode(StartingAddress >> PAGE_SHIFT,
                                                 EndingAddress >> PAGE_SHIFT,
                                                 &Process->VadRoot);
    if (!FoundVad)
    {
        DPRINT1("Could not find a VAD for this allocation\n");
        Status = STATUS_CONFLICTING_ADDRESSES;
        goto FailPath;
    }

    //
    // These kinds of VADs are illegal for this Windows function when trying to
    // commit an existing range
    //
    if ((FoundVad->u.VadFlags.VadType == VadAwe) ||
        (FoundVad->u.VadFlags.VadType == VadDevicePhysicalMemory) ||
        (FoundVad->u.VadFlags.VadType == VadLargePages))
    {
        DPRINT1("Illegal VAD for attempting a MEM_COMMIT\n");
        Status = STATUS_CONFLICTING_ADDRESSES;
        goto FailPath;
    }

    //
    // Make sure that this address range actually fits within the VAD for it
    //
    if (((StartingAddress >> PAGE_SHIFT) < FoundVad->StartingVpn) &&
        ((EndingAddress >> PAGE_SHIFT) > FoundVad->EndingVpn))
    {
        DPRINT1("Address range does not fit into the VAD\n");
        Status = STATUS_CONFLICTING_ADDRESSES;
        goto FailPath;
    }

    //
    // If this is an existing section view, we call the old RosMm routine which
    // has the relevant code required to handle the section scenario. In the future
    // we will limit this even more so that there's almost nothing that the code
    // needs to do, and it will become part of section.c in RosMm
    //
    MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, (PVOID)PAGE_ROUND_DOWN(PBaseAddress));
    if (MemoryArea->Type != MEMORY_AREA_OWNED_BY_ARM3)
    {
        return MiRosAllocateVirtualMemory(ProcessHandle,
                                          Process,
                                          MemoryArea,
                                          AddressSpace,
                                          UBaseAddress,
                                          Attached,
                                          URegionSize,
                                          AllocationType,
                                          Protect);
    }

    //
    // This is a specific ReactOS check because we do not support Section VADs
    //
    ASSERT(FoundVad->u.VadFlags.VadType == VadNone);
    ASSERT(FoundVad->u.VadFlags.PrivateMemory == TRUE);

    //
    // While this is an actual Windows check
    //
    ASSERT(FoundVad->u.VadFlags.VadType != VadRotatePhysical);

    //
    // Throw out attempts to use copy-on-write through this API path
    //
    if ((Protect & PAGE_WRITECOPY) || (Protect & PAGE_EXECUTE_WRITECOPY))
    {
        DPRINT1("Write copy attempted when not allowed\n");
        Status = STATUS_INVALID_PAGE_PROTECTION;
        goto FailPath;
    }

    //
    // Initialize a demand-zero PTE
    //
    TempPte.u.Long = 0;
    TempPte.u.Soft.Protection = ProtectionMask;

    //
    // Get the PTE, PDE and the last PTE for this address range
    //
    PointerPde = MiAddressToPde(StartingAddress);
    PointerPte = MiAddressToPte(StartingAddress);
    LastPte = MiAddressToPte(EndingAddress);

    //
    // Update the commit charge in the VAD as well as in the process, and check
    // if this commit charge was now higher than the last recorded peak, in which
    // case we also update the peak
    //
    FoundVad->u.VadFlags.CommitCharge += (1 + LastPte - PointerPte);
    Process->CommitCharge += (1 + LastPte - PointerPte);
    if (Process->CommitCharge > Process->CommitChargePeak)
    {
        Process->CommitChargePeak = Process->CommitCharge;
    }

    //
    // Lock the working set while we play with user pages and page tables
    //
    //MiLockWorkingSet(CurrentThread, AddressSpace);

    //
    // Make the current page table valid, and then loop each page within it
    //
    MiMakePdeExistAndMakeValid(PointerPde, Process, MM_NOIRQL);
    while (PointerPte <= LastPte)
    {
        //
        // Have we crossed into a new page table?
        //
        if (!(((ULONG_PTR)PointerPte) & (SYSTEM_PD_SIZE - 1)))
        {
            //
            // Get the PDE and now make it valid too
            //
            PointerPde = MiAddressToPte(PointerPte);
            MiMakePdeExistAndMakeValid(PointerPde, Process, MM_NOIRQL);
        }

        //
        // Is this a zero PTE as expected?
        //
        if (PointerPte->u.Long == 0)
        {
            //
            // First increment the count of pages in the page table for this
            // process
            //
            UsedPageTableEntries = &MmWorkingSetList->UsedPageTableEntries[MiGetPdeOffset(MiPteToAddress(PointerPte))];
            (*UsedPageTableEntries)++;
            ASSERT((*UsedPageTableEntries) <= PTE_COUNT);

            //
            // And now write the invalid demand-zero PTE as requested
            //
            MI_WRITE_INVALID_PTE(PointerPte, TempPte);
        }
        else if (PointerPte->u.Long == MmDecommittedPte.u.Long)
        {
            //
            // If the PTE was already decommitted, there is nothing else to do
            // but to write the new demand-zero PTE
            //
            MI_WRITE_INVALID_PTE(PointerPte, TempPte);
        }
        else if (!(ChangeProtection) && (Protect != MiGetPageProtection(PointerPte)))
        {
            //
            // We don't handle these scenarios yet
            //
            if (PointerPte->u.Soft.Valid == 0)
            {
                ASSERT(PointerPte->u.Soft.Prototype == 0);
                ASSERT(PointerPte->u.Soft.PageFileHigh == 0);
            }

            //
            // There's a change in protection, remember this for later, but do
            // not yet handle it.
            //
            DPRINT1("Protection change to: 0x%lx not implemented\n", Protect);
            ChangeProtection = TRUE;
        }

        //
        // Move to the next PTE
        //
        PointerPte++;
    }

    //
    // This path is not yet handled
    //
    ASSERT(ChangeProtection == FALSE);

    //
    // Release the working set lock, unlock the address space, and detach from
    // the target process if it was not the current process. Also dereference the
    // target process if this wasn't the case.
    //
    //MiUnlockProcessWorkingSet(Process, CurrentThread);
    Status = STATUS_SUCCESS;
FailPath:
    MmUnlockAddressSpace(AddressSpace);
FailPathNoLock:
    if (Attached) KeUnstackDetachProcess(&ApcState);
    if (ProcessHandle != NtCurrentProcess()) ObDereferenceObject(Process);

    //
    // Use SEH to write back the base address and the region size. In the case
    // of an exception, we strangely do return back the exception code, even
    // though the memory *has* been allocated. This mimics Windows behavior and
    // there is not much we can do about it.
    //
    _SEH2_TRY
    {
        *URegionSize = PRegionSize;
        *UBaseAddress = (PVOID)StartingAddress;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
    return Status;
}

VOID
NTAPI
MiProcessValidPteList(IN PMMPTE *ValidPteList,
                      IN ULONG Count)
{
    KIRQL OldIrql;
    ULONG i;
    MMPTE TempPte;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn1, Pfn2;

    //
    // Acquire the PFN lock and loop all the PTEs in the list
    //
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
    for (i = 0; i != Count; i++)
    {
        //
        // The PTE must currently be valid
        //
        TempPte = *ValidPteList[i];
        ASSERT(TempPte.u.Hard.Valid == 1);

        //
        // Get the PFN entry for the page itself, and then for its page table
        //
        PageFrameIndex = PFN_FROM_PTE(&TempPte);
        Pfn1 = MiGetPfnEntry(PageFrameIndex);
        Pfn2 = MiGetPfnEntry(Pfn1->u4.PteFrame);

        //
        // Decrement the share count on the page table, and then on the page
        // itself
        //
        MiDecrementShareCount(Pfn2, Pfn1->u4.PteFrame);
        MI_SET_PFN_DELETED(Pfn1);
        MiDecrementShareCount(Pfn1, PageFrameIndex);

        //
        // Make the page decommitted
        //
        MI_WRITE_INVALID_PTE(ValidPteList[i], MmDecommittedPte);
    }

    //
    // All the PTEs have been dereferenced and made invalid, flush the TLB now
    // and then release the PFN lock
    //
    KeFlushCurrentTb();
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
}

ULONG
NTAPI
MiDecommitPages(IN PVOID StartingAddress,
                IN PMMPTE EndingPte,
                IN PEPROCESS Process,
                IN PMMVAD Vad)
{
    PMMPTE PointerPde, PointerPte, CommitPte = NULL;
    ULONG CommitReduction = 0;
    PMMPTE ValidPteList[256];
    ULONG PteCount = 0;
    PMMPFN Pfn1;
    MMPTE PteContents;
    PUSHORT UsedPageTableEntries;
    PETHREAD CurrentThread = PsGetCurrentThread();

    //
    // Get the PTE and PTE for the address, and lock the working set
    // If this was a VAD for a MEM_COMMIT allocation, also figure out where the
    // commited range ends so that we can do the right accounting.
    //
    PointerPde = MiAddressToPde(StartingAddress);
    PointerPte = MiAddressToPte(StartingAddress);
    if (Vad->u.VadFlags.MemCommit) CommitPte = MiAddressToPte(Vad->EndingVpn << PAGE_SHIFT);
    MiLockWorkingSet(CurrentThread, &Process->Vm);

    //
    // Make the PDE valid, and now loop through each page's worth of data
    //
    MiMakePdeExistAndMakeValid(PointerPde, Process, MM_NOIRQL);
    while (PointerPte <= EndingPte)
    {
        //
        // Check if we've crossed a PDE boundary
        //
        if ((((ULONG_PTR)PointerPte) & (SYSTEM_PD_SIZE - 1)) == 0)
        {
            //
            // Get the new PDE and flush the valid PTEs we had built up until
            // now. This helps reduce the amount of TLB flushing we have to do.
            // Note that Windows does a much better job using timestamps and
            // such, and does not flush the entire TLB all the time, but right
            // now we have bigger problems to worry about than TLB flushing.
            //
            PointerPde = MiAddressToPde(StartingAddress);
            if (PteCount)
            {
                MiProcessValidPteList(ValidPteList, PteCount);
                PteCount = 0;
            }

            //
            // Make this PDE valid
            //
            MiMakePdeExistAndMakeValid(PointerPde, Process, MM_NOIRQL);
        }

        //
        // Read this PTE. It might be active or still demand-zero.
        //
        PteContents = *PointerPte;
        if (PteContents.u.Long)
        {
            //
            // The PTE is active. It might be valid and in a working set, or
            // it might be a prototype PTE or paged out or even in transition.
            //
            if (PointerPte->u.Long == MmDecommittedPte.u.Long)
            {
                //
                // It's already decommited, so there's nothing for us to do here
                //
                CommitReduction++;
            }
            else
            {
                //
                // Remove it from the counters, and check if it was valid or not
                //
                //Process->NumberOfPrivatePages--;
                if (PteContents.u.Hard.Valid)
                {
                    //
                    // It's valid. At this point make sure that it is not a ROS
                    // PFN. Also, we don't support ProtoPTEs in this code path.
                    //
                    Pfn1 = MiGetPfnEntry(PteContents.u.Hard.PageFrameNumber);
                    ASSERT(MI_IS_ROS_PFN(Pfn1) == FALSE);
                    ASSERT(Pfn1->u3.e1.PrototypePte == FALSE);

                    //
                    // Flush any pending PTEs that we had not yet flushed, if our
                    // list has gotten too big, then add this PTE to the flush list.
                    //
                    if (PteCount == 256)
                    {
                        MiProcessValidPteList(ValidPteList, PteCount);
                        PteCount = 0;
                    }
                    ValidPteList[PteCount++] = PointerPte;
                }
                else
                {
                    //
                    // We do not support any of these other scenarios at the moment
                    //
                    ASSERT(PteContents.u.Soft.Prototype == 0);
                    ASSERT(PteContents.u.Soft.Transition == 0);
                    ASSERT(PteContents.u.Soft.PageFileHigh == 0);

                    //
                    // So the only other possibility is that it is still a demand
                    // zero PTE, in which case we undo the accounting we did
                    // earlier and simply make the page decommitted.
                    //
                    //Process->NumberOfPrivatePages++;
                    MI_WRITE_INVALID_PTE(PointerPte, MmDecommittedPte);
                }
            }
        }
        else
        {
            //
            // This used to be a zero PTE and it no longer is, so we must add a
            // reference to the pagetable.
            //
            UsedPageTableEntries = &MmWorkingSetList->UsedPageTableEntries[MiGetPdeOffset(StartingAddress)];
            (*UsedPageTableEntries)++;
            ASSERT((*UsedPageTableEntries) <= PTE_COUNT);

            //
            // Next, we account for decommitted PTEs and make the PTE as such
            //
            if (PointerPte > CommitPte) CommitReduction++;
            MI_WRITE_INVALID_PTE(PointerPte, MmDecommittedPte);
        }

        //
        // Move to the next PTE and the next address
        //
        PointerPte++;
        StartingAddress = (PVOID)((ULONG_PTR)StartingAddress + PAGE_SIZE);
    }

    //
    // Flush any dangling PTEs from the loop in the last page table, and then
    // release the working set and return the commit reduction accounting.
    //
    if (PteCount) MiProcessValidPteList(ValidPteList, PteCount);
    MiUnlockWorkingSet(CurrentThread, &Process->Vm);
    return CommitReduction;
}

/*
* @implemented
*/
NTSTATUS
NTAPI
NtFreeVirtualMemory(IN HANDLE ProcessHandle,
                    IN PVOID* UBaseAddress,
                    IN PSIZE_T URegionSize,
                    IN ULONG FreeType)
{
    PMEMORY_AREA MemoryArea;
    ULONG PRegionSize;
    PVOID PBaseAddress;
    ULONG CommitReduction = 0;
    ULONG_PTR StartingAddress, EndingAddress;
    PMMVAD Vad;
    NTSTATUS Status;
    PEPROCESS Process;
    PMMSUPPORT AddressSpace;
    PETHREAD CurrentThread = PsGetCurrentThread();
    PEPROCESS CurrentProcess = PsGetCurrentProcess();
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    KAPC_STATE ApcState;
    BOOLEAN Attached = FALSE;
    PAGED_CODE();

    //
    // Only two flags are supported
    //
    if (!(FreeType & (MEM_RELEASE | MEM_DECOMMIT)))
    {
        DPRINT1("Invalid FreeType\n");
        return STATUS_INVALID_PARAMETER_4;
    }

    //
    // Check if no flag was used, or if both flags were used
    //
    if (!((FreeType & (MEM_DECOMMIT | MEM_RELEASE))) ||
         ((FreeType & (MEM_DECOMMIT | MEM_RELEASE)) == (MEM_DECOMMIT | MEM_RELEASE)))
    {
        DPRINT1("Invalid FreeType combination\n");
        return STATUS_INVALID_PARAMETER_4;
    }

    //
    // Enter SEH for probe and capture. On failure, return back to the caller
    // with an exception violation.
    //
    _SEH2_TRY
    {
        //
        // Check for user-mode parameters and make sure that they are writeable
        //
        if (PreviousMode != KernelMode)
        {
            ProbeForWritePointer(UBaseAddress);
            ProbeForWriteUlong(URegionSize);
        }

        //
        // Capture the current values
        //
        PBaseAddress = *UBaseAddress;
        PRegionSize = *URegionSize;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    //
    // Make sure the allocation isn't past the user area
    //
    if (PBaseAddress >= MM_HIGHEST_USER_ADDRESS)
    {
        DPRINT1("Virtual free base above User Space\n");
        return STATUS_INVALID_PARAMETER_2;
    }

    //
    // Make sure the allocation wouldn't overflow past the user area
    //
    if (((ULONG_PTR)MM_HIGHEST_USER_ADDRESS - (ULONG_PTR)PBaseAddress) < PRegionSize)
    {
        DPRINT1("Region size would overflow into kernel-memory\n");
        return STATUS_INVALID_PARAMETER_3;
    }

    //
    // If this is for the current process, just use PsGetCurrentProcess
    //
    if (ProcessHandle == NtCurrentProcess())
    {
        Process = CurrentProcess;
    }
    else
    {
        //
        // Otherwise, reference the process with VM rights and attach to it if
        // this isn't the current process. We must attach because we'll be touching
        // PTEs and PDEs that belong to user-mode memory, and also touching the
        // Working Set which is stored in Hyperspace.
        //
        Status = ObReferenceObjectByHandle(ProcessHandle,
                                           PROCESS_VM_OPERATION,
                                           PsProcessType,
                                           PreviousMode,
                                           (PVOID*)&Process,
                                           NULL);
        if (!NT_SUCCESS(Status)) return Status;
        if (CurrentProcess != Process)
        {
            KeStackAttachProcess(&Process->Pcb, &ApcState);
            Attached = TRUE;
        }
    }

    //
    // Lock the address space
    //
    AddressSpace = MmGetCurrentAddressSpace();
    MmLockAddressSpace(AddressSpace);

    //
    // If the address space is being deleted, fail the de-allocation since it's
    // too late to do anything about it
    //
    if (Process->VmDeleted)
    {
        DPRINT1("Process is dead\n");
        Status = STATUS_PROCESS_IS_TERMINATING;
        goto FailPath;
    }

    //
    // Compute start and end addresses, and locate the VAD
    //
    StartingAddress = (ULONG_PTR)PAGE_ALIGN(PBaseAddress);
    EndingAddress = ((ULONG_PTR)PBaseAddress + PRegionSize - 1) | (PAGE_SIZE - 1);
    Vad = MiLocateAddress((PVOID)StartingAddress);
    if (!Vad)
    {
        DPRINT1("Unable to find VAD for address 0x%p\n", StartingAddress);
        Status = STATUS_MEMORY_NOT_ALLOCATED;
        goto FailPath;
    }

    //
    // If the range exceeds the VAD's ending VPN, fail this request
    //
    if (Vad->EndingVpn < (EndingAddress >> PAGE_SHIFT))
    {
        DPRINT1("Address 0x%p is beyond the VAD\n", EndingAddress);
        Status = STATUS_UNABLE_TO_FREE_VM;
        goto FailPath;
    }

    //
    // These ASSERTs are here because ReactOS ARM3 does not currently implement
    // any other kinds of VADs.
    //
    ASSERT(Vad->u.VadFlags.PrivateMemory == 1);
    ASSERT(Vad->u.VadFlags.NoChange == 0);
    ASSERT(Vad->u.VadFlags.VadType == VadNone);

    //
    // Finally, make sure there is a ReactOS Mm MEMORY_AREA for this allocation
    // and that is is an ARM3 memory area, and not a section view, as we currently
    // don't support freeing those though this interface.
    //
    MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, (PVOID)StartingAddress);
    ASSERT(MemoryArea);
    ASSERT(MemoryArea->Type == MEMORY_AREA_OWNED_BY_ARM3);

    //
    //  Now we can try the operation. First check if this is a RELEASE or a DECOMMIT
    //
    if (FreeType & MEM_RELEASE)
    {
        //
        // Is the caller trying to remove the whole VAD, or remove only a portion
        // of it? If no region size is specified, then the assumption is that the
        // whole VAD is to be destroyed
        //
        if (!PRegionSize)
        {
            //
            // The caller must specify the base address identically to the range
            // that is stored in the VAD.
            //
            if (((ULONG_PTR)PBaseAddress >> PAGE_SHIFT) != Vad->StartingVpn)
            {
                DPRINT1("Address 0x%p does not match the VAD\n", PBaseAddress);
                Status = STATUS_FREE_VM_NOT_AT_BASE;
                goto FailPath;
            }

            //
            // Now compute the actual start/end addresses based on the VAD
            //
            StartingAddress = Vad->StartingVpn << PAGE_SHIFT;
            EndingAddress = (Vad->EndingVpn << PAGE_SHIFT) | (PAGE_SIZE - 1);

            //
            // Finally lock the working set and remove the VAD from the VAD tree
            //
            MiLockWorkingSet(CurrentThread, &Process->Vm);
            ASSERT(Process->VadRoot.NumberGenericTableElements >= 1);
            MiRemoveNode((PMMADDRESS_NODE)Vad, &Process->VadRoot);
        }
        else
        {
            //
            // This means the caller wants to release a specific region within
            // the range. We have to find out which range this is -- the following
            // possibilities exist plus their union (CASE D):
            //
            // STARTING ADDRESS                                   ENDING ADDRESS
            // [<========][========================================][=========>]
            //   CASE A                  CASE B                       CASE C
            //
            //
            // First, check for case A or D
            //
            if ((StartingAddress >> PAGE_SHIFT) == Vad->StartingVpn)
            {
                //
                // Check for case D
                //
                if ((EndingAddress >> PAGE_SHIFT) == Vad->EndingVpn)
                {
                    //
                    // This is the easiest one to handle -- it is identical to
                    // the code path above when the caller sets a zero region size
                    // and the whole VAD is destroyed
                    //
                    MiLockWorkingSet(CurrentThread, &Process->Vm);
                    ASSERT(Process->VadRoot.NumberGenericTableElements >= 1);
                    MiRemoveNode((PMMADDRESS_NODE)Vad, &Process->VadRoot);
                }
                else
                {
                    //
                    // This case is pretty easy too -- we compute a bunch of
                    // pages to decommit, and then push the VAD's starting address
                    // a bit further down, then decrement the commit charge
                    //
                    // NOT YET IMPLEMENTED IN ARM3.
                    //
                    ASSERT(FALSE);

                    //
                    // After analyzing the VAD, set it to NULL so that we don't
                    // free it in the exit path
                    //
                    Vad = NULL;
                }
            }
            else
            {
                //
                // This is case B or case C. First check for case C
                //
                if ((EndingAddress >> PAGE_SHIFT) == Vad->EndingVpn)
                {
                    //
                    // This is pretty easy and similar to case A. We compute the
                    // amount of pages to decommit, update the VAD's commit charge
                    // and then change the ending address of the VAD to be a bit
                    // smaller.
                    //
                    // NOT YET IMPLEMENTED IN ARM3.
                    //
                    ASSERT(FALSE);
                }
                else
                {
                    //
                    // This is case B and the hardest one. Because we are removing
                    // a chunk of memory from the very middle of the VAD, we must
                    // actually split the VAD into two new VADs and compute the
                    // commit charges for each of them, and reinsert new charges.
                    //
                    // NOT YET IMPLEMENTED IN ARM3.
                    //
                    ASSERT(FALSE);
                }

                //
                // After analyzing the VAD, set it to NULL so that we don't
                // free it in the exit path
                //
                Vad = NULL;
            }
        }

        //
        // Now we have a range of pages to dereference, so call the right API
        // to do that and then release the working set, since we're done messing
        // around with process pages.
        //
        MiDeleteVirtualAddresses(StartingAddress, EndingAddress, NULL);
        MiUnlockWorkingSet(CurrentThread, &Process->Vm);
        Status = STATUS_SUCCESS;

FinalPath:
        //
        // Update the process counters
        //
        PRegionSize = EndingAddress - StartingAddress + 1;
        Process->CommitCharge -= CommitReduction;
        if (FreeType & MEM_RELEASE) Process->VirtualSize -= PRegionSize;

        //
        // Unlock the address space and free the VAD in failure cases. Next,
        // detach from the target process so we can write the region size and the
        // base address to the correct source process, and dereference the target
        // process.
        //
        MmUnlockAddressSpace(AddressSpace);
        if (Vad) ExFreePool(Vad);
        if (Attached) KeUnstackDetachProcess(&ApcState);
        if (ProcessHandle != NtCurrentProcess()) ObDereferenceObject(Process);

        //
        // Use SEH to safely return the region size and the base address of the
        // deallocation. If we get an access violation, don't return a failure code
        // as the deallocation *has* happened. The caller will just have to figure
        // out another way to find out where it is (such as VirtualQuery).
        //
        _SEH2_TRY
        {
            *URegionSize = PRegionSize;
            *UBaseAddress = (PVOID)StartingAddress;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
        }
        _SEH2_END;
        return Status;
    }

    //
    // This is the decommit path.
    // If the caller did not specify a region size, first make sure that this
    // region is actually committed. If it is, then compute the ending address
    // based on the VAD.
    //
    if (!PRegionSize)
    {
        if (((ULONG_PTR)PBaseAddress >> PAGE_SHIFT) != Vad->StartingVpn)
        {
            DPRINT1("Decomitting non-committed memory\n");
            Status = STATUS_FREE_VM_NOT_AT_BASE;
            goto FailPath;
        }
        EndingAddress = (Vad->EndingVpn << PAGE_SHIFT) | (PAGE_SIZE - 1);
    }

    //
    // Decommit the PTEs for the range plus the actual backing pages for the
    // range, then reduce that amount from the commit charge in the VAD
    //
    CommitReduction = MiAddressToPte(EndingAddress) -
                      MiAddressToPte(StartingAddress) +
                      1 -
                      MiDecommitPages((PVOID)StartingAddress,
                                      MiAddressToPte(EndingAddress),
                                      Process,
                                      Vad);
    ASSERT(CommitReduction);
    Vad->u.VadFlags.CommitCharge -= CommitReduction;
    ASSERT(Vad->u.VadFlags.CommitCharge);

    //
    // We are done, go to the exit path without freeing the VAD as it remains
    // valid since we have not released the allocation.
    //
    Vad = NULL;
    Status = STATUS_SUCCESS;
    goto FinalPath;

    //
    // In the failure path, we detach and derefernece the target process, and
    // return whatever failure code was sent.
    //
FailPath:
    if (Attached) KeUnstackDetachProcess(&ApcState);
    if (ProcessHandle != NtCurrentProcess()) ObDereferenceObject(Process);
    return Status;
}

/* EOF */
