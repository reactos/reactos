/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            mkernel/mm/lock.c
 * PURPOSE:         Locking/unlocking virtual memory areas
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 */

BOOL VirtualUnlock(LPVOID lpAddress, DWORD cbSize)
/*
 * FUNCTION: Unlocks pages from the virtual address space of the current
 * process
 * ARGUMENTS:
 *         lpAddress = Beginning of the region to unlock
 *         cbSize = Size (in bytes) of the region to unlock
 * RETURNS: Success or failure
 */
{
   return(FALSE);
}

BOOL VirtualLock(LPVOID lpvAddress, DWORD cbSize)
/*
 * FUNCTION: Prevents range of memory from being swapped out
 * RECEIVES:
 *              lpvAddress - The base of the range to lock
 *              cbSize - The size of the range to lock
 * RETURNS:
 *              TRUE - the function succeeds
 *              FALSE - the function failed (use GetLastError for details)
 *
 * NOTES: I'm guessing the kernel loads every page as well as locking it.
 */
{
   unsigned int first_page = PAGE_ROUND_DOWN((int)lpvAddress);
   unsigned int length = PAGE_ROUND_DOWN( ((int)lpvAddress) + cbSize) -
                         first_page;
   memory_area* marea=NULL;
   memory_area* current=NULL;
   unsigned int i;
   
   /*
    * Check the process isn't trying to lock too much
    */
   if ( ((length/PAGESIZE)+1) > 30)
     {
	SetLastError(0);
	return(FALSE);
     }

   /*
    * Find the corresponding vmarea(s)
    */
   marea = find_first_marea(memory_area_list_head,first_page,
			    length);
   if (marea==NULL)
     {
	SetLastError(0);
	return(FALSE);
     }
   
   /*
    * Check the memory areas are committed, continuous and not
    * PAGE_NOACCESS
    */
   current=marea;
   if (current->base != first_page)
     {
	SetLastError(0);
	return(FALSE);
     }
   while (current!=NULL && current->base < (first_page+length) )
     {
	if (!(current->state & MEM_COMMIT) ||
	    current->access & PAGE_NOACCESS)
	  {
	     SetLastError(0);
	     return(FALSE);
	  }
	if ( current->next==NULL)
	  {
	     if ((current->base + current->length) !=
		 (first_page+length) )
	       {
		  SetLastError(0);
		  return(FALSE);
	       }
	  }
	else
	  {
	     if ( (current->base+current->length) !=
		 current->next->base)
	       {
		  SetLastError(0);
		  return(FALSE);
	       }
	  }
	current=current->next;
     }
   
   /*
    * Lock/load the areas in memory
    * (the pages aren't loaded just by touching them to avoid the
    * overhead of a page fault)
    */
   current=marea;
   while (current!=NULL && current->base < (first_page+length) )
     {
	marea->lock = TRUE;
	for (i=0; i<current->length; i++)
	  {
	     if (!current->load_page(marea,i))
                        {
			   /*
			    * If the page couldn't be loaded we unlock
			    * the locked pages and abort
			    */
			   VirtualUnlock(lpvAddress,
					 current->base+i-PAGESIZE);
			   SetLastError(0);
			   return(FALSE);
                        }
	  }
     }
   
   return(TRUE);
}
