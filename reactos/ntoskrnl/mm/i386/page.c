/* $Id: page.c,v 1.11 2000/07/06 14:34:51 dwelch Exp $
 *
 * COPYRIGHT:   See COPYING in the top directory
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
#include <internal/mmhal.h>
#include <string.h>
#include <internal/ex.h>
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

extern ULONG MiNrFreePages;

#define PA_BIT_PRESENT   (0)
#define PA_BIT_READWRITE (1)
#define PA_BIT_USER      (2)
#define PA_BIT_DIRTY     (6)

#define PA_PRESENT  (1<<PA_BIT_PRESENT)
#define PA_DIRTY    (1<<PA_BIT_DIRTY)

#define PAGETABLE_MAP     (0xf0000000)
#define PAGEDIRECTORY_MAP (0xf0000000 + (PAGETABLE_MAP / (1024)))

/* FUNCTIONS ***************************************************************/

static ULONG ProtectToPTE(ULONG flProtect)
{
   ULONG Attributes = 0;
   
   if (flProtect & PAGE_NOACCESS || flProtect & PAGE_GUARD)
     {
	Attributes = 0;
     }
   if (flProtect & PAGE_READWRITE || flProtect & PAGE_EXECUTE_READWRITE)
     {
	Attributes = PA_WRITE | PA_USER;
     }
   if (flProtect & PAGE_READONLY || flProtect & PAGE_EXECUTE ||
       flProtect & PAGE_EXECUTE_READ)
     {
	Attributes = PA_READ | PA_USER; 
      }
   return(Attributes);
}

#define ADDR_TO_PDE(v) (PULONG)(PAGEDIRECTORY_MAP + \
                                (((ULONG)v / (1024 * 1024))&(~0x3)))
#define ADDR_TO_PTE(v) (PULONG)(PAGETABLE_MAP + ((((ULONG)v / 1024))&(~0x3)))

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
   FLUSH_TLB;
   if (Process != NULL && Process != CurrentProcess)
     {
	KeDetachProcess();
     }
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

VOID MmDeletePageEntry(PEPROCESS Process, PVOID Address, BOOL FreePage)
{
   PULONG page_tlb;
   PULONG page_dir;
   PEPROCESS CurrentProcess = PsGetCurrentProcess();
   
   if (Process != NULL && Process != CurrentProcess)
     {
	KeAttachProcess(Process);
     }
   page_dir = ADDR_TO_PDE(Address);
   if ((*page_dir) == 0)
     {
	if (Process != NULL && Process != CurrentProcess)
	  {
	     KeDetachProcess();
	  }	
	return;
     }
   page_tlb = ADDR_TO_PTE(Address);
   if (FreePage && PAGE_MASK(*page_tlb) != 0)
     {
	if (PAGE_MASK(*page_tlb) >= 0x400000)
	  {
	     DbgPrint("MmDeletePageEntry(Address %x) Physical %x Free %d, "
                      "Entry %x\n",
		      Address, PAGE_MASK(*page_tlb), MiNrFreePages,
		      *page_tlb);
	     KeBugCheck(0);
	  }
        MmDereferencePage((PVOID)PAGE_MASK(*page_tlb));
     }
   *page_tlb = 0;
   if (Process != NULL && Process != CurrentProcess)
     {
	KeDetachProcess();
     }
}

BOOLEAN MmIsPageTablePresent(PVOID PAddress)
{
   PULONG page_dir;
   ULONG Address = (ULONG)PAddress;
   
   page_dir = ADDR_TO_PDE(Address);
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

BOOLEAN MmIsPagePresent(PEPROCESS Process, PVOID Address)
{
   return((MmGetPageEntryForProcess1(Process, Address)) & PA_PRESENT);
}


VOID MmSetPage(PEPROCESS Process,
	       PVOID Address, 
	       ULONG flProtect,
	       ULONG PhysicalAddress)
{
   PEPROCESS CurrentProcess = PsGetCurrentProcess();
   ULONG Attributes = 0;
   
   if (((ULONG)PhysicalAddress) >= 0x400000)
     {
	DbgPrint("MmSetPage(Process %x, Address %x, PhysicalAddress %x)\n",
		 Process, Address, PhysicalAddress);
	KeBugCheck(0);
     }
   
   Attributes = ProtectToPTE(flProtect);
   
   if (Process != NULL && Process != CurrentProcess)
     {
	KeAttachProcess(Process);
     }
   (*MmGetPageEntry(Address)) = PhysicalAddress | Attributes;
   FLUSH_TLB;
   if (Process != NULL && Process != CurrentProcess)
     {
	KeDetachProcess();
     }
}

VOID MmSetPageProtect(PEPROCESS Process,
		      PVOID Address,
		      ULONG flProtect)
{
   ULONG Attributes = 0;
   PULONG PageEntry;
   PEPROCESS CurrentProcess = PsGetCurrentProcess();
   
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

PHYSICAL_ADDRESS STDCALL MmGetPhysicalAddress(PVOID vaddr)
/*
 * FUNCTION: Returns the physical address corresponding to a virtual address
 */
{
   PHYSICAL_ADDRESS p;

   p.QuadPart = 0;
   
   DPRINT("MmGetPhysicalAddress(vaddr %x)\n", vaddr);
   
   p.u.LowPart = PAGE_MASK(*MmGetPageEntry(vaddr));
   p.u.HighPart = 0;
   
   return p;
}


/* EOF */
