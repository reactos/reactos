/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/i386/page.c
 * PURPOSE:         Low level memory managment manipulation
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES ***************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, MmInitGlobalKernelPageDirectory)
#pragma alloc_text(INIT, MiInitPageDirectoryMap)
#endif


/* GLOBALS *****************************************************************/

#define PA_BIT_PRESENT   (0)
#define PA_BIT_READWRITE (1)
#define PA_BIT_USER      (2)
#define PA_BIT_WT        (3)
#define PA_BIT_CD        (4)
#define PA_BIT_ACCESSED  (5)
#define PA_BIT_DIRTY     (6)
#define PA_BIT_GLOBAL	 (8)

#define PA_PRESENT   (1 << PA_BIT_PRESENT)
#define PA_READWRITE (1 << PA_BIT_READWRITE)
#define PA_USER      (1 << PA_BIT_USER)
#define PA_DIRTY     (1 << PA_BIT_DIRTY)
#define PA_WT        (1 << PA_BIT_WT)
#define PA_CD        (1 << PA_BIT_CD)
#define PA_ACCESSED  (1 << PA_BIT_ACCESSED)
#define PA_GLOBAL    (1 << PA_BIT_GLOBAL)

#define HYPERSPACE		(0xc0400000)
#define IS_HYPERSPACE(v)	(((ULONG)(v) >= HYPERSPACE && (ULONG)(v) < HYPERSPACE + 0x400000))

ULONG MmGlobalKernelPageDirectory[1024];

#define PTE_TO_PFN(X)  ((X) >> PAGE_SHIFT)
#define PFN_TO_PTE(X)  ((X) << PAGE_SHIFT)

#if defined(__GNUC__)
#define PTE_TO_PAGE(X) ((LARGE_INTEGER)(LONGLONG)(PAGE_MASK(X)))
#else
__inline LARGE_INTEGER PTE_TO_PAGE(ULONG npage)
{
   LARGE_INTEGER dummy;
   dummy.QuadPart = (LONGLONG)(PAGE_MASK(npage));
   return dummy;
}
#endif

/* FUNCTIONS ***************************************************************/

BOOLEAN MmUnmapPageTable(PULONG Pt);

ULONG_PTR
NTAPI
MiFlushTlbIpiRoutine(ULONG_PTR Address)
{
   if (Address == (ULONG_PTR)-1)
   {
      KeFlushCurrentTb();
   }
   else if (Address == (ULONG_PTR)-2)
   {
      KeFlushCurrentTb();
   }
   else
   {
       __invlpg((PVOID)Address);
   }
   return 0;
}

VOID
MiFlushTlb(PULONG Pt, PVOID Address)
{
   if ((Pt && MmUnmapPageTable(Pt)) || Address >= MmSystemRangeStart)
   {
      __invlpg(Address);
   }
}



PULONG
MmGetPageDirectory(VOID)
{
   return (PULONG)__readcr3();
}

static ULONG
ProtectToPTE(ULONG flProtect)
{
   ULONG Attributes = 0;

   if (flProtect & (PAGE_NOACCESS|PAGE_GUARD))
   {
      Attributes = 0;
   }
   else if (flProtect & PAGE_IS_WRITABLE)
   {
      Attributes = PA_PRESENT | PA_READWRITE;
   }
   else if (flProtect & (PAGE_IS_READABLE | PAGE_IS_EXECUTABLE))
   {
      Attributes = PA_PRESENT;
   }
   else
   {
      DPRINT1("Unknown main protection type.\n");
      KEBUGCHECK(0);
   }
    
   if (flProtect & PAGE_SYSTEM)
   {
   }
   else
   {
      Attributes = Attributes | PA_USER;
   }
   if (flProtect & PAGE_NOCACHE)
   {
      Attributes = Attributes | PA_CD;
   }
   if (flProtect & PAGE_WRITETHROUGH)
   {
      Attributes = Attributes | PA_WT;
   }
   return(Attributes);
}

NTSTATUS
NTAPI
Mmi386ReleaseMmInfo(PEPROCESS Process)
{
   PUSHORT LdtDescriptor;
   ULONG LdtBase;
   PULONG Pde;
   PULONG PageDir;
   ULONG i, j;

   DPRINT("Mmi386ReleaseMmInfo(Process %x)\n",Process);

   LdtDescriptor = (PUSHORT) &Process->Pcb.LdtDescriptor;
   LdtBase = LdtDescriptor[1] |
             ((LdtDescriptor[2] & 0xff) << 16) |
             ((LdtDescriptor[3] & ~0xff) << 16);

   DPRINT("LdtBase: %x\n", LdtBase);

   if (LdtBase)
   {
      ExFreePool((PVOID) LdtBase);
   }

    PageDir = MmCreateHyperspaceMapping(PTE_TO_PFN(Process->Pcb.DirectoryTableBase.u.LowPart));
    for (i = 0; i < ADDR_TO_PDE_OFFSET(MmSystemRangeStart); i++)
    {
        if (PageDir[i] != 0)
        {
            DPRINT1("Pde for %08x - %08x is not freed, RefCount %d\n",
                    i * 4 * 1024 * 1024, (i + 1) * 4 * 1024 * 1024 - 1,
                    ((PMADDRESS_SPACE)&Process->VadRoot)->PageTableRefCountTable[i]);
            Pde = MmCreateHyperspaceMapping(PTE_TO_PFN(PageDir[i]));
            for (j = 0; j < 1024; j++)
            {
                if(Pde[j] != 0)
                {
                    if (Pde[j] & PA_PRESENT)
                    {
                        DPRINT1("Page at %08x is not freed\n",
                                i * 4 * 1024 * 1024 + j * PAGE_SIZE);
                    }
                    else
                    {
                        DPRINT1("Swapentry %x at %x is not freed\n",
                                Pde[j], i * 4 * 1024 * 1024 + j * PAGE_SIZE);
                        
                    }
                }
            }
            MmDeleteHyperspaceMapping(Pde);
            MmReleasePageMemoryConsumer(MC_NPPOOL, PTE_TO_PFN(PageDir[i]));
        }
    }
    MmReleasePageMemoryConsumer(MC_NPPOOL, PTE_TO_PFN(PageDir[ADDR_TO_PDE_OFFSET(HYPERSPACE)]));
    MmDeleteHyperspaceMapping(PageDir);
    MmReleasePageMemoryConsumer(MC_NPPOOL, PTE_TO_PFN(Process->Pcb.DirectoryTableBase.u.LowPart));
    
#if defined(__GNUC__)

   Process->Pcb.DirectoryTableBase.QuadPart = 0LL;
#else

   Process->Pcb.DirectoryTableBase.QuadPart = 0;
#endif

   DPRINT("Finished Mmi386ReleaseMmInfo()\n");
   return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
MmInitializeHandBuiltProcess(IN PEPROCESS Process,
                             IN PLARGE_INTEGER DirectoryTableBase)
{
    /* Share the directory base with the idle process */
    *DirectoryTableBase = PsGetCurrentProcess()->Pcb.DirectoryTableBase;
    
    /* Initialize the Addresss Space */
    MmInitializeAddressSpace(Process, (PMADDRESS_SPACE)&Process->VadRoot);
    
    /* The process now has an address space */
    Process->HasAddressSpace = TRUE;
    return STATUS_SUCCESS;
}

BOOLEAN
STDCALL
MmCreateProcessAddressSpace(IN ULONG MinWs,
                            IN PEPROCESS Process,
                            IN PLARGE_INTEGER DirectoryTableBase)
{
   NTSTATUS Status;
   ULONG i, j;
   PFN_TYPE Pfn[2];
   PULONG PageDirectory;

   DPRINT("MmCopyMmInfo(Src %x, Dest %x)\n", MinWs, Process);

   for (i = 0; i < 2; i++)
   {
      Status = MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &Pfn[i]);
      if (!NT_SUCCESS(Status))
      {
          for (j = 0; j < i; j++)
          {
              MmReleasePageMemoryConsumer(MC_NPPOOL, Pfn[j]);
          }
          
          return FALSE;
      }
   }

      PageDirectory = MmCreateHyperspaceMapping(Pfn[0]);

      memcpy(PageDirectory + ADDR_TO_PDE_OFFSET(MmSystemRangeStart),
             MmGlobalKernelPageDirectory + ADDR_TO_PDE_OFFSET(MmSystemRangeStart),
             (1024 - ADDR_TO_PDE_OFFSET(MmSystemRangeStart)) * sizeof(ULONG));

      DPRINT("Addr %x\n",ADDR_TO_PDE_OFFSET(PAGETABLE_MAP));
      PageDirectory[ADDR_TO_PDE_OFFSET(PAGETABLE_MAP)] = PFN_TO_PTE(Pfn[0]) | PA_PRESENT | PA_READWRITE;
      PageDirectory[ADDR_TO_PDE_OFFSET(HYPERSPACE)] = PFN_TO_PTE(Pfn[1]) | PA_PRESENT | PA_READWRITE;

      MmDeleteHyperspaceMapping(PageDirectory);

   DirectoryTableBase->QuadPart = PFN_TO_PTE(Pfn[0]);
   DPRINT("Finished MmCopyMmInfo(): %I64x\n", DirectoryTableBase->QuadPart);
   return TRUE;
}

VOID
NTAPI
MmDeletePageTable(PEPROCESS Process, PVOID Address)
{
   PEPROCESS CurrentProcess = PsGetCurrentProcess();

   if (Process != NULL && Process != CurrentProcess)
   {
      KeAttachProcess(&Process->Pcb);
   }

      MiAddressToPde(Address)->u.Long = 0;
      MiFlushTlb((PULONG)MiAddressToPde(Address),
          MiAddressToPte(Address));

   if (Address >= MmSystemRangeStart)
   {
      KEBUGCHECK(0);
      //       MmGlobalKernelPageDirectory[ADDR_TO_PDE_OFFSET(Address)] = 0;
   }
   if (Process != NULL && Process != CurrentProcess)
   {
      KeDetachProcess();
   }
}

VOID
NTAPI
MmFreePageTable(PEPROCESS Process, PVOID Address)
{
   PEPROCESS CurrentProcess = PsGetCurrentProcess();
   ULONG i;
   PFN_TYPE Pfn;
   PULONG PageTable;

   DPRINT("ProcessId %d, Address %x\n", Process->UniqueProcessId, Address);
   if (Process != NULL && Process != CurrentProcess)
   {
      KeAttachProcess(&Process->Pcb);
   }

      PageTable = (PULONG)PAGE_ROUND_DOWN((PVOID)MiAddressToPte(Address));
      for (i = 0; i < 1024; i++)
      {
         if (PageTable[i] != 0)
         {
            DbgPrint("Page table entry not clear at %x/%x (is %x)\n",
                     ((ULONG)Address / (4*1024*1024)), i, PageTable[i]);
            KEBUGCHECK(0);
         }
      }
      Pfn = MiAddressToPde(Address)->u.Hard.PageFrameNumber;
      MiAddressToPde(Address)->u.Long = 0;
      MiFlushTlb((PULONG)MiAddressToPde(Address), MiAddressToPte(Address));

   if (Address >= MmSystemRangeStart)
   {
      //    MmGlobalKernelPageDirectory[ADDR_TO_PDE_OFFSET(Address)] = 0;
      KEBUGCHECK(0);
   }
   else
   {
      MmReleasePageMemoryConsumer(MC_NPPOOL, Pfn);
   }
   if (Process != NULL && Process != CurrentProcess)
   {
      KeDetachProcess();
   }
}

static PULONG
MmGetPageTableForProcess(PEPROCESS Process, PVOID Address, BOOLEAN Create)
{
   ULONG PdeOffset = ADDR_TO_PDE_OFFSET(Address);
   NTSTATUS Status;
   PFN_TYPE Pfn;
   ULONG Entry;
   PULONG Pt, PageDir;

   if (Address < MmSystemRangeStart && Process && Process != PsGetCurrentProcess())
   {
      PageDir = MmCreateHyperspaceMapping(PTE_TO_PFN(Process->Pcb.DirectoryTableBase.LowPart));
      if (PageDir == NULL)
      {
         KEBUGCHECK(0);
      }
      if (0 == InterlockedCompareExchangeUL(&PageDir[PdeOffset], 0, 0))
      {
         if (Create == FALSE)
	 {
	    MmDeleteHyperspaceMapping(PageDir);
	    return NULL;
	 }
         Status = MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &Pfn);
	 if (!NT_SUCCESS(Status) || Pfn == 0)
	 {
	    KEBUGCHECK(0);
	 }
         Entry = InterlockedCompareExchangeUL(&PageDir[PdeOffset], PFN_TO_PTE(Pfn) | PA_PRESENT | PA_READWRITE | PA_USER, 0);
	 if (Entry != 0)
	 {
	    MmReleasePageMemoryConsumer(MC_NPPOOL, Pfn);
	    Pfn = PTE_TO_PFN(Entry);
	 }
      }
      else
      {
         Pfn = PTE_TO_PFN(PageDir[PdeOffset]);
      }
      MmDeleteHyperspaceMapping(PageDir);
      Pt = MmCreateHyperspaceMapping(Pfn);
      if (Pt == NULL)
      {
         KEBUGCHECK(0);
      }
      return Pt + ADDR_TO_PTE_OFFSET(Address);
   }
   PageDir = (PULONG)MiAddressToPde(Address);
   if (0 == InterlockedCompareExchangeUL(PageDir, 0, 0))
   {
      if (Address >= MmSystemRangeStart)
      {
         if (0 == InterlockedCompareExchangeUL(&MmGlobalKernelPageDirectory[PdeOffset], 0, 0))
	 {
	    if (Create == FALSE)
	    {
               return NULL;
	    }
            Status = MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &Pfn);
	    if (!NT_SUCCESS(Status) || Pfn == 0)
	    {
	       KEBUGCHECK(0);
	    }
	    Entry = PFN_TO_PTE(Pfn) | PA_PRESENT | PA_READWRITE;
            if (Ke386GlobalPagesEnabled)
	    {
	       Entry |= PA_GLOBAL;
	    }
	    if(0 != InterlockedCompareExchangeUL(&MmGlobalKernelPageDirectory[PdeOffset], Entry, 0))
	    {
	       MmReleasePageMemoryConsumer(MC_NPPOOL, Pfn);
	    }
	 }
         (void)InterlockedExchangeUL(PageDir, MmGlobalKernelPageDirectory[PdeOffset]);
      }
      else
      {
	 if (Create == FALSE)
	 {
            return NULL;
	 }
         Status = MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &Pfn);
	 if (!NT_SUCCESS(Status) || Pfn == 0)
	 {
	    KEBUGCHECK(0);
	 }
         Entry = InterlockedCompareExchangeUL(PageDir, PFN_TO_PTE(Pfn) | PA_PRESENT | PA_READWRITE | PA_USER, 0);
	 if (Entry != 0)
	 {
	    MmReleasePageMemoryConsumer(MC_NPPOOL, Pfn);
	 }
      }
   }
   return (PULONG)MiAddressToPte(Address);
}

BOOLEAN MmUnmapPageTable(PULONG Pt)
{
      if (Pt >= (PULONG)PAGETABLE_MAP && Pt < (PULONG)PAGETABLE_MAP + 1024*1024)
      {
         return TRUE;
      }

   if (Pt)
   {
      MmDeleteHyperspaceMapping((PVOID)PAGE_ROUND_DOWN(Pt));
   }
   return FALSE;
}

static ULONG MmGetPageEntryForProcess(PEPROCESS Process, PVOID Address)
{
   ULONG Pte;
   PULONG Pt;

   Pt = MmGetPageTableForProcess(Process, Address, FALSE);
   if (Pt)
   {
      Pte = *Pt;
      MmUnmapPageTable(Pt);
      return Pte;
   }
   return 0;
}

PFN_TYPE
NTAPI
MmGetPfnForProcess(PEPROCESS Process,
                   PVOID Address)
{
      ULONG Entry;
      Entry = MmGetPageEntryForProcess(Process, Address);
      if (!(Entry & PA_PRESENT))
      {
         return 0;
      }
      return(PTE_TO_PFN(Entry));
}

VOID
NTAPI
MmDisableVirtualMapping(PEPROCESS Process, PVOID Address, BOOLEAN* WasDirty, PPFN_TYPE Page)
/*
 * FUNCTION: Delete a virtual mapping
 */
{
   BOOLEAN WasValid;
      ULONG Pte;
      PULONG Pt;

      Pt = MmGetPageTableForProcess(Process, Address, FALSE);
      if (Pt == NULL)
      {
         KEBUGCHECK(0);
      }
      /*
       * Atomically disable the present bit and get the old value.
       */
      do
      {
        Pte = *Pt;
      } while (Pte != InterlockedCompareExchangeUL(Pt, Pte & ~PA_PRESENT, Pte));

      MiFlushTlb(Pt, Address);
      WasValid = (PAGE_MASK(Pte) != 0);
      if (!WasValid)
      {
         KEBUGCHECK(0);
      }

      /*
       * Return some information to the caller
       */
      if (WasDirty != NULL)
      {
         *WasDirty = Pte & PA_DIRTY;
      }
      if (Page != NULL)
      {
         *Page = PTE_TO_PFN(Pte);
      }
}

VOID
NTAPI
MmRawDeleteVirtualMapping(PVOID Address)
{
      PULONG Pt;

      Pt = MmGetPageTableForProcess(NULL, Address, FALSE);
      if (Pt && *Pt)
      {
         /*
          * Set the entry to zero
          */
         (void)InterlockedExchangeUL(Pt, 0);
         MiFlushTlb(Pt, Address);
      }
}

VOID
NTAPI
MmDeleteVirtualMapping(PEPROCESS Process, PVOID Address, BOOLEAN FreePage,
                       BOOLEAN* WasDirty, PPFN_TYPE Page)
/*
 * FUNCTION: Delete a virtual mapping
 */
{
   BOOLEAN WasValid = FALSE;
   PFN_TYPE Pfn;
   ULONG Pte;
   PULONG Pt;

   DPRINT("MmDeleteVirtualMapping(%x, %x, %d, %x, %x)\n",
          Process, Address, FreePage, WasDirty, Page);

      Pt = MmGetPageTableForProcess(Process, Address, FALSE);

      if (Pt == NULL)
      {
         if (WasDirty != NULL)
         {
            *WasDirty = FALSE;
         }
         if (Page != NULL)
         {
            *Page = 0;
         }
         return;
      }

      /*
       * Atomically set the entry to zero and get the old value.
       */
      Pte = InterlockedExchangeUL(Pt, 0);

      MiFlushTlb(Pt, Address);

      WasValid = (PAGE_MASK(Pte) != 0);
      if (WasValid)
      {
         Pfn = PTE_TO_PFN(Pte);
         MmMarkPageUnmapped(Pfn);
      }
      else
      {
         Pfn = 0;
      }

      if (FreePage && WasValid)
      {
         MmReleasePageMemoryConsumer(MC_NPPOOL, Pfn);
      }

      /*
       * Return some information to the caller
       */
      if (WasDirty != NULL)
      {
         *WasDirty = Pte & PA_DIRTY ? TRUE : FALSE;
      }
      if (Page != NULL)
      {
         *Page = Pfn;
      }
   /*
    * Decrement the reference count for this page table.
    */
   if (Process != NULL && WasValid &&
       ((PMADDRESS_SPACE)&Process->VadRoot)->PageTableRefCountTable != NULL &&
       Address < MmSystemRangeStart)
   {
      PUSHORT Ptrc;
      ULONG Idx;

      Ptrc = ((PMADDRESS_SPACE)&Process->VadRoot)->PageTableRefCountTable;
      Idx = ADDR_TO_PAGE_TABLE(Address);

      Ptrc[Idx]--;
      if (Ptrc[Idx] == 0)
      {
         MmFreePageTable(Process, Address);
      }
   }
}

VOID
NTAPI
MmDeletePageFileMapping(PEPROCESS Process, PVOID Address,
                        SWAPENTRY* SwapEntry)
/*
 * FUNCTION: Delete a virtual mapping
 */
{
      ULONG Pte;
      PULONG Pt;

      Pt = MmGetPageTableForProcess(Process, Address, FALSE);

      if (Pt == NULL)
      {
         *SwapEntry = 0;
         return;
      }

      /*
       * Atomically set the entry to zero and get the old value.
       */
      Pte = InterlockedExchangeUL(Pt, 0);

      MiFlushTlb(Pt, Address);

      /*
       * Decrement the reference count for this page table.
       */
      if (Process != NULL && Pte &&
          ((PMADDRESS_SPACE)&Process->VadRoot)->PageTableRefCountTable != NULL &&
          Address < MmSystemRangeStart)
      {
         PUSHORT Ptrc;

         Ptrc = ((PMADDRESS_SPACE)&Process->VadRoot)->PageTableRefCountTable;

         Ptrc[ADDR_TO_PAGE_TABLE(Address)]--;
         if (Ptrc[ADDR_TO_PAGE_TABLE(Address)] == 0)
         {
            MmFreePageTable(Process, Address);
         }
      }


      /*
       * Return some information to the caller
       */
      *SwapEntry = Pte >> 1;
}

BOOLEAN
Mmi386MakeKernelPageTableGlobal(PVOID PAddress)
{
      PULONG Pt, Pde;
      Pde = (PULONG)MiAddressToPde(PAddress);
      if (*Pde == 0)
      {
         Pt = MmGetPageTableForProcess(NULL, PAddress, FALSE);
         if (Pt != NULL)
         {
            return TRUE;
         }
      }
   return(FALSE);
}

BOOLEAN
NTAPI
MmIsDirtyPage(PEPROCESS Process, PVOID Address)
{
      return MmGetPageEntryForProcess(Process, Address) & PA_DIRTY ? TRUE : FALSE;
}

BOOLEAN
NTAPI
MmIsAccessedAndResetAccessPage(PEPROCESS Process, PVOID Address)
{
    PULONG Pt;
    ULONG Pte;

    if (Address < MmSystemRangeStart && Process == NULL)
    {
        DPRINT1("MmIsAccessedAndResetAccessPage is called for user space without a process.\n");
        KEBUGCHECK(0);
    }

    Pt = MmGetPageTableForProcess(Process, Address, FALSE);
    if (Pt == NULL)
    {
        KEBUGCHECK(0);
    }

    do
    {
        Pte = *Pt;
    } while (Pte != InterlockedCompareExchangeUL(Pt, Pte & ~PA_ACCESSED, Pte));

    if (Pte & PA_ACCESSED)
    {
        MiFlushTlb(Pt, Address);
        return TRUE;
    }
    else
    {
        MmUnmapPageTable(Pt);
        return FALSE;
    }
}

VOID
NTAPI
MmSetCleanPage(PEPROCESS Process, PVOID Address)
{
    PULONG Pt;
    ULONG Pte;

    if (Address < MmSystemRangeStart && Process == NULL)
    {
        DPRINT1("MmSetCleanPage is called for user space without a process.\n");
        KEBUGCHECK(0);
    }

    Pt = MmGetPageTableForProcess(Process, Address, FALSE);

    if (Pt == NULL)
    {
        KEBUGCHECK(0);
    }

    do
    {
        Pte = *Pt;
    } while (Pte != InterlockedCompareExchangeUL(Pt, Pte & ~PA_DIRTY, Pte));

    if (Pte & PA_DIRTY)
    {
        MiFlushTlb(Pt, Address);
    }
    else
    {
        MmUnmapPageTable(Pt);
    }
}

VOID
NTAPI
MmSetDirtyPage(PEPROCESS Process, PVOID Address)
{
      PULONG Pt;
      ULONG Pte;

   if (Address < MmSystemRangeStart && Process == NULL)
   {
      DPRINT1("MmSetDirtyPage is called for user space without a process.\n");
      KEBUGCHECK(0);
   }

      Pt = MmGetPageTableForProcess(Process, Address, FALSE);
      if (Pt == NULL)
      {
         KEBUGCHECK(0);
      }

      do
      {
         Pte = *Pt;
      } while (Pte != InterlockedCompareExchangeUL(Pt, Pte | PA_DIRTY, Pte));
      if (!(Pte & PA_DIRTY))
      {
         MiFlushTlb(Pt, Address);
      }
      else
      {
         MmUnmapPageTable(Pt);
      }
}

VOID
NTAPI
MmEnableVirtualMapping(PEPROCESS Process, PVOID Address)
{
      PULONG Pt;
      ULONG Pte;

      Pt = MmGetPageTableForProcess(Process, Address, FALSE);
      if (Pt == NULL)
      {
         KEBUGCHECK(0);
      }

      do
      {
         Pte = *Pt;
      } while (Pte != InterlockedCompareExchangeUL(Pt, Pte | PA_PRESENT, Pte));
      if (!(Pte & PA_PRESENT))
      {
         MiFlushTlb(Pt, Address);
      }
      else
      {
         MmUnmapPageTable(Pt);
      }
}

BOOLEAN
NTAPI
MmIsPagePresent(PEPROCESS Process, PVOID Address)
{
      return MmGetPageEntryForProcess(Process, Address) & PA_PRESENT ? TRUE : FALSE;
}

BOOLEAN
NTAPI
MmIsPageSwapEntry(PEPROCESS Process, PVOID Address)
{
      ULONG Entry;
      Entry = MmGetPageEntryForProcess(Process, Address);
      return !(Entry & PA_PRESENT) && Entry != 0 ? TRUE : FALSE;
}

NTSTATUS
NTAPI
MmCreateVirtualMappingForKernel(PVOID Address,
                                ULONG flProtect,
                                PPFN_TYPE Pages,
				ULONG PageCount)
{
   ULONG Attributes;
   ULONG i;
   PVOID Addr;
   ULONG PdeOffset, oldPdeOffset;
   PULONG Pt;
   ULONG Pte;
   BOOLEAN NoExecute = FALSE;

   DPRINT("MmCreateVirtualMappingForKernel(%x, %x, %x, %d)\n",
           Address, flProtect, Pages, PageCount);

   if (Address < MmSystemRangeStart)
   {
      DPRINT1("MmCreateVirtualMappingForKernel is called for user space\n");
      KEBUGCHECK(0);
   }

   Attributes = ProtectToPTE(flProtect);
   if (Attributes & 0x80000000)
   {
      NoExecute = TRUE;
   }
   Attributes &= 0xfff;
   if (Ke386GlobalPagesEnabled)
   {
      Attributes |= PA_GLOBAL;
   }

   Addr = Address;

      oldPdeOffset = ADDR_TO_PDE_OFFSET(Addr);
      Pt = MmGetPageTableForProcess(NULL, Addr, TRUE);
      if (Pt == NULL)
      {
         KEBUGCHECK(0);
      }
      Pt--;

      for (i = 0; i < PageCount; i++, Addr = (PVOID)((ULONG_PTR)Addr + PAGE_SIZE))
      {
         if (!(Attributes & PA_PRESENT) && Pages[i] != 0)
         {
            DPRINT1("Setting physical address but not allowing access at address "
                    "0x%.8X with attributes %x/%x.\n",
                    Addr, Attributes, flProtect);
            KEBUGCHECK(0);
         }

         PdeOffset = ADDR_TO_PDE_OFFSET(Addr);
         if (oldPdeOffset != PdeOffset)
         {
            Pt = MmGetPageTableForProcess(NULL, Addr, TRUE);
	    if (Pt == NULL)
	    {
	       KEBUGCHECK(0);
	    }
         }
         else
         {
            Pt++;
         }
         oldPdeOffset = PdeOffset;

         Pte = *Pt;
         if (Pte != 0)
         {
            KEBUGCHECK(0);
         }
         (void)InterlockedExchangeUL(Pt, PFN_TO_PTE(Pages[i]) | Attributes);
      }

   return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
MmCreatePageFileMapping(PEPROCESS Process,
                        PVOID Address,
                        SWAPENTRY SwapEntry)
{
   PULONG Pt;
   ULONG Pte;

   if (Process == NULL && Address < MmSystemRangeStart)
   {
      DPRINT1("No process\n");
      KEBUGCHECK(0);
   }
   if (Process != NULL && Address >= MmSystemRangeStart)
   {
      DPRINT1("Setting kernel address with process context\n");
      KEBUGCHECK(0);
   }
   if (SwapEntry & (1 << 31))
   {
      KEBUGCHECK(0);
   }

      Pt = MmGetPageTableForProcess(Process, Address, TRUE);
      if (Pt == NULL)
      {
         KEBUGCHECK(0);
      }
      Pte = *Pt;
      if (PAGE_MASK((Pte)) != 0)
      {
         MmMarkPageUnmapped(PTE_TO_PFN((Pte)));
      }
      (void)InterlockedExchangeUL(Pt, SwapEntry << 1);
      if (Pte != 0)
      {
         MiFlushTlb(Pt, Address);
      }
      else
      {
         MmUnmapPageTable(Pt);
      }

   if (Process != NULL &&
       ((PMADDRESS_SPACE)&Process->VadRoot)->PageTableRefCountTable != NULL &&
       Address < MmSystemRangeStart)
   {
     PUSHORT Ptrc;
     ULONG Idx;

     Ptrc = ((PMADDRESS_SPACE)&Process->VadRoot)->PageTableRefCountTable;
     Idx = ADDR_TO_PAGE_TABLE(Address);
     Ptrc[Idx]++;
   }
   return(STATUS_SUCCESS);
}


NTSTATUS
NTAPI
MmCreateVirtualMappingUnsafe(PEPROCESS Process,
                             PVOID Address,
                             ULONG flProtect,
                             PPFN_TYPE Pages,
                             ULONG PageCount)
{
   ULONG Attributes;
   PVOID Addr;
   ULONG i;
   ULONG oldPdeOffset, PdeOffset;
   PULONG Pt = NULL;
   ULONG Pte;
   BOOLEAN NoExecute = FALSE;

   DPRINT("MmCreateVirtualMappingUnsafe(%x, %x, %x, %x (%x), %d)\n",
          Process, Address, flProtect, Pages, *Pages, PageCount);

   if (Process == NULL)
   {
      if (Address < MmSystemRangeStart)
      {
         DPRINT1("No process\n");
         KEBUGCHECK(0);
      }
      if (PageCount > 0x10000 ||
	  (ULONG_PTR) Address / PAGE_SIZE + PageCount > 0x100000)
      {
         DPRINT1("Page count to large\n");
	 KEBUGCHECK(0);
      }
   }
   else
   {
      if (Address >= MmSystemRangeStart)
      {
         DPRINT1("Setting kernel address with process context\n");
         KEBUGCHECK(0);
      }
      if (PageCount > (ULONG_PTR)MmSystemRangeStart / PAGE_SIZE ||
	  (ULONG_PTR) Address / PAGE_SIZE + PageCount >
	  (ULONG_PTR)MmSystemRangeStart / PAGE_SIZE)
      {
         DPRINT1("Page Count to large\n");
	 KEBUGCHECK(0);
      }
   }

   Attributes = ProtectToPTE(flProtect);
   if (Attributes & 0x80000000)
   {
      NoExecute = TRUE;
   }
   Attributes &= 0xfff;
   if (Address >= MmSystemRangeStart)
   {
      Attributes &= ~PA_USER;
      if (Ke386GlobalPagesEnabled)
      {
	 Attributes |= PA_GLOBAL;
      }
   }
   else
   {
      Attributes |= PA_USER;
   }

   Addr = Address;
      oldPdeOffset = ADDR_TO_PDE_OFFSET(Addr) + 1;
      for (i = 0; i < PageCount; i++, Addr = (PVOID)((ULONG_PTR)Addr + PAGE_SIZE))
      {
         if (!(Attributes & PA_PRESENT) && Pages[i] != 0)
         {
            DPRINT1("Setting physical address but not allowing access at address "
                    "0x%.8X with attributes %x/%x.\n",
                    Addr, Attributes, flProtect);
            KEBUGCHECK(0);
         }
         PdeOffset = ADDR_TO_PDE_OFFSET(Addr);
         if (oldPdeOffset != PdeOffset)
         {
            MmUnmapPageTable(Pt);
	    Pt = MmGetPageTableForProcess(Process, Addr, TRUE);
	    if (Pt == NULL)
	    {
	       KEBUGCHECK(0);
	    }
         }
         else
         {
            Pt++;
         }
         oldPdeOffset = PdeOffset;

         Pte = *Pt;
         MmMarkPageMapped(Pages[i]);
         if (PAGE_MASK((Pte)) != 0 && !((Pte) & PA_PRESENT))
         {
            KEBUGCHECK(0);
         }
         if (PAGE_MASK((Pte)) != 0)
         {
            MmMarkPageUnmapped(PTE_TO_PFN((Pte)));
         }
	 (void)InterlockedExchangeUL(Pt, PFN_TO_PTE(Pages[i]) | Attributes);
         if (Address < MmSystemRangeStart &&
	     ((PMADDRESS_SPACE)&Process->VadRoot)->PageTableRefCountTable != NULL &&
             Attributes & PA_PRESENT)
         {
            PUSHORT Ptrc;

            Ptrc = ((PMADDRESS_SPACE)&Process->VadRoot)->PageTableRefCountTable;

            Ptrc[ADDR_TO_PAGE_TABLE(Addr)]++;
         }
         if (Pte != 0)
         {
            if (Address > MmSystemRangeStart ||
                (Pt >= (PULONG)PAGETABLE_MAP && Pt < (PULONG)PAGETABLE_MAP + 1024*1024))
            {
               MiFlushTlb(Pt, Address);
            }
         }
      }
      if (Addr > Address)
      {
         MmUnmapPageTable(Pt);
      }

   return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
MmCreateVirtualMapping(PEPROCESS Process,
                       PVOID Address,
                       ULONG flProtect,
                       PPFN_TYPE Pages,
                       ULONG PageCount)
{
   ULONG i;

   for (i = 0; i < PageCount; i++)
   {
      if (!MmIsPageInUse(Pages[i]))
      {
         DPRINT1("Page at address %x not in use\n", PFN_TO_PTE(Pages[i]));
         KEBUGCHECK(0);
      }
   }

   return(MmCreateVirtualMappingUnsafe(Process,
                                       Address,
                                       flProtect,
                                       Pages,
                                       PageCount));
}

ULONG
NTAPI
MmGetPageProtect(PEPROCESS Process, PVOID Address)
{
   ULONG Entry;
   ULONG Protect;
    
      Entry = MmGetPageEntryForProcess(Process, Address);


   if (!(Entry & PA_PRESENT))
   {
      Protect = PAGE_NOACCESS;
   }
   else
   {
      if (Entry & PA_READWRITE)
      {
         Protect = PAGE_READWRITE;
      }
      else
      {
         Protect = PAGE_EXECUTE_READ;
      }
      if (Entry & PA_CD)
      {
         Protect |= PAGE_NOCACHE;
      }
      if (Entry & PA_WT)
      {
         Protect |= PAGE_WRITETHROUGH;
      }
      if (!(Entry & PA_USER))
      {
         Protect |= PAGE_SYSTEM;
      }

   }
   return(Protect);
}

VOID
NTAPI
MmSetPageProtect(PEPROCESS Process, PVOID Address, ULONG flProtect)
{
   ULONG Attributes = 0;
   BOOLEAN NoExecute = FALSE;
   PULONG Pt;

   DPRINT("MmSetPageProtect(Process %x  Address %x  flProtect %x)\n",
          Process, Address, flProtect);

   Attributes = ProtectToPTE(flProtect);
   if (Attributes & 0x80000000)
   {
      NoExecute = TRUE;
   }
   Attributes &= 0xfff;
   if (Address >= MmSystemRangeStart)
   {
      Attributes &= ~PA_USER;
      if (Ke386GlobalPagesEnabled)
      {
	 Attributes |= PA_GLOBAL;
      }
   }
   else
   {
      Attributes |= PA_USER;
   }

      Pt = MmGetPageTableForProcess(Process, Address, FALSE);
      if (Pt == NULL)
      {
         KEBUGCHECK(0);
      }
      InterlockedExchange((PLONG)Pt, PAGE_MASK(*Pt) | Attributes | (*Pt & (PA_ACCESSED|PA_DIRTY)));
      MiFlushTlb(Pt, Address);
}

/*
 * @implemented
 */
PHYSICAL_ADDRESS STDCALL
MmGetPhysicalAddress(PVOID vaddr)
/*
 * FUNCTION: Returns the physical address corresponding to a virtual address
 */
{
   PHYSICAL_ADDRESS p;
   ULONG Pte;

   DPRINT("MmGetPhysicalAddress(vaddr %x)\n", vaddr);
      Pte = MmGetPageEntryForProcess(NULL, vaddr);
      if (Pte != 0 && Pte & PA_PRESENT)
      {
         p.QuadPart = PAGE_MASK(Pte);
         p.u.LowPart |= (ULONG_PTR)vaddr & (PAGE_SIZE - 1);
      }
      else
      {
         p.QuadPart = 0;
      }
   return p;
}

PVOID
NTAPI
MmCreateHyperspaceMapping(PFN_TYPE Page)
{
   PVOID Address;
   ULONG i;

      ULONG Entry;
      PULONG Pte;
      Entry = PFN_TO_PTE(Page) | PA_PRESENT | PA_READWRITE;
      Pte = (PULONG)MiAddressToPte(HYPERSPACE) + Page % 1024;
      if (Page & 1024)
      {
         for (i = Page % 1024; i < 1024; i++, Pte++)
         {
            if (0 == InterlockedCompareExchange((PLONG)Pte, (LONG)Entry, 0))
            {
               break;
            }
         }
         if (i >= 1024)
         {
            Pte = (PULONG)MiAddressToPte(HYPERSPACE);
            for (i = 0; i < Page % 1024; i++, Pte++)
            {
               if (0 == InterlockedCompareExchange((PLONG)Pte, (LONG)Entry, 0))
               {
                  break;
               }
            }
            if (i >= Page % 1024)
            {
               KEBUGCHECK(0);
            }
         }
      }
      else
      {
         for (i = Page % 1024; (LONG)i >= 0; i--, Pte--)
         {
            if (0 == InterlockedCompareExchange((PLONG)Pte, (LONG)Entry, 0))
            {
               break;
            }
         }
         if ((LONG)i < 0)
         {
            Pte = (PULONG)MiAddressToPte(HYPERSPACE) + 1023;
            for (i = 1023; i > Page % 1024; i--, Pte--)
            {
               if (0 == InterlockedCompareExchange((PLONG)Pte, (LONG)Entry, 0))
               {
                  break;
               }
            }
            if (i <= Page % 1024)
            {
               KEBUGCHECK(0);
            }
         }
      }
   Address = (PVOID)((ULONG_PTR)HYPERSPACE + i * PAGE_SIZE);
   __invlpg(Address);
   return Address;
}

PFN_TYPE
NTAPI
MmChangeHyperspaceMapping(PVOID Address, PFN_TYPE NewPage)
{
   PFN_TYPE Pfn;
   ULONG Entry;

   ASSERT (IS_HYPERSPACE(Address));

      Entry = InterlockedExchange((PLONG)MiAddressToPte(Address), PFN_TO_PTE(NewPage) | PA_PRESENT | PA_READWRITE);
      Pfn = PTE_TO_PFN(Entry);

   __invlpg(Address);
   return Pfn;
}

PFN_TYPE
NTAPI
MmDeleteHyperspaceMapping(PVOID Address)
{
   PFN_TYPE Pfn;
   ULONG Entry;

   ASSERT (IS_HYPERSPACE(Address));

      Entry = InterlockedExchange((PLONG)MiAddressToPte(Address), 0);
      Pfn = PTE_TO_PFN(Entry);

   __invlpg(Address);
   return Pfn;
}

VOID
NTAPI
MmUpdatePageDir(PEPROCESS Process, PVOID Address, ULONG Size)
{
   ULONG StartOffset, EndOffset, Offset;
   PULONG Pde;

   if (Address < MmSystemRangeStart)
   {
      KEBUGCHECK(0);
   }

      StartOffset = ADDR_TO_PDE_OFFSET(Address);
      EndOffset = ADDR_TO_PDE_OFFSET((PVOID)((ULONG_PTR)Address + Size));

      if (Process != NULL && Process != PsGetCurrentProcess())
      {
         Pde = MmCreateHyperspaceMapping(PTE_TO_PFN(Process->Pcb.DirectoryTableBase.u.LowPart));
      }
      else
      {
         Pde = (PULONG)PAGEDIRECTORY_MAP;
      }
      for (Offset = StartOffset; Offset <= EndOffset; Offset++)
      {
         if (Offset != ADDR_TO_PDE_OFFSET(PAGETABLE_MAP))
         {
            (void)InterlockedCompareExchangeUL(&Pde[Offset], MmGlobalKernelPageDirectory[Offset], 0);
         }
      }
      if (Pde != (PULONG)PAGEDIRECTORY_MAP)
      {
         MmDeleteHyperspaceMapping(Pde);
      }
}

VOID
INIT_FUNCTION
NTAPI
MmInitGlobalKernelPageDirectory(VOID)
{
   ULONG i;
   PULONG CurrentPageDirectory = (PULONG)PAGEDIRECTORY_MAP;

   DPRINT("MmInitGlobalKernelPageDirectory()\n");

      for (i = ADDR_TO_PDE_OFFSET(MmSystemRangeStart); i < 1024; i++)
      {
         if (i != ADDR_TO_PDE_OFFSET(PAGETABLE_MAP) &&
	     i != ADDR_TO_PDE_OFFSET(HYPERSPACE) &&
             0 == MmGlobalKernelPageDirectory[i] && 0 != CurrentPageDirectory[i])
         {
            MmGlobalKernelPageDirectory[i] = CurrentPageDirectory[i];
	    if (Ke386GlobalPagesEnabled)
	    {
               MmGlobalKernelPageDirectory[i] |= PA_GLOBAL;
               CurrentPageDirectory[i] |= PA_GLOBAL;
	    }
         }
      }
}

ULONG
NTAPI
MiGetUserPageDirectoryCount(VOID)
{
   return ADDR_TO_PDE_OFFSET(MmSystemRangeStart);
}

VOID
INIT_FUNCTION
NTAPI
MiInitPageDirectoryMap(VOID)
{
   MEMORY_AREA* kernel_map_desc = NULL;
   MEMORY_AREA* hyperspace_desc = NULL;
   PHYSICAL_ADDRESS BoundaryAddressMultiple;
   PVOID BaseAddress;
   NTSTATUS Status;

   DPRINT("MiInitPageDirectoryMap()\n");

   BoundaryAddressMultiple.QuadPart = 0;
   BaseAddress = (PVOID)PAGETABLE_MAP;
   Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                               MEMORY_AREA_SYSTEM,
                               &BaseAddress,
		               0x400000,
                               PAGE_READWRITE,
                               &kernel_map_desc,
                               TRUE,
                               0,
                               BoundaryAddressMultiple);
   if (!NT_SUCCESS(Status))
   {
      KEBUGCHECK(0);
   }
   BaseAddress = (PVOID)HYPERSPACE;
   Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                               MEMORY_AREA_SYSTEM,
                               &BaseAddress,
		               0x400000,
                               PAGE_READWRITE,
                               &hyperspace_desc,
                               TRUE,
                               0,
                               BoundaryAddressMultiple);
   if (!NT_SUCCESS(Status))
   {
      KEBUGCHECK(0);
   }
}

/* EOF */
