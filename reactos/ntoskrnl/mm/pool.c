/*
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         ntoskrnl/mm/pool.c
 * PURPOSE:      Implements the kernel memory pool
 * PROGRAMMER:   David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *               27/05/98: Created
 *               10/06/98: Bug fixes by Iwan Fatahi (i_fatahi@hotmail.com)
 *                         in take_block (if current bigger than required)
 *                         in remove_from_used_list 
 *                         in ExFreePool
 */

/* INCLUDES ****************************************************************/

#include <internal/stddef.h>
#include <internal/mm.h>
#include <internal/hal/page.h>
#include <internal/pool.h>
#include <internal/bitops.h>
#include <internal/kernel.h>

#define NDEBUG
#include <internal/debug.h>

#include <ddk/ntddk.h>

/* TYPES *******************************************************************/

/*
 * fields present at the start of a block (this is for internal use only)
 */
typedef struct _block_hdr
{
        unsigned int size;
        struct _block_hdr* previous;
        struct _block_hdr* next;
} block_hdr;

/* GLOBALS *****************************************************************/

/*
 * Memory managment initalized symbol for the base of the pool
 */
extern unsigned int kernel_pool_base;

/*
 * Pointer to the first block in the free list
 */
static block_hdr* free_list_head = NULL;
static block_hdr* used_list_head = NULL;
static unsigned int nr_free_blocks = 0;
static unsigned int nr_used_blocks = 0;

#define ALLOC_MAP_SIZE (NONPAGED_POOL_SIZE / PAGESIZE)

/*
 * One bit for each page in the kmalloc region
 *      If set then the page is used by a kmalloc block
 */
static unsigned int alloc_map[ALLOC_MAP_SIZE/32]={0,};

/* FUNCTIONS ***************************************************************/

static void validate_free_list(void)
/*
 * FUNCTION: Validate the integrity of the list of free blocks
 */
{
        block_hdr* current=free_list_head;
        unsigned int blocks_seen=0;

        while (current!=NULL)
        {
                unsigned int base_addr = (int)current;
                if (base_addr < (kernel_pool_base) ||
                    (base_addr+current->size) >
                    (kernel_pool_base)+NONPAGED_POOL_SIZE)
                    {		       
                        printk("Block %x found outside pool area\n",current);
		        printk("Size %d\n",current->size);
		        printk("Limits are %x %x\n",kernel_pool_base,
			       kernel_pool_base+NONPAGED_POOL_SIZE);
                        KeBugCheck(KBUG_POOL_FREE_LIST_CORRUPT);
                    }
                blocks_seen++;
                if (blocks_seen > nr_free_blocks)
                {
                        printk("Too many blocks on list\n");
                        KeBugCheck(KBUG_POOL_FREE_LIST_CORRUPT);
                }
//                verify_for_write(base_addr,current->size);                
                if (current->next!=NULL&&current->next->previous!=current)
                {
                        printk("%s:%d:Break in list (current %x next %x "
			       "current->next->previous %x)\n",
                               __FILE__,__LINE__,current,current->next,
			       current->next->previous);
                        KeBugCheck(KBUG_POOL_FREE_LIST_CORRUPT);
                }
                current=current->next;
        }
}

static void validate_used_list(void)
/*
 * FUNCTION: Validate the integrity of the list of used blocks
 */
{
        block_hdr* current=used_list_head;
        unsigned int blocks_seen=0;

        while (current!=NULL)
        {
                unsigned int base_addr = (int)current;
                if (base_addr < (kernel_pool_base) ||
                    (base_addr+current->size) >
                    (kernel_pool_base)+NONPAGED_POOL_SIZE)
                    {
                        printk("Block %x found outside pool area\n",current);
                        for(;;);
                    }
                blocks_seen++;
                if (blocks_seen > nr_used_blocks)
                {
                        printk("Too many blocks on list\n");
                        for(;;);
                }
//                verify_for_write(base_addr,current->size);
                if (current->next!=NULL&&current->next->previous!=current)
                {
                        printk("Break in list (current %x next %x)\n",
                               current,current->next);
                        for(;;);
                }
                current=current->next;
        }

}

static void check_duplicates(block_hdr* blk)
/*
 * FUNCTION: Check a block has no duplicates
 * ARGUMENTS:
 *           blk = block to check
 * NOTE: Bug checks if duplicates are found
 */
{
        unsigned int base = (int)blk;
        unsigned int last = ((int)blk) + +sizeof(block_hdr) + blk->size;

        block_hdr* current=free_list_head;
        while (current!=NULL)
        {
                if ( (int)current > base && (int)current < last ) 
                {
                        printk("intersecting blocks on list\n");
                        for(;;);
                }
                if  ( (int)current < base &&
                      ((int)current + current->size + sizeof(block_hdr))
                      > base )
                {
                        printk("intersecting blocks on list\n");
                        for(;;);
                }
                current=current->next;
        }
        current=used_list_head;
        while (current!=NULL)
        {
                if ( (int)current > base && (int)current < last ) 
                {
                        printk("intersecting blocks on list\n");
                        for(;;);
                }
                if  ( (int)current < base &&
                      ((int)current + current->size + sizeof(block_hdr))
                      > base )
                {
                        printk("intersecting blocks on list\n");
                        for(;;);
                }
                current=current->next;
        }

}

static void validate_kernel_pool(void)
/*
 * FUNCTION: Checks the integrity of the kernel memory heap
 */
{
   block_hdr* current=NULL;
   
   validate_free_list();
   validate_used_list();

   current=free_list_head;
   while (current!=NULL)
     {
	check_duplicates(current);
	current=current->next;
     }
   current=used_list_head;
   while (current!=NULL)
     {
	check_duplicates(current);
	current=current->next;
     }
}

static void add_to_free_list(block_hdr* blk)
/*
 * FUNCTION: add the block to the free list (internal)
 */
{
        blk->next=free_list_head;
        blk->previous=NULL;
        if (free_list_head!=NULL)
        {
                free_list_head->previous=blk;
        }
        free_list_head=blk;
        nr_free_blocks++;
}

static void add_to_used_list(block_hdr* blk)
/*
 * FUNCTION: add the block to the used list (internal)
 */
{
        blk->next=used_list_head;
        blk->previous=NULL;
        if (used_list_head!=NULL)
        {
                used_list_head->previous=blk;
        }
        used_list_head=blk;
        nr_used_blocks++;
}


static void remove_from_free_list(block_hdr* current)
{
        if (current->next==NULL&&current->previous==NULL)
        {
                free_list_head=NULL;                                
        }
        else
        {
                if (current->previous!=NULL)
                {
                        current->previous->next=current->next;
                }
                if (current->next!=NULL)
                {
		   DPRINT("current->previous %x\n",current->previous);
                        current->next->previous=current->previous;
                }
        }
        nr_free_blocks--;
}

#ifdef BROKEN_VERSION_OF_REMOVE_FROM_FREE_LIST
static void remove_from_free_list(block_hdr* current)
{
        if (current->next==NULL&&current->previous==NULL)
        {
                free_list_head=NULL;                                
        }
        else
        {
                if (current->next==NULL)
                {
                        current->previous->next=NULL;
                }
                else
                {
                        current->previous->next=current->next;
                }
                if (current->previous==NULL)
                {
                        current->next->previous=NULL;                            
                }
                else
                {
                        current->next->previous=current->previous;
                }
        }
        nr_free_blocks--;
}
#endif

static void remove_from_used_list(block_hdr* current)
{
        if (current->next==NULL&&current->previous==NULL)
        {
                used_list_head=NULL;                                
        }
        else
        {	   
                if (current->previous==NULL)
                {
		   current->next->previous=NULL;
		   used_list_head=current->next;
                }
                else
                {
		   current->previous->next=current->next;
                }
                if (current->next!=NULL)
                {
                        current->next->previous=current->previous;
                }
                else
                {
                        current->previous->next=NULL;
                }
        }
        nr_used_blocks--;
}


inline static void* block_to_address(block_hdr* blk)
/*
 * FUNCTION: Translate a block header address to the corresponding block
 * address (internal)
 */
{
        return ( (void *) ((int)blk + sizeof(block_hdr)) );
}

inline static block_hdr* address_to_block(void* addr)
{
        return (block_hdr *)
               ( ((int)addr) - sizeof(block_hdr) );
}

static unsigned int alloc_pool_region(unsigned int nr_pages)
/*
 * FUNCTION: Allocates a region of pages within the nonpaged pool area
 */
{
   unsigned int start = 0;
   unsigned int length = 0;
   unsigned int i,j;
   
   DPRINT("alloc_pool_region(nr_pages = %d)\n",nr_pages);

   for (i=1; i<ALLOC_MAP_SIZE;i++)
     {
	if (!test_bit(i%32,&alloc_map[i/32]))
	  {
	     if (length == 0)
	       {
		  start=i;
		  length = 1;
	       }
	     else
	       {
		  length++;
	       }
	     if (length==nr_pages)
	       {
                  DPRINT("found region at %d for %d\n",start,
			 length);
		  for (j=start;j<(start+length);j++)
		    {
                       DPRINT("Writing %x\n",&alloc_map[j/32]);                                        
		       set_bit(j%32,&alloc_map[j/32]);
		    }
                  DPRINT("returning %x\n",(start*PAGESIZE)
			 +kernel_pool_base);
		  return((start*PAGESIZE)+kernel_pool_base);
	       }
	  }
	else
	  {
	     start=0;
	     length=0;
	  }
     }
   printk("CRITICAL: Out of kmalloc space\n");
   for(;;);
   return(0);
}

static block_hdr* grow_kernel_pool(unsigned int size)
/*
 * FUNCTION: Grow the executive heap to accomodate a block of at least 'size'
 * bytes
 */
{
   unsigned int total_size = size + sizeof(block_hdr);
   unsigned int nr_pages = PAGE_ROUND_UP(total_size) / PAGESIZE;
   unsigned int start = alloc_pool_region(nr_pages);
   block_hdr* used_blk=NULL;
   block_hdr* free_blk=NULL;
   int i;
   
   DPRINT("growing heap for block size %d, ",size);
   DPRINT("start %x\n",start);
   
   for (i=0;i<nr_pages;i++)
     {
	set_page(start+(i*PAGESIZE),PA_SYSTEM | PA_WRITE | PA_READ,
		 get_free_page());
     }

   
   if ((PAGESIZE-(total_size%PAGESIZE))>(2*sizeof(block_hdr)))
     {
	used_blk = (struct _block_hdr *)start;
	DPRINT("Creating block at %x\n",start);
        used_blk->size = size;
	add_to_used_list(used_blk);
	
	free_blk = (block_hdr *)(start + sizeof(block_hdr) + size);
	DPRINT("Creating block at %x\n",free_blk);
	free_blk->size = (nr_pages * PAGESIZE) -((sizeof(block_hdr)*2) + size);
	add_to_free_list(free_blk);
     }
   else
     {
	used_blk = (struct _block_hdr *)start;
	used_blk->size = nr_pages * PAGESIZE;
	add_to_used_list(used_blk);
     }
   
   validate_kernel_pool();
   return(used_blk);
}

static void* take_block(block_hdr* current, unsigned int size)
/*
 * FUNCTION: Allocate a used block of least 'size' from the specified
 * free block
 * RETURNS: The address of the created memory block
 */
{
   /*
    * If the block is much bigger than required then split it and
    * return a pointer to the allocated section. If the difference
    * between the sizes is marginal it makes no sense to have the
    * extra overhead 
    */
   if (current->size > (1 + size + sizeof(block_hdr)))
     {
	/*
	 * Replace the bigger block with a smaller block in the
	 * same position in the list
	 */
	block_hdr* free_blk = (block_hdr *)(((int)current)
					    + sizeof(block_hdr) + size);
	free_blk->next = current->next;
	free_blk->previous = current->previous;
	if (current->next) 
	  {
	     current->next->previous = free_blk;
	  }
	if (current->previous)
	  {
	     current->previous->next = free_blk;
	  }
	free_blk->size = current->size - (sizeof(block_hdr) + size);
	if (current==free_list_head)
                {
		   free_list_head=free_blk;
                }
	
	current->size=size;
	add_to_used_list(current);
	
	validate_kernel_pool();
	return(block_to_address(current));
     }
   
   /*
    * Otherwise allocate the whole block
    */
   remove_from_free_list(current);
   add_to_used_list(current);
   
   validate_kernel_pool();
   return(block_to_address(current));
}

asmlinkage VOID ExFreePool(PVOID block)
/*
 * FUNCTION: Releases previously allocated memory
 * ARGUMENTS:
 *        block = block to free
 */
{
   block_hdr* blk=address_to_block(block);
   DPRINT("(%s:%d) freeing block %x\n",__FILE__,__LINE__,blk);
   
   validate_kernel_pool();
   /*
    * Please don't change the order
    */
   remove_from_used_list(blk);
   add_to_free_list(blk);
   
   validate_kernel_pool();
}

#define CACHE_ALIGNMENT (16)

PVOID ExAllocatePool(ULONG type, ULONG size)
/*
 * FUNCTION: Allocates memory from the pool
 * ARGUMENTS:
 *      size = minimum size of the block to be allocated
 *      type = the type of memory to use for the block 
 * RETURNS:
 *      the address of the block if it succeeds
 */
{
   PVOID Block;
   
   if (type == NonPagedPoolCacheAligned || 
       type == NonPagedPoolCacheAlignedMustS)
     {
	size = size + CACHE_ALIGNMENT;
     }
   
   switch(type)
     {
      case NonPagedPool:
      case NonPagedPoolMustSucceed:
      case NonPagedPoolCacheAligned:
      case NonPagedPoolCacheAlignedMustS:
	Block = ExAllocateNonPagedPool(type,size);
	break;
	
      case PagedPool:
      case PagedPoolCacheAligned:
	Block = ExAllocatePagedPool(type,size);
	break;
	
      default:
	return(NULL);
     };
   
   if ((type==NonPagedPoolMustSucceed || type==NonPagedPoolCacheAlignedMustS)
       && Block==NULL)     
     {
	KeBugCheck(MUST_SUCCEED_POOL_EMPTY);
     }
   if (type == NonPagedPoolCacheAligned || 
       type == NonPagedPoolCacheAlignedMustS)
     {
	Block = Block + CACHE_ALIGNMENT - (((int)Block)%CACHE_ALIGNMENT);
     }
   return(Block);
}

static PVOID ExAllocatePagedPool(ULONG type, ULONG size)
{
   UNIMPLEMENTED;
}

static PVOID ExAllocateNonPagedPool(ULONG type, ULONG size)
{
   block_hdr* current=NULL;
   
   DPRINT("kmalloc(size %d)\n",size);
   validate_kernel_pool();
   
   /*
    * accomodate this useful idiom
    */
   if (size==0)
     {
	return(NULL);
     }
   
   /*
    * Look for an already created block of sufficent size
    */
   current=free_list_head;
   
   while (current!=NULL)
     {
	DPRINT("current %x size %x next %x\n",current,current->size,
	       current->next);
	if (current->size>=size)
	  {
	     DPRINT("found block %x of size %d\n",current,size);
	     return(take_block(current,size));
	  }
	current=current->next;
     }
   
   /*
    * Otherwise create a new block
    */
   return(block_to_address(grow_kernel_pool(size)));
}

PVOID ExAllocatePoolWithQuota(POOL_TYPE PoolType, ULONG NumberOfBytes)
{
   PVOID Block;
   PKTHREAD current = KeGetCurrentThread();
   
   Block = ExAllocatePool(PoolType,NumberOfBytes);
   switch(PoolType)
     {
      case NonPagedPool:
      case NonPagedPoolMustSucceed:
      case NonPagedPoolCacheAligned:
      case NonPagedPoolCacheAlignedMustS:
//	current->NPagedPoolQuota = current->NPagedPoolQuota - NumberOfBytes;
	break;
	
      case PagedPool:
      case PagedPoolCacheAligned:
//	current->PagedPoolQuota = current->PagedPoolQuota - NumberOfBytes;
	break;	
     };
   return(Block);
}
   
PVOID ExAllocatePoolWithQuotaTag(POOL_TYPE PoolType, ULONG NumberOfBytes,
				 ULONG Tag)
{
   PVOID Block;
   Block=ExAllocatePoolWithQuota(PoolType,NumberOfBytes+sizeof(ULONG));
   ((ULONG *)Block)[0]=Tag;
   return(Block+4);
}

PVOID ExAllocatePoolWithTag(POOL_TYPE PoolType, ULONG NumberOfBytes,
				 ULONG Tag)
/*
 * FUNCTION: Allocates pool memory and inserts a caller supplied tag before
 * the block allocated
 * ARGUMENTS:
 *        PoolType = Type of memory to allocate
 *        NumberOfBytes = Number of bytes to allocate
 *        Tag = Tag 
 * RETURNS: The address of the block allocated
 */
{
   PVOID Block;
   Block=ExAllocatePool(PoolType,NumberOfBytes+sizeof(ULONG));
   ((ULONG *)Block)[0]=Tag;
   return(Block+4);
}
