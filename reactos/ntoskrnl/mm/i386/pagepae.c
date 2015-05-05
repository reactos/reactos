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
#include <debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, MmInitGlobalKernelPageDirectory)
#endif


/* GLOBALS *****************************************************************/

#define PA_BIT_PRESENT   (0)
#define PA_BIT_READWRITE (1)
#define PA_BIT_USER      (2)
#define PA_BIT_WT        (3)
#define PA_BIT_CD        (4)
#define PA_BIT_ACCESSED  (5)
#define PA_BIT_DIRTY     (6)
#define PA_BIT_GLOBAL    (8)

#define PA_PRESENT   (1 << PA_BIT_PRESENT)
#define PA_READWRITE (1 << PA_BIT_READWRITE)
#define PA_USER      (1 << PA_BIT_USER)
#define PA_DIRTY     (1 << PA_BIT_DIRTY)
#define PA_WT        (1 << PA_BIT_WT)
#define PA_CD        (1 << PA_BIT_CD)
#define PA_ACCESSED  (1 << PA_BIT_ACCESSED)
#define PA_GLOBAL    (1 << PA_BIT_GLOBAL)

#define PAGEDIRECTORY_MAP       (0xc0000000 + (PTE_BASE / (1024)))
#define PAE_PAGEDIRECTORY_MAP   (0xc0000000 + (PTE_BASE / (512)))

#define HYPERSPACE              (Ke386Pae ? 0xc0800000 : 0xc0400000)
#define IS_HYPERSPACE(v)        (((ULONG)(v) >= HYPERSPACE && (ULONG)(v) < HYPERSPACE + 0x400000))

static ULONG MmGlobalKernelPageDirectory[1024];
static ULONGLONG MmGlobalKernelPageDirectoryForPAE[2048];

#define PTE_TO_PFN(X)  ((X) >> PAGE_SHIFT)
#define PFN_TO_PTE(X)  ((X) << PAGE_SHIFT)

#define PAE_PTE_TO_PFN(X)   (PAE_PAGE_MASK(X) >> PAGE_SHIFT)
#define PAE_PFN_TO_PTE(X)   ((X) << PAGE_SHIFT)

#define PAGE_MASK(x)		((x)&(~0xfff))
#define PAE_PAGE_MASK(x)	((x)&(~0xfffLL))

extern BOOLEAN Ke386Pae;
extern BOOLEAN Ke386NoExecute;

/* FUNCTIONS ***************************************************************/

BOOLEAN MmUnmapPageTable(PULONG Pt);

ULONG_PTR
NTAPI
MiFlushTlbIpiRoutine(ULONG_PTR Address)
{
   if (Address == (ULONGLONG)-1)
   {
      KeFlushCurrentTb();
   }
   else if (Address == (ULONGLONG)-2)
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
#ifdef CONFIG_SMP
   if (Pt)
   {
      MmUnmapPageTable(Pt);
   }
   if (KeNumberProcessors > 1)
   {
      KeIpiGenericCall(MiFlushTlbIpiRoutine, (ULONG_PTR)Address);
   }
   else
   {
      MiFlushTlbIpiRoutine((ULONG_PTR)Address);
   }
#else
   if ((Pt && MmUnmapPageTable(Pt)) || Address >= MmSystemRangeStart)
   {
      __invlpg(Address);
   }
#endif
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
      ASSERT(FALSE);
   }
   if (Ke386NoExecute &&
       !(flProtect & PAGE_IS_EXECUTABLE))
   {
      Attributes = Attributes | 0x80000000;
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

#define ADDR_TO_PAGE_TABLE(v) (((ULONG)(v)) / (1024 * PAGE_SIZE))

#define ADDR_TO_PDE(v) (PULONG)(PAGEDIRECTORY_MAP + \
                                ((((ULONG)(v)) / (1024 * 1024))&(~0x3)))
#define ADDR_TO_PTE(v) (PULONG)(PTE_BASE + ((((ULONG)(v) / 1024))&(~0x3)))

#define ADDR_TO_PDE_OFFSET(v) ((((ULONG)(v)) / (1024 * PAGE_SIZE)))

#define ADDR_TO_PTE_OFFSET(v)  ((((ULONG)(v)) % (1024 * PAGE_SIZE)) / PAGE_SIZE)


#define PAE_ADDR_TO_PAGE_TABLE(v)   (((ULONG)(v)) / (512 * PAGE_SIZE))

#define PAE_ADDR_TO_PDE(v)          (PULONGLONG) (PAE_PAGEDIRECTORY_MAP + \
                                                  ((((ULONG_PTR)(v)) / (512 * 512))&(~0x7)))
#define PAE_ADDR_TO_PTE(v)          (PULONGLONG) (PTE_BASE + ((((ULONG_PTR)(v) / 512))&(~0x7)))


#define PAE_ADDR_TO_PDTE_OFFSET(v)  (((ULONG_PTR)(v)) / (512 * 512 * PAGE_SIZE))

#define PAE_ADDR_TO_PDE_PAGE_OFFSET(v)   ((((ULONG_PTR)(v)) % (512 * 512 * PAGE_SIZE)) / (512 * PAGE_SIZE))

#define PAE_ADDR_TO_PDE_OFFSET(v)   (((ULONG_PTR)(v))/ (512 * PAGE_SIZE))

#define PAE_ADDR_TO_PTE_OFFSET(v)   ((((ULONG_PTR)(v)) % (512 * PAGE_SIZE)) / PAGE_SIZE)

BOOLEAN
NTAPI
MmCreateProcessAddressSpace(IN ULONG MinWs,
                            IN PEPROCESS Process,
                            IN PLARGE_INTEGER DirectoryTableBase)
{
   NTSTATUS Status;
   ULONG i, j;
   PFN_NUMBER Pfn[7];
   ULONG Count;

   DPRINT("MmCopyMmInfo(Src %x, Dest %x)\n", MinWs, Process);

   Count = Ke386Pae ? 7 : 2;

   for (i = 0; i < Count; i++)
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

   if (Ke386Pae)
   {
      PULONGLONG PageDirTable;
      PULONGLONG PageDir;

      PageDirTable = MmCreateHyperspaceMapping(Pfn[0]);
      for (i = 0; i < 4; i++)
      {
         PageDirTable[i] = PAE_PFN_TO_PTE(Pfn[1+i]) | PA_PRESENT;
      }
      MmDeleteHyperspaceMapping(PageDirTable);
      for (i = PAE_ADDR_TO_PDTE_OFFSET(MmSystemRangeStart); i < 4; i++)
      {
         PageDir = (PULONGLONG)MmCreateHyperspaceMapping(Pfn[i+1]);
         memcpy(PageDir, &MmGlobalKernelPageDirectoryForPAE[i * 512], 512 * sizeof(ULONGLONG));
         if (PAE_ADDR_TO_PDTE_OFFSET(PTE_BASE) == i)
         {
            for (j = 0; j < 4; j++)
            {
               PageDir[PAE_ADDR_TO_PDE_PAGE_OFFSET(PTE_BASE) + j] = PAE_PFN_TO_PTE(Pfn[1+j]) | PA_PRESENT | PA_READWRITE;
            }
         }
         if (PAE_ADDR_TO_PDTE_OFFSET(HYPERSPACE) == i)
         {
            PageDir[PAE_ADDR_TO_PDE_PAGE_OFFSET(HYPERSPACE)] = PAE_PFN_TO_PTE(Pfn[5]) | PA_PRESENT | PA_READWRITE;
            PageDir[PAE_ADDR_TO_PDE_PAGE_OFFSET(HYPERSPACE)+1] = PAE_PFN_TO_PTE(Pfn[6]) | PA_PRESENT | PA_READWRITE;
         }
         MmDeleteHyperspaceMapping(PageDir);
      }
   }
   else
   {
      PULONG PageDirectory;
      PageDirectory = MmCreateHyperspaceMapping(Pfn[0]);

      memcpy(PageDirectory + ADDR_TO_PDE_OFFSET(MmSystemRangeStart),
             MmGlobalKernelPageDirectory + ADDR_TO_PDE_OFFSET(MmSystemRangeStart),
             (1024 - ADDR_TO_PDE_OFFSET(MmSystemRangeStart)) * sizeof(ULONG));

      DPRINT("Addr %x\n",ADDR_TO_PDE_OFFSET(PTE_BASE));
      PageDirectory[ADDR_TO_PDE_OFFSET(PTE_BASE)] = PFN_TO_PTE(Pfn[0]) | PA_PRESENT | PA_READWRITE;
      PageDirectory[ADDR_TO_PDE_OFFSET(HYPERSPACE)] = PFN_TO_PTE(Pfn[1]) | PA_PRESENT | PA_READWRITE;

      MmDeleteHyperspaceMapping(PageDirectory);
   }

   DirectoryTableBase->QuadPart = PFN_TO_PTE(Pfn[0]);
   DPRINT("Finished MmCopyMmInfo(): %I64x\n", DirectoryTableBase->QuadPart);
   return TRUE;
}

VOID
NTAPI
MmFreePageTable(PEPROCESS Process, PVOID Address)
{
   PEPROCESS CurrentProcess = PsGetCurrentProcess();
   ULONG i;
   PFN_NUMBER Pfn;

   DPRINT("ProcessId %d, Address %x\n", Process->UniqueProcessId, Address);
   if (Process != NULL && Process != CurrentProcess)
   {
      KeAttachProcess(&Process->Pcb);
   }
   if (Ke386Pae)
   {
      PULONGLONG PageTable;
      ULONGLONG ZeroPte = 0LL;
      PageTable = (PULONGLONG)PAGE_ROUND_DOWN((PVOID)PAE_ADDR_TO_PTE(Address));
      for (i = 0; i < 512; i++)
      {
         if (PageTable[i] != 0LL)
         {
            DbgPrint("Page table entry not clear at %x/%x (is %I64x)\n",
                     ((ULONG)Address / (4*1024*1024)), i, PageTable[i]);
            ASSERT(FALSE);
         }
      }
      Pfn = PAE_PTE_TO_PFN(*(PAE_ADDR_TO_PDE(Address)));
      (void)ExfpInterlockedExchange64UL(PAE_ADDR_TO_PDE(Address), &ZeroPte);
      MiFlushTlb((PULONG)PAE_ADDR_TO_PDE(Address), PAE_ADDR_TO_PTE(Address));
   }
   else
   {
      PULONG PageTable;
      PageTable = (PULONG)PAGE_ROUND_DOWN((PVOID)ADDR_TO_PTE(Address));
      for (i = 0; i < 1024; i++)
      {
         if (PageTable[i] != 0)
         {
            DbgPrint("Page table entry not clear at %x/%x (is %x)\n",
                     ((ULONG)Address / (4*1024*1024)), i, PageTable[i]);
            ASSERT(FALSE);
         }
      }
      Pfn = PTE_TO_PFN(*(ADDR_TO_PDE(Address)));
      *(ADDR_TO_PDE(Address)) = 0;
      MiFlushTlb(ADDR_TO_PDE(Address), ADDR_TO_PTE(Address));
   }

   if (Address >= MmSystemRangeStart)
   {
      //    MmGlobalKernelPageDirectory[ADDR_TO_PDE_OFFSET(Address)] = 0;
      ASSERT(FALSE);
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

static PULONGLONG
MmGetPageTableForProcessForPAE(PEPROCESS Process, PVOID Address, BOOLEAN Create)
{
   NTSTATUS Status;
   PFN_NUMBER Pfn;
   ULONGLONG Entry;
   ULONGLONG ZeroEntry = 0LL;
   PULONGLONG Pt;
   PULONGLONG PageDir;
   PULONGLONG PageDirTable;

   DPRINT("MmGetPageTableForProcessForPAE(%x %x %d)\n",
          Process, Address, Create);
   if (Address >= (PVOID)PTE_BASE && Address < (PVOID)((ULONG_PTR)PTE_BASE + 0x800000))
   {
      ASSERT(FALSE);
   }
   if (Address < MmSystemRangeStart && Process && Process != PsGetCurrentProcess())
   {
      PageDirTable = MmCreateHyperspaceMapping(PAE_PTE_TO_PFN(Process->Pcb.DirectoryTableBase.QuadPart));
      if (PageDirTable == NULL)
      {
         ASSERT(FALSE);
      }
      PageDir = MmCreateHyperspaceMapping(PAE_PTE_TO_PFN(PageDirTable[PAE_ADDR_TO_PDTE_OFFSET(Address)]));
      MmDeleteHyperspaceMapping(PageDirTable);
      if (PageDir == NULL)
      {
         ASSERT(FALSE);
      }
      PageDir += PAE_ADDR_TO_PDE_PAGE_OFFSET(Address);
      Entry = ExfInterlockedCompareExchange64UL(PageDir, &ZeroEntry, &ZeroEntry);
      if (Entry == 0LL)
      {
         if (Create == FALSE)
         {
            MmDeleteHyperspaceMapping(PageDir);
            return NULL;
         }
         Status = MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &Pfn);
         if (!NT_SUCCESS(Status))
         {
            ASSERT(FALSE);
         }
         Entry = PFN_TO_PTE(Pfn) | PA_PRESENT | PA_READWRITE | PA_USER;
         Entry = ExfInterlockedCompareExchange64UL(PageDir, &Entry, &ZeroEntry);
         if (Entry != 0LL)
         {
            MmReleasePageMemoryConsumer(MC_NPPOOL, Pfn);
            Pfn = PAE_PTE_TO_PFN(Entry);
         }
      }
      else
      {
         Pfn = PAE_PTE_TO_PFN(Entry);
      }
      MmDeleteHyperspaceMapping(PageDir);
      Pt = MmCreateHyperspaceMapping(Pfn);
      if (Pt == NULL)
      {
         ASSERT(FALSE);
      }
      return Pt + PAE_ADDR_TO_PTE_OFFSET(Address);
   }
   PageDir = PAE_ADDR_TO_PDE(Address);
   if (0LL == ExfInterlockedCompareExchange64UL(PageDir, &ZeroEntry, &ZeroEntry))
   {
      if (Address >= MmSystemRangeStart)
      {
         if (MmGlobalKernelPageDirectoryForPAE[PAE_ADDR_TO_PDE_OFFSET(Address)] == 0LL)
         {
            if (Create == FALSE)
            {
               return NULL;
            }
            Status = MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &Pfn);
            if (!NT_SUCCESS(Status))
            {
               ASSERT(FALSE);
            }
            Entry = PAE_PFN_TO_PTE(Pfn) | PA_PRESENT | PA_READWRITE;
            if (Ke386GlobalPagesEnabled)
            {
               Entry |= PA_GLOBAL;
            }
            if (0LL != ExfInterlockedCompareExchange64UL(&MmGlobalKernelPageDirectoryForPAE[PAE_ADDR_TO_PDE_OFFSET(Address)], &Entry, &ZeroEntry))
            {
               MmReleasePageMemoryConsumer(MC_NPPOOL, Pfn);
            }
         }
         (void)ExfInterlockedCompareExchange64UL(PageDir, &MmGlobalKernelPageDirectoryForPAE[PAE_ADDR_TO_PDE_OFFSET(Address)], &ZeroEntry);
      }
      else
      {
         if (Create == FALSE)
         {
            return NULL;
         }
         Status = MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &Pfn);
         if (!NT_SUCCESS(Status))
         {
            ASSERT(FALSE);
         }
         Entry = PFN_TO_PTE(Pfn) | PA_PRESENT | PA_READWRITE | PA_USER;
         Entry = ExfInterlockedCompareExchange64UL(PageDir, &Entry, &ZeroEntry);
         if (Entry != 0LL)
         {
            MmReleasePageMemoryConsumer(MC_NPPOOL, Pfn);
         }
      }
   }
   return (PULONGLONG)PAE_ADDR_TO_PTE(Address);
}

static PULONG
MmGetPageTableForProcess(PEPROCESS Process, PVOID Address, BOOLEAN Create)
{
   ULONG PdeOffset = ADDR_TO_PDE_OFFSET(Address);
   NTSTATUS Status;
   PFN_NUMBER Pfn;
   ULONG Entry;
   PULONG Pt, PageDir;

   if (Address < MmSystemRangeStart && Process && Process != PsGetCurrentProcess())
   {
      PageDir = MmCreateHyperspaceMapping(PTE_TO_PFN(Process->Pcb.DirectoryTableBase.LowPart));
      if (PageDir == NULL)
      {
         ASSERT(FALSE);
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
            ASSERT(FALSE);
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
         ASSERT(FALSE);
      }
      return Pt + ADDR_TO_PTE_OFFSET(Address);
   }
   PageDir = ADDR_TO_PDE(Address);
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
               ASSERT(FALSE);
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
            ASSERT(FALSE);
         }
         Entry = InterlockedCompareExchangeUL(PageDir, PFN_TO_PTE(Pfn) | PA_PRESENT | PA_READWRITE | PA_USER, 0);
         if (Entry != 0)
         {
            MmReleasePageMemoryConsumer(MC_NPPOOL, Pfn);
         }
      }
   }
   return (PULONG)ADDR_TO_PTE(Address);
}

BOOLEAN MmUnmapPageTable(PULONG Pt)
{
   if (Ke386Pae)
   {
      if ((PULONGLONG)Pt >= (PULONGLONG)PTE_BASE && (PULONGLONG)Pt < (PULONGLONG)PTE_BASE + 4*512*512)
      {
         return TRUE;
      }
   }
   else
   {
      if (Pt >= (PULONG)PTE_BASE && Pt < (PULONG)PTE_BASE + 1024*1024)
      {
         return TRUE;
      }
   }
   if (Pt)
   {
      MmDeleteHyperspaceMapping((PVOID)PAGE_ROUND_DOWN(Pt));
   }
   return FALSE;
}

static ULONGLONG MmGetPageEntryForProcessForPAE(PEPROCESS Process, PVOID Address)
{
   ULONGLONG Pte;
   PULONGLONG Pt;

   Pt = MmGetPageTableForProcessForPAE(Process, Address, FALSE);
   if (Pt)
   {
      Pte = *Pt;
      MmUnmapPageTable((PULONG)Pt);
      return Pte;
   }
   return 0;
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

PFN_NUMBER
NTAPI
MmGetPfnForProcess(PEPROCESS Process,
                   PVOID Address)
{

   if (Ke386Pae)
   {
      ULONGLONG Entry;
      Entry = MmGetPageEntryForProcessForPAE(Process, Address);
      if (!(Entry & PA_PRESENT))
      {
         return 0;
      }
      return(PAE_PTE_TO_PFN(Entry));
   }
   else
   {
      ULONG Entry;
      Entry = MmGetPageEntryForProcess(Process, Address);
      if (!(Entry & PA_PRESENT))
      {
         return 0;
      }
      return(PTE_TO_PFN(Entry));
   }
}

VOID
NTAPI
MmDeleteVirtualMapping(PEPROCESS Process, PVOID Address,
                       BOOLEAN* WasDirty, PPFN_NUMBER Page)
/*
 * FUNCTION: Delete a virtual mapping
 */
{
   BOOLEAN WasValid = FALSE;
   PFN_NUMBER Pfn;

   DPRINT("MmDeleteVirtualMapping(%x, %x, %d, %x, %x)\n",
          Process, Address, WasDirty, Page);
   if (Ke386Pae)
   {
      ULONGLONG Pte;
      PULONGLONG Pt;

      Pt = MmGetPageTableForProcessForPAE(Process, Address, FALSE);
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
      Pte = 0LL;
      Pte = ExfpInterlockedExchange64UL(Pt, &Pte);

      MiFlushTlb((PULONG)Pt, Address);

      WasValid = PAE_PAGE_MASK(Pte) != 0 ? TRUE : FALSE;
      if (WasValid)
      {
         Pfn = PAE_PTE_TO_PFN(Pte);
         MmMarkPageUnmapped(Pfn);
      }
      else
      {
         Pfn = 0;
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
   }
   else
   {
      ULONG Pte;
      PULONG Pt;

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
      Idx = Ke386Pae ? PAE_ADDR_TO_PAGE_TABLE(Address) : ADDR_TO_PAGE_TABLE(Address);

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
   if (Ke386Pae)
   {
      ULONGLONG Pte;
      PULONGLONG Pt;

      Pt = MmGetPageTableForProcessForPAE(Process, Address, FALSE);
      if (Pt == NULL)
      {
         *SwapEntry = 0;
         return;
      }

      /*
       * Atomically set the entry to zero and get the old value.
       */
      Pte = 0LL;
      Pte = ExfpInterlockedExchange64UL(Pt, &Pte);

      MiFlushTlb((PULONG)Pt, Address);

      /*
       * Decrement the reference count for this page table.
       */
      if (Process != NULL && Pte &&
          ((PMADDRESS_SPACE)&Process->VadRoot)->PageTableRefCountTable != NULL &&
          Address < MmSystemRangeStart)
      {
         PUSHORT Ptrc;

         Ptrc = ((PMADDRESS_SPACE)&Process->VadRoot)->PageTableRefCountTable;

         Ptrc[PAE_ADDR_TO_PAGE_TABLE(Address)]--;
         if (Ptrc[PAE_ADDR_TO_PAGE_TABLE(Address)] == 0)
         {
            MmFreePageTable(Process, Address);
         }
      }


      /*
       * Return some information to the caller
       */
      *SwapEntry = Pte >> 1;
   }
   else
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
}

BOOLEAN
Mmi386MakeKernelPageTableGlobal(PVOID PAddress)
{
   if (Ke386Pae)
   {
      PULONGLONG Pt;
      PULONGLONG Pde;
      Pde = PAE_ADDR_TO_PDE(PAddress);
      if (*Pde == 0LL)
      {
         Pt = MmGetPageTableForProcessForPAE(NULL, PAddress, FALSE);
#if 0
         /* Non existing mappings are not cached within the tlb. We must not invalidate this entry */
         FLASH_TLB_ONE(PAddress);
#endif
         if (Pt != NULL)
         {
            return TRUE;
         }
      }
   }
   else
   {
      PULONG Pt, Pde;
      Pde = ADDR_TO_PDE(PAddress);
      if (*Pde == 0)
      {
         Pt = MmGetPageTableForProcess(NULL, PAddress, FALSE);
#if 0
         /* Non existing mappings are not cached within the tlb. We must not invalidate this entry */
         FLASH_TLB_ONE(PAddress);
#endif
         if (Pt != NULL)
         {
            return TRUE;
         }
      }
   }
   return(FALSE);
}

BOOLEAN
NTAPI
MmIsDirtyPage(PEPROCESS Process, PVOID Address)
{
   if (Ke386Pae)
   {
      return MmGetPageEntryForProcessForPAE(Process, Address) & PA_DIRTY ? TRUE : FALSE;
   }
   else
   {
      return MmGetPageEntryForProcess(Process, Address) & PA_DIRTY ? TRUE : FALSE;
   }
}

VOID
NTAPI
MmSetCleanPage(PEPROCESS Process, PVOID Address)
{
   if (Address < MmSystemRangeStart && Process == NULL)
   {
      DPRINT1("MmSetCleanPage is called for user space without a process.\n");
      ASSERT(FALSE);
   }
   if (Ke386Pae)
   {
      PULONGLONG Pt;
      ULONGLONG Pte;
      ULONGLONG tmpPte;

      Pt = MmGetPageTableForProcessForPAE(Process, Address, FALSE);

      if (Pt == NULL)
      {
         ASSERT(FALSE);
      }

      do
      {
         Pte = *Pt;
         tmpPte = Pte & ~PA_DIRTY;
      } while (Pte != ExfInterlockedCompareExchange64UL(Pt, &tmpPte, &Pte));

      if (Pte & PA_DIRTY)
      {
         MiFlushTlb((PULONG)Pt, Address);
      }
      else
      {
         MmUnmapPageTable((PULONG)Pt);
      }
   }
   else
   {
      PULONG Pt;
      ULONG Pte;

      Pt = MmGetPageTableForProcess(Process, Address, FALSE);

      if (Pt == NULL)
      {
         ASSERT(FALSE);
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
}

VOID
NTAPI
MmSetDirtyPage(PEPROCESS Process, PVOID Address)
{
   if (Address < MmSystemRangeStart && Process == NULL)
   {
      DPRINT1("MmSetDirtyPage is called for user space without a process.\n");
      ASSERT(FALSE);
   }
   if (Ke386Pae)
   {
      PULONGLONG Pt;
      ULONGLONG Pte;
      ULONGLONG tmpPte;

      Pt = MmGetPageTableForProcessForPAE(Process, Address, FALSE);
      if (Pt == NULL)
      {
         ASSERT(FALSE);
      }

      do
      {
         Pte = *Pt;
         tmpPte = Pte | PA_DIRTY;
      } while (Pte != ExfInterlockedCompareExchange64UL(Pt, &tmpPte, &Pte));
      if (!(Pte & PA_DIRTY))
      {
         MiFlushTlb((PULONG)Pt, Address);
      }
      else
      {
         MmUnmapPageTable((PULONG)Pt);
      }
   }
   else
   {
      PULONG Pt;
      ULONG Pte;

      Pt = MmGetPageTableForProcess(Process, Address, FALSE);
      if (Pt == NULL)
      {
         ASSERT(FALSE);
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
}

BOOLEAN
NTAPI
MmIsPagePresent(PEPROCESS Process, PVOID Address)
{
   if (Ke386Pae)
   {
      return MmGetPageEntryForProcessForPAE(Process, Address) & PA_PRESENT ? TRUE : FALSE;
   }
   else
   {
      return MmGetPageEntryForProcess(Process, Address) & PA_PRESENT ? TRUE : FALSE;
   }
}

BOOLEAN
NTAPI
MmIsPageSwapEntry(PEPROCESS Process, PVOID Address)
{
   if (Ke386Pae)
   {
      ULONGLONG Entry;
      Entry = MmGetPageEntryForProcessForPAE(Process, Address);
      return !(Entry & PA_PRESENT) && Entry != 0 ? TRUE : FALSE;
   }
   else
   {
      ULONG Entry;
      Entry = MmGetPageEntryForProcess(Process, Address);
      return !(Entry & PA_PRESENT) && Entry != 0 ? TRUE : FALSE;
   }
}

NTSTATUS
NTAPI
MmCreatePageFileMapping(PEPROCESS Process,
                        PVOID Address,
                        SWAPENTRY SwapEntry)
{
   if (Process == NULL && Address < MmSystemRangeStart)
   {
      DPRINT1("No process\n");
      ASSERT(FALSE);
   }
   if (Process != NULL && Address >= MmSystemRangeStart)
   {
      DPRINT1("Setting kernel address with process context\n");
      ASSERT(FALSE);
   }
   if (SwapEntry & (1 << 31))
   {
      ASSERT(FALSE);
   }

   if (Ke386Pae)
   {
      PULONGLONG Pt;
      ULONGLONG Pte;
      ULONGLONG tmpPte;

      Pt = MmGetPageTableForProcessForPAE(Process, Address, TRUE);
      if (Pt == NULL)
      {
         ASSERT(FALSE);
      }
      tmpPte = SwapEntry << 1;
      Pte = ExfpInterlockedExchange64UL(Pt, &tmpPte);
      if (PAE_PAGE_MASK((Pte)) != 0)
      {
         MmMarkPageUnmapped(PAE_PTE_TO_PFN((Pte)));
      }

      if (Pte != 0)
      {
         MiFlushTlb((PULONG)Pt, Address);
      }
      else
      {
         MmUnmapPageTable((PULONG)Pt);
      }
   }
   else
   {
      PULONG Pt;
      ULONG Pte;

      Pt = MmGetPageTableForProcess(Process, Address, TRUE);
      if (Pt == NULL)
      {
         ASSERT(FALSE);
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
   }
   if (Process != NULL &&
       ((PMADDRESS_SPACE)&Process->VadRoot)->PageTableRefCountTable != NULL &&
       Address < MmSystemRangeStart)
   {
     PUSHORT Ptrc;
     ULONG Idx;

     Ptrc = ((PMADDRESS_SPACE)&Process->VadRoot)->PageTableRefCountTable;
     Idx = Ke386Pae ? PAE_ADDR_TO_PAGE_TABLE(Address) : ADDR_TO_PAGE_TABLE(Address);
     Ptrc[Idx]++;
   }
   return(STATUS_SUCCESS);
}


NTSTATUS
NTAPI
MmCreateVirtualMappingUnsafe(PEPROCESS Process,
                             PVOID Address,
                             ULONG flProtect,
                             PPFN_NUMBER Pages,
                             ULONG PageCount)
{
   ULONG Attributes;
   PVOID Addr;
   ULONG i;
   ULONG oldPdeOffset, PdeOffset;
   BOOLEAN NoExecute = FALSE;

   DPRINT("MmCreateVirtualMappingUnsafe(%x, %x, %x, %x (%x), %d)\n",
          Process, Address, flProtect, Pages, *Pages, PageCount);

   if (Process == NULL)
   {
      if (Address < MmSystemRangeStart)
      {
         DPRINT1("No process\n");
         ASSERT(FALSE);
      }
      if (PageCount > 0x10000 ||
          (ULONG_PTR) Address / PAGE_SIZE + PageCount > 0x100000)
      {
         DPRINT1("Page count to large\n");
         ASSERT(FALSE);
      }
   }
   else
   {
      if (Address >= MmSystemRangeStart)
      {
         DPRINT1("Setting kernel address with process context\n");
         ASSERT(FALSE);
      }
      if (PageCount > (ULONG_PTR)MmSystemRangeStart / PAGE_SIZE ||
          (ULONG_PTR) Address / PAGE_SIZE + PageCount >
          (ULONG_PTR)MmSystemRangeStart / PAGE_SIZE)
      {
         DPRINT1("Page Count to large\n");
         ASSERT(FALSE);
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

   if (Ke386Pae)
   {
      ULONGLONG Pte, tmpPte;
      PULONGLONG Pt = NULL;

      oldPdeOffset = PAE_ADDR_TO_PDE_OFFSET(Addr) + 1;
      for (i = 0; i < PageCount; i++, Addr = (PVOID)((ULONG_PTR)Addr + PAGE_SIZE))
      {
         if (!(Attributes & PA_PRESENT) && Pages[i] != 0)
         {
            DPRINT1("Setting physical address but not allowing access at address "
                    "0x%.8X with attributes %x/%x.\n",
                    Addr, Attributes, flProtect);
            ASSERT(FALSE);
         }
         PdeOffset = PAE_ADDR_TO_PDE_OFFSET(Addr);
         if (oldPdeOffset != PdeOffset)
         {
            MmUnmapPageTable((PULONG)Pt);
            Pt = MmGetPageTableForProcessForPAE(Process, Addr, TRUE);
            if (Pt == NULL)
            {
               ASSERT(FALSE);
            }
         }
         else
         {
            Pt++;
         }
         oldPdeOffset = PdeOffset;

         MmMarkPageMapped(Pages[i]);
         tmpPte = PAE_PFN_TO_PTE(Pages[i]) | Attributes;
         if (NoExecute)
         {
            tmpPte |= 0x8000000000000000LL;
         }
         Pte = ExfpInterlockedExchange64UL(Pt, &tmpPte);
         if (PAE_PAGE_MASK((Pte)) != 0LL && !((Pte) & PA_PRESENT))
         {
            ASSERT(FALSE);
         }
         if (PAE_PAGE_MASK((Pte)) != 0LL)
         {
            MmMarkPageUnmapped(PAE_PTE_TO_PFN((Pte)));
         }
         if (Address < MmSystemRangeStart &&
             ((PMADDRESS_SPACE)&Process->VadRoot)->PageTableRefCountTable != NULL &&
             Attributes & PA_PRESENT)
         {
            PUSHORT Ptrc;

            Ptrc = ((PMADDRESS_SPACE)&Process->VadRoot)->PageTableRefCountTable;

            Ptrc[PAE_ADDR_TO_PAGE_TABLE(Addr)]++;
         }
         if (Pte != 0LL)
         {
            if (Address > MmSystemRangeStart ||
                (Pt >= (PULONGLONG)PTE_BASE && Pt < (PULONGLONG)PTE_BASE + 4*512*512))
            {
              MiFlushTlb((PULONG)Pt, Address);
            }
         }
      }
      if (Addr > Address)
      {
         MmUnmapPageTable((PULONG)Pt);
      }
   }
   else
   {
      PULONG Pt = NULL;
      ULONG Pte;
      oldPdeOffset = ADDR_TO_PDE_OFFSET(Addr) + 1;
      for (i = 0; i < PageCount; i++, Addr = (PVOID)((ULONG_PTR)Addr + PAGE_SIZE))
      {
         if (!(Attributes & PA_PRESENT) && Pages[i] != 0)
         {
            DPRINT1("Setting physical address but not allowing access at address "
                    "0x%.8X with attributes %x/%x.\n",
                    Addr, Attributes, flProtect);
            ASSERT(FALSE);
         }
         PdeOffset = ADDR_TO_PDE_OFFSET(Addr);
         if (oldPdeOffset != PdeOffset)
         {
            MmUnmapPageTable(Pt);
            Pt = MmGetPageTableForProcess(Process, Addr, TRUE);
            if (Pt == NULL)
            {
               ASSERT(FALSE);
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
            ASSERT(FALSE);
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
                (Pt >= (PULONG)PTE_BASE && Pt < (PULONG)PTE_BASE + 1024*1024))
            {
               MiFlushTlb(Pt, Address);
            }
         }
      }
      if (Addr > Address)
      {
         MmUnmapPageTable(Pt);
      }
   }
   return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
MmCreateVirtualMapping(PEPROCESS Process,
                       PVOID Address,
                       ULONG flProtect,
                       PPFN_NUMBER Pages,
                       ULONG PageCount)
{
   ULONG i;

   for (i = 0; i < PageCount; i++)
   {
      if (!MmIsPageInUse(Pages[i]))
      {
         DPRINT1("Page at address %x not in use\n", PFN_TO_PTE(Pages[i]));
         ASSERT(FALSE);
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
   if (Ke386Pae)
   {
      Entry = MmGetPageEntryForProcessForPAE(Process, Address);
   }
   else
   {
      Entry = MmGetPageEntryForProcess(Process, Address);
   }

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
   if (Ke386Pae)
   {
      PULONGLONG Pt;
      ULONGLONG tmpPte, Pte;

      Pt = MmGetPageTableForProcessForPAE(Process, Address, FALSE);
      if (Pt == NULL)
      {
         DPRINT1("Address %x\n", Address);
         ASSERT(FALSE);
      }
      do
      {
        Pte = *Pt;
        tmpPte = PAE_PAGE_MASK(Pte) | Attributes | (Pte & (PA_ACCESSED|PA_DIRTY));
        if (NoExecute)
        {
           tmpPte |= 0x8000000000000000LL;
        }
        else
        {
           tmpPte &= ~0x8000000000000000LL;
        }
      } while (Pte != ExfInterlockedCompareExchange64UL(Pt, &tmpPte, &Pte));

      MiFlushTlb((PULONG)Pt, Address);
   }
   else
   {
      PULONG Pt;

      Pt = MmGetPageTableForProcess(Process, Address, FALSE);
      if (Pt == NULL)
      {
         ASSERT(FALSE);
      }
      InterlockedExchange((PLONG)Pt, PAGE_MASK(*Pt) | Attributes | (*Pt & (PA_ACCESSED|PA_DIRTY)));
      MiFlushTlb(Pt, Address);
   }
}

VOID
INIT_FUNCTION
NTAPI
MmInitGlobalKernelPageDirectory(VOID)
{
   ULONG i;

   DPRINT("MmInitGlobalKernelPageDirectory()\n");

   if (Ke386Pae)
   {
      PULONGLONG CurrentPageDirectory = (PULONGLONG)PAE_PAGEDIRECTORY_MAP;
      for (i = PAE_ADDR_TO_PDE_OFFSET(MmSystemRangeStart); i < 4 * 512; i++)
      {
         if (!(i >= PAE_ADDR_TO_PDE_OFFSET(PTE_BASE) && i < PAE_ADDR_TO_PDE_OFFSET(PTE_BASE) + 4) &&
             !(i >= PAE_ADDR_TO_PDE_OFFSET(HYPERSPACE) && i < PAE_ADDR_TO_PDE_OFFSET(HYPERSPACE) + 2) &&
             0LL == MmGlobalKernelPageDirectoryForPAE[i] && 0LL != CurrentPageDirectory[i])
         {
            (void)ExfpInterlockedExchange64UL(&MmGlobalKernelPageDirectoryForPAE[i], &CurrentPageDirectory[i]);
            if (Ke386GlobalPagesEnabled)
            {
               MmGlobalKernelPageDirectoryForPAE[i] |= PA_GLOBAL;
               CurrentPageDirectory[i] |= PA_GLOBAL;
            }
         }
      }
   }
   else
   {
      PULONG CurrentPageDirectory = (PULONG)PAGEDIRECTORY_MAP;
      for (i = ADDR_TO_PDE_OFFSET(MmSystemRangeStart); i < 1024; i++)
      {
         if (i != ADDR_TO_PDE_OFFSET(PTE_BASE) &&
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
}

/* EOF */
