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

#include <internal/mmhal.h>
#include <internal/mm.h>
#include <string.h>
#include <internal/string.h>
#include <internal/bitops.h>
#include <ddk/ntddk.h>

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

ULONG MmGetPageEntryForProcess(PEPROCESS Process, PVOID Address)
{
   ULONG Entry;
   
   if (Process != NULL && Process != PsGetCurrentProcess())
     {
	KeAttachProcess(Process);
     }
   Entry = *MmGetPageEntry(Address);
   if (Process != NULL && Process != PsGetCurrentProcess())
     {
	KeDetachProcess();
     }
   return(Entry);
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
//	(*page_dir) = get_free_page() | (PA_READ | PA_WRITE);
	(*page_dir) = get_free_page() | 0x7;
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
   
   ULONG Attributes = 0;
   
   DPRINT("MmSetPage(Process %x, Address %x, flProtect %x, "
	  "PhysicalAddress %x)\n",Process,Address,flProtect,
	  PhysicalAddress);
   
   Attributes = ProtectToPTE(flProtect);
   
   if (Process != NULL && Process != PsGetCurrentProcess())
     {
	KeAttachProcess(Process);
     }
   (*MmGetPageEntry(Address)) = PhysicalAddress | Attributes;
   FLUSH_TLB;
   if (Process != NULL && Process != PsGetCurrentProcess())
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
   
   Attributes = ProtectToPTE(flProtect);

   if (Process != PsGetCurrentProcess())
     {
	KeAttachProcess(Process);
     }
   PageEntry = MmGetPageEntry(Address);
   (*PageEntry) = PAGE_MASK(*PageEntry) | Attributes;
   FLUSH_TLB;
   if (Process != PsGetCurrentProcess())
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
