/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: page.c,v 1.67 2004/08/01 07:27:25 hbirr Exp $
 *
 * PROJECT:     ReactOS kernel
 * FILE:        ntoskrnl/mm/i386/page.c
 * PURPOSE:     low level memory managment manipulation
 * PROGRAMER:   David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *              9/3/98: Created
 */

/* INCLUDES ***************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <internal/i386/mm.h>
#include <internal/ex.h>
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

#define PA_BIT_PRESENT   (0)
#define PA_BIT_READWRITE (1)
#define PA_BIT_USER      (2)
#define PA_BIT_WT        (3)
#define PA_BIT_CD        (4)
#define PA_BIT_ACCESSED  (5)
#define PA_BIT_DIRTY     (6)

#define PA_PRESENT   (1 << PA_BIT_PRESENT)
#define PA_READWRITE (1 << PA_BIT_READWRITE)
#define PA_USER      (1 << PA_BIT_USER)
#define PA_DIRTY     (1 << PA_BIT_DIRTY)
#define PA_WT        (1 << PA_BIT_WT)
#define PA_CD        (1 << PA_BIT_CD)
#define PA_ACCESSED  (1 << PA_BIT_ACCESSED)
#define PA_DIRTY     (1 << PA_BIT_DIRTY)

#define PAGETABLE_MAP     (0xf0000000)
#define PAGEDIRECTORY_MAP (0xf0000000 + (PAGETABLE_MAP / (1024)))

ULONG MmGlobalKernelPageDirectory[1024] = {0, };

#if defined(__GNUC__)
#define PTE_TO_PFN(X)  ((X) >> PAGE_SHIFT)
#define PFN_TO_PTE(X)  ((X) << PAGE_SHIFT)	
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

PULONG
MmGetPageDirectory(VOID)
{
   unsigned int page_dir=0;
   Ke386GetPageTableDirectory(page_dir);
   return((PULONG)page_dir);
}

static ULONG
ProtectToPTE(ULONG flProtect)
{
   ULONG Attributes = 0;

   if (flProtect & (PAGE_NOACCESS|PAGE_GUARD))
   {
      Attributes = 0;
   }
   else if (flProtect & (PAGE_READWRITE|PAGE_EXECUTE_READWRITE))
   {
      Attributes = PA_PRESENT | PA_READWRITE;
   }
   else if (flProtect & (PAGE_READONLY|PAGE_EXECUTE|PAGE_EXECUTE_READ))
   {
      Attributes = PA_PRESENT;
   }
   else
   {
      DPRINT1("Unknown main protection type.\n");
      KEBUGCHECK(0);
   }
   if (!(flProtect & PAGE_SYSTEM))
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

#define ADDR_TO_PAGE_TABLE(v) (((ULONG)(v)) / (4 * 1024 * 1024))

#define ADDR_TO_PDE(v) (PULONG)(PAGEDIRECTORY_MAP + \
                                ((((ULONG)(v)) / (1024 * 1024))&(~0x3)))
#define ADDR_TO_PTE(v) (PULONG)(PAGETABLE_MAP + ((((ULONG)(v) / 1024))&(~0x3)))

#define ADDR_TO_PDE_OFFSET(v) ((((ULONG)(v)) / (4 * 1024 * 1024)))

NTSTATUS Mmi386ReleaseMmInfo(PEPROCESS Process)
{
   PUSHORT LdtDescriptor;
   ULONG LdtBase;

   DPRINT("Mmi386ReleaseMmInfo(Process %x)\n",Process);

   LdtDescriptor = (PUSHORT) &Process->Pcb.LdtDescriptor[0];
   LdtBase = LdtDescriptor[1] |
             ((LdtDescriptor[2] & 0xff) << 16) |
             ((LdtDescriptor[3] & ~0xff) << 16);

   DPRINT("LdtBase: %x\n", LdtBase);

   if (LdtBase)
   {
      ExFreePool((PVOID) LdtBase);
   }

   MmReleasePageMemoryConsumer(MC_NPPOOL, Process->Pcb.DirectoryTableBase.QuadPart >> PAGE_SHIFT);
#if defined(__GNUC__)

   Process->Pcb.DirectoryTableBase.QuadPart = 0LL;
#else

   Process->Pcb.DirectoryTableBase.QuadPart = 0;
#endif

   DPRINT("Finished Mmi386ReleaseMmInfo()\n");
   return(STATUS_SUCCESS);
}

NTSTATUS MmCopyMmInfo(PEPROCESS Src, PEPROCESS Dest)
{
   PHYSICAL_ADDRESS PhysPageDirectory;
   PULONG PageDirectory;
   PKPROCESS KProcess = &Dest->Pcb;

   DPRINT("MmCopyMmInfo(Src %x, Dest %x)\n", Src, Dest);

   PageDirectory = ExAllocatePage();
   if (PageDirectory == NULL)
   {
      return(STATUS_UNSUCCESSFUL);
   }
   PhysPageDirectory = MmGetPhysicalAddress(PageDirectory);
   KProcess->DirectoryTableBase = PhysPageDirectory;

   memset(PageDirectory,0, ADDR_TO_PDE_OFFSET(KERNEL_BASE) * sizeof(ULONG));
   memcpy(PageDirectory + ADDR_TO_PDE_OFFSET(KERNEL_BASE),
          MmGlobalKernelPageDirectory + ADDR_TO_PDE_OFFSET(KERNEL_BASE),
          (1024 - ADDR_TO_PDE_OFFSET(KERNEL_BASE)) * sizeof(ULONG));

   DPRINT("Addr %x\n",PAGETABLE_MAP / (4*1024*1024));
   PageDirectory[PAGETABLE_MAP / (4*1024*1024)] =
      PhysPageDirectory.u.LowPart | PA_PRESENT | PA_READWRITE;

   ExUnmapPage(PageDirectory);

   DPRINT("Finished MmCopyMmInfo()\n");
   return(STATUS_SUCCESS);
}

VOID MmDeletePageTable(PEPROCESS Process, PVOID Address)
{
   PEPROCESS CurrentProcess = PsGetCurrentProcess();

   if (Process != NULL && Process != CurrentProcess)
   {
      KeAttachProcess(Process);
   }
   *(ADDR_TO_PDE(Address)) = 0;
   if (Address >= (PVOID)KERNEL_BASE)
   {
      KEBUGCHECK(0);
      //       MmGlobalKernelPageDirectory[ADDR_TO_PDE_OFFSET(Address)] = 0;
   }
   FLUSH_TLB;
   if (Process != NULL && Process != CurrentProcess)
   {
      KeDetachProcess();
   }
}

VOID MmFreePageTable(PEPROCESS Process, PVOID Address)
{
   PEPROCESS CurrentProcess = PsGetCurrentProcess();
   PULONG PageTable;
   ULONG i;
   ULONG npage;

   if (Process != NULL && Process != CurrentProcess)
   {
      KeAttachProcess(Process);
   }

   PageTable = (PULONG)PAGE_ROUND_DOWN((PVOID)ADDR_TO_PTE(Address));
   for (i = 0; i < 1024; i++)
   {
      if (PageTable[i] != 0)
      {
         DbgPrint("Page table entry not clear at %x/%x (is %x)\n",
                  ((ULONG)Address / 4*1024*1024), i, PageTable[i]);
         KEBUGCHECK(0);
      }
   }
   npage = *(ADDR_TO_PDE(Address));
   *(ADDR_TO_PDE(Address)) = 0;
   FLUSH_TLB;

   if (Address >= (PVOID)KERNEL_BASE)
   {
      //    MmGlobalKernelPageDirectory[ADDR_TO_PDE_OFFSET(Address)] = 0;
      KEBUGCHECK(0);
   }
   else
   {
      MmReleasePageMemoryConsumer(MC_NPPOOL, PTE_TO_PFN(npage));
   }
   if (Process != NULL && Process != CurrentProcess)
   {
      KeDetachProcess();
   }
}

PULONG MmGetPageEntry(PVOID PAddress, BOOL CreatePde)
/*
 * FUNCTION: Get a pointer to the page table entry for a virtual address
 */
{
   PULONG Pde, kePde;
   PFN_TYPE Pfn;
   NTSTATUS Status;

   DPRINT("MmGetPageEntry(Address %x)\n", PAddress);

   Pde = ADDR_TO_PDE(PAGE_ROUND_DOWN(PAddress));
   if (*Pde == 0)
   {
      if (PAddress >= (PVOID)KERNEL_BASE)
      {
         kePde = MmGlobalKernelPageDirectory + ADDR_TO_PDE_OFFSET(PAddress);
         if (*kePde == 0)
	 {
            if (CreatePde == FALSE)
            {
               return NULL;
            }
            Status = MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &Pfn);
            if (!NT_SUCCESS(Status) || Pfn == 0)
            {
               KEBUGCHECK(0);
            }
            if (0 == InterlockedCompareExchange(kePde, PFN_TO_PTE(Pfn) | PA_PRESENT | PA_READWRITE, 0))
	    {
	       Pfn = 0;
	    }
	 }
	 else
	 {
	    Pfn = 0;
	 }
	 *Pde = *kePde;
#if 0
	 /* Non existing mappings are not cached within the tlb. We must not invalidate this entry */
         FLUSH_TLB_ONE(ADDR_TO_PTE(PAddress));
#endif
      }
      else
      {
         if (CreatePde == FALSE)
         {
            return NULL;
         }
         Status = MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &Pfn);
         if (!NT_SUCCESS(Status) || Pfn == 0)
         {
            KEBUGCHECK(0);
         }
         if (0 == InterlockedCompareExchange(Pde, PFN_TO_PTE(Pfn) | PA_PRESENT | PA_READWRITE | PA_USER, 0))
	 {
	    Pfn = 0;
	 }
#if 0
	 /* Non existing mappings are not cached within the tlb. We must not invalidate this entry */
         FLUSH_TLB_ONE(ADDR_TO_PTE(PAddress));
#endif
      }
      if (Pfn != 0)
      {
	 MmReleasePageMemoryConsumer(MC_NPPOOL, Pfn);
      }
   }
   return (PULONG)ADDR_TO_PTE(PAddress);
}

ULONG MmGetPageEntryForProcess(PEPROCESS Process, PVOID Address)
{
   PULONG Pte;
   ULONG oldPte;
   PEPROCESS CurrentProcess = PsGetCurrentProcess();

   if (Process != NULL && Process != CurrentProcess)
   {
      KeAttachProcess(Process);
   }
   Pte = MmGetPageEntry(Address, FALSE);
   oldPte = Pte != NULL ? *Pte : 0;
   if (Process != NULL && Process != CurrentProcess)
   {
      KeDetachProcess();
   }
   return oldPte;
}

PFN_TYPE
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
MmDisableVirtualMapping(PEPROCESS Process, PVOID Address, BOOL* WasDirty, PPFN_TYPE Page)
/*
 * FUNCTION: Delete a virtual mapping 
 */
{
   PULONG Pte;
   ULONG oldPte;
   PEPROCESS CurrentProcess = PsGetCurrentProcess();
   BOOLEAN WasValid;

   /*
    * If we are setting a page in another process we need to be in its
    * context.
    */
   if (Process != NULL && Process != CurrentProcess)
   {
      KeAttachProcess(Process);
   }

   Pte = MmGetPageEntry(Address, FALSE);
   if (Pte == NULL)
   {
      KEBUGCHECK(0);
   }

   /*
    * Atomically set the entry to zero and get the old value.
    */
   oldPte = *Pte;
   *Pte = oldPte & (~PA_PRESENT);
   FLUSH_TLB_ONE(Address);
   WasValid = (PAGE_MASK(oldPte) != 0);
   if (!WasValid)
   {
      KEBUGCHECK(0);
   }

   /*
    * If necessary go back to the original context
    */
   if (Process != NULL && Process != CurrentProcess)
   {
      KeDetachProcess();
   }

   /*
    * Return some information to the caller
    */
   if (WasDirty != NULL)
   {
      *WasDirty = oldPte & PA_DIRTY;
   }
   if (Page != NULL)
   {
      *Page = PTE_TO_PFN(oldPte);
   }
}

VOID
MmRawDeleteVirtualMapping(PVOID Address)
{
   PULONG Pte;

   /*
    * Set the page directory entry, we may have to copy the entry from
    * the global page directory.
    */

   Pte = MmGetPageEntry(Address, FALSE);
   if (Pte)
   {
      /*
       * Set the entry to zero
       */
      *Pte = 0;
      FLUSH_TLB_ONE(Address);
   }
}

VOID
MmDeleteVirtualMapping(PEPROCESS Process, PVOID Address, BOOL FreePage,
                       BOOL* WasDirty, PPFN_TYPE Page)
/*
 * FUNCTION: Delete a virtual mapping 
 */
{
   ULONG oldPte;
   PULONG Pte;
   PEPROCESS CurrentProcess = PsGetCurrentProcess();
   BOOLEAN WasValid;

   /*
    * If we are setting a page in another process we need to be in its
    * context.
    */
   if (Process != NULL && Process != CurrentProcess)
   {
      KeAttachProcess(Process);
   }

   Pte = MmGetPageEntry(Address, FALSE);
   if (Pte == NULL)
   {
      if (Process != NULL && Process != CurrentProcess)
      {
         KeDetachProcess();
      }
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
   oldPte = InterlockedExchange(Pte, 0);
   if (oldPte)
   {
      FLUSH_TLB_ONE(Address);
   }
   WasValid = (PAGE_MASK(oldPte) != 0);
   if (WasValid)
   {
      MmMarkPageUnmapped(PTE_TO_PFN(oldPte));
   }
   if (FreePage && WasValid)
   {
      MmReleasePageMemoryConsumer(MC_NPPOOL, PTE_TO_PFN(oldPte));
   }

   /*
    * Decrement the reference count for this page table.
    */
   if (Process != NULL && WasValid &&
       Process->AddressSpace.PageTableRefCountTable != NULL &&
       Address < (PVOID)KERNEL_BASE)
   {
      PUSHORT Ptrc;

      Ptrc = Process->AddressSpace.PageTableRefCountTable;

      Ptrc[ADDR_TO_PAGE_TABLE(Address)]--;
      if (Ptrc[ADDR_TO_PAGE_TABLE(Address)] == 0)
      {
         MmFreePageTable(Process, Address);
      }
   }

   /*
    * If necessary go back to the original context
    */
   if (Process != NULL && Process != CurrentProcess)
   {
      KeDetachProcess();
   }

   /*
    * Return some information to the caller
    */
   if (WasDirty != NULL)
   {
      *WasDirty = oldPte & PA_DIRTY ? TRUE : FALSE;
   }
   if (Page != NULL)
   {
      *Page = PTE_TO_PFN(oldPte);
   }
}

VOID
MmDeletePageFileMapping(PEPROCESS Process, PVOID Address,
                        SWAPENTRY* SwapEntry)
/*
 * FUNCTION: Delete a virtual mapping 
 */
{
   ULONG oldPte;
   PULONG Pte;
   PEPROCESS CurrentProcess = PsGetCurrentProcess();
   BOOLEAN WasValid = FALSE;

   /*
    * If we are setting a page in another process we need to be in its
    * context.
    */
   if (Process != NULL && Process != CurrentProcess)
   {
      KeAttachProcess(Process);
   }

   Pte = MmGetPageEntry(Address, FALSE);
 
   if (Pte == NULL)
   {
      if (Process != NULL && Process != CurrentProcess)
      {
         KeDetachProcess();
      }
      *SwapEntry = 0;
      return;
   }

   /*
    * Atomically set the entry to zero and get the old value.
    */
   oldPte = InterlockedExchange(Pte, 0);
   FLUSH_TLB_ONE(Address);

   WasValid = PAGE_MASK(oldPte) == 0 ? FALSE : TRUE;

   /*
    * Decrement the reference count for this page table.
    */
   if (Process != NULL && WasValid &&
         Process->AddressSpace.PageTableRefCountTable != NULL &&
         Address < (PVOID)KERNEL_BASE)
   {
      PUSHORT Ptrc;

      Ptrc = Process->AddressSpace.PageTableRefCountTable;

      Ptrc[ADDR_TO_PAGE_TABLE(Address)]--;
      if (Ptrc[ADDR_TO_PAGE_TABLE(Address)] == 0)
      {
         MmFreePageTable(Process, Address);
      }
   }

   /*
    * If necessary go back to the original context
    */
   if (Process != NULL && Process != CurrentProcess)
   {
      KeDetachProcess();
   }

   /*
    * Return some information to the caller
    */
   *SwapEntry = oldPte >> 1;
}

BOOLEAN
Mmi386MakeKernelPageTableGlobal(PVOID PAddress)
{
   PULONG Pte, Pde;

   Pde = ADDR_TO_PDE(PAddress);
   if (*Pde == 0)
   {
      Pte = MmGetPageEntry(PAddress, FALSE);
#if 0
      /* Non existing mappings are not cached within the tlb. We must not invalidate this entry */
      FLASH_TLB_ONE(PAddress);
#endif
      if (Pte != NULL)
      {
         return TRUE;
      }
   }
   return(FALSE);
}

BOOLEAN MmIsDirtyPage(PEPROCESS Process, PVOID Address)
{
   return MmGetPageEntryForProcess(Process, Address) & PA_DIRTY ? TRUE : FALSE;
}

BOOLEAN
MmIsAccessedAndResetAccessPage(PEPROCESS Process, PVOID Address)
{
   ULONG oldPte;
   PULONG Pte;
   PEPROCESS CurrentProcess;

   if (Process)
   {
      CurrentProcess = PsGetCurrentProcess();
      if (Process != CurrentProcess)
      {
         KeAttachProcess(Process);
      }
   }
   else
   {
      if (((ULONG)Address & ~0xFFF) < KERNEL_BASE)
      {
         DPRINT1("MmIsAccessedAndResetAccessPage is called for user space without a process.\n");
         KEBUGCHECK(0);
      }
      CurrentProcess = NULL;
   }

   Pte = MmGetPageEntry(Address, FALSE);
   if (Pte == NULL)
   {
      KEBUGCHECK(0);
   }
   oldPte = *Pte;
   if (oldPte & PA_ACCESSED)
   {
      *Pte = *Pte & (~PA_ACCESSED);
      FLUSH_TLB_ONE(Address);
   }
   if (Process != CurrentProcess)
   {
      KeDetachProcess();
   }

   return oldPte & PA_ACCESSED ? TRUE : FALSE;
}

VOID MmSetCleanPage(PEPROCESS Process, PVOID Address)
{
   PULONG Pte;
   PEPROCESS CurrentProcess;

   if (Process)
   {
      CurrentProcess = PsGetCurrentProcess();
      if (Process != CurrentProcess)
      {
         KeAttachProcess(Process);
      }
   }
   else
   {
      if (Address < (PVOID)KERNEL_BASE)
      {
         DPRINT1("MmSetCleanPage is called for user space without a process.\n");
         KEBUGCHECK(0);
      }
      CurrentProcess = NULL;
   }
   Pte = MmGetPageEntry(Address, FALSE);
   if (Pte == NULL)
   {
      KEBUGCHECK(0);
   }
   if (*Pte & PA_DIRTY)
   {
      *Pte = *Pte & (~PA_DIRTY);
      FLUSH_TLB_ONE(Address);
   }
   if (Process != CurrentProcess)
   {
      KeDetachProcess();
   }
}

VOID MmSetDirtyPage(PEPROCESS Process, PVOID Address)
{
   PULONG Pte;
   PEPROCESS CurrentProcess = NULL;

   if (Process)
   {
      CurrentProcess = PsGetCurrentProcess();
      if (Process != CurrentProcess)
      {
         KeAttachProcess(Process);
      }
   }
   else
   {
      if (Address < (PVOID)KERNEL_BASE)
      {
         DPRINT1("MmSetDirtyPage is called for user space without a process.\n");
         KEBUGCHECK(0);
      }
      CurrentProcess = NULL;
   }
   Pte = MmGetPageEntry(Address, FALSE);
   if (Pte == NULL)
   {
      KEBUGCHECK(0);
   }
   if (!(*Pte & PA_DIRTY))
   {
      *Pte = *Pte | PA_DIRTY;
      FLUSH_TLB_ONE(Address);
   }
   if (Process != CurrentProcess)
   {
      KeDetachProcess();
   }
}

VOID MmEnableVirtualMapping(PEPROCESS Process, PVOID Address)
{
   PULONG Pte;
   PEPROCESS CurrentProcess = PsGetCurrentProcess();

   if (Process != CurrentProcess)
   {
      KeAttachProcess(Process);
   }
   Pte = MmGetPageEntry(Address, FALSE);
   if (Pte == NULL)
   {
      KEBUGCHECK(0);
   }
   if (!(*Pte & PA_PRESENT))
   {
      *Pte = *Pte | PA_PRESENT;
      FLUSH_TLB_ONE(Address);
   }
   if (Process != CurrentProcess)
   {
      KeDetachProcess();
   }
}

BOOLEAN MmIsPagePresent(PEPROCESS Process, PVOID Address)
{
   return MmGetPageEntryForProcess(Process, Address) & PA_PRESENT ? TRUE : FALSE;
}

BOOLEAN MmIsPageSwapEntry(PEPROCESS Process, PVOID Address)
{
   ULONG Entry;
   Entry = MmGetPageEntryForProcess(Process, Address);
   return !(Entry & PA_PRESENT) && Entry != 0 ? TRUE : FALSE;
}

NTSTATUS
MmCreateVirtualMappingForKernel(PVOID Address,
                                ULONG flProtect,
                                PPFN_TYPE Pages,
				ULONG PageCount)
{
   ULONG Attributes, oldPte;
   PULONG Pte;
   ULONG i;
   PVOID Addr;

   if (Address < (PVOID)KERNEL_BASE)
   {
      DPRINT1("MmCreateVirtualMappingForKernel is called for user space\n");
      KEBUGCHECK(0);
   }

   Attributes = ProtectToPTE(flProtect);
   Addr = Address;
   for (i = 0; i < PageCount; i++, Addr += PAGE_SIZE)
   {
      if (!(Attributes & PA_PRESENT) && Pages[i] != 0)
      {
         DPRINT1("Setting physical address but not allowing access at address "
                 "0x%.8X with attributes %x/%x.\n",
                 Addr, Attributes, flProtect);
         KEBUGCHECK(0);
      }

      Pte = MmGetPageEntry(Addr, TRUE);
      if (Pte == NULL)
      {
         KEBUGCHECK(0);
      }
      oldPte = *Pte;
      if (PAGE_MASK((oldPte)) != 0)
      {
         KEBUGCHECK(0);
      }
      *Pte = PFN_TO_PTE(Pages[i]) | Attributes;
      if (oldPte != 0)
      {
         FLUSH_TLB_ONE(Addr);
      }
   }

   return(STATUS_SUCCESS);
}

NTSTATUS
MmCreatePageFileMapping(PEPROCESS Process,
                        PVOID Address,
                        SWAPENTRY SwapEntry)
{
   PEPROCESS CurrentProcess;
   PULONG Pte;
   ULONG oldPte;

   if (Process != NULL)
   {
      CurrentProcess = PsGetCurrentProcess();
   }
   else
   {
      CurrentProcess = NULL;
   }

   if (Process == NULL && Address < (PVOID)KERNEL_BASE)
   {
      DPRINT1("No process\n");
      KEBUGCHECK(0);
   }
   if (Process != NULL && Address >= (PVOID)KERNEL_BASE)
   {
      DPRINT1("Setting kernel address with process context\n");
      KEBUGCHECK(0);
   }
   if (SwapEntry & (1 << 31))
   {
      KEBUGCHECK(0);
   }

   if (Process != NULL && Process != CurrentProcess)
   {
      KeAttachProcess(Process);
   }
   Pte = MmGetPageEntry(Address, TRUE);
   if (Pte == NULL)
   {
      KEBUGCHECK(0);
   }
   oldPte = *Pte;
   if (PAGE_MASK((oldPte)) != 0)
   {
      MmMarkPageUnmapped(PTE_TO_PFN((oldPte)));
   }
   *Pte = SwapEntry << 1;
   if (Process != NULL &&
         Process->AddressSpace.PageTableRefCountTable != NULL &&
         Address < (PVOID)KERNEL_BASE)
   {
      PUSHORT Ptrc;

      Ptrc = Process->AddressSpace.PageTableRefCountTable;

      Ptrc[ADDR_TO_PAGE_TABLE(Address)]++;
   }
   if (oldPte != 0)
   {
      FLUSH_TLB_ONE(Address);
   }
   if (Process != NULL && Process != CurrentProcess)
   {
      KeDetachProcess();
   }
   return(STATUS_SUCCESS);
}


NTSTATUS
MmCreateVirtualMappingUnsafe(PEPROCESS Process,
                             PVOID Address,
                             ULONG flProtect,
                             PPFN_TYPE Pages,
                             ULONG PageCount)
{
   PEPROCESS CurrentProcess;
   ULONG Attributes;
   PVOID Addr = Address;
   ULONG i;
   PULONG Pte;
   ULONG oldPte;

   if (Process != NULL)
   {
      CurrentProcess = PsGetCurrentProcess();
   }
   else
   {
      CurrentProcess = NULL;
   }

   if (Process == NULL && Address < (PVOID)KERNEL_BASE)
   {
      DPRINT1("No process\n");
      KEBUGCHECK(0);
   }
   if (Process != NULL && Address >= (PVOID)KERNEL_BASE)
   {
      DPRINT1("Setting kernel address with process context\n");
      KEBUGCHECK(0);
   }

   Attributes = ProtectToPTE(flProtect);

   if (Process != NULL && Process != CurrentProcess)
   {
      CHECKPOINT1;
      KeAttachProcess(Process);
   }

   for (i = 0; i < PageCount; i++, Addr += PAGE_SIZE)
   {
      Pte = MmGetPageEntry(Addr, TRUE);
      if (Pte == NULL)
      {
         KEBUGCHECK(0);
      }
      if (!(Attributes & PA_PRESENT) && Pages[i] != 0)
      {
         DPRINT1("Setting physical address but not allowing access at address "
                 "0x%.8X with attributes %x/%x.\n",
                 Addr, Attributes, flProtect);
         KEBUGCHECK(0);
      }
      oldPte = *Pte;
      MmMarkPageMapped(Pages[i]);
      if (PAGE_MASK((oldPte)) != 0 && !((oldPte) & PA_PRESENT))
      {
         KEBUGCHECK(0);
      }
      if (PAGE_MASK((oldPte)) != 0)
      {
         MmMarkPageUnmapped(PTE_TO_PFN((oldPte)));
      }
      *Pte = PFN_TO_PTE(Pages[i]) | Attributes;
      if (Address < (PVOID)KERNEL_BASE &&
	  Process->AddressSpace.PageTableRefCountTable != NULL &&
          Attributes & PA_PRESENT)
      {
         PUSHORT Ptrc;

         Ptrc = Process->AddressSpace.PageTableRefCountTable;

         Ptrc[ADDR_TO_PAGE_TABLE(Addr)]++;
      }
      if (oldPte != 0)
      {
	 FLUSH_TLB_ONE(Addr);
      }
   }
   if (Process != NULL && Process != CurrentProcess)
   {
      KeDetachProcess();
   }
   return(STATUS_SUCCESS);
}

NTSTATUS
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
         DPRINT1("Page at address %x not usable\n", Pages[i] << PAGE_SHIFT);
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
MmGetPageProtect(PEPROCESS Process, PVOID Address)
{
   ULONG Entry;
   ULONG Protect;

   Entry = MmGetPageEntryForProcess(Process, Address);

   if (!(Entry & PA_PRESENT))
   {
      Protect = PAGE_NOACCESS;
   }
   else if (Entry & PA_READWRITE)
   {
      Protect = PAGE_READWRITE;
   }
   else
   {
      Protect = PAGE_EXECUTE_READ;
   }
   return(Protect);
}

VOID
MmSetPageProtect(PEPROCESS Process, PVOID Address, ULONG flProtect)
{
   ULONG Attributes = 0;
   PULONG Pte;
   PEPROCESS CurrentProcess = PsGetCurrentProcess();

   DPRINT("MmSetPageProtect(Process %x  Address %x  flProtect %x)\n",
          Process, Address, flProtect);

   Attributes = ProtectToPTE(flProtect);
   if (Process != NULL && Process != CurrentProcess)
   {
      KeAttachProcess(Process);
   }
   Pte = MmGetPageEntry(Address, TRUE);
   if (Pte == NULL)
   {
      KEBUGCHECK(0);
   }
   *Pte = PAGE_MASK(*Pte) | Attributes;
   FLUSH_TLB_ONE(Address);
   if (Process != NULL && Process != CurrentProcess)
   {
      KeDetachProcess();
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
   PULONG Pte;

   DPRINT("MmGetPhysicalAddress(vaddr %x)\n", vaddr);

   Pte = MmGetPageEntry(vaddr, FALSE);
   if (Pte != NULL && *Pte & PA_PRESENT)
   {
      p.QuadPart = PAGE_MASK(*Pte);
      p.u.LowPart |= (ULONG_PTR)vaddr & (PAGE_SIZE - 1);
   }
   else
   {
      p.QuadPart = 0;
   }

   return p;
}


VOID
MmUpdateStackPageDir(PULONG LocalPageDir, PKTHREAD PThread)
{
   unsigned EntryBase = ADDR_TO_PDE_OFFSET(PThread->StackLimit);
   unsigned EntryTop  = ADDR_TO_PDE_OFFSET((char*)PThread->InitialStack - PAGE_SIZE);

   if (0 == LocalPageDir[EntryBase])
   {
      LocalPageDir[EntryBase] = MmGlobalKernelPageDirectory[EntryBase];
   }
   if (EntryBase != EntryTop && 0 == LocalPageDir[EntryTop])
   {
      LocalPageDir[EntryTop] = MmGlobalKernelPageDirectory[EntryTop];
   }
}

VOID INIT_FUNCTION
MmInitGlobalKernelPageDirectory(VOID)
{
   ULONG i;
   PULONG CurrentPageDirectory = (PULONG)PAGEDIRECTORY_MAP;

   for (i = ADDR_TO_PDE_OFFSET(KERNEL_BASE); i < 1024; i++)
   {
      if (i != ADDR_TO_PDE_OFFSET(PAGETABLE_MAP) &&
            0 == MmGlobalKernelPageDirectory[i] && 0 != CurrentPageDirectory[i])
      {
         MmGlobalKernelPageDirectory[i] = CurrentPageDirectory[i];
      }
   }
}

/* EOF */
