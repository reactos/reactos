/*
 * COPYRIGHT:   See COPYING in the top directory
 * PROJECT:     ReactOS kernel
 * FILE:        ntoskrnl/mm/virtual.c
 * PURPOSE:     implementing the Virtualxxx section of the win32 api
 * PROGRAMMER:  David Welch
 * UPDATE HISTORY:
 *              09/4/98: Created
 *              10/6/98: Corrections from Fatahi (i_fatahi@hotmail.com)
 */
 
/* INCLUDE *****************************************************************/

#include <windows.h>

#include <internal/hal/segment.h>
#include <internal/mm.h>
#include <internal/hal/page.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES *******************************************************************/

extern unsigned int etext;
extern unsigned int end;

/*
 * These two are statically declared because mm is initalized before the
 * memory pool
 */
static memory_area kernel_text_desc;
static memory_area kernel_data_desc;
static memory_area kernel_param_desc;
static memory_area kernel_pool_desc;

/*
 * Head of the list of system memory areas 
 */
memory_area* system_memory_area_list_head=&kernel_text_desc;

/*
 * Head of the list of user memory areas (this should be per process)
 */
memory_area* memory_area_list_head=NULL;

/*
 * The base address of the kmalloc region
 */
unsigned int kernel_pool_base = 0;


/* FUNCTIONS ****************************************************************/

void VirtualInit(boot_param* bp)
/*
 * FUNCTION: Intialize the memory areas list
 * ARGUMENTS:
 *           bp = Pointer to the boot parameters
 *           kernel_len = Length of the kernel
 */
{
   unsigned int kernel_len = bp->end_mem - bp->start_mem;
   
   DPRINT("VirtualInit() %x\n",bp);
   
   /*
    * Setup the system area descriptor list
    */
   kernel_text_desc.base = KERNEL_BASE;
   kernel_text_desc.length = ((ULONG)&etext) - KERNEL_BASE;
   kernel_text_desc.previous = NULL;
   kernel_text_desc.next = &kernel_data_desc;
   kernel_text_desc.load_page=NULL;
   kernel_text_desc.access = PAGE_EXECUTE_READ;
   
   kernel_data_desc.base = PAGE_ROUND_UP(((ULONG)&etext));
   kernel_data_desc.length = ((ULONG)&end) - kernel_text_desc.base;
   kernel_data_desc.previous = &kernel_text_desc;
   kernel_data_desc.next = &kernel_param_desc;
   kernel_data_desc.load_page=NULL;
   kernel_data_desc.access = PAGE_READWRITE;
   
   kernel_param_desc.base = PAGE_ROUND_UP(((ULONG)&end));
   kernel_param_desc.length =  kernel_len - (kernel_data_desc.length +
					     kernel_text_desc.length);
   kernel_param_desc.previous = &kernel_data_desc;
   kernel_param_desc.next = &kernel_pool_desc;
   kernel_param_desc.load_page=NULL;
   
   /*
    * The kmalloc area starts one page after the kernel area
    */
   kernel_pool_desc.base = KERNEL_BASE+ PAGE_ROUND_UP(kernel_len) + PAGESIZE;
   kernel_pool_desc.length = NONPAGED_POOL_SIZE;
   kernel_pool_desc.previous = &kernel_param_desc;
   kernel_pool_desc.next = NULL;
   kernel_pool_desc.load_page=NULL;

   kernel_pool_base = kernel_pool_desc.base;
   DPRINT("kmalloc_region_base %x\n",kernel_pool_base);
}


memory_area* find_first_marea(memory_area* list_head, unsigned int base, 
			      unsigned int length)
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
{
        memory_area* current=list_head;
        for (;;)
        {
	   if (current==NULL)
	     {
//		printk("current is null\n");
		return(NULL);
	     }
//	   printk("current %x current->base %x\n",current,current->base);
	   if (current->base == base && length==0)
	     {
		return(current);
	     }
	   
	   if (current->base >= base)
	     {
		if (current->base < (base+length))
		  {
		     return(current);
		  }
		else
		  {
		     return(current->previous);
		  }
	     }
	   if (current->next==NULL)
	     {
		return(current);
	     }
	   current=current->next;
        }
        return(NULL);
}

asmlinkage int page_fault_handler(unsigned int edi,
                                  unsigned int esi, unsigned int ebp,
                                  unsigned int esp, unsigned int ebx,
                                  unsigned int edx, unsigned int ecx,
                                  unsigned int eax, 
                                  unsigned int type,
                                  unsigned int ds,
                                  unsigned short int error_code,
                                  unsigned int eip,
                                  unsigned int cs, unsigned int eflags,
                                  unsigned int esp0, unsigned int ss0)
/*
 * FUNCTION: Handle a page fault
 */
{
   memory_area* marea=NULL;

   /*
    * Get the address for the page fault
    */
   unsigned int cr2;
   __asm__("movl %%cr2,%0\n\t" : "=d" (cr2));                
   DPRINT("Page fault at address %x with eip %x\n",cr2,eip);

   cr2 = PAGE_ROUND_DOWN(cr2);
   
   /*
    * Find the memory area for the faulting address
    */
   if (cr2>=KERNEL_BASE)
     {
	/*
	 * Check permissions
	 */
	if (cs!=KERNEL_CS)
	  {
	     printk("%s:%d\n",__FILE__,__LINE__);
	     return(0);
	  }
	marea=find_first_marea(system_memory_area_list_head,cr2,
			       PAGESIZE);
     }
   else
     {
	marea=find_first_marea(memory_area_list_head,cr2,
			       PAGESIZE);
     }
   
   /*
    * If the access was to an invalid area of memory raise an exception
    */
   if (marea==NULL || marea->load_page==NULL)
     {
	printk("%s:%d\n",__FILE__,__LINE__);
	return(0);
     }
   
   /*
    * Check the access was within the area
    */
   if (cr2 > (marea->base + marea->length) || cr2 < marea->base)
     {
        DPRINT("base was %x length %x\n",marea->base,marea->length);
	DPRINT("%s:%d\n",__FILE__,__LINE__);
	return(0);
     }
   
   /*
    * Call the region specific page fault handler
    */
   return(marea->load_page(marea,cr2 - marea->base));
}



static LPVOID allocate_marea(DWORD dwSize, DWORD flAllocationType,
                             DWORD flProtect, memory_area** list_head,
                             unsigned int last_addr,
                             BOOL (*fn)(memory_area* marea,
					unsigned int address))
/*
 * FUNCTION:
 */
{
   memory_area* current=*list_head;
   memory_area* previous;
   memory_area* ndesc=NULL;
   
   previous=current;
   while (current!=NULL)
     {
	last_addr = PAGE_ROUND_UP(current->base + current->length);
	previous=current;
	current=current->next;
     }
   ndesc = ExAllocatePool(NonPagedPool,sizeof(memory_area));
   ndesc->access=flProtect;
   ndesc->state=flAllocationType;
   ndesc->lock=FALSE;
   ndesc->base=last_addr+PAGESIZE;
   ndesc->length=dwSize;
   ndesc->previous=previous;
   if (previous!=NULL)
     {
	ndesc->next=previous->next;
        previous->next=ndesc;
	if (previous->next!=NULL)
	  {
	     previous->next->previous=ndesc;
	  }
     }
   else
     {
	*list_head=ndesc;
	ndesc->next=NULL;
	ndesc->previous=NULL;
     }
   ndesc->load_page=fn;
   DPRINT("VirtualAlloc() returning %x\n",ndesc->base);
   return((LPVOID)ndesc->base);
}

void commit_region_free(memory_area* marea)
/*
 * FUNCTION: Decommits the region
 */
{
   int i;
   for (i=0;i<marea->length;i=i+PAGESIZE)
     {
	if (is_page_present(marea->base+marea->length))
	  {
             free_page(MmGetPhysicalAddress(marea->base+marea->length),1);
	  }
     }
}

BOOL commit_region_load_page(memory_area* marea, unsigned int address)
/*
 * FUNCTION: Handles a page fault on a committed region of memory
 * ARGUMENTS:
 *          marea = memory area 
 *          address = address of faulting access
 * RETURNS: TRUE if the page was loaded
 *          FALSE if an exception should be generated
 */
{
   set_page(marea->base+address,0x7,get_free_page());   
   return(TRUE);
}

asmlinkage LPVOID STDCALL VirtualAlloc(LPVOID lpAddress, DWORD dwSize,
	     DWORD flAllocationType,
             DWORD flProtect)
{
/*
 * FUNCTION: Create a memory area
 * ARGUMENTS:
 *      lpAddress = the base address of the area or NULL if the system
 *                  decides the base
 *      dwSize = the size of the area
 *      flAllocationType = the type of allocation
 *              MEM_COMMIT = accessible
 *              MEM_RESERVE = not accessible but can't be allocated
 *      flProtect = what protection to give the area
 * RETURNS: The base address of the block
 */
        DPRINT("VirtualAlloc() lpAddress %x dwSize %x flAllocationType %x ",
               lpAddress,dwSize,flAllocationType);
        DPRINT("flProtect %x\n",flProtect);

        if (lpAddress==NULL)
        {
                /*
                 * We decide the address                 
                 */
                if (flProtect & PAGE_SYSTEM)
                {
                        return(allocate_marea(dwSize,flAllocationType,
                               flProtect,&system_memory_area_list_head,
                               KERNEL_BASE,commit_region_load_page));
                }
                else
                {
                        return(allocate_marea(dwSize,flAllocationType,
                               flProtect,&memory_area_list_head,PAGESIZE,
                               commit_region_load_page));
                }
        }
        else
        {
                memory_area* marea=NULL;
                if (lpAddress < ((PVOID)KERNEL_BASE))
                {
                        marea=find_first_marea(memory_area_list_head,
                                               (unsigned int)lpAddress,dwSize);
                }
                else
                {
                        marea=find_first_marea(system_memory_area_list_head,
                                               (unsigned int)lpAddress,dwSize);
                }

                /*
                 * Check someone hasn't already allocated that area
                 */
                if (marea!=NULL && marea->base <= (unsigned int)lpAddress &&
                    (marea->base + marea->length) > (unsigned int)lpAddress)
                {
                        SetLastError(ERROR_INVALID_DATA);
                        return(0);
                }

                /*
                 * Grab the area
                 */
        }
}

BOOL WINAPI VirtualFreeEx(HANDLE hProcess, LPVOID lpAddress, DWORD dwSize, 
			  DWORD dwFreeType)
/*
 * FUNCTION: Releases, decommits or both a region of memory within the
 * virtual address of a specified process
 * ARGUMENTS:
 *        hProcess = Process to act on
 *        lpAddress = starting virtual address to free 
 *        dwSize = Size in bytes of the memory region to free
 *        dwFreeType = Type of free operation
 * RETURNS: On success non-zero
 *          On failure zero
 * NOTE: This tries to optimize for the most common case which is for
 * regions to be decommitted in large chunks. If a process alternatedly 
 * decommitted every other page in a region this function would be extremely 
 * inefficent.
 */
{
   memory_area* marea = NULL;
   
   /*
    * Check our permissions for hProcess here
    */
   
   /*
    * Get the list of memory areas corresponding to hProcess here
    */
   
   /*
    * Do the business
    */
   if (dwFreeType==MEM_RELEASE)
     {
	if (dwSize!=0)
	  {
             SetLastError(ERROR_INVALID_BLOCK_LENGTH);
	     return(0);
	  }
	
	marea = find_first_marea(memory_area_list_head,(unsigned int)lpAddress,
				 1);
	
        if (marea==NULL || marea->base!=((int)lpAddress))
	  {
             SetLastError(ERROR_INVALID_BLOCK);
	     return(0);
	  }
	
	/*
	 * Ask the memory area to destroy itself
	 */
	marea->free(marea);
	
	/*
	 * Change the area attributes
	 */
	marea->access=PAGE_NOACCESS;
	marea->state=MEM_FREE;
	marea->type=MEM_PRIVATE;
     }
   else
     {
     }
}

BOOL WINAPI VirtualProtectEx(HANDLE hProcess, LPVOID lpAddress,
	DWORD dwSize, DWORD flNewProtect, PDWORD lpflOldProtect)
/*
 * FUNCTION: 
 */
{
}

DWORD WINAPI VirtualQueryEx(HANDLE hProcess, LPCVOID lpAddress,
	PMEMORY_BASIC_INFORMATION lpBuffer, DWORD dwLength)
/*
 * FUNCTION: Provides information about a range of pages within the virtual
 * address space of a specified process.
 * ARGUMENTS:
 *         hProcess = Handle of process
 *         lpAddress = Address of region
 *         lpBuffer = Buffer to store information
 *         dwLength = length of region
 * RETURNS: The number of bytes transferred into the buffer
 */
{
   /*
    * Find the memory area 
    */
   memory_area* marea = find_first_marea(memory_area_list_head,
					 (unsigned int) lpAddress,
                                         dwLength);

   /*
    * Check it exists
    */
   if (marea==NULL || marea->base!=((int)lpAddress))
     {
	SetLastError(0);
	return(0);
     }
        
   lpBuffer->BaseAddress = (void *)lpAddress;
   lpBuffer->AllocationBase = (void *)marea->base;
   lpBuffer->AllocationProtect = marea->initial_access;
   lpBuffer->RegionSize = marea->length;
   lpBuffer->State = marea->state;
   lpBuffer->Protect = marea->access;
   lpBuffer->Type = marea->type;
   
   return(sizeof(MEMORY_BASIC_INFORMATION));
}

BOOLEAN MmIsNonPagedSystemAddressValid(PVOID VirtualAddress)
{
   UNIMPLEMENTED;
}

BOOLEAN MmIsAddressValid(PVOID VirtualAddress)
/*
 * FUNCTION: Checks whether the given address is valid for a read or write
 * ARGUMENTS:
 *          VirtualAddress = address to check
 * RETURNS: True if the access would be valid
 *          False if the access would cause a page fault
 * NOTES: This function checks whether a byte access to the page would
 *        succeed. Is this realistic for RISC processors which don't
 *        allow byte granular access?
 */
{
   memory_area* marea;
   
   if (VirtualAddress >= (PVOID)KERNEL_BASE)
     {
	marea = find_first_marea(system_memory_area_list_head,
				 (unsigned int)VirtualAddress,1);
     }
   else
     {
	marea = find_first_marea(memory_area_list_head,
				 (unsigned int)VirtualAddress,1);
     } 
   
   if (marea==NULL)
     {
	return(FALSE);
     }
   
   if (marea->access == PAGE_NOACCESS || marea->access & PAGE_GUARD ||
       marea->state == MEM_FREE || marea->state == MEM_RESERVE)
     {
	return(FALSE);
     }
   
   return(TRUE);
}

