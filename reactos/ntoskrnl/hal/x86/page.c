/*
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
#include <internal/string.h>
#include <internal/bitops.h>
#include <internal/ex.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

#define PA_BIT_PRESENT   (0)
#define PA_BIT_READWRITE (1)
#define PA_BIT_USER      (2)

#define PA_PRESENT (1<<PA_BIT_PRESENT)

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
#define ADDR_TO_PTE(v) (PULONG)(PAGETABLE_MAP + ((ULONG)v / 1024))

NTSTATUS Mmi386ReleaseMmInfo(PEPROCESS Process)
{
   ULONG i,j,addr;
   
   DPRINT("Mmi386ReleaseMmInfo(Process %x)\n",Process);
   
   MmFreePage(Process->Pcb.PageTableDirectory, 1);
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
   PhysPageDirectory = (PULONG)
     GET_LARGE_INTEGER_LOW_PART(MmGetPhysicalAddress(PageDirectory));
   KProcess->PageTableDirectory = PhysPageDirectory;   
   CurrentPageDirectory = (PULONG)PAGEDIRECTORY_MAP;
   
   memset(PageDirectory,0,PAGESIZE);
   for (i=768; i<896; i++)
     {
	PageDirectory[i] = CurrentPageDirectory[i];
     }
   DPRINT("Addr %x\n",0xf0000000 / (4*1024*1024));
   PageDirectory[0xf0000000 / (4*1024*1024)] = (ULONG)PhysPageDirectory | 0x7;
   
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
   if (FreePage)
     {
	MmFreePage(PAGE_MASK(*page_tlb),1);
     }
   *page_tlb = 0;
   if (Process != NULL && Process != CurrentProcess)
     {
	KeDetachProcess();
     }
}


PULONG MmGetPageEntry(PVOID PAddress)
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
	(*page_dir) = ((ULONG)MmAllocPage()) | 0x7;
	FLUSH_TLB;
     }
   page_tlb = ADDR_TO_PTE(Address);
   DPRINT("page_tlb %x\n",page_tlb);
   return(page_tlb);
}

BOOLEAN MmIsPagePresent(PEPROCESS Process, PVOID Address)
{
   return((MmGetPageEntryForProcess(Process, Address)) & PA_PRESENT);
}


VOID MmSetPage(PEPROCESS Process,
	       PVOID Address, 
	       ULONG flProtect,
	       ULONG PhysicalAddress)
{
   PEPROCESS CurrentProcess = PsGetCurrentProcess();
   ULONG Attributes = 0;
   
   DPRINT("MmSetPage(Process %x, Address %x, flProtect %x, "
	  "PhysicalAddress %x)\n",Process,Address,flProtect,
	  PhysicalAddress);
   
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

PHYSICAL_ADDRESS MmGetPhysicalAddress(PVOID vaddr)
/*
 * FUNCTION: Returns the physical address corresponding to a virtual address
 */
{
  PHYSICAL_ADDRESS p = INITIALIZE_LARGE_INTEGER;

  DPRINT("MmGetPhysicalAddress(vaddr %x)\n", vaddr);
   
  SET_LARGE_INTEGER_HIGH_PART(p, 0);
  SET_LARGE_INTEGER_LOW_PART(p, PAGE_MASK(*MmGetPageEntry(vaddr)));
   
  return p;
}
