/*
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         ntoskrnl/mm/pool.c
 * PURPOSE:      Implements the kernel memory pool
 * PROGRAMMER:   David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *               27/05/98: Created
 *               10/06/98: Bug fixes by Iwan Fatahi (i_fatahi@hotmail.com)
 *                         in take_block (if current bigger than required)
 *                         in remove_from_used_list 
 *                         in ExFreePool
 *               23/08/98: Fixes from Robert Bergkvist (fragdance@hotmail.com)
 */

/* INCLUDES ****************************************************************/

#include <string.h>
#include <internal/string.h>
#include <internal/stddef.h>
#include <internal/mm.h>
#include <internal/mmhal.h>
#include <internal/bitops.h>
#include <internal/ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>

#include <ddk/ntddk.h>


#if 0
#define VALIDATE_POOL validate_kernel_pool()
#else
#define VALIDATE_POOL
#endif

#if 0
#define POOL_TRACE(args...) do { DbgPrint(args); } while(0);
#else
#define POOL_TRACE(args...)
#endif

/* TYPES *******************************************************************/

#define BLOCK_HDR_MAGIC (0xdeadbeef)

/*
 * fields present at the start of a block (this is for internal use only)
 */
typedef struct _block_hdr
{
   ULONG magic;
   ULONG size;
   struct _block_hdr* previous;
   struct _block_hdr* next;
   ULONG tag;
} block_hdr;

/* GLOBALS *****************************************************************/

/*
 * Memory managment initalized symbol for the base of the pool
 */
static unsigned int kernel_pool_base = 0;

/*
 * Pointer to the first block in the free list
 */
static block_hdr* free_list_head = NULL;
static block_hdr* used_list_head = NULL;
static ULONG nr_free_blocks;
ULONG EiNrUsedBlocks = 0;

#define ALLOC_MAP_SIZE (NONPAGED_POOL_SIZE / PAGESIZE)

/*
 * One bit for each page in the kmalloc region
 *      If set then the page is used by a kmalloc block
 */
static unsigned int alloc_map[ALLOC_MAP_SIZE/32]={0,};

unsigned int EiFreeNonPagedPool = 0;
unsigned int EiUsedNonPagedPool = 0;

/* FUNCTIONS ***************************************************************/

VOID ExInitNonPagedPool(ULONG BaseAddress)
{
   kernel_pool_base=BaseAddress;
}

#if 0
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

	if (current->magic != BLOCK_HDR_MAGIC)
	  {
	     DbgPrint("Bad block magic (probable pool corruption) at %x\n",
		      current);
	     KeBugCheck(KBUG_POOL_FREE_LIST_CORRUPT);
	  }
	
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
	
	if (current->magic != BLOCK_HDR_MAGIC)
	  {
	     DbgPrint("Bad block magic (probable pool corruption) at %x\n",
		      current);
	     KeBugCheck(KBUG_POOL_FREE_LIST_CORRUPT);
	  }
	if (base_addr < (kernel_pool_base) ||
	    (base_addr+current->size) >
	    (kernel_pool_base)+NONPAGED_POOL_SIZE)
	  {
	     printk("Block %x found outside pool area\n",current);
	     for(;;);
	  }
	blocks_seen++;
	if (blocks_seen > EiNrUsedBlocks)
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
	if (current->magic != BLOCK_HDR_MAGIC)
	  {
	     DbgPrint("Bad block magic (probable pool corruption) at %x\n",
		      current);
	     KeBugCheck(KBUG_POOL_FREE_LIST_CORRUPT);
	  }

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
#endif

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
        EiNrUsedBlocks++;
}


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
	   else if (current->previous==NULL)
	     {
		current->next->previous=NULL;
		free_list_head=current->next;
	     }
	   else
	     {
		current->next->previous=current->previous;
		current->previous->next=current->next;
	     }
        }
        nr_free_blocks--;
}


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
        EiNrUsedBlocks--;
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
   
   OLD_DPRINT("alloc_pool_region(nr_pages = %d)\n",nr_pages);

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
                  OLD_DPRINT("found region at %d for %d\n",start,
			 length);
		  for (j=start;j<(start+length);j++)
		    {
		       set_bit(j%32,&alloc_map[j/32]);
		    }
                  OLD_DPRINT("returning %x\n",(start*PAGESIZE)
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
   printk("CRITICAL: Out of non-paged pool space\n");
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
   
   OLD_DPRINT("growing heap for block size %d, ",size);
   OLD_DPRINT("start %x\n",start);
   
   for (i=0;i<nr_pages;i++)
     {
	MmSetPage(NULL,
		  (PVOID)(start + (i*PAGESIZE)),
		  PAGE_READWRITE,
		  get_free_page());
     }

   
   if ((PAGESIZE-(total_size%PAGESIZE))>(2*sizeof(block_hdr)))
     {
	used_blk = (struct _block_hdr *)start;
	OLD_DPRINT("Creating block at %x\n",start);
	used_blk->magic = BLOCK_HDR_MAGIC;
        used_blk->size = size;
	add_to_used_list(used_blk);
	
	free_blk = (block_hdr *)(start + sizeof(block_hdr) + size);
	OLD_DPRINT("Creating block at %x\n",free_blk);
	free_blk->magic = BLOCK_HDR_MAGIC;
	free_blk->size = (nr_pages * PAGESIZE) -((sizeof(block_hdr)*2) + size);
	add_to_free_list(free_blk);
	
	EiFreeNonPagedPool = EiFreeNonPagedPool + free_blk->size;
	EiUsedNonPagedPool = EiUsedNonPagedPool + used_blk->size;
     }
   else
     {
	used_blk = (struct _block_hdr *)start;
	used_blk->magic = BLOCK_HDR_MAGIC;
	used_blk->size = nr_pages * PAGESIZE;
	add_to_used_list(used_blk);
	
	EiUsedNonPagedPool = EiUsedNonPagedPool + used_blk->size;
     }
   
   VALIDATE_POOL;
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
	block_hdr* free_blk;
	
	EiFreeNonPagedPool = EiFreeNonPagedPool - current->size;
	
	/*
	 * Replace the bigger block with a smaller block in the
	 * same position in the list
	 */
        free_blk = (block_hdr *)(((int)current)
				 + sizeof(block_hdr) + size);		
	free_blk->magic = BLOCK_HDR_MAGIC;
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
	
	EiUsedNonPagedPool = EiUsedNonPagedPool + current->size;
	EiFreeNonPagedPool = EiFreeNonPagedPool + free_blk->size;
	
	VALIDATE_POOL;
	return(block_to_address(current));
     }
   
   /*
    * Otherwise allocate the whole block
    */
   remove_from_free_list(current);
   add_to_used_list(current);
   
   EiFreeNonPagedPool = EiFreeNonPagedPool - current->size;
   EiUsedNonPagedPool = EiUsedNonPagedPool + current->size;
   
   VALIDATE_POOL;
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
   OLD_DPRINT("(%s:%d) freeing block %x\n",__FILE__,__LINE__,blk);
   
   POOL_TRACE("ExFreePool(block %x), size %d, caller %x\n",block,blk->size,
            ((PULONG)&block)[-1]);
   
   VALIDATE_POOL;
   
   if (blk->magic != BLOCK_HDR_MAGIC)
     {
	DbgPrint("ExFreePool of non-allocated address %x\n",block);
	KeBugCheck(0);
	return;
     }
   
   /*
    * Please don't change the order
    */
   remove_from_used_list(blk);
   add_to_free_list(blk);
   
   EiUsedNonPagedPool = EiUsedNonPagedPool - blk->size;
   EiFreeNonPagedPool = EiFreeNonPagedPool + blk->size;   
   
   VALIDATE_POOL;
}

PVOID ExAllocateNonPagedPoolWithTag(ULONG type, 
				    ULONG size, 
				    ULONG Tag,
				    PVOID Caller)
{
   block_hdr* current = NULL;
   PVOID block;
   block_hdr* best = NULL;
   
   POOL_TRACE("ExAllocatePool(NumberOfBytes %d) caller %x ",
	      size,Caller);
   
//   DbgPrint("Blocks on free list %d\n",nr_free_blocks);
//   DbgPrint("Blocks on used list %d\n",eiNrUsedblocks);
//   OLD_DPRINT("ExAllocateNonPagedPool(type %d, size %d)\n",type,size);
   VALIDATE_POOL;
   
   /*
    * accomodate this useful idiom
    */
   if (size==0)
     {
	POOL_TRACE("= NULL\n");
	return(NULL);
     }
   
   /*
    * Look for an already created block of sufficent size
    */
   current=free_list_head;
   
//   defrag_free_list();
   
   while (current!=NULL)
     {
	OLD_DPRINT("current %x size %x next %x\n",current,current->size,
	       current->next);
	if (current->size>=size &&
	    (best == NULL ||
	     current->size < best->size)) 
	  {
	     best = current;
	  }
	current=current->next;
     }
   if (best != NULL)
     {
	OLD_DPRINT("found block %x of size %d\n",best,size);
	block=take_block(best,size);
	VALIDATE_POOL;
	memset(block,0,size);
	POOL_TRACE("= %x\n",block);
	return(block);
     }
	  
   
   /*
    * Otherwise create a new block
    */
   block=block_to_address(grow_kernel_pool(size));
   VALIDATE_POOL;
   memset(block,0,size);
   POOL_TRACE("= %x\n",block);
   return(block);
}

