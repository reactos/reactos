/*
 * Higher level memory managment definitions
 */

#ifndef __MM_H
#define __MM_H

#define PAGE_SYSTEM (0x80000000)

#include <internal/linkage.h>
#include <internal/kernel.h>
#include <windows.h>

typedef struct _memory_area
/*
 * PURPOSE: Describes an area of virtual memory 
 */
{
   /*
    * Access protection
    */
   unsigned int access;
   
   /*
    * Memory region base
    */
   unsigned int base;
   
   /*
    * Memory region length
    */
   unsigned int length;
   
   /*
    * Memory type (Mapped file, mapped from an executable or private)
    */
   unsigned int type;

   /*
    * Memory region state (committed, reserved or free)
    */
   unsigned int state;
   
   /*
    * Original access protection
    */   
   unsigned int initial_access;
   
   /*
    * Used to maintain the linked list of memory areas
    */
   struct _memory_area* previous;
   struct _memory_area* next;
   
   /*
    * True the region is locked
    */
   BOOL lock;
   
   /*
    * FUNCTION: Decommits all the pages in the regions
    */
   void (*free)(struct _memory_area* marea);
   
   /*
    * FUNCTION: Handles a page fault by loading the required page
    * RECEIVES:
    *           marea = the memory area
    *           address = the relative address of the page to load
    * RETURNS:
    *           TRUE = the access should be restarted
    *           FALSE = the access was illegal and an exception should
    *                   be generated
    * NOTES: This function is guarrented to be called within the context
    * of the thread which required a page to be loaded
    */
   BOOL (*load_page)(struct _memory_area* marea, unsigned int address);
} memory_area;


/*
 * FUNCTION: Gets a page with a restricted max physical address (i.e.
 * suitable for dma)
 * RETURNS:
 *      The physical address of the page if it succeeds
 *      NULL if it fails.
 * NOTES: This is very inefficent because the list isn't sorted. On the
 * other hand sorting the list would be quite expensive especially if dma
 * is only used infrequently. Perhaps a special cache of dma pages should
 * be maintained?
 */
unsigned int get_dma_page(unsigned int max_address);

/*
 * FUNCTION: Allocate a page and return its physical address
 * RETURNS: The physical address of the page allocated
 */
asmlinkage unsigned int get_free_page(void);

/*
 * FUNCTION: Adds pages to the free list
 * ARGUMENTS:
 *          physical_base = Physical address of the base of the region to
 *                          be freed
 *          nr = number of continuous pages to free
 */
asmlinkage void free_page(unsigned int physical_base, unsigned int nr);

/*
 * FUNCTION: Returns the physical address mapped by a given virtual address 
 * ARGUMENTS:
 *          vaddr = virtual address to query
 * RETURNS: The physical address if present in memory
 *          Zero if paged out or invalid
 * NOTE: This doesn't do any synchronization
 */
unsigned int get_page_physical_address(unsigned int vaddr);

void mark_page_not_writable(unsigned int vaddr);

void VirtualInit(boot_param* bp);

/*
 * FUNCTION: Returns the first memory area starting in the region or the last 
 *           one before the start of the region
 * ARGUMENTS:
 *           list_head = Head of the list of memory areas to search
 *           base = base address of the region
 *           length = length of the region
 * RETURNS: A pointer to the area found or
 *          NULL if the region was before the first region on the list
 */
memory_area* find_first_marea(memory_area* list_head, unsigned int base, 
			      unsigned int length);

/*
 * Head of the list of system memory areas 
 */
extern memory_area* system_memory_area_list_head;

/*
 * Head of the list of user memory areas (this should be per process)
 */
extern memory_area* memory_area_list_head;

#endif
