/* $Id$
 *
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

#define PAGETABLE_MAP		(0xc0000000)
#define PAGEDIRECTORY_MAP	(0xc0000000 + (PAGETABLE_MAP / (1024)))

#define PAE_PAGEDIRECTORY_MAP	(0xc0000000 + (PAGETABLE_MAP / (512)))

#define HYPERSPACE		(Ke386Pae ? 0xc0800000 : 0xc0400000)
#define IS_HYPERSPACE(v)	(((ULONG)(v) >= HYPERSPACE && (ULONG)(v) < HYPERSPACE + 0x400000))

ULONG MmGlobalKernelPageDirectory[1024];
ULONGLONG MmGlobalKernelPageDirectoryForPAE[2048];

#define PTE_TO_PFN(X)  ((X) >> PAGE_SHIFT)
#define PFN_TO_PTE(X)  ((X) << PAGE_SHIFT)

#define PAE_PTE_TO_PFN(X)   (PAE_PAGE_MASK(X) >> PAGE_SHIFT)
#define PAE_PFN_TO_PTE(X)   ((X) << PAGE_SHIFT)

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

extern BOOLEAN Ke386Pae;
extern BOOLEAN Ke386NoExecute;

/* FUNCTIONS ***************************************************************/

BOOLEAN MmUnmapPageTable(PULONG Pt);

VOID
STDCALL
MiFlushTlbIpiRoutine(PVOID Address)
{
   if (Address == (PVOID)0xffffffff)
   {
      KeFlushCurrentTb();
   }
   else if (Address == (PVOID)0xfffffffe)
   {
      KeFlushCurrentTb();
   }
   else
   {
       __invlpg(Address);
   }
}

VOID
MiFlushTlb(PULONG Pt, PVOID Address)
{
#ifdef CONFIG_SMP
   if (Pt)
   {
      MmUnmapPageTable(Pt);
   }
   if (KeNumberProcessors>1)
   {
      KeIpiGenericCall(MiFlushTlbIpiRoutine, Address);
   }
   else
   {
      MiFlushTlbIpiRoutine(Address);
   }
#else
   if ((Pt && MmUnmapPageTable(Pt)) || Address >= MmSystemRangeStart)
   {
      __invlpg(Address);
   }
#endif
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
#define ADDR_TO_PTE(v) (PULONG)(PAGETABLE_MAP + ((((ULONG)(v) / 1024))&(~0x3)))

#define ADDR_TO_PDE_OFFSET(v) ((((ULONG)(v)) / (1024 * PAGE_SIZE)))

#define ADDR_TO_PTE_OFFSET(v)  ((((ULONG)(v)) % (1024 * PAGE_SIZE)) / PAGE_SIZE)


#define PAE_ADDR_TO_PAGE_TABLE(v)   (((ULONG)(v)) / (512 * PAGE_SIZE))

#define PAE_ADDR_TO_PDE(v)	    (PULONGLONG) (PAE_PAGEDIRECTORY_MAP + \
                                                  ((((ULONG_PTR)(v)) / (512 * 512))&(~0x7)))
#define PAE_ADDR_TO_PTE(v)	    (PULONGLONG) (PAGETABLE_MAP + ((((ULONG_PTR)(v) / 512))&(~0x7)))


#define PAE_ADDR_TO_PDTE_OFFSET(v)  (((ULONG_PTR)(v)) / (512 * 512 * PAGE_SIZE))

#define PAE_ADDR_TO_PDE_PAGE_OFFSET(v)   ((((ULONG_PTR)(v)) % (512 * 512 * PAGE_SIZE)) / (512 * PAGE_SIZE))

#define PAE_ADDR_TO_PDE_OFFSET(v)   (((ULONG_PTR)(v))/ (512 * PAGE_SIZE))

#define PAE_ADDR_TO_PTE_OFFSET(v)   ((((ULONG_PTR)(v)) % (512 * PAGE_SIZE)) / PAGE_SIZE)


NTSTATUS
NTAPI
Mmi386ReleaseMmInfo(PEPROCESS Process)
{
   PUSHORT LdtDescriptor;
   ULONG LdtBase;
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

   if (Ke386Pae)
   {
      PULONGLONG PageDirTable;
      PULONGLONG PageDir;
      PULONGLONG Pde;
      ULONG k;

      PageDirTable = (PULONGLONG)MmCreateHyperspaceMapping(PAE_PTE_TO_PFN(Process->Pcb.DirectoryTableBase.QuadPart));
      for (i = 0; i < 4; i++)
      {
         PageDir = (PULONGLONG)MmCreateHyperspaceMapping(PAE_PTE_TO_PFN(PageDirTable[i]));
         if (i < PAE_ADDR_TO_PDTE_OFFSET(MmSystemRangeStart))
	 {
	    for (j = 0; j < 512; j++)
	    {
	       if (PageDir[j] != 0LL)
	       {
                  DPRINT1("ProcessId %d, Pde for %08x - %08x is not freed, RefCount %d\n",
		          Process->UniqueProcessId,
	                  (i * 512 + j) * 512 * PAGE_SIZE, (i * 512 + j + 1) * 512 * PAGE_SIZE - 1,
		          ((PMADDRESS_SPACE)&Process->VadRoot)->PageTableRefCountTable[i*512 + j]);
                  Pde = MmCreateHyperspaceMapping(PAE_PTE_TO_PFN(PageDir[j]));
	          for (k = 0; k < 512; k++)
	          {
	             if(Pde[k] != 0)
	             {
	                if (Pde[k] & PA_PRESENT)
	                {
	                   DPRINT1("Page at %08x is not freed\n",
		                   (i * 512 + j) * 512 * PAGE_SIZE + k * PAGE_SIZE);
	                }
	                else
	                {
	                   DPRINT1("Swapentry %x at %x is not freed\n",
		                   (i * 512 + j) * 512 * PAGE_SIZE + k * PAGE_SIZE);
	                }
		     }
		  }
		  MmDeleteHyperspaceMapping(Pde);
	    	  MmReleasePageMemoryConsumer(MC_NPPOOL, PAE_PTE_TO_PFN(PageDir[j]));
	       }
	    }
	 }
	 if (i == PAE_ADDR_TO_PDTE_OFFSET(HYPERSPACE))
	 {
	    MmReleasePageMemoryConsumer(MC_NPPOOL, PAE_PTE_TO_PFN(PageDir[PAE_ADDR_TO_PDE_PAGE_OFFSET(HYPERSPACE)]));
	    MmReleasePageMemoryConsumer(MC_NPPOOL, PAE_PTE_TO_PFN(PageDir[PAE_ADDR_TO_PDE_PAGE_OFFSET(HYPERSPACE)+1]));
	 }
	 MmDeleteHyperspaceMapping(PageDir);
         MmReleasePageMemoryConsumer(MC_NPPOOL, PAE_PTE_TO_PFN(PageDirTable[i]));
      }
      MmDeleteHyperspaceMapping((PVOID)PageDirTable);
      MmReleasePageMemoryConsumer(MC_NPPOOL, PAE_PTE_TO_PFN(Process->Pcb.DirectoryTableBase.QuadPart));
   }
   else
   {
      PULONG Pde;
      PULONG PageDir;
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
   }

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
   PFN_TYPE Pfn[7];
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
	 if (PAE_ADDR_TO_PDTE_OFFSET(PAGETABLE_MAP) == i)
	 {
            for (j = 0; j < 4; j++)
            {
               PageDir[PAE_ADDR_TO_PDE_PAGE_OFFSET(PAGETABLE_MAP) + j] = PAE_PFN_TO_PTE(Pfn[1+j]) | PA_PRESENT | PA_READWRITE;
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

      DPRINT("Addr %x\n",ADDR_TO_PDE_OFFSET(PAGETABLE_MAP));
      PageDirectory[ADDR_TO_PDE_OFFSET(PAGETABLE_MAP)] = PFN_TO_PTE(Pfn[0]) | PA_PRESENT | PA_READWRITE;
      PageDirectory[ADDR_TO_PDE_OFFSET(HYPERSPACE)] = PFN_TO_PTE(Pfn[1]) | PA_PRESENT | PA_READWRITE;

      MmDeleteHyperspaceMapping(PageDirectory);
   }

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

   if (Ke386Pae)
   {
      ULONGLONG ZeroPde = 0LL;
      (void)ExfpInterlockedExchange64UL(PAE_ADDR_TO_PDE(Address), &ZeroPde);
      MiFlushTlb((PULONG)PAE_ADDR_TO_PDE(Address), PAE_ADDR_TO_PTE(Address));
   }
   else
   {
      *(ADDR_TO_PDE(Address)) = 0;
      MiFlushTlb(ADDR_TO_PDE(Address), ADDR_TO_PTE(Address));
   }
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
            KEBUGCHECK(0);
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
            KEBUGCHECK(0);
         }
      }
      Pfn = PTE_TO_PFN(*(ADDR_TO_PDE(Address)));
      *(ADDR_TO_PDE(Address)) = 0;
      MiFlushTlb(ADDR_TO_PDE(Address), ADDR_TO_PTE(Address));
   }

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

static PULONGLONG
MmGetPageTableForProcessForPAE(PEPROCESS Process, PVOID Address, BOOLEAN Create)
{
   NTSTATUS Status;
   PFN_TYPE Pfn;
   ULONGLONG Entry;
   ULONGLONG ZeroEntry = 0LL;
   PULONGLONG Pt;
   PULONGLONG PageDir;
   PULONGLONG PageDirTable;

   DPRINT("MmGetPageTableForProcessForPAE(%x %x %d)\n",
          Process, Address, Create);
   if (Address >= (PVOID)PAGETABLE_MAP && Address < (PVOID)((ULONG_PTR)PAGETABLE_MAP + 0x800000))
   {
      KEBUGCHECK(0);
   }
   if (Address < MmSystemRangeStart && Process && Process != PsGetCurrentProcess())
   {
      PageDirTable = MmCreateHyperspaceMapping(PAE_PTE_TO_PFN(Process->Pcb.DirectoryTableBase.QuadPart));
      if (PageDirTable == NULL)
      {
         KEBUGCHECK(0);
      }
      PageDir = MmCreateHyperspaceMapping(PAE_PTE_TO_PFN(PageDirTable[PAE_ADDR_TO_PDTE_OFFSET(Address)]));
      MmDeleteHyperspaceMapping(PageDirTable);
      if (PageDir == NULL)
      {
         KEBUGCHECK(0);
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
	    KEBUGCHECK(0);
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
         KEBUGCHECK(0);
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
	       KEBUGCHECK(0);
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
	    KEBUGCHECK(0);
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
   return (PULONG)ADDR_TO_PTE(Address);
}

BOOLEAN MmUnmapPageTable(PULONG Pt)
{
   if (Ke386Pae)
   {
      if ((PULONGLONG)Pt >= (PULONGLONG)PAGETABLE_MAP && (PULONGLONG)Pt < (PULONGLONG)PAGETABLE_MAP + 4*512*512)
      {
         return TRUE;
      }
   }
   else
   {
      if (Pt >= (PULONG)PAGETABLE_MAP && Pt < (PULONG)PAGETABLE_MAP + 1024*1024)
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

PFN_TYPE
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
MmDisableVirtualMapping(PEPROCESS Process, PVOID Address, BOOLEAN* WasDirty, PPFN_TYPE Page)
/*
 * FUNCTION: Delete a virtual mapping
 */
{
   BOOLEAN WasValid;
   if (Ke386Pae)
   {
      ULONGLONG Pte;
      ULONGLONG tmpPte;
      PULONGLONG Pt;

      Pt = MmGetPageTableForProcessForPAE(Process, Address, FALSE);
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
	tmpPte = Pte & ~PA_PRESENT;
      } while (Pte != ExfInterlockedCompareExchange64UL(Pt, &tmpPte, &Pte));

      MiFlushTlb((PULONG)Pt, Address);
      WasValid = PAE_PAGE_MASK(Pte) != 0LL ? TRUE : FALSE;
      if (!WasValid)
      {
         KEBUGCHECK(0);
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
         *Page = PAE_PTE_TO_PFN(Pte);
      }
   }
   else
   {
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
}

VOID
NTAPI
MmRawDeleteVirtualMapping(PVOID Address)
{
   if (Ke386Pae)
   {
      PULONGLONG Pt;
      ULONGLONG ZeroPte = 0LL;
      Pt = MmGetPageTableForProcessForPAE(NULL, Address, FALSE);
      if (Pt)
      {
         /*
          * Set the entry to zero
          */
	 (void)ExfpInterlockedExchange64UL(Pt, &ZeroPte);
         MiFlushTlb((PULONG)Pt, Address);
      }
   }
   else
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

   DPRINT("MmDeleteVirtualMapping(%x, %x, %d, %x, %x)\n",
          Process, Address, FreePage, WasDirty, Page);
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

BOOLEAN
NTAPI
MmIsAccessedAndResetAccessPage(PEPROCESS Process, PVOID Address)
{
   if (Address < MmSystemRangeStart && Process == NULL)
   {
      DPRINT1("MmIsAccessedAndResetAccessPage is called for user space without a process.\n");
      KEBUGCHECK(0);
   }
   if (Ke386Pae)
   {
      PULONGLONG Pt;
      ULONGLONG Pte;
      ULONGLONG tmpPte;

      Pt = MmGetPageTableForProcessForPAE(Process, Address, FALSE);
      if (Pt == NULL)
      {
         KEBUGCHECK(0);
      }

      do
      {
         Pte = *Pt;
	 tmpPte = Pte & ~PA_ACCESSED;
      } while (Pte != ExfInterlockedCompareExchange64UL(Pt, &tmpPte, &Pte));

      if (Pte & PA_ACCESSED)
      {
         MiFlushTlb((PULONG)Pt, Address);
         return TRUE;
      }
      else
      {
         MmUnmapPageTable((PULONG)Pt);
         return FALSE;
      }
   }
   else
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
}

VOID
NTAPI
MmSetCleanPage(PEPROCESS Process, PVOID Address)
{
   if (Address < MmSystemRangeStart && Process == NULL)
   {
      DPRINT1("MmSetCleanPage is called for user space without a process.\n");
      KEBUGCHECK(0);
   }
   if (Ke386Pae)
   {
      PULONGLONG Pt;
      ULONGLONG Pte;
      ULONGLONG tmpPte;

      Pt = MmGetPageTableForProcessForPAE(Process, Address, FALSE);

      if (Pt == NULL)
      {
         KEBUGCHECK(0);
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
}

VOID
NTAPI
MmSetDirtyPage(PEPROCESS Process, PVOID Address)
{
   if (Address < MmSystemRangeStart && Process == NULL)
   {
      DPRINT1("MmSetDirtyPage is called for user space without a process.\n");
      KEBUGCHECK(0);
   }
   if (Ke386Pae)
   {
      PULONGLONG Pt;
      ULONGLONG Pte;
      ULONGLONG tmpPte;

      Pt = MmGetPageTableForProcessForPAE(Process, Address, FALSE);
      if (Pt == NULL)
      {
         KEBUGCHECK(0);
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
}

VOID
NTAPI
MmEnableVirtualMapping(PEPROCESS Process, PVOID Address)
{
   if (Ke386Pae)
   {
      PULONGLONG Pt;
      ULONGLONG Pte;
      ULONGLONG tmpPte;

      Pt = MmGetPageTableForProcessForPAE(Process, Address, FALSE);
      if (Pt == NULL)
      {
         KEBUGCHECK(0);
      }

      do
      {
         Pte = *Pt;
	 tmpPte = Pte | PA_PRESENT;
      } while (Pte != ExfInterlockedCompareExchange64UL(Pt, &tmpPte, &Pte));
      if (!(Pte & PA_PRESENT))
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
MmCreateVirtualMappingForKernel(PVOID Address,
                                ULONG flProtect,
                                PPFN_TYPE Pages,
				ULONG PageCount)
{
   ULONG Attributes;
   ULONG i;
   PVOID Addr;
   ULONG PdeOffset, oldPdeOffset;
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

   if (Ke386Pae)
   {
      PULONGLONG Pt = NULL;
      ULONGLONG Pte;

      oldPdeOffset = PAE_ADDR_TO_PDE_OFFSET(Addr) + 1;
      for (i = 0; i < PageCount; i++, Addr = (PVOID)((ULONG_PTR)Addr + PAGE_SIZE))
      {
         if (!(Attributes & PA_PRESENT) && Pages[i] != 0)
         {
            DPRINT1("Setting physical address but not allowing access at address "
                    "0x%.8X with attributes %x/%x.\n",
                    Addr, Attributes, flProtect);
            KEBUGCHECK(0);
         }

         PdeOffset = PAE_ADDR_TO_PDE_OFFSET(Addr);
         if (oldPdeOffset != PdeOffset)
         {
            Pt = MmGetPageTableForProcessForPAE(NULL, Addr, TRUE);
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

	 Pte = PFN_TO_PTE(Pages[i]) | Attributes;
	 if (NoExecute)
	 {
	    Pte |= 0x8000000000000000LL;
	 }
         Pte = ExfpInterlockedExchange64UL(Pt, &Pte);
         if (Pte != 0LL)
         {
            KEBUGCHECK(0);
         }
      }
   }
   else
   {
      PULONG Pt;
      ULONG Pte;

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
   }

   return(STATUS_SUCCESS);
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

   if (Ke386Pae)
   {
      PULONGLONG Pt;
      ULONGLONG Pte;
      ULONGLONG tmpPte;

      Pt = MmGetPageTableForProcessForPAE(Process, Address, TRUE);
      if (Pt == NULL)
      {
         KEBUGCHECK(0);
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
                             PPFN_TYPE Pages,
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
            KEBUGCHECK(0);
         }
         PdeOffset = PAE_ADDR_TO_PDE_OFFSET(Addr);
         if (oldPdeOffset != PdeOffset)
         {
            MmUnmapPageTable((PULONG)Pt);
	    Pt = MmGetPageTableForProcessForPAE(Process, Addr, TRUE);
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

         MmMarkPageMapped(Pages[i]);
	 tmpPte = PAE_PFN_TO_PTE(Pages[i]) | Attributes;
	 if (NoExecute)
	 {
	    tmpPte |= 0x8000000000000000LL;
	 }
         Pte = ExfpInterlockedExchange64UL(Pt, &tmpPte);
         if (PAE_PAGE_MASK((Pte)) != 0LL && !((Pte) & PA_PRESENT))
         {
            KEBUGCHECK(0);
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
                (Pt >= (PULONGLONG)PAGETABLE_MAP && Pt < (PULONGLONG)PAGETABLE_MAP + 4*512*512))
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
      if (!MmIsUsablePage(Pages[i]))
      {
          /* Is this an attempt to map KUSER_SHARED_DATA? */
         if ((Address == (PVOID)0x7FFE0000) && (PageCount == 1) && (Pages[0] == 2))
         {
            // allow
         }
         else
         {
            DPRINT1("Page at address %x not usable\n", PFN_TO_PTE(Pages[i]));
            KEBUGCHECK(0);
         }
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
         KEBUGCHECK(0);
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
         KEBUGCHECK(0);
      }
      InterlockedExchange((PLONG)Pt, PAGE_MASK(*Pt) | Attributes | (*Pt & (PA_ACCESSED|PA_DIRTY)));
      MiFlushTlb(Pt, Address);
   }
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

   DPRINT("MmGetPhysicalAddress(vaddr %x)\n", vaddr);
   if (Ke386Pae)
   {
      ULONGLONG Pte;
      Pte = MmGetPageEntryForProcessForPAE(NULL, vaddr);
      if (Pte != 0 && Pte & PA_PRESENT)
      {
         p.QuadPart = PAE_PAGE_MASK(Pte);
         p.u.LowPart |= (ULONG_PTR)vaddr & (PAGE_SIZE - 1);
      }
      else
      {
         p.QuadPart = 0;
      }
   }
   else
   {
      ULONG Pte;
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
   }
   return p;
}

PVOID
NTAPI
MmCreateHyperspaceMapping(PFN_TYPE Page)
{
   PVOID Address;
   ULONG i;

   if (Ke386Pae)
   {
      ULONGLONG Entry;
      ULONGLONG ZeroEntry = 0LL;
      PULONGLONG Pte;

      Entry = PFN_TO_PTE(Page) | PA_PRESENT | PA_READWRITE;
      Pte = PAE_ADDR_TO_PTE(HYPERSPACE) + Page % 1024;

      if (Page & 1024)
      {
         for (i = Page %1024; i < 1024; i++, Pte++)
         {
            if (0LL == ExfInterlockedCompareExchange64UL(Pte, &Entry, &ZeroEntry))
	    {
	       break;
	    }
         }
         if (i >= 1024)
         {
            Pte = PAE_ADDR_TO_PTE(HYPERSPACE);
	    for (i = 0; i < Page % 1024; i++, Pte++)
	    {
               if (0LL == ExfInterlockedCompareExchange64UL(Pte, &Entry, &ZeroEntry))
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
         for (i = Page %1024; (LONG)i >= 0; i--, Pte--)
         {
            if (0LL == ExfInterlockedCompareExchange64UL(Pte, &Entry, &ZeroEntry))
	    {
	       break;
	    }
         }
         if ((LONG)i < 0)
         {
            Pte = PAE_ADDR_TO_PTE(HYPERSPACE) + 1023;
	    for (i = 1023; i > Page % 1024; i--, Pte--)
	    {
               if (0LL == ExfInterlockedCompareExchange64UL(Pte, &Entry, &ZeroEntry))
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
   }
   else
   {
      ULONG Entry;
      PULONG Pte;
      Entry = PFN_TO_PTE(Page) | PA_PRESENT | PA_READWRITE;
      Pte = ADDR_TO_PTE(HYPERSPACE) + Page % 1024;
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
            Pte = ADDR_TO_PTE(HYPERSPACE);
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
            Pte = ADDR_TO_PTE(HYPERSPACE) + 1023;
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
   ASSERT (IS_HYPERSPACE(Address));
   if (Ke386Pae)
   {
      ULONGLONG Entry = PAE_PFN_TO_PTE(NewPage) | PA_PRESENT | PA_READWRITE;
      Entry = (ULONG)ExfpInterlockedExchange64UL(PAE_ADDR_TO_PTE(Address), &Entry);
      Pfn = PAE_PTE_TO_PFN(Entry);
   }
   else
   {
      ULONG Entry;
      Entry = InterlockedExchange((PLONG)ADDR_TO_PTE(Address), PFN_TO_PTE(NewPage) | PA_PRESENT | PA_READWRITE);
      Pfn = PTE_TO_PFN(Entry);
   }
   __invlpg(Address);
   return Pfn;
}

PFN_TYPE
NTAPI
MmDeleteHyperspaceMapping(PVOID Address)
{
   PFN_TYPE Pfn;
   ASSERT (IS_HYPERSPACE(Address));
   if (Ke386Pae)
   {
      ULONGLONG Entry = 0LL;
      Entry = (ULONG)ExfpInterlockedExchange64UL(PAE_ADDR_TO_PTE(Address), &Entry);
      Pfn = PAE_PTE_TO_PFN(Entry);
   }
   else
   {
      ULONG Entry;
      Entry = InterlockedExchange((PLONG)ADDR_TO_PTE(Address), 0);
      Pfn = PTE_TO_PFN(Entry);
   }
   __invlpg(Address);
   return Pfn;
}

VOID
NTAPI
MmUpdatePageDir(PEPROCESS Process, PVOID Address, ULONG Size)
{
   ULONG StartOffset, EndOffset, Offset;

   if (Address < MmSystemRangeStart)
   {
      KEBUGCHECK(0);
   }
   if (Ke386Pae)
   {
      PULONGLONG PageDirTable;
      PULONGLONG Pde;
      ULONGLONG ZeroPde = 0LL;
      ULONG i;

      for (i = PAE_ADDR_TO_PDTE_OFFSET(Address); i <= PAE_ADDR_TO_PDTE_OFFSET((PVOID)((ULONG_PTR)Address + Size)); i++)
      {
         if (i == PAE_ADDR_TO_PDTE_OFFSET(Address))
	 {
            StartOffset = PAE_ADDR_TO_PDE_PAGE_OFFSET(Address);
	 }
	 else
	 {
	    StartOffset = 0;
	 }
	 if (i == PAE_ADDR_TO_PDTE_OFFSET((PVOID)((ULONG_PTR)Address + Size)))
	 {
	    EndOffset = PAE_ADDR_TO_PDE_PAGE_OFFSET((PVOID)((ULONG_PTR)Address + Size));
	 }
	 else
	 {
	    EndOffset = 511;
	 }

         if (Process != NULL && Process != PsGetCurrentProcess())
         {
            PageDirTable = MmCreateHyperspaceMapping(PAE_PTE_TO_PFN(Process->Pcb.DirectoryTableBase.QuadPart));
            Pde = (PULONGLONG)MmCreateHyperspaceMapping(PTE_TO_PFN(PageDirTable[i]));
	    MmDeleteHyperspaceMapping(PageDirTable);
         }
         else
         {
            Pde = (PULONGLONG)PAE_PAGEDIRECTORY_MAP + i*512;
         }

         for (Offset = StartOffset; Offset <= EndOffset; Offset++)
         {
            if (i * 512 + Offset < PAE_ADDR_TO_PDE_OFFSET(PAGETABLE_MAP) || i * 512 + Offset >= PAE_ADDR_TO_PDE_OFFSET(PAGETABLE_MAP)+4)
            {
               (void)ExfInterlockedCompareExchange64UL(&Pde[Offset], &MmGlobalKernelPageDirectoryForPAE[i*512 + Offset], &ZeroPde);
            }
         }
         MmUnmapPageTable((PULONG)Pde);
      }
   }
   else
   {
      PULONG Pde;
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
         if (!(i >= PAE_ADDR_TO_PDE_OFFSET(PAGETABLE_MAP) && i < PAE_ADDR_TO_PDE_OFFSET(PAGETABLE_MAP) + 4) &&
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
}

ULONG
NTAPI
MiGetUserPageDirectoryCount(VOID)
{
   return Ke386Pae ? PAE_ADDR_TO_PDE_OFFSET(MmSystemRangeStart) : ADDR_TO_PDE_OFFSET(MmSystemRangeStart);
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
		               Ke386Pae ? 0x800000 : 0x400000,
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
