/*
 * COPYRIGHT:   See COPYING in the top directory
 * PROJECT:     ReactOS kernel v0.0.2
 * FILE:        hal/page.cc
 * PURPOSE:     low level memory managment manipulation
 * PROGRAMER:   David Welch
 * UPDATE HISTORY:
 *              9/3/98: Created
 */

/* INCLUDES ***************************************************************/

#include <internal/kernel.h>
#include <internal/hal/page.h>
#include <internal/mm.h>
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

/*
 * The mark_page_xxxx manipulate the attributes of a page. Use the
 * higher level functions for synchronization. These functions only work
 * on present pages. 
 */

void mark_page_not_present(unsigned int vaddr)
/*
 * FUNCTION: Marks the page as not present
 * ARGUMENTS:
 *         vaddr = The virtual address to affect
 */
{
        clear_bit(PA_BIT_PRESENT,get_page_entry(vaddr));
        FLUSH_TLB;
}

void mark_page_present(unsigned int vaddr)
/*
 * FUNCTION: Marks the page as present
 * ARGUMENTS:
 *         vaddr = The virtual address to affect
 */
{
        set_bit(PA_BIT_PRESENT,get_page_entry(vaddr));
        FLUSH_TLB;
}

void mark_page_not_writable(unsigned int vaddr)
/*
 * FUNCTION: Marks the page as not writable by any process
 * ARGUMENTS:
 *         vaddr = The virtual address to affect
 */
{
        clear_bit(PA_BIT_READWRITE,get_page_entry(vaddr));
        FLUSH_TLB;
}

void mark_page_writable(unsigned int vaddr)
/*
 * FUNCTION: Marks the page as writable by any process
 * ARGUMENTS:
 *         vaddr = The virtual address to affect
 */
{
        set_bit(PA_BIT_READWRITE,get_page_entry(vaddr));
        FLUSH_TLB;
}

void mark_page_user(unsigned int vaddr)
/*
 * FUNCTION: Marks the page as user accessible
 * ARGUMENTS:
 *         vaddr = The virtual address to affect
 */
{
        set_bit(PA_BIT_USER,get_page_entry(vaddr));
        FLUSH_TLB;
}

void mark_page_system(unsigned int vaddr)
/*
 * FUNCTION: Marks the page as system only
 * ARGUMENTS:
 *         vaddr = The virtual address to affect
 */
{
        clear_bit(PA_BIT_USER,get_page_entry(vaddr));
        FLUSH_TLB;
}



void set_page(unsigned int vaddr, unsigned int attributes,
              unsigned int physaddr)
/*
 * FUNCTION: Set the page entry of a virtual address
 * ARGUMENTS:
 *          vaddr = Virtual address 
 *          attributes = Access attributes for the page
 *          physaddr = Physical address to map the virtual address to
 * NOTE: In future this won't flush the TLB
 */
{
        DPRINT("set_page(vaddr %x attributes %x physaddr %x)\n",vaddr,
                attributes,physaddr);
        *get_page_entry(vaddr)=physaddr | attributes;
        FLUSH_TLB;
}


PHYSICAL_ADDRESS MmGetPhysicalAddress(PVOID vaddr)
/*
 * FUNCTION: Returns the physical address corresponding to a virtual address
 */
{
        DPRINT("get_page_physical_address(vaddr %x)\n",vaddr);
        return(PAGE_MASK(*get_page_entry((unsigned int)vaddr)));
}

BOOL is_page_present(unsigned int vaddr)
/*
 * FUNCTION: Tests if a page is present at the address
 * RETURNS:
 *      True: If an access to the page would happen without any page faults
 *      False: If an access to the page would involve page faults
 * NOTES: The information is only guarrented to remain true if the caller has
 * locked the page. The function does not have any side effects when used
 * from an irq handler so it can be used as a 'sanity' test when accessing a
 * buffer from an irq.
 */
{
#if 0        
   unsigned int* page_dir = physical_to_linear(current_task->cr3);
#else
   unsigned int* page_dir = get_page_directory();
#endif
   unsigned int* page_tlb = NULL;
   
   /*
    * Check the page directory exists
    */
   if (!(page_dir[VADDR_TO_PD_OFFSET(vaddr)]&PA_PRESENT))
     {
	return(FALSE);
     }

   page_tlb = (unsigned int *)physical_to_linear(
                             PAGE_MASK(page_dir[VADDR_TO_PD_OFFSET(vaddr)]));

   if (!(page_tlb[VADDR_TO_PT_OFFSET(vaddr)/4]&PA_PRESENT))
     {
	return(FALSE);
     }
   
   return(TRUE);
}

unsigned int* get_page_entry(unsigned int vaddr)
/*
 * FUNCTION: Returns a pointer to a page entry
 * NOTE: This function will create a page table if none exists so just to
 * check if mem exists use the is_page_present function
 */
{
   unsigned int page_table;
   unsigned int* page_tlb;
   
#if 0        
   unsigned int* page_dir = physical_to_linear(current_task->cr3);
#else
   unsigned int* page_dir = get_page_directory();
#endif

   DPRINT("vaddr %x ",vaddr);
   page_tlb = (unsigned int *)physical_to_linear(
			     PAGE_MASK(page_dir[VADDR_TO_PD_OFFSET(vaddr)]));
   DPRINT("page_tlb %x\n",page_tlb);

   if (PAGE_MASK(page_dir[VADDR_TO_PD_OFFSET(vaddr)])==0)
     {
	DPRINT("Creating new page directory\n",0);
	page_table = get_free_page();  // Returns a physical address
	page_tlb=(unsigned int *)physical_to_linear(page_table);
	memset(page_tlb,0,PAGESIZE);
	page_dir[VADDR_TO_PD_OFFSET(vaddr)]=page_table+0x7;
	
     }
   return(&page_tlb[VADDR_TO_PT_OFFSET(vaddr)/4]);
}


