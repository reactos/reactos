/*
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         ntoskrnl/mm/freelist.c
 * PURPOSE:      Handle the list of free physical pages
 * PROGRAMMER:   David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *               27/05/98: Created
 *               18/08/98: Added a fix from Robert Bergkvist
 */

/*
 * NOTE: The list of free pages is implemented as an unsorted double linked 
 * list. This should make added or removing pages fast when you don't care
 * about the physical address. Because the entirety of physical memory is
 * mapped from 0xd0000000 upwards it is easy to do a mapping between 
 * physical and linear address. 
 */

/* INCLUDES ****************************************************************/

#include <internal/stddef.h>
#include <internal/hal/page.h>
#include <internal/mm.h>
#include <internal/ntoskrnl.h>
#include <internal/bitops.h>
#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES *******************************************************************/

typedef struct _free_page
/*
 * PURPOSE: At the start of every region of free physical pages
 */
{
        struct _free_page* next;
        struct _free_page* previous;
        unsigned int nr_pages;
} free_page_hdr;

/* GLOBALS ****************************************************************/

/*
 * PURPOSE: Points to the first page in the free list
 */
free_page_hdr* free_page_list_head=NULL;

/* FUNCTIONS *************************************************************/

void free_page(unsigned int physical_base, unsigned int nr)
/*
 * FUNCTION: Add a physically continuous range of pages to the free list
 * ARGUMENTS:
 *         physical_base = The first physical address to free
 *         nr = the size of the region (in pages) to free
 * NOTES: This function attempts to keep the list partially unfragmented 
 */
{
   unsigned int eflags;           
   free_page_hdr* hdr=NULL;

   DPRINT("Freeing %x to %x\n",physical_base,physical_base
	  + (nr*PAGESIZE));
   
   /*
    * This must be atomic
    */
   __asm__("pushf\n\tpop %0\n\tcli\n\t"
	   : "=d" (eflags));
   
   /*
    * 
    */
   hdr = (free_page_hdr *)physical_to_linear(physical_base);
   
   DPRINT("free_page_hdr %x\n",hdr);
   DPRINT("free_page_list_head %x\n",free_page_list_head);
   
   if (free_page_list_head!=NULL)
     {
	free_page_list_head->previous=hdr;
     }
   hdr->next=free_page_list_head;
   hdr->previous=NULL;
   hdr->nr_pages = nr;
   free_page_list_head=hdr;
   
   __asm__("push %0\n\tpopf\n\t"
	   :
	   : "d" (eflags));
}

unsigned int get_dma_page(unsigned int max_address)
/*
 * FUNCTION: Gets a page with a restricted max physical address (i.e.
 * suitable for dma)
 * ARGUMENTS:
 *         max_address = The maximum address usable by the caller
 * RETURNS:
 *      The physical address of the page if it succeeds
 *      NULL if it fails.
 * NOTES: This is very inefficent because the list isn't sorted. On the
 * other hand sorting the list would be quite expensive especially if dma
 * is only used infrequently. Perhaps a special cache of dma pages should
 * be maintained?
 */
{
   free_page_hdr* current=NULL;

   if (free_page_list_head==NULL)
     {
	printk("CRITICAL: Unable to allocate page\n");
	KeBugCheck(KBUG_OUT_OF_MEMORY);
     }
   
   /*
    * Walk the free page list looking for suitable memory
    */
   current = free_page_list_head;
   while (current!=NULL)
     {
	if ( ((int)current) < max_address)
	  {
	     /*
	      * We take the first page from the region 
	      */
	     free_page_hdr* nhdr = (free_page_hdr *)(((int)current)+PAGESIZE);
	     if (current->previous!=NULL)
	       {
		  current->previous->next=nhdr;
	       }
	     if (current->next!=NULL)
	       {
		  current->next->previous=nhdr;
	       }
	     nhdr->next=current->next;
	     nhdr->previous=current->previous;
	     nhdr->nr_pages=current->nr_pages-1;
	     if (free_page_list_head==current)
	       {
		  free_page_list_head=nhdr;
	       }
	     
	     return ((int)current);
	  }
	
	current=current->next;
     }
   return(NULL);
}

unsigned int get_free_page(void)
/*
 * FUNCTION: Allocates a page 
 * RETURNS: The physical address of the page allocated
 */
{
   unsigned int addr;

   /*
    * This must be atomic wrt everything
    */
   unsigned int eflags;
   __asm__("pushf\n\tpop %0\n\tcli\n\t"
	   : "=d" (eflags));
   CHECKPOINT;
   /*
    * If we are totally out of memory then panic
    */
   if (free_page_list_head==NULL)
     {
	printk("CRITICAL: Unable to allocate page\n");
	KeBugCheck(KBUG_OUT_OF_MEMORY);
     }
   CHECKPOINT;
   addr = 0;
   CHECKPOINT;
   if (free_page_list_head->nr_pages>1)
     {
	free_page_list_head->nr_pages--;
	addr = ((unsigned int)free_page_list_head) +
	  (free_page_list_head->nr_pages * PAGESIZE);
     }
   else
     {
	addr = (unsigned int)free_page_list_head;
	free_page_list_head = free_page_list_head -> next;
     }
   CHECKPOINT;
   __asm__("push %0\n\tpopf\n\t"
	   :
	   : "d" (eflags));
   
   addr = addr - (IDMAP_BASE);
   DPRINT("allocated %x\n",addr);
   CHECKPOINT;
   return(addr);
}




