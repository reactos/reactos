/*
 * COPYRIGHT:   See COPYING in the top directory
 * PROJECT:     ReactOS kernel
 * FILE:        ntoskrnl/hal/x86/page.c
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

PULONG MmGetPageEntry(PEPROCESS Process, PVOID PAddress)
{
   ULONG page_table;
   PULONG page_tlb;
   PULONG page_dir;
   ULONG Address = (ULONG)PAddress;
   
   DPRINT("MmGetPageEntry(Process %x, Address %x)\n",Process,Address);
   
   if (Process != NULL)
     {
	page_dir = Process->Pcb.PageTableDirectory;
     }
   else
     {
	page_dir = (PULONG)get_page_directory();
     } 
   
   DPRINT("page_dir %x\n",page_dir);
   page_tlb = (PULONG)physical_to_linear(
			     PAGE_MASK(page_dir[VADDR_TO_PD_OFFSET(Address)]));
   DPRINT("page_tlb %x\n",page_tlb);

   if (PAGE_MASK(page_dir[VADDR_TO_PD_OFFSET(Address)])==0)
     {
	DPRINT("Creating new page directory\n",0);
	page_table = get_free_page();  // Returns a physical address
	page_tlb=(PULONG)physical_to_linear(page_table);
	memset(page_tlb,0,PAGESIZE);
	page_dir[VADDR_TO_PD_OFFSET(Address)]=page_table+0x7;
	
     }
   DPRINT("Returning %x\n",page_tlb[VADDR_TO_PT_OFFSET(Address)/4]);
   return(&page_tlb[VADDR_TO_PT_OFFSET(Address)/4]);
}

BOOLEAN MmIsPagePresent(PEPROCESS Process, PVOID Address)
{
   return((*MmGetPageEntry(Process, Address)) & PA_PRESENT);
}

VOID MmSetPage(PEPROCESS Process,
	       PVOID Address, 
	       ULONG flProtect,
	       ULONG PhysicalAddress)
{
   
   ULONG Attributes = 0;
   
   Attributes = ProtectToPTE(flProtect);
   
   (*MmGetPageEntry(Process, Address)) = PhysicalAddress | Attributes;
   FLUSH_TLB;
}

VOID MmSetPageProtect(PEPROCESS Process,
		      PVOID Address,
		      ULONG flProtect)
{
   ULONG Attributes = 0;
   PULONG PageEntry;
   
   Attributes = ProtectToPTE(flProtect);
   
   PageEntry = MmGetPageEntry(Process,Address);
   (*PageEntry) = PAGE_MASK(*PageEntry) | Attributes;
   FLUSH_TLB;
}

PHYSICAL_ADDRESS MmGetPhysicalAddress(PVOID vaddr)
/*
 * FUNCTION: Returns the physical address corresponding to a virtual address
 */
{
  PHYSICAL_ADDRESS p;

  DPRINT("MmGetPhysicalAddress(vaddr %x)\n", vaddr);
   
  SET_LARGE_INTEGER_HIGH_PART(p, 0);
  SET_LARGE_INTEGER_LOW_PART(p, PAGE_MASK(*MmGetPageEntry(NULL,vaddr)));
   
  return p;
}
