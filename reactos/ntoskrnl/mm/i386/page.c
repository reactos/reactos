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
/* $Id: page.c,v 1.53 2003/06/19 19:01:01 gvg Exp $
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

#define PTE_TO_PAGE(X) ((LARGE_INTEGER)(LONGLONG)(PAGE_MASK(X)))

/* FUNCTIONS ***************************************************************/

PULONG 
MmGetPageDirectory(VOID)
{
   unsigned int page_dir=0;
   __asm__("movl %%cr3,%0\n\t"
                : "=r" (page_dir));
   return((PULONG)page_dir);
}

static ULONG 
ProtectToPTE(ULONG flProtect)
{
  ULONG Attributes = 0;
  
  if (flProtect & PAGE_NOACCESS || flProtect & PAGE_GUARD)
    {
      Attributes = 0;
    }
  else if (flProtect & PAGE_READWRITE || flProtect & PAGE_EXECUTE_READWRITE)
    {
      Attributes = PA_PRESENT | PA_READWRITE;
    }
  else if (flProtect & PAGE_READONLY || flProtect & PAGE_EXECUTE ||
	   flProtect & PAGE_EXECUTE_READ)
    {
      Attributes = PA_PRESENT; 
    }
  else
    {
      DPRINT1("Unknown main protection type.\n");
      KeBugCheck(0);
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
#define ADDR_TO_PTE(v) (PULONG)(PAGETABLE_MAP + ((((ULONG)v / 1024))&(~0x3)))

#define ADDR_TO_PDE_OFFSET(v) ((((ULONG)(v)) / (4 * 1024 * 1024)))

NTSTATUS Mmi386ReleaseMmInfo(PEPROCESS Process)
{
   DPRINT("Mmi386ReleaseMmInfo(Process %x)\n",Process);
   
   MmReleasePageMemoryConsumer(MC_NPPOOL, Process->Pcb.DirectoryTableBase);
   Process->Pcb.DirectoryTableBase.QuadPart = 0LL;
   
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
         KeBugCheck(0);
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
	 KeBugCheck(0);
      }
   }
   npage = *(ADDR_TO_PDE(Address));
   *(ADDR_TO_PDE(Address)) = 0;
   FLUSH_TLB;

   if (Address >= (PVOID)KERNEL_BASE)
   {
//    MmGlobalKernelPageDirectory[ADDR_TO_PDE_OFFSET(Address)] = 0;
      KeBugCheck(0);
   }
   else
   {
      MmReleasePageMemoryConsumer(MC_NPPOOL, PTE_TO_PAGE(npage));
   }
   if (Process != NULL && Process != CurrentProcess)
   {
      KeDetachProcess();
   }
}

NTSTATUS MmGetPageEntry2(PVOID PAddress, PULONG* Pte, BOOLEAN MayWait)
/*
 * FUNCTION: Get a pointer to the page table entry for a virtual address
 */
{
   PULONG Pde, kePde;
   PHYSICAL_ADDRESS npage;
   BOOLEAN Free = FALSE;
   NTSTATUS Status;
   KIRQL oldIrql;
   
   DPRINT("MmGetPageEntry(Address %x)\n", PAddress);
   
   Pde = ADDR_TO_PDE(PAddress);
   if (*Pde == 0)     
   {
      if (PAddress >= (PVOID)KERNEL_BASE)
      {
	 kePde = MmGlobalKernelPageDirectory + ADDR_TO_PDE_OFFSET(PAddress);
         oldIrql = KeRaiseIrqlToSynchLevel();
	 if (*kePde != 0)
	 {
	    *Pde = *kePde;
	    FLUSH_TLB;
	 }
         else
	 {
	    KeLowerIrql(oldIrql);
	    Status = MmRequestPageMemoryConsumer(MC_NPPOOL, MayWait, &npage);
	    if (!NT_SUCCESS(Status))
	    {
	       KeBugCheck(0);
	    }
	    oldIrql = KeRaiseIrqlToSynchLevel();
	    /* An other thread can set this pde entry, we must check again */
	    if (*kePde == 0)
	    {
	       *kePde = npage.u.LowPart | PA_PRESENT | PA_READWRITE;
	    }
	    else
	    {
	       Free = TRUE;
	    }
	    *Pde = *kePde;
	    FLUSH_TLB;
	 }
	 KeLowerIrql(oldIrql);
      }
      else
      {
	 Status = MmRequestPageMemoryConsumer(MC_NPPOOL, MayWait, &npage);
         if (!NT_SUCCESS(Status))
	 {
	    return(Status);
	 }
	 *Pde = npage.u.LowPart | PA_PRESENT | PA_READWRITE | PA_USER;
	 FLUSH_TLB;
      }
   }
   *Pte = (PULONG)ADDR_TO_PTE(PAddress);
   if (Free)
   {
      MmReleasePageMemoryConsumer(MC_NPPOOL, npage);
   }
   return STATUS_SUCCESS;
}

ULONG MmGetPageEntryForProcess(PEPROCESS Process, PVOID Address)
{
   ULONG Entry;
   PEPROCESS CurrentProcess = PsGetCurrentProcess();
   
   if (Process != NULL && Process != CurrentProcess)
     {
	KeAttachProcess(Process);
     }
   Entry = *MmGetPageEntry(Address);
   if (Process != NULL && Process != CurrentProcess)
     {
	KeDetachProcess();
     }
   return(Entry);
}

ULONG MmGetPageEntry1(PVOID PAddress)
/*
 * FUNCTION: Get a pointer to the page table entry for a virtual address
 */
{
   PULONG Pde, kePde;
   ULONG Entry = 0;
   
   DPRINT("MmGetPageEntry(Address %x)\n", PAddress);
   
   Pde = ADDR_TO_PDE(PAddress);
   if (*Pde == 0)
   {
       if (PAddress >= (PVOID)KERNEL_BASE)
       {
          kePde = MmGlobalKernelPageDirectory + ADDR_TO_PDE_OFFSET(PAddress);
	  if (*kePde != 0)
	  {
	     *Pde = *kePde;
	     FLUSH_TLB;
	     Entry = *(PULONG)ADDR_TO_PTE(PAddress);
	  }
       }
   }
   else
   {
      Entry = *(PULONG)ADDR_TO_PTE(PAddress);
   }
   return Entry;
}

ULONG MmGetPageEntryForProcess1(PEPROCESS Process, PVOID Address)
{
   ULONG Entry;
   PEPROCESS CurrentProcess = PsGetCurrentProcess();
   
   if (Process != NULL && Process != CurrentProcess)
     {
	KeAttachProcess(Process);
     }
   Entry = MmGetPageEntry1(Address);
   if (Process != NULL && Process != CurrentProcess)
     {
	KeDetachProcess();
     }
   return(Entry);
}


PHYSICAL_ADDRESS
MmGetPhysicalAddressForProcess(PEPROCESS Process,
			       PVOID Address)
{
   ULONG PageEntry;
   
   PageEntry = MmGetPageEntryForProcess(Process, Address);
   
   if (!(PageEntry & PA_PRESENT))
     {
	return((LARGE_INTEGER)0LL);
     }
   return(PTE_TO_PAGE(PageEntry));
}

VOID
MmDisableVirtualMapping(PEPROCESS Process, PVOID Address, BOOL* WasDirty, PHYSICAL_ADDRESS* PhysicalAddr)
/*
 * FUNCTION: Delete a virtual mapping 
 */
{
   ULONG Pte;
   PULONG Pde;
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

   /*
    * Set the page directory entry, we may have to copy the entry from
    * the global page directory.
    */
   Pde = ADDR_TO_PDE(Address);
   if ((*Pde) == 0 && 
       MmGlobalKernelPageDirectory[ADDR_TO_PDE_OFFSET(Address)] != 0)
     {
       (*Pde) = MmGlobalKernelPageDirectory[ADDR_TO_PDE_OFFSET(Address)];
       FLUSH_TLB;
     }
   if ((*Pde) == 0)
     {
       KeBugCheck(0);
     }

   /*
    * Atomically set the entry to zero and get the old value.
    */
   Pte = *ADDR_TO_PTE(Address);
   *ADDR_TO_PTE(Address) = Pte & (~PA_PRESENT);
   FLUSH_TLB;
   WasValid = (PAGE_MASK(Pte) != 0);
   if (!WasValid)
     {
       KeBugCheck(0);
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
       *WasDirty = Pte & PA_DIRTY;
     }
   if (PhysicalAddr != NULL)
     {
       PhysicalAddr->u.HighPart = 0;
       PhysicalAddr->u.LowPart = PAGE_MASK(Pte);
     }
}

VOID
MmDeleteVirtualMapping(PEPROCESS Process, PVOID Address, BOOL FreePage,
		       BOOL* WasDirty, PHYSICAL_ADDRESS* PhysicalAddr)
/*
 * FUNCTION: Delete a virtual mapping 
 */
{
   ULONG Pte;
   PULONG Pde, kePde;
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

   /*
    * Set the page directory entry, we may have to copy the entry from
    * the global page directory.
    */
   Pde = ADDR_TO_PDE(Address);
   if (*Pde == 0 && Address >= (PVOID)KERNEL_BASE)
   {
      kePde = MmGlobalKernelPageDirectory + ADDR_TO_PDE_OFFSET(Address);
      if (*kePde != 0)
      {
         *Pde = *kePde;
	 FLUSH_TLB;
      }
   }
   
   if (*Pde == 0)
     {
	if (Process != NULL && Process != CurrentProcess)
	  {
	     KeDetachProcess();
	  }	
	if (WasDirty != NULL)
	  {
	    *WasDirty = FALSE;
	  }
	if (PhysicalAddr != NULL)
	  {
	    *PhysicalAddr = (LARGE_INTEGER)0LL;
	  }
	return;
     }

   /*
    * Atomically set the entry to zero and get the old value.
    */
   Pte = (ULONG)InterlockedExchange((PLONG)ADDR_TO_PTE(Address), 0);
   FLUSH_TLB;
   WasValid = (PAGE_MASK(Pte) != 0);
   if (WasValid)
     {
       MmMarkPageUnmapped(PTE_TO_PAGE(Pte));
     }
   if (FreePage && WasValid)
     {
       MmReleasePageMemoryConsumer(MC_NPPOOL, PTE_TO_PAGE(Pte));
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
       if (Pte & PA_DIRTY)
	 {
	   *WasDirty = TRUE;
	 }
       else
	 {
	   *WasDirty = FALSE;
	 }
     }
   if (PhysicalAddr != NULL)
     {
       *PhysicalAddr = PTE_TO_PAGE(Pte);
     }
}

VOID
MmDeletePageFileMapping(PEPROCESS Process, PVOID Address, 
			SWAPENTRY* SwapEntry) 
/*
 * FUNCTION: Delete a virtual mapping 
 */
{
   ULONG Pte;
   PULONG Pde, kePde;
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

   /*
    * Set the page directory entry, we may have to copy the entry from
    * the global page directory.
    */
   Pde = ADDR_TO_PDE(Address);
   if (*Pde == 0)
   {
      if (Address >= (PVOID)KERNEL_BASE)
      {
         kePde = MmGlobalKernelPageDirectory + ADDR_TO_PDE_OFFSET(Address);
	 if (*kePde != 0)
	 {
	    *Pde = *kePde;
	    FLUSH_TLB;
	 }
      }
   }
   if (*Pde == 0)
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
   Pte = (ULONG)InterlockedExchange((PLONG)ADDR_TO_PTE(Address), 0);
   FLUSH_TLB;

   WasValid = PAGE_MASK(Pte) == 0 ? FALSE : TRUE;

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
   *SwapEntry = Pte >> 1;
}

BOOLEAN 
Mmi386MakeKernelPageTableGlobal(PVOID PAddress)
{
   PULONG kePde, Pde;
   
   Pde = ADDR_TO_PDE(PAddress);
   if (*Pde == 0)
   {
      kePde = MmGlobalKernelPageDirectory + ADDR_TO_PDE_OFFSET(PAddress);
      if (*kePde != 0)
      {
         *Pde = *kePde;
	 FLUSH_TLB;
         return(TRUE);
      }
   }
   return(FALSE);
}

BOOLEAN MmIsPageTablePresent(PVOID PAddress)
{
   PULONG Pde, kePde;
   
   Pde = ADDR_TO_PDE(PAddress);
   if (*Pde == 0)
   {
      kePde = MmGlobalKernelPageDirectory + ADDR_TO_PDE_OFFSET(PAddress);
      if (*kePde != 0)
      {
         *Pde = *kePde;
	 FLUSH_TLB;
	 return TRUE;
      }
   }
   return FALSE;
}

NTSTATUS MmCreatePageTable(PVOID PAddress)
{
   PULONG Pde, kePde;
   PHYSICAL_ADDRESS npage;
   NTSTATUS Status;
   
   DPRINT("MmGetPageEntry(Address %x)\n", PAddress);
   
   Pde = ADDR_TO_PDE(PAddress);
   DPRINT("page_dir %x *page_dir %x\n", Pde, *Pde);
   if (*Pde == 0 && PAddress >= (PVOID)KERNEL_BASE)
   {
      kePde = MmGlobalKernelPageDirectory + ADDR_TO_PDE_OFFSET(PAddress);
      if (*kePde != 0)
      {
	 *Pde = *kePde;
	 FLUSH_TLB;
	 return STATUS_SUCCESS;
      }
      /* Should we create a kernel page table? */
      DPRINT1("!!!!!!!!!!!!!!!!!!\n");
      return STATUS_UNSUCCESSFUL;
   }

   if (*Pde == 0)
   {
      Status = MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &npage);
      if (!NT_SUCCESS(Status))
      {
         return(Status);
      }
      MiZeroPage(npage);
      *Pde = npage.u.LowPart | PA_PRESENT | PA_READWRITE | PA_USER;
      FLUSH_TLB;
   }
   return(STATUS_SUCCESS);
}

PULONG MmGetPageEntry(PVOID PAddress)
/*
 * FUNCTION: Get a pointer to the page table entry for a virtual address
 */
{
   PULONG Pde, kePde;
   PHYSICAL_ADDRESS npage;
   KIRQL oldIrql;
   BOOLEAN Free = FALSE;
   NTSTATUS Status;
   
   DPRINT("MmGetPageEntry(Address %x)\n", PAddress);
   
   Pde = ADDR_TO_PDE(PAddress);
   DPRINT("page_dir %x *page_dir %x\n",Pde,*Pde);
   if (*Pde == 0)
   {
      if (PAddress >= (PVOID)KERNEL_BASE)
      {
	 oldIrql = KeRaiseIrqlToSynchLevel();
	 kePde = MmGlobalKernelPageDirectory + ADDR_TO_PDE_OFFSET(PAddress);
	 if (*kePde != 0)
	 {
	    *Pde = *kePde;
	    FLUSH_TLB;
	 }
	 else
	 {
	    KeLowerIrql(oldIrql);
	    Status = MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &npage);
            if (!NT_SUCCESS(Status))
	    {
	       KeBugCheck(0);
	    }
	    MiZeroPage(npage);
	    oldIrql = KeRaiseIrqlToSynchLevel();
	    if (*kePde != 0)
	    {
	       *Pde = *kePde;
	       FLUSH_TLB;
	       Free = TRUE;
	    }
	    else
	    {
               *Pde = *kePde = npage.u.LowPart | PA_PRESENT | PA_READWRITE;
	       FLUSH_TLB;
	    }
	 }
	 KeLowerIrql(oldIrql);
      }
      else
      {
	 Status = MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &npage);
         if (!NT_SUCCESS(Status))
	 {
	    KeBugCheck(0);
	 }
	 MiZeroPage(npage);
	 *Pde = npage.u.LowPart | PA_PRESENT | PA_READWRITE | PA_USER;
	 FLUSH_TLB;
      }
      if (Free)
      {
	 MmReleasePageMemoryConsumer(MC_NPPOOL, npage);
      }
   }

   return ADDR_TO_PTE(PAddress);
}

BOOLEAN MmIsDirtyPage(PEPROCESS Process, PVOID Address)
{
   return((MmGetPageEntryForProcess(Process, Address)) & PA_DIRTY);
}

BOOLEAN 
MmIsAccessedAndResetAccessPage(PEPROCESS Process, PVOID Address)
{
   PULONG PageEntry;
   PEPROCESS CurrentProcess;
   BOOLEAN Accessed;

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
           KeBugCheck(0);
	 }
       CurrentProcess = NULL;
     }

   PageEntry = MmGetPageEntry(Address);
   Accessed = (*PageEntry) & PA_ACCESSED;
   if (Accessed)
     {
       (*PageEntry) = (*PageEntry) & (~PA_ACCESSED);
       FLUSH_TLB;
     }
   if (Process != CurrentProcess)
     {
	KeDetachProcess();
     }

   return(Accessed);
}

VOID MmSetCleanPage(PEPROCESS Process, PVOID Address)
{
   PULONG PageEntry;
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
           DPRINT1("MmSetCleanPage is called for user space without a process.\n");
           KeBugCheck(0);
	 }
       CurrentProcess = NULL;
     }
   PageEntry = MmGetPageEntry(Address);
   (*PageEntry) = (*PageEntry) & (~PA_DIRTY);
   FLUSH_TLB;
   if (Process != CurrentProcess)
     {
	KeDetachProcess();
     }
}

VOID MmSetDirtyPage(PEPROCESS Process, PVOID Address)
{
   PULONG PageEntry;
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
       if (((ULONG)Address & ~0xFFF) < KERNEL_BASE)
         {
           DPRINT1("MmSetDirtyPage is called for user space without a process.\n");
           KeBugCheck(0);
	 }
       CurrentProcess = NULL;
     }
   PageEntry = MmGetPageEntry(Address);
   (*PageEntry) = (*PageEntry) | PA_DIRTY;
   FLUSH_TLB;
   if (Process != CurrentProcess)
     {
	KeDetachProcess();
     }
}

VOID MmEnableVirtualMapping(PEPROCESS Process, PVOID Address)
{
   PULONG PageEntry;
   PEPROCESS CurrentProcess = PsGetCurrentProcess();
   
   if (Process != CurrentProcess)
     {
	KeAttachProcess(Process);
     }
   PageEntry = MmGetPageEntry(Address);
   (*PageEntry) = (*PageEntry) | PA_PRESENT;
   FLUSH_TLB;
   if (Process != CurrentProcess)
     {
	KeDetachProcess();
     }
}

BOOLEAN MmIsPagePresent(PEPROCESS Process, PVOID Address)
{
   return((MmGetPageEntryForProcess1(Process, Address)) & PA_PRESENT);
}

BOOLEAN MmIsPageSwapEntry(PEPROCESS Process, PVOID Address)
{
  ULONG Pte;
  Pte = MmGetPageEntryForProcess1(Process, Address);
  return((!(Pte & PA_PRESENT)) && Pte != 0);
}

NTSTATUS 
MmCreateVirtualMappingForKernel(PVOID Address, 
				ULONG flProtect,
				PHYSICAL_ADDRESS PhysicalAddress)
{
  PEPROCESS CurrentProcess;
  ULONG Attributes;
  PULONG Pte;
  NTSTATUS Status;
  PEPROCESS Process = NULL;

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
       KeBugCheck(0);
     }
   if (Process != NULL && Address >= (PVOID)KERNEL_BASE)
     {
       DPRINT1("Setting kernel address with process context\n");
       KeBugCheck(0);
     }
   Attributes = ProtectToPTE(flProtect);
   
   if (Process != NULL && Process != CurrentProcess)
     {
	KeAttachProcess(Process);
     }
   
   Status = MmGetPageEntry2(Address, &Pte, FALSE);
   if (!NT_SUCCESS(Status))
     {
	if (Process != NULL && Process != CurrentProcess)
	  {
	     KeDetachProcess();
	  }
	return(Status);
     }
   if (PAGE_MASK((*Pte)) != 0 && !((*Pte) & PA_PRESENT))
     {
       KeBugCheck(0);
     }
   if (PAGE_MASK((*Pte)) != 0)
     {
       MmMarkPageUnmapped(PTE_TO_PAGE((*Pte)));
     }
   *Pte = PhysicalAddress.QuadPart | Attributes;
   if (Process != NULL && 
       Process->AddressSpace.PageTableRefCountTable != NULL &&
       Address < (PVOID)KERNEL_BASE &&
       Attributes & PA_PRESENT)
     {
	PUSHORT Ptrc;
	
	Ptrc = Process->AddressSpace.PageTableRefCountTable;
	
	Ptrc[ADDR_TO_PAGE_TABLE(Address)]++;
     }
   FLUSH_TLB;
   if (Process != NULL && Process != CurrentProcess)
     {
	KeDetachProcess();
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
  NTSTATUS Status;
  
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
      KeBugCheck(0);
     }
  if (Process != NULL && Address >= (PVOID)KERNEL_BASE)
    {
      DPRINT1("Setting kernel address with process context\n");
      KeBugCheck(0);
    }
  if (SwapEntry & (1 << 31))
    {
      KeBugCheck(0);
    }

  if (Process != NULL && Process != CurrentProcess)
    {
	KeAttachProcess(Process);
    }
  
  Status = MmGetPageEntry2(Address, &Pte, FALSE);
  if (!NT_SUCCESS(Status))
    {
	if (Process != NULL && Process != CurrentProcess)
	  {
	    KeDetachProcess();
	  }
	return(Status);
    }
  if (PAGE_MASK((*Pte)) != 0)
    {
      MmMarkPageUnmapped(PTE_TO_PAGE((*Pte)));
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
  FLUSH_TLB;
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
			     PHYSICAL_ADDRESS PhysicalAddress,
			     BOOLEAN MayWait)
{
   PEPROCESS CurrentProcess;
   ULONG Attributes;
   PULONG Pte;
   NTSTATUS Status;

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
       KeBugCheck(0);
     }
   if (Process != NULL && Address >= (PVOID)KERNEL_BASE)
     {
       DPRINT1("Setting kernel address with process context\n");
       KeBugCheck(0);
     }
   MmMarkPageMapped(PhysicalAddress);
   
   Attributes = ProtectToPTE(flProtect);
   if (!(Attributes & PA_PRESENT) && PhysicalAddress.QuadPart != 0)
     {
       DPRINT1("Setting physical address but not allowing access at address "
	       "0x%.8X with attributes %x/%x.\n", 
	       Address, Attributes, flProtect);
       KeBugCheck(0);
     }
   
   if (Process != NULL && Process != CurrentProcess)
     {
	KeAttachProcess(Process);
     }
   
   Status = MmGetPageEntry2(Address, &Pte, MayWait);
   if (!NT_SUCCESS(Status))
     {
	if (Process != NULL && Process != CurrentProcess)
	  {
	     KeDetachProcess();
	  }
	return(Status);
     }
   if (PAGE_MASK((*Pte)) != 0 && !((*Pte) & PA_PRESENT))
     {
       KeBugCheck(0);
     }
   if (PAGE_MASK((*Pte)) != 0)
     {
       MmMarkPageUnmapped(PTE_TO_PAGE((*Pte)));
     }
   *Pte = PhysicalAddress.QuadPart | Attributes;
   if (Process != NULL && 
       Process->AddressSpace.PageTableRefCountTable != NULL &&
       Address < (PVOID)KERNEL_BASE &&
       Attributes & PA_PRESENT)
     {
	PUSHORT Ptrc;
	
	Ptrc = Process->AddressSpace.PageTableRefCountTable;
	
	Ptrc[ADDR_TO_PAGE_TABLE(Address)]++;
     }
   FLUSH_TLB;
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
		       PHYSICAL_ADDRESS PhysicalAddress,
		       BOOLEAN MayWait)
{
  if (!MmIsUsablePage(PhysicalAddress))
    {
      DPRINT1("Page at address %x not usable\n", PhysicalAddress);
      KeBugCheck(0);
    }
  
  return(MmCreateVirtualMappingUnsafe(Process,
				      Address,
				      flProtect,
				      PhysicalAddress,
				      MayWait));
}

ULONG
MmGetPageProtect(PEPROCESS Process, PVOID Address)
{
  ULONG Entry;
  ULONG Protect;

  Entry = MmGetPageEntryForProcess1(Process, Address);

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
   PULONG PageEntry;
   PEPROCESS CurrentProcess = PsGetCurrentProcess();

   DPRINT("MmSetPageProtect(Process %x  Address %x  flProtect %x)\n",
     Process, Address, flProtect);

   Attributes = ProtectToPTE(flProtect);
   if (Process != NULL && Process != CurrentProcess)
     {
	KeAttachProcess(Process);
     }
   PageEntry = MmGetPageEntry(Address);
   (*PageEntry) = PAGE_MASK(*PageEntry) | Attributes;
   FLUSH_TLB;
   if (Process != NULL && Process != CurrentProcess)
     {
	KeDetachProcess();
     }
}

PHYSICAL_ADDRESS STDCALL 
MmGetPhysicalAddress(PVOID vaddr)
/*
 * FUNCTION: Returns the physical address corresponding to a virtual address
 */
{
   PHYSICAL_ADDRESS p;
   ULONG Pte;

   DPRINT("MmGetPhysicalAddress(vaddr %x)\n", vaddr);

   Pte = *MmGetPageEntry(vaddr);
   if (Pte & PA_PRESENT)
     {
       p.QuadPart = PAGE_MASK(Pte);
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
  unsigned EntryTop = ADDR_TO_PDE_OFFSET(PThread->InitialStack - PAGE_SIZE);

  if (0 == LocalPageDir[EntryBase])
    {
      LocalPageDir[EntryBase] = MmGlobalKernelPageDirectory[EntryBase];
    }
  if (EntryBase != EntryTop && 0 == LocalPageDir[EntryTop])
    {
      LocalPageDir[EntryTop] = MmGlobalKernelPageDirectory[EntryTop];
    }
}

VOID 
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
