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
/* $Id: page.c,v 1.26 2001/04/13 16:12:26 chorns Exp $
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
   if (flProtect & PAGE_READWRITE || flProtect & PAGE_EXECUTE_READWRITE)
     {
       Attributes = PA_PRESENT | PA_READWRITE;
     }
   if (flProtect & PAGE_READONLY || flProtect & PAGE_EXECUTE ||
       flProtect & PAGE_EXECUTE_READ)
     {
       Attributes = PA_PRESENT; 
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
                                (((ULONG)v / (1024 * 1024))&(~0x3)))
#define ADDR_TO_PTE(v) (PULONG)(PAGETABLE_MAP + ((((ULONG)v / 1024))&(~0x3)))

#define ADDR_TO_PDE_OFFSET(v) (((ULONG)v / (4 * 1024 * 1024)))

NTSTATUS Mmi386ReleaseMmInfo(PEPROCESS Process)
{
   DPRINT("Mmi386ReleaseMmInfo(Process %x)\n",Process);
   
   MmDereferencePage(Process->Pcb.PageTableDirectory);
   Process->Pcb.PageTableDirectory = NULL;
   
   DPRINT("Finished Mmi386ReleaseMmInfo()\n");
   return(STATUS_SUCCESS);
}

NTSTATUS MmCopyMmInfo(PEPROCESS Src, PEPROCESS Dest)
{
   PULONG PhysPageDirectory;
   PULONG PageDirectory;
   PULONG CurrentPageDirectory;
   PKPROCESS KProcess = &Dest->Pcb;
   ULONG i;
   
   DPRINT("MmCopyMmInfo(Src %x, Dest %x)\n", Src, Dest);
   
   PageDirectory = ExAllocatePage();
   if (PageDirectory == NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   PhysPageDirectory = (PULONG)(MmGetPhysicalAddress(PageDirectory)).u.LowPart;
   KProcess->PageTableDirectory = PhysPageDirectory;   
   CurrentPageDirectory = (PULONG)PAGEDIRECTORY_MAP;
   
   memset(PageDirectory,0,PAGESIZE);
   for (i=768; i<896; i++)
     {
	PageDirectory[i] = CurrentPageDirectory[i];
     }
   for (i=961; i<1024; i++)
     {
	PageDirectory[i] = CurrentPageDirectory[i];
     }
   DPRINT("Addr %x\n",PAGETABLE_MAP / (4*1024*1024));
   PageDirectory[PAGETABLE_MAP / (4*1024*1024)] = 
     (ULONG)PhysPageDirectory | 0x7;
   
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
       MmGlobalKernelPageDirectory[ADDR_TO_PDE_OFFSET(Address)] = 0;
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
   if (Address >= (PVOID)KERNEL_BASE)
     {
       MmGlobalKernelPageDirectory[ADDR_TO_PDE_OFFSET(Address)] = 0;
     }
   MmDereferencePage((PVOID)PAGE_MASK(npage));
   FLUSH_TLB;
   if (Process != NULL && Process != CurrentProcess)
     {
	KeDetachProcess();
     }
}

NTSTATUS MmGetPageEntry2(PVOID PAddress, PULONG* Pte)
/*
 * FUNCTION: Get a pointer to the page table entry for a virtual address
 */
{
   PULONG Pde;
   ULONG Address = (ULONG)PAddress;
   ULONG npage;
   
   DPRINT("MmGetPageEntry(Address %x)\n", Address);
   
   Pde = ADDR_TO_PDE(Address);
   if ((*Pde) == 0)     
     {
       if (Address >= KERNEL_BASE &&
	   MmGlobalKernelPageDirectory[ADDR_TO_PDE_OFFSET(Address)] != 0)
	 {
	   (*Pde) = MmGlobalKernelPageDirectory[ADDR_TO_PDE_OFFSET(Address)];
	   FLUSH_TLB;
	 }
       else
	 {
	   npage = (ULONG)MmAllocPage(0);
	   if (npage == 0)
	     {
	       return(STATUS_NO_MEMORY);
	     }
	   (*Pde) = npage | 0x7;		
	   if (Address >= KERNEL_BASE)
	     {
	       MmGlobalKernelPageDirectory[ADDR_TO_PDE_OFFSET(Address)] = 
		 *Pde;
	     }
	   memset((PVOID)PAGE_ROUND_DOWN(ADDR_TO_PTE(Address)), 0, PAGESIZE);
	   FLUSH_TLB;
	 }
     }
   *Pte = ADDR_TO_PTE(Address);
   return(STATUS_SUCCESS);
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
   PULONG page_tlb;
   PULONG page_dir;
   ULONG Address = (ULONG)PAddress;
   
   DPRINT("MmGetPageEntry(Address %x)\n", Address);
   
   page_dir = ADDR_TO_PDE(Address);
   if ((*page_dir) == 0 &&
       MmGlobalKernelPageDirectory[ADDR_TO_PDE_OFFSET(Address)] != 0)
     {
       (*page_dir) = MmGlobalKernelPageDirectory[ADDR_TO_PDE_OFFSET(Address)];
       FLUSH_TLB;
     }
   DPRINT("page_dir %x *page_dir %x\n",page_dir,*page_dir);
   if ((*page_dir) == 0)
     {
	return(0);
     }
   page_tlb = ADDR_TO_PTE(Address);
   DPRINT("page_tlb %x\n",page_tlb);
   return(*page_tlb);
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


ULONG MmGetPhysicalAddressForProcess(PEPROCESS Process,
				     PVOID Address)
{
   ULONG PageEntry;
   
   PageEntry = MmGetPageEntryForProcess(Process, Address);
   
   if (!(PageEntry & PA_PRESENT))
     {
	return(0);
     }
   return(PAGE_MASK(PageEntry));
}

VOID
MmDeleteVirtualMapping(PEPROCESS Process, PVOID Address, BOOL FreePage,
		       BOOL* WasDirty, ULONG* PhysicalAddr)
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
	if (Process != NULL && Process != CurrentProcess)
	  {
	     KeDetachProcess();
	  }	
	return;
     }

   /*
    * Atomically set the entry to zero and get the old value.
    */
   Pte = (ULONG)InterlockedExchange((PLONG)ADDR_TO_PTE(Address), 0);
   WasValid = (PAGE_MASK(Pte) != 0);
   if (WasValid)
     {
       MmMarkPageUnmapped((PVOID)PAGE_MASK(Pte));
     }
   if (FreePage && WasValid)
     {
        MmDereferencePage((PVOID)PAGE_MASK(Pte));
     }

   /*
    * Decrement the reference count for this page table.
    */
   if (Process != NULL && WasValid &&
       Process->AddressSpace.PageTableRefCountTable != NULL &&
       ADDR_TO_PAGE_TABLE(Address) < 768)
     {
	PUSHORT Ptrc;
	
	Ptrc = Process->AddressSpace.PageTableRefCountTable;
	
	Ptrc[ADDR_TO_PAGE_TABLE(Address)]--;
#if 1
	if (Ptrc[ADDR_TO_PAGE_TABLE(Address)] == 0)
	  {
	     MmFreePageTable(Process, Address);
	  }
#endif
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
       *PhysicalAddr = PAGE_MASK(Pte);
     }
}

BOOLEAN 
Mmi386MakeKernelPageTableGlobal(PVOID PAddress)
{
   PULONG page_dir;
   ULONG Address = (ULONG)PAddress;
   
   page_dir = ADDR_TO_PDE(Address);
   if ((*page_dir) == 0 &&
       MmGlobalKernelPageDirectory[ADDR_TO_PDE_OFFSET(Address)] != 0)
     {
       (*page_dir) = MmGlobalKernelPageDirectory[ADDR_TO_PDE_OFFSET(Address)];
       FLUSH_TLB;
       return(TRUE);
     }
   return(FALSE);
}

BOOLEAN MmIsPageTablePresent(PVOID PAddress)
{
   PULONG page_dir;
   ULONG Address = (ULONG)PAddress;
   
   page_dir = ADDR_TO_PDE(Address);
   if ((*page_dir) == 0 &&
       MmGlobalKernelPageDirectory[ADDR_TO_PDE_OFFSET(Address)] != 0)
     {
       (*page_dir) = MmGlobalKernelPageDirectory[ADDR_TO_PDE_OFFSET(Address)];
       FLUSH_TLB;
     }
   return((*page_dir) == 0);
}

NTSTATUS MmCreatePageTable(PVOID PAddress)
{
   PULONG page_dir;
   ULONG Address = (ULONG)PAddress;
   ULONG npage;
   
   DPRINT("MmGetPageEntry(Address %x)\n", Address);
   
   page_dir = ADDR_TO_PDE(Address);
   DPRINT("page_dir %x *page_dir %x\n",page_dir,*page_dir);
   if ((*page_dir) == 0 &&
       MmGlobalKernelPageDirectory[ADDR_TO_PDE_OFFSET(Address)] != 0)
     {
       (*page_dir) = MmGlobalKernelPageDirectory[ADDR_TO_PDE_OFFSET(Address)];
       FLUSH_TLB;
     }
   if ((*page_dir) == 0)
     {
	npage = (ULONG)MmAllocPage(0);
	if (npage == 0)
	  {
	     return(STATUS_UNSUCCESSFUL);
	  }
	(*page_dir) = npage | 0x7;
	memset((PVOID)PAGE_ROUND_DOWN(ADDR_TO_PTE(Address)), 0, PAGESIZE);
	FLUSH_TLB;
     }
   return(STATUS_SUCCESS);
}

PULONG MmGetPageEntry(PVOID PAddress)
/*
 * FUNCTION: Get a pointer to the page table entry for a virtual address
 */
{
   PULONG page_tlb;
   PULONG page_dir;
   ULONG Address = (ULONG)PAddress;
   ULONG npage;
   
   DPRINT("MmGetPageEntry(Address %x)\n", Address);
   
   page_dir = ADDR_TO_PDE(Address);
   DPRINT("page_dir %x *page_dir %x\n",page_dir,*page_dir);
   if ((*page_dir) == 0 &&
       MmGlobalKernelPageDirectory[ADDR_TO_PDE_OFFSET(Address)] != 0)
     {
       (*page_dir) = MmGlobalKernelPageDirectory[ADDR_TO_PDE_OFFSET(Address)];
       FLUSH_TLB;
     }
   if ((*page_dir) == 0)
     {
	npage = (ULONG)MmAllocPage(0);
	if (npage == 0)
	  {
	     KeBugCheck(0);
	  }
	(*page_dir) = npage | 0x7;
	memset((PVOID)PAGE_ROUND_DOWN(ADDR_TO_PTE(Address)), 0, PAGESIZE);
	FLUSH_TLB;
     }
   page_tlb = ADDR_TO_PTE(Address);
   DPRINT("page_tlb %x\n",page_tlb);
   return(page_tlb);
}

BOOLEAN MmIsPageDirty(PEPROCESS Process, PVOID Address)
{
   return((MmGetPageEntryForProcess(Process, Address)) & PA_DIRTY);
}

BOOLEAN 
MmIsAccessedAndResetAccessPage(PEPROCESS Process, PVOID Address)
{
   PULONG PageEntry;
   PEPROCESS CurrentProcess = PsGetCurrentProcess();
   BOOLEAN Accessed;

   if (Process != CurrentProcess)
     {
	KeAttachProcess(Process);
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
   PEPROCESS CurrentProcess = PsGetCurrentProcess();
   
   if (Process != CurrentProcess)
     {
	KeAttachProcess(Process);
     }
   PageEntry = MmGetPageEntry(Address);
   (*PageEntry) = (*PageEntry) & (~PA_DIRTY);
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

NTSTATUS 
MmCreateVirtualMappingUnsafe(PEPROCESS Process,
			     PVOID Address, 
			     ULONG flProtect,
			     ULONG PhysicalAddress)
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
   MmMarkPageMapped((PVOID)PhysicalAddress);
   
   Attributes = ProtectToPTE(flProtect);
   
   if (Process != NULL && Process != CurrentProcess)
     {
	KeAttachProcess(Process);
     }
   
   Status = MmGetPageEntry2(Address, &Pte);
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
       MmMarkPageUnmapped((PVOID)PAGE_MASK((*Pte)));
     }
   *Pte = PhysicalAddress | Attributes;
   if (Process != NULL && 
       Process->AddressSpace.PageTableRefCountTable != NULL &&
       ADDR_TO_PAGE_TABLE(Address) < 768 &&
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
		       ULONG PhysicalAddress)
{
	 if (!MmIsUsablePage((PVOID)PhysicalAddress))
     {
       DPRINT1("Page at address %x not usable\n", PhysicalAddress);
       KeBugCheck(0);
     }

   return(MmCreateVirtualMappingUnsafe(Process,
				       Address,
				       flProtect,
				       PhysicalAddress));
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
   if (Process != CurrentProcess)
     {
	KeAttachProcess(Process);
     }
   PageEntry = MmGetPageEntry(Address);
   (*PageEntry) = PAGE_MASK(*PageEntry) | Attributes;
   FLUSH_TLB;
   if (Process != CurrentProcess)
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

   DPRINT("MmGetPhysicalAddress(vaddr %x)\n", vaddr);
   
   p.QuadPart = PAGE_MASK(*MmGetPageEntry(vaddr));
   
   return p;
}


/* EOF */
