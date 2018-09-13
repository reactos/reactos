/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dmpaddr.c

Abstract:

    Temporary routine to print valid addresses within an
    address space.

Author:

    Lou Perazzoli (loup) 20-Mar-1989

Environment:

    Kernel Mode.

Revision History:

--*/

#include "mi.h"

#if DBG

BOOLEAN
MiFlushUnusedSectionInternal (
    IN PCONTROL_AREA ControlArea
    );

#endif //DBG

#if DBG
VOID
MiDumpValidAddresses (
    )

{
    ULONG va = 0;
    ULONG i,j;
    PMMPTE PointerPde;
    PMMPTE PointerPte;

    PointerPde = MiGetPdeAddress (va);


    for (i = 0; i < PDE_PER_PAGE; i++) {
        if (PointerPde->u.Hard.Valid) {
            DbgPrint("  **valid PDE, element %ld  %lx %lx\n",i,i,
                          PointerPde->u.Long);
            PointerPte = MiGetPteAddress (va);
            for (j = 0 ; j < PTE_PER_PAGE; j++) {
                if (PointerPte->u.Hard.Valid) {
                    DbgPrint("Valid address at %lx pte %lx\n", (ULONG)va,
                          PointerPte->u.Long);
                }
                va += PAGE_SIZE;
                PointerPte++;
            }
        } else {
            va += (ULONG)PDE_PER_PAGE * (ULONG)PAGE_SIZE;
        }

        PointerPde++;
    }

    return;

}

#endif //DBG

#if DBG
VOID
MiFormatPte (
    IN PMMPTE PointerPte
    )

{

//       int j;
//       unsigned long pte;
       PMMPTE proto_pte;
       PSUBSECTION subsect;

//   struct a_bit {
//       unsigned long biggies : 31;
//       unsigned long bitties : 1;
//       };
//
//      struct a_bit print_pte;


    if (MmIsAddressValid (PointerPte) == FALSE) {
        DbgPrint("   cannot dump PTE %p - it's not valid\n\n",
                 (ULONG_PTR)PointerPte);
        return;
    }

    proto_pte = MiPteToProto(PointerPte);
    subsect = MiGetSubsectionAddress(PointerPte);

    DbgPrint("***DumpPTE at %p contains %p\n",
             (ULONG_PTR)PointerPte,
             PointerPte->u.Long);

    DbgPrint("   protoaddr %p subsectaddr %p\n\n",
             (ULONG_PTR)proto_pte,
             (ULONG_PTR)subsect);

    return;

//      DbgPrint("page frame number 0x%lx  proto PTE address 0x%lx\n",
//
//      DbgPrint("PTE is 0x%lx\n", PTETOULONG(the_pte));
//
//      proto_pte = MiPteToProto(PointerPte);
//
//      DbgPrint("page frame number 0x%lx  proto PTE address 0x%lx\n",
//            PointerPte->u.Hard.PageFrameNumber,*(PULONG)&proto_pte);
//
//      DbgPrint("  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0  \n");
//      DbgPrint(" +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ \n");
//      DbgPrint(" |      pfn                              |c|p|t|r|r|d|a|c|p|o|w|v| \n");
//      DbgPrint(" |                                       |o|r|r|s|s|t|c|a|b|w|r|l| \n");
//      DbgPrint(" |                                       |w|o|n|v|v|y|c|c|o|n|t|d| \n");
//      DbgPrint(" +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ \n ");
//      pte = PTETOULONG(the_pte);
//
//      for (j = 0; j < 32; j++) {
//         *(PULONG)& print_pte =  pte;
//         DbgPrint(" %lx",print_pte.bitties);
//         pte = pte << 1;
//      }
//       DbgPrint("\n");
//

}
#endif //DBG

#if DBG

VOID
MiDumpWsl ( )

{
    ULONG i;
    PMMWSLE wsle;

    DbgPrint("***WSLE cursize %lx frstfree %lx  Min %lx  Max %lx\n",
        PsGetCurrentProcess()->Vm.WorkingSetSize,
        MmWorkingSetList->FirstFree,
        PsGetCurrentProcess()->Vm.MinimumWorkingSetSize,
        PsGetCurrentProcess()->Vm.MaximumWorkingSetSize);

    DbgPrint("   quota %lx   firstdyn %lx  last ent %lx  next slot %lx\n",
        MmWorkingSetList->Quota,
        MmWorkingSetList->FirstDynamic,
        MmWorkingSetList->LastEntry,
        MmWorkingSetList->NextSlot);

    wsle = MmWsle;

    for (i = 0; i < MmWorkingSetList->LastEntry; i++) {
        DbgPrint(" index %lx  %lx\n",i,wsle->u1.Long);
        wsle++;
    }
    return;

}

#endif //DBG

#if 0 //COMMENTED OUT!!!
VOID
MiFlushUnusedSections (
    VOID
    )

/*++

Routine Description:

    This routine rumages through the PFN database and attempts
    to close any unused sections.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PMMPFN LastPfn;
    PMMPFN Pfn1;
    PSUBSECTION Subsection;
    KIRQL OldIrql;

    LOCK_PFN (OldIrql);
    Pfn1 = MI_PFN_ELEMENT (MmLowestPhysicalPage + 1);
    LastPfn = MI_PFN_ELEMENT(MmHighestPhysicalPage);

    while (Pfn1 < LastPfn) {
        if (Pfn1->OriginalPte.u.Soft.Prototype == 1) {
            if ((Pfn1->u3.e1.PageLocation == ModifiedPageList) ||
                (Pfn1->u3.e1.PageLocation == StandbyPageList)) {

                //
                // Make sure the PTE is not waiting for I/O to complete.
                //

                if (MI_IS_PFN_DELETED (Pfn1)) {

                    Subsection = MiGetSubsectionAddress (&Pfn1->OriginalPte);
                    MiFlushUnusedSectionInternal (Subsection->ControlArea);
                }
            }
        }
        Pfn1++;
    }

    UNLOCK_PFN (OldIrql);
    return;
}

BOOLEAN
MiFlushUnusedSectionInternal (
    IN PCONTROL_AREA ControlArea
    )

{
    BOOLEAN result;
    KIRQL OldIrql = APC_LEVEL;

    if ((ControlArea->NumberOfMappedViews != 0) ||
        (ControlArea->NumberOfSectionReferences != 0)) {

        //
        // The segment is currently in use.
        //

        return FALSE;
    }

    //
    // The segment has no references, delete it.  If the segment
    // is already being deleted, set the event field in the control
    // area and wait on the event.
    //

    if ((ControlArea->u.Flags.BeingDeleted) ||
        (ControlArea->u.Flags.BeingCreated)) {

        return TRUE;
    }

    //
    // Set the being deleted flag and up the number of mapped views
    // for the segment.  Upping the number of mapped views prevents
    // the segment from being deleted and passed to the deletion thread
    // while we are forcing a delete.
    //

    ControlArea->u.Flags.BeingDeleted = 1;
    ControlArea->NumberOfMappedViews = 1;

    //
    // This is a page file backed or image Segment.  The Segment is being
    // deleted, remove all references to the paging file and physical memory.
    //

    UNLOCK_PFN (OldIrql);

    MiCleanSection (ControlArea);

    LOCK_PFN (OldIrql);
    return TRUE;
}
#endif //0


#if DBG

#define ALLOC_SIZE ((ULONG)8*1024)
#define MM_SAVED_CONTROL 64
#define MM_KERN_MAP_SIZE 64

#define MM_NONPAGED_POOL_MARK ((PUCHAR)(ULONG_PTR)0xfffff123)
#define MM_PAGED_POOL_MARK    ((PUCHAR)(ULONG_PTR)0xfffff124)
#define MM_KERNEL_STACK_MARK  ((PUCHAR)(ULONG_PTR)0xfffff125)

extern ULONG_PTR MmSystemPtesStart[MaximumPtePoolTypes];
extern ULONG_PTR MmSystemPtesEnd[MaximumPtePoolTypes];

typedef struct _KERN_MAP {
    ULONG_PTR StartVa;
    ULONG_PTR EndVa;
    PLDR_DATA_TABLE_ENTRY Entry;
} KERN_MAP, *PKERN_MAP;

ULONG
MiBuildKernelMap (
    IN ULONG NumberOfElements,
    IN OUT PKERN_MAP KernelMap
    );

NTSTATUS
MmMemoryUsage (
    IN PVOID Buffer,
    IN ULONG Size,
    IN ULONG Type,
    OUT PULONG OutLength
    )

/*++

Routine Description:

    This routine (debugging only) dumps the current memory usage by
    walking the PFN database.

Arguments:

    Buffer - Supplies a buffer in which to copy the data.

    Size - Supplies the size of the buffer.

    Type - Supplies a value of 0 to dump everything,
           a value of 1 to dump only valid pages.

    OutLength - Returns how much data was written into the buffer.

Return Value:

    None.

--*/

{
    PMMPFN LastPfn;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    PSUBSECTION Subsection;
    KIRQL OldIrql;
    PSYSTEM_MEMORY_INFORMATION MemInfo;
    PSYSTEM_MEMORY_INFO Info;
    PSYSTEM_MEMORY_INFO InfoStart;
    PSYSTEM_MEMORY_INFO InfoEnd;
    PUCHAR String;
    PUCHAR Master;
    PCONTROL_AREA ControlArea;
    BOOLEAN Found;
    BOOLEAN FoundMap;
    PMDL Mdl;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG Length;
    PEPROCESS Process;
    PUCHAR End;
    PCONTROL_AREA SavedControl[MM_SAVED_CONTROL];
    PSYSTEM_MEMORY_INFO  SavedInfo[MM_SAVED_CONTROL];
    ULONG j;
    ULONG ControlCount = 0;
    PUCHAR PagedSection = NULL;
    ULONG Failed;
    UCHAR PageFileMapped[] = "PageFile Mapped";
    UCHAR MetaFile[] =       "Fs Meta File";
    UCHAR NoName[] =         "No File Name";
    UCHAR NonPagedPool[] =   "NonPagedPool";
    UCHAR PagedPool[] =      "PagedPool";
    UCHAR KernelStack[] =    "Kernel Stack";
    PUCHAR NameString;
    KERN_MAP KernMap[MM_KERN_MAP_SIZE];
    ULONG KernSize;
    ULONG_PTR VirtualAddress;
    PLDR_DATA_TABLE_ENTRY DataTableEntry;

    Mdl = MmCreateMdl (NULL, Buffer, Size);
    try {

        MmProbeAndLockPages (Mdl, KeGetPreviousMode(), IoWriteAccess);

    } except (EXCEPTION_EXECUTE_HANDLER) {

        ExFreePool (Mdl);
        return GetExceptionCode();
    }

    MemInfo = MmGetSystemAddressForMdl (Mdl);
    InfoStart = &MemInfo->Memory[0];
    InfoEnd = InfoStart;
    End = (PUCHAR)MemInfo + Size;

    Pfn1 = MI_PFN_ELEMENT (MmLowestPhysicalPage + 1);
    LastPfn = MI_PFN_ELEMENT(MmHighestPhysicalPage);

    KernSize = MiBuildKernelMap (MM_KERN_MAP_SIZE, &KernMap[0]);

    LOCK_PFN (OldIrql);

    while (Pfn1 < LastPfn) {

        Info = InfoStart;
        FoundMap = FALSE;

        if ((Pfn1->u3.e1.PageLocation != FreePageList) &&
            (Pfn1->u3.e1.PageLocation != ZeroedPageList) &&
            (Pfn1->u3.e1.PageLocation != BadPageList)) {

            if (Type == 1) {
                if (Pfn1->u3.e1.PageLocation != ActiveAndValid) {
                    Pfn1++;
                    continue;
                }
            }
            if (Pfn1->OriginalPte.u.Soft.Prototype == 1) {
                Subsection = MiGetSubsectionAddress (&Pfn1->OriginalPte);
                Master = (PUCHAR)Subsection->ControlArea;
                ControlArea = Subsection->ControlArea;
                if (!MmIsAddressValid(ControlArea)) {
                    DbgPrint ("Pfnp %lx not found %lx\n",Pfn1 - MmPfnDatabase,
                                                    (ULONG_PTR)Pfn1->PteAddress);
                    Pfn1++;
                    continue;
                }
                if (ControlArea->FilePointer != NULL)  {
                    if (!MmIsAddressValid(ControlArea->FilePointer)) {
                        Pfn1++;
                        continue;
                    }
                }

            } else {

                FoundMap = TRUE;
                VirtualAddress = (ULONG_PTR)MiGetVirtualAddressMappedByPte (Pfn1->PteAddress);

                if ((VirtualAddress >= (ULONG_PTR)MmPagedPoolStart) &&
                    (VirtualAddress <= (ULONG_PTR)MmPagedPoolEnd)) {

                    //
                    // This is paged pool, put it in the paged pool cell.
                    //

                    Master = MM_PAGED_POOL_MARK;

                } else if ((VirtualAddress >= (ULONG_PTR)MmNonPagedPoolStart) &&
                    (VirtualAddress <= (ULONG_PTR)MmNonPagedPoolEnd)) {

                    //
                    // This is nonpaged pool, put it in the nonpaged pool cell.
                    //

                    Master = MM_NONPAGED_POOL_MARK;

                } else {
                    FoundMap = FALSE;
                    for (j=0; j < KernSize; j++) {
                        if ((VirtualAddress >= KernMap[j].StartVa) &&
                            (VirtualAddress < KernMap[j].EndVa)) {
                            Master = (PUCHAR)&KernMap[j];
                            FoundMap = TRUE;
                            break;
                        }
                    }
                }

                if (!FoundMap) {
                    if (((ULONG_PTR)Pfn1->PteAddress >= MmSystemPtesStart[SystemPteSpace]) &&
                           ((ULONG_PTR)Pfn1->PteAddress <= MmSystemPtesEnd[SystemPteSpace])) {

                        //
                        // This is kernel stack.
                        //

                        Master = MM_KERNEL_STACK_MARK;
                    } else {
                        Pfn2 = MI_PFN_ELEMENT (Pfn1->PteFrame);
                        Master = (PUCHAR)Pfn2->PteFrame;
                        if (((ULONG_PTR)Master == 0) || ((ULONG_PTR)Master > MmHighestPhysicalPage)) {
                            DbgPrint ("Pfn %lx not found %lx\n",Pfn1 - MmPfnDatabase,
                                                    (ULONG_PTR)Pfn1->PteAddress);
                            Pfn1++;
                            continue;
                        }
                    }
                }
            }

            //
            // See if there is already a master info block.
            //

            Found = FALSE;
            while (Info < InfoEnd) {
                if (Info->StringOffset == Master) {
                    Found = TRUE;
                    break;
                }
                Info += 1;
            }

            if (!Found) {

                Info = InfoEnd;
                InfoEnd += 1;
                if ((PUCHAR)Info >= ((PUCHAR)InfoStart + Size) - sizeof(SYSTEM_MEMORY_INFO)) {
                    status = STATUS_DATA_OVERRUN;
                    goto Done;
                }

                RtlZeroMemory (Info, sizeof(*Info));
                Info->StringOffset = Master;
            }

            if ((Pfn1->u3.e1.PageLocation == StandbyPageList) ||
                (Pfn1->u3.e1.PageLocation == TransitionPage)) {

                Info->TransitionCount += 1;

            } else if ((Pfn1->u3.e1.PageLocation == ModifiedPageList) ||
                (Pfn1->u3.e1.PageLocation == ModifiedNoWritePageList)) {
                Info->ModifiedCount += 1;

            } else {
                Info->ValidCount += 1;
                if (Type == 1) {
                    if ((Pfn1->PteAddress >= MiGetPdeAddress (0x0)) &&
                        (Pfn1->PteAddress <= MiGetPdeAddress (0xFFFFFFFF))) {
                        Info->PageTableCount += 1;
                    }
                }
            }
            if (Type != 1) {
                if ((Pfn1->PteAddress >= MiGetPdeAddress (0x0)) &&
                    (Pfn1->PteAddress <= MiGetPdeAddress (0xFFFFFFFF))) {
                    Info->PageTableCount += 1;
                }
            }
        }
        Pfn1++;
    }

    MemInfo->StringStart = (ULONG)((PUCHAR)Buffer + (ULONG_PTR)InfoEnd - (PUCHAR)MemInfo);
    String = (PUCHAR)InfoEnd;

    //
    // Process strings...
    //

    Info = InfoStart;
    while (Info < InfoEnd) {
        if (Info->StringOffset > (PUCHAR)MM_HIGHEST_USER_ADDRESS) {

            //
            // Make sure this is not stacks or other areas.
            //

            Length = 0;
            ControlArea = NULL;

            if (Info->StringOffset == MM_NONPAGED_POOL_MARK) {
                Length = 14;
                NameString = NonPagedPool;
            } else if (Info->StringOffset == MM_PAGED_POOL_MARK) {
                Length = 14;
                NameString = PagedPool;
            } else if (Info->StringOffset == MM_KERNEL_STACK_MARK) {
                Length = 14;
                NameString = KernelStack;
            } else if (((PUCHAR)Info->StringOffset >= (PUCHAR)&KernMap[0]) &&
                       ((PUCHAR)Info->StringOffset <= (PUCHAR)&KernMap[MM_KERN_MAP_SIZE])) {

                DataTableEntry = ((PKERN_MAP)Info->StringOffset)->Entry;
                NameString = (PUCHAR)DataTableEntry->BaseDllName.Buffer;
                Length = DataTableEntry->BaseDllName.Length;
            } else {
                //
                // This points to a control area.
                // Get the file name.
                //

                ControlArea = (PCONTROL_AREA)(Info->StringOffset);
                NameString = (PUCHAR)&ControlArea->FilePointer->FileName.Buffer[0];
            }

            Info->StringOffset = NULL;
            Failed = TRUE;
            if (Length == 0) {
                if (MmIsAddressValid (&ControlArea->FilePointer->FileName.Length)) {
                    Length = ControlArea->FilePointer->FileName.Length;
                    if (Length == 0) {
                        if (ControlArea->u.Flags.NoModifiedWriting) {
                            Length = 14;
                            NameString = MetaFile;
                        } else if (ControlArea->u.Flags.File == 0) {
                            NameString = PageFileMapped;
                            Length = 16;

                        } else {
                            NameString = NoName;
                            Length = 14;
                        }
                    }
                }
            }

            if ((String+Length+2) >= End) {
                status = STATUS_DATA_OVERRUN;
                goto Done;
            }
            if (MmIsAddressValid (&NameString[0]) &&
                MmIsAddressValid (&NameString[Length - 1])) {
                RtlMoveMemory (String,
                               NameString,
                               Length );
                Info->StringOffset = (PUCHAR)Buffer + ((PUCHAR)String - (PUCHAR)MemInfo);
                String[Length] = 0;
                String[Length + 1] = 0;
                String += Length + 2;
                Failed = FALSE;
            }
            if (Failed && ControlArea) {
                if (!(ControlArea->u.Flags.BeingCreated ||
                      ControlArea->u.Flags.BeingDeleted) &&
                      (ControlCount < MM_SAVED_CONTROL)) {
                    SavedControl[ControlCount] = ControlArea;
                    SavedInfo[ControlCount] = Info;
                    ControlArea->NumberOfSectionReferences += 1;
                    ControlCount += 1;
                }
            }

        } else {

            //
            // Process...
            //

            Pfn1 = MI_PFN_ELEMENT (PtrToUlong(Info->StringOffset));
            Info->StringOffset = NULL;
            if ((String+16) >= End) {
                status = STATUS_DATA_OVERRUN;
                goto Done;
            }

            Process = (PEPROCESS)Pfn1->u1.Event;
            if (Pfn1->PteAddress == MiGetPteAddress (PDE_BASE)) {
                Info->StringOffset = (PUCHAR)Buffer + ((PUCHAR)String - (PUCHAR)MemInfo);
                RtlMoveMemory (String,
                               &Process->ImageFileName[0],
                               16);
                String += 16;
            } else {

                Info->StringOffset = PagedSection;
                if (PagedSection == NULL) {
                    Info->StringOffset = (PUCHAR)Buffer + ((PUCHAR)String - (PUCHAR)MemInfo);
                    RtlMoveMemory (String,
                                   &PageFileMapped,
                                   16);
                    PagedSection = Info->StringOffset;
                    String += 16;
                }
            }
        }

        Info += 1;
    }

Done:
    UNLOCK_PFN (OldIrql);
    while (ControlCount != 0) {

        //
        // Process all the pagable name strings.
        //

        ControlCount -= 1;
        ControlArea = SavedControl[ControlCount];
        Info = SavedInfo[ControlCount];
        NameString = (PUCHAR)&ControlArea->FilePointer->FileName.Buffer[0];
        Length = ControlArea->FilePointer->FileName.Length;
        if (Length == 0) {
            if (ControlArea->u.Flags.NoModifiedWriting) {
                Length = 12;
                NameString = MetaFile;
            } else if (ControlArea->u.Flags.File == 0) {
                NameString = PageFileMapped;
                Length = 16;

            } else {
                NameString = NoName;
                Length = 12;
            }
        }
        if ((String+Length+2) >= End) {
            status = STATUS_DATA_OVERRUN;
        }
        if (status != STATUS_DATA_OVERRUN) {
            RtlMoveMemory (String,
                           NameString,
                           Length );
            Info->StringOffset = (PUCHAR)Buffer + ((PUCHAR)String - (PUCHAR)MemInfo);
            String[Length] = 0;
            String[Length + 1] = 0;
            String += Length + 2;
        }

        LOCK_PFN (OldIrql);
        ControlArea->NumberOfSectionReferences -= 1;
        MiCheckForControlAreaDeletion (ControlArea);
        UNLOCK_PFN (OldIrql);
    }
    *OutLength = (ULONG)((PUCHAR)String - (PUCHAR)MemInfo);
    MmUnlockPages (Mdl);
    ExFreePool (Mdl);;
    return status;
}
#else //DBG

NTSTATUS
MmMemoryUsage (
    IN PVOID Buffer,
    IN ULONG Size,
    IN ULONG Type,
    OUT PULONG OutLength
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

#endif //DBG


#if DBG
ULONG
MiBuildKernelMap (
    IN ULONG NumberOfElements,
    IN OUT PKERN_MAP KernelMap
    )

{
    PLIST_ENTRY Next;
    PLIST_ENTRY NextEntry;
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    ULONG i = 0;

    KeEnterCriticalRegion();
    ExAcquireResourceShared (&PsLoadedModuleResource, TRUE);

    NextEntry = PsLoadedModuleList.Flink;
    do {

        DataTableEntry = CONTAINING_RECORD(NextEntry,
                                           LDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);

        KernelMap[i].Entry = DataTableEntry;
        KernelMap[i].StartVa = (ULONG_PTR)DataTableEntry->DllBase;
        KernelMap[i].EndVa = KernelMap[i].StartVa +
                                         (ULONG_PTR)DataTableEntry->SizeOfImage;
        i += 1;
        if (i == NumberOfElements) {
            break;
        }
        Next = DataTableEntry->InLoadOrderLinks.Flink;

        NextEntry = NextEntry->Flink;
    } while (NextEntry != &PsLoadedModuleList);

    ExReleaseResource (&PsLoadedModuleResource);
    KeLeaveCriticalRegion();

    return i;
}
#endif //DBG



#if DBG
VOID
MiFlushCache (
    VOID
    )

/*++

Routine Description:

    This routine (debugging only) flushes the "cache" by moving
    all pages from the standby list to the free list.  Modified
    pages are not affected.

Arguments:

    None.

Return Value:

    None.

--*/

{
    KIRQL OldIrql;
    PFN_NUMBER Page;

    LOCK_PFN (OldIrql);

    while (MmPageLocationList[StandbyPageList]->Total != 0) {

        Page = MiRemovePageFromList (MmPageLocationList[StandbyPageList]);

        //
        // A page has been removed from the standby list.  The
        // PTE which refers to this page is currently in the transition
        // state and must have its original contents restored to free
        // the last reference to this physical page.
        //

        // MiRestoreTransitionPte (Page);  <-- Done by MiRemove above

        //
        // Put the page into the free list.
        //

        MiInsertPageInList (MmPageLocationList[FreePageList], Page);
    }

    UNLOCK_PFN (OldIrql);
    return;
}
VOID
MiDumpReferencedPages (
    VOID
    )

/*++

Routine Description:

    This routine (debugging only) dumps all PFN entries which appear
    to be locked in memory for i/o.

Arguments:

    None.

Return Value:

    None.

--*/

{
    KIRQL OldIrql;
    PMMPFN Pfn1;
    PMMPFN PfnLast;

    LOCK_PFN (OldIrql);

    Pfn1 = MI_PFN_ELEMENT (MmLowestPhysicalPage);
    PfnLast = MI_PFN_ELEMENT (MmHighestPhysicalPage);

    while (Pfn1 <= PfnLast) {

        if ((Pfn1->u2.ShareCount == 0) && (Pfn1->u3.e2.ReferenceCount != 0)) {
            MiFormatPfn (Pfn1);
        }

        if (Pfn1->u3.e2.ReferenceCount > 1) {
            MiFormatPfn (Pfn1);
        }

        Pfn1 += 1;
    }

    UNLOCK_PFN (OldIrql);
    return;
}

#endif //DBG
