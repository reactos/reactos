/*
 * kernel/heap.c
 * Copyright (C) 1996, Onno Hovers, All rights reserved
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this software; see the file COPYING.LIB. If
 * not, write to the Free Software Foundation, Inc., 675 Mass Ave,
 * Cambridge, MA 02139, USA.
 *
 * Win32 heap functions (HeapXXX).
 *
 */

/*
 * Adapted for the ReactOS system libraries by David Welch (welch@mcmail.com)
 * Put the type definitions of the heap in a seperate header. Boudewijn Dekker
 */

#include <kernel32/proc.h>
#include <kernel32/kernel32.h>
#include <kernel32/heap.h>
#include <internal/string.h>


static HEAP_BUCKET __HeapDefaultBuckets[]=
{
  { NULL,  16, 18, 504  },
  { NULL,  24, 30, 1016 },
  { NULL,  32, 24, 1016 },
  { NULL,  48, 17, 1016 },
  { NULL,  64, 27, 2040 },
  { NULL,  96, 19, 2040 },
  { NULL, 128, 29, 4088 },
  { NULL, 256, 15, 4088 },
};

PHEAP	__ProcessHeap;

static BOOL   __HeapCommit(PHEAP pheap, LPVOID start, LPVOID end);
static BOOL   __HeapDecommit(PHEAP pheap, LPVOID start, LPVOID end);
static LPVOID __HeapAlloc(PHEAP pheap, ULONG flags, ULONG size, ULONG tag);
static VOID   __HeapFreeRest(PHEAP pheap, PHEAP_BLOCK pfree, ULONG allocsize,
                             ULONG newsize);
static LPVOID __HeapReAlloc(PHEAP pheap, ULONG flags, LPVOID pold, DWORD size);
static BOOL   __HeapFree(PHEAP pheap, ULONG flags, LPVOID pmem);
static PHEAP_SUBALLOC __HeapAllocSub(PHEAP pheap, PHEAP_BUCKET pbucket);
static LPVOID __HeapAllocFragment(PHEAP pheap, ULONG flags, ULONG size);
static LPVOID __HeapReAllocFragment(PHEAP pheap, ULONG flags,
                                    LPVOID pold, ULONG size);
static BOOL   __HeapFreeFragment(PHEAP pheap, ULONG flags, LPVOID ptr);
static PHEAP  __HeapPrepare(LPVOID base, ULONG minsize, ULONG maxsize,
                            ULONG flags);



/*********************************************************************
*                      __HeapCommit                                  *
*                                                                    *
* commits a range of memory in the heap                              *
*********************************************************************/
static BOOL __HeapCommit(PHEAP pheap, LPVOID start, LPVOID end)
{
   dprintf("__HeapCommit( 0x%lX, 0x%lX, 0x%lX)\n",
           (ULONG) pheap, (ULONG) start, (ULONG) end);

   if(end >= pheap->LastBlock)
      pheap->LastBlock=end;
   if (VirtualAlloc(start,end-start,MEM_COMMIT,PAGE_READWRITE)!=start)
     {
	return(FALSE);
     }
   return(TRUE);
}

/*********************************************************************
*                      __HeapDecommit                                *
*                                                                    *
* decommits a range of memory in the heap                            *
*********************************************************************/
static BOOL __HeapDecommit(PHEAP pheap, LPVOID start, LPVOID end)
{
   dprintf("__HeapDecommit( 0x%lX, 0x%lX, 0x%lX)\n",
           (ULONG) pheap, (ULONG) start, (ULONG) end);
#ifdef NOT
   __VirtualDump();
#endif
   if((end >= pheap->LastBlock)&&(start<= pheap->LastBlock))
      pheap->LastBlock=start;
   
   return(VirtualFree(start,end-start,MEM_RESERVE));
}

/*********************************************************************
*                      __HeapAlloc                                   *
*                                                                    *
* allocates a range of memory from the heap                          *
*********************************************************************/
static LPVOID __HeapAlloc(PHEAP pheap, ULONG flags, ULONG size, ULONG tag)
{
   PHEAP_BLOCK	pfree;
   PHEAP_BLOCK	palloc;
   PHEAP_BLOCK	pnext;
   LPVOID	commitstart;
   LPVOID	commitend;
   ULONG	freesize;
   ULONG	allocsize;
   
   dprintf("__HeapAlloc(pheap %x, flags %x, size %d, tag %x)\n",
	   pheap,flags,size,tag);
   
   pfree=&(pheap->Start);
   allocsize=SIZE_ROUND(size);
   freesize=HEAP_SIZE(pfree);
   /* look for a free region of memory: simple First Fit */
   while( HEAP_ISALLOC(pfree) || ( freesize<allocsize ))
   {
      pfree=HEAP_NEXT(pfree);
      if((LPVOID) pfree>=pheap->End)
         return __ErrorReturnNull(ERROR_OUTOFMEMORY);
      freesize=HEAP_SIZE(pfree);
   }
   palloc=pfree;

   if(freesize>allocsize)
   {
      /* commit necessary memory */
      commitstart=(LPVOID) ROUNDDOWN((ULONG) palloc+HEAP_ADMIN_SIZE,PAGESIZE);
      commitend  =(LPVOID) ROUNDUP  ((ULONG) palloc+
                                     allocsize+2*HEAP_ADMIN_SIZE, PAGESIZE);
      if(commitstart<commitend)
         if(!__HeapCommit(pheap, commitstart, commitend))
             return __ErrorReturnNull(ERROR_OUTOFMEMORY);

      /* split this block in two */
      pfree= (LPVOID) palloc+ HEAP_ADMIN_SIZE + allocsize;

      /* update admin */
      pfree->Size    =(freesize-allocsize-HEAP_ADMIN_SIZE) | HEAP_FREE_TAG;
      pfree->PrevSize=(LPVOID)pfree-(LPVOID)palloc;

      pnext=HEAP_NEXT(pfree);
      if((LPVOID) pnext < pheap->End )
         pnext->PrevSize=freesize-allocsize;
   }
   else
   {
      /* commit necessary memory */
      commitstart=(LPVOID) ROUNDDOWN((ULONG) palloc+HEAP_ADMIN_SIZE, PAGESIZE);
      commitend  =(LPVOID) ROUNDUP((ULONG) palloc+HEAP_ADMIN_SIZE +allocsize,
                           PAGESIZE);
      if(commitstart<commitend)
         if(!__HeapCommit(pheap, commitstart, commitend))
            return __ErrorReturnNull(ERROR_OUTOFMEMORY);
   }
   /* update our administration */
   palloc->Size= size | tag;
   if((flags | pheap->Flags)& HEAP_ZERO_MEMORY)
      FillMemory((LPVOID)palloc+HEAP_ADMIN_SIZE, allocsize, 0);
   return (LPVOID)palloc+HEAP_ADMIN_SIZE;
}

/*********************************************************************
*                      __HeapFreeRest                                *
*                                                                    *
* used by realloc to free a part of the heap                         *
*********************************************************************/
static VOID __HeapFreeRest(PHEAP pheap, PHEAP_BLOCK pfree,
                           ULONG allocsize, ULONG newsize)
{
   PHEAP_BLOCK	pnext;
    
   if(allocsize==newsize)
   {
       pfree->PrevSize=allocsize+HEAP_ADMIN_SIZE;
       return;
   }
   
   pfree->Size     = (allocsize-newsize-HEAP_ADMIN_SIZE) | HEAP_FREE_TAG;
   pfree->PrevSize = newsize+HEAP_ADMIN_SIZE;

   pnext=HEAP_NEXT(pfree);
   /* if there is a free region of memory after us, join it */
   if(((LPVOID) pnext< pheap->End)&& HEAP_ISFREE(pnext))
   {
      pfree->Size = (HEAP_SIZE(pfree)+HEAP_SIZE(pnext) + HEAP_ADMIN_SIZE) |
                    HEAP_FREE_TAG;

      pnext->Size=0;
      pnext->PrevSize=0;
   }

   pnext=HEAP_NEXT(pfree);
   if((LPVOID) pnext< pheap->End)
      pnext->PrevSize=(LPVOID)pnext-(LPVOID)pfree;
}

/*********************************************************************
*                      __HeapReAlloc                                 *
*                                                                    *
* reallocates a range of memory from the heap                        *
*********************************************************************/

static LPVOID __HeapReAlloc(PHEAP pheap, ULONG flags, LPVOID pold, DWORD size)
{
   PHEAP_BLOCK	prealloc=(PHEAP_BLOCK)((LPVOID)pold-HEAP_ADMIN_SIZE);
   PHEAP_BLOCK	pnext;
   LPVOID	pmem;
   LPVOID	commitstart;
   LPVOID	commitend;
   ULONG	allocsize;
   ULONG	newsize;
   ULONG	oldsize;

   /* check that this is a valid allocated block */
   if(!HEAP_ISALLOC(prealloc))
      return __ErrorReturnNull(ERROR_INVALID_PARAMETER);

   allocsize = HEAP_RSIZE(prealloc);
   newsize =   SIZE_ROUND(size);
   /*
    *  cases: size=0				free memory
    *       [ size<HEAP_FRAGMENT_THRESHOLD	realloc ]
    *         newsize<previous size		free rest
    *         newsize=previous size		nop
    *         newsize>previous size		try to merge
    *						else realloc
    */
   if(size==0)
   {
      dprintf("__HeapReAlloc: freeing memory\n");
      __HeapFree(pheap, flags, pold);
      return NULL;
   }
#ifdef NOT
   else if(size < HEAP_FRAGMENT_THRESHOLD)
   {
      /* alloc a new fragment */
      pmem=__HeapAllocFragment(pheap, flags, size);
      if(pmem)
	 CopyMemory(pmem, pold, size);
      return pmem;
   }
#endif
   else if(newsize < allocsize )
   {
      dprintf("__HeapReAlloc: shrinking memory\n");
      /* free remaining region of memory */
      prealloc->Size=size | HEAP_NORMAL_TAG;
      pnext=HEAP_NEXT(prealloc);
      __HeapFreeRest(pheap, pnext, allocsize, newsize);

      /* decommit unnecessary memory */
      commitstart=(LPVOID) ROUNDUP((ULONG) pnext+HEAP_ADMIN_SIZE ,PAGESIZE);
      commitend  =(LPVOID) ROUNDDOWN((ULONG) pnext+HEAP_ADMIN_SIZE+
                                      HEAP_SIZE(pnext), PAGESIZE);
      if(commitstart<commitend)
         __HeapDecommit(pheap, commitstart, commitend);
      return pold;
   }
   else if(newsize == allocsize )
   {
      dprintf("__HeapReAlloc: no changes\n");
      /* nothing to do */
      prealloc->Size= size | HEAP_NORMAL_TAG;
      return pold;
   }
   else if(newsize > allocsize)
   {
      /* try to merge */
      pnext=HEAP_NEXT(prealloc);

      if(((LPVOID) pnext< pheap->End)&& HEAP_ISFREE(pnext) &&
         (HEAP_SIZE(pnext) + HEAP_ADMIN_SIZE >=newsize-allocsize))
      {
         dprintf("__HeapReAlloc: joining memory\n");
         oldsize=HEAP_SIZE(prealloc);
         prealloc->Size=size | HEAP_NORMAL_TAG;

         /* commit new memory if necessary */
         commitstart=(LPVOID) ROUNDDOWN((ULONG) pnext+HEAP_ADMIN_SIZE,
                                        PAGESIZE);
         commitend  =(LPVOID) ROUNDUP((ULONG) pnext+newsize-allocsize+
                                         HEAP_ADMIN_SIZE, PAGESIZE);
         if(commitstart<commitend)
            if(!__HeapCommit(pheap, commitstart, commitend))
               return __ErrorReturnNull(ERROR_OUTOFMEMORY);

         __HeapFreeRest(pheap, HEAP_NEXT(prealloc),
                           allocsize+HEAP_ADMIN_SIZE+HEAP_SIZE(pnext), newsize);
      
         if((flags|pheap->Flags)&HEAP_ZERO_MEMORY)
            memset(pold+oldsize, 0, size-oldsize);
         return pold;
      }
      else
      {
         if((flags&HEAP_REALLOC_IN_PLACE_ONLY)==0)
         {
            dprintf("__HeapReAlloc: allocating new memory\n");
            /* alloc a new piece of memory */
            oldsize=HEAP_SIZE(prealloc);
            pmem=__HeapAlloc(pheap, flags, size, HEAP_NORMAL_TAG);
            if(pmem)
	       CopyMemory(pmem, pold, oldsize);
            if((flags|pheap->Flags)&HEAP_ZERO_MEMORY)
               memset(pmem + oldsize, 0, size-oldsize);
            __HeapFree(pheap, flags, pold);
            return pmem;
         }
         else
            return __ErrorReturnNull(ERROR_OUTOFMEMORY);
      }
   }
   return NULL;
}

/*********************************************************************
*                      __HeapFree                                    *
*                                                                    *
* frees a range of memory from the heap                              *
*********************************************************************/

static BOOL __HeapFree(PHEAP pheap, ULONG flags, LPVOID ptr)
{
   PHEAP_BLOCK	pfree=(PHEAP_BLOCK)((LPVOID)ptr-HEAP_ADMIN_SIZE);
   PHEAP_BLOCK	pprev;
   PHEAP_BLOCK	pnext;
   LPVOID	decommitstart;
   LPVOID	decommitend;

   /* check that this is a valid allocated block */
   if(!HEAP_ISALLOC(pfree))
      return FALSE;

   pfree->Size = HEAP_RSIZE(pfree) | HEAP_FREE_TAG;

   /* if there is a free region of memory before us, join it */
   pprev=HEAP_PREV(pfree);
   pnext=HEAP_NEXT(pfree);
   if((pprev!=pfree) && HEAP_ISFREE(pprev))
   {
      pprev->Size = (HEAP_SIZE(pprev)+HEAP_SIZE(pfree) + HEAP_ADMIN_SIZE) |
                    HEAP_FREE_TAG;
      if((LPVOID) pnext<pheap->End)
         pnext->PrevSize=(LPVOID)pnext-(LPVOID)pprev;

      pfree->Size=0;
      pfree->PrevSize=0;
      pfree=pprev;
   }
   /* if there is a free region of memory after us, join it */
   if(((LPVOID) pnext< pheap->End)&& HEAP_ISFREE(pnext))
   {
      pfree->Size = (HEAP_SIZE(pfree)+HEAP_SIZE(pnext) + HEAP_ADMIN_SIZE) |
                    HEAP_FREE_TAG;

      pnext->Size=0;
      pnext->PrevSize=0;

      pnext=HEAP_NEXT(pfree);
      if((LPVOID) pnext< pheap->End)
         pnext->PrevSize=(LPVOID)pnext-(LPVOID)pfree;
   }

   /* decommit unnecessary memory */
   decommitstart=(LPVOID) ROUNDUP((ULONG) pfree+HEAP_ADMIN_SIZE ,PAGESIZE);
   decommitend  =(LPVOID) ROUNDDOWN((ULONG) pfree+HEAP_ADMIN_SIZE+
                                    HEAP_SIZE(pfree), PAGESIZE);
   if(decommitstart<decommitend)
      __HeapDecommit(pheap, decommitstart, decommitend);

   return TRUE;
}

/*********************************************************************
*                      __HeapAllocSub                                *
*                                                                    *
* allocates a range of memory that is used to allocate small         *
* fragments                                                          *
*********************************************************************/
PHEAP_SUBALLOC __HeapAllocSub(PHEAP pheap, PHEAP_BUCKET pbucket)
{
   INT			i;
   INT			add;
   PHEAP_SUBALLOC	psub;
   PHEAP_FRAGMENT	pprev;
   PHEAP_FRAGMENT	pnext;
   PHEAP_FRAGMENT	palloc;

   psub=(PHEAP_SUBALLOC) __HeapAlloc(pheap, 0, pbucket->TotalSize,
                                     HEAP_SUB_TAG);
   if(!psub)
      return __ErrorReturnNull(ERROR_OUTOFMEMORY);

   /* initialize suballoc */
   palloc=(PHEAP_FRAGMENT) ((LPVOID)psub + sizeof(HEAP_SUBALLOC));
   psub->FirstFree=palloc;
   psub->NumberFree=pbucket->Number;
   psub->Bitmap=0;
   psub->Next=pbucket->FirstFree;
   psub->Prev=NULL;
   psub->Bucket=pbucket;
   pbucket->FirstFree=psub;

   /* initialize free fragments */
   add=pbucket->Size+HEAP_FRAG_ADMIN_SIZE;
   pprev=NULL;
   for(i=0;i<pbucket->Number;i++)
   {
      pnext=(PHEAP_FRAGMENT)((LPVOID)palloc+add);
      palloc->Magic=HEAP_FRAG_MAGIC;
      palloc->Number=i;
      palloc->Size=pbucket->Size;
      palloc->Sub=psub;
      palloc->FreeNext=pnext;
      palloc->FreePrev=pprev;
      pprev=palloc;
      palloc=pnext;
   }
   pprev->FreeNext=NULL;
   return psub;
}

/*********************************************************************
*                      __HeapAllocFragment                           *
*                                                                    *
* allocates a small fragment of memory from the heap                 *
*********************************************************************/
static LPVOID __HeapAllocFragment(PHEAP pheap, ULONG flags, ULONG size )
{
   PHEAP_BUCKET		pbucket;
   PHEAP_SUBALLOC	psub;
   PHEAP_FRAGMENT	palloc;
   INT			nalloc;

   /* get bucket size */
   pbucket=pheap->Bucket;
   while(size>pbucket->Size)
   {
      pbucket++;
   }
   /* get suballoc */
   psub   = pbucket->FirstFree;
   if(!psub)
      psub = __HeapAllocSub(pheap, pbucket);
   if(!psub)
      return NULL;

   /* do our bookkeeping */
   palloc = psub->FirstFree;
   psub->FirstFree = palloc->FreeNext;
   nalloc = palloc->Number;
   psub->NumberFree--;
   psub->Bitmap|=(1<<nalloc);

   /* advance freelist */
   if(!psub->NumberFree)
      pbucket->FirstFree=psub->Next;

   /* initialize allocated block */
   palloc->Magic=HEAP_FRAG_MAGIC;
   palloc->Size=size;

   if((flags|pheap->Flags)&HEAP_ZERO_MEMORY)
      memset((LPVOID)palloc+HEAP_FRAG_ADMIN_SIZE, 0, pbucket->Size);
   return (LPVOID) palloc+HEAP_FRAG_ADMIN_SIZE;
}

/*********************************************************************
*                      __HeapReAllocFragment                         *
*                                                                    *
* reallocates a small fragment of memory                             *
*********************************************************************/
static LPVOID __HeapReAllocFragment(PHEAP pheap, ULONG flags,
                                    LPVOID pold, ULONG size )
{
   PHEAP_BUCKET		pbucket;
   PHEAP_SUBALLOC	psub;
   PHEAP_FRAGMENT	pfrag=(PHEAP_FRAGMENT)
                               ((LPVOID)pold-HEAP_FRAG_ADMIN_SIZE);
   LPVOID		pmem;

   /* sanity checks */
   if(pfrag->Magic!=HEAP_FRAG_MAGIC)
      return __ErrorReturnNull(ERROR_INVALID_PARAMETER);

   /* get bucket size */
   psub=pfrag->Sub;
   pbucket=psub->Bucket;
   if(size<=pbucket->Size)
   {
      pfrag->Size=size;
      return pold;
   }
   else
   {
      if((flags&HEAP_REALLOC_IN_PLACE_ONLY)==0)
      {
         /* alloc a new piece of memory */
         if(size>HEAP_FRAGMENT_THRESHOLD)
            pmem=__HeapAlloc(pheap, flags, size, HEAP_NORMAL_TAG);
         else
            pmem=__HeapAllocFragment(pheap, flags, size);

         if(pmem)
	    CopyMemory(pmem, pold, size);
         if((flags|pheap->Flags)&HEAP_ZERO_MEMORY)
            memset(pmem+pfrag->Size, 0, size-pfrag->Size);

         __HeapFreeFragment(pheap, flags, pold);
         return pmem;
      }
   }
   return NULL;
}

/*********************************************************************
*                      __HeapFreeFragment                            *
*                                                                    *
* frees a small fragment of memory                                   *
*********************************************************************/
static BOOL __HeapFreeFragment(PHEAP pheap, ULONG flags, LPVOID pfree )
{
   PHEAP_BUCKET		pbucket;
   PHEAP_SUBALLOC	psub;
   PHEAP_FRAGMENT	pfrag=(PHEAP_FRAGMENT)
                               ((LPVOID)pfree-HEAP_FRAG_ADMIN_SIZE);
   INT			nalloc;

   /* sanity checks */
   if(pfrag->Magic!=HEAP_FRAG_MAGIC)
      return __ErrorReturnFalse(ERROR_INVALID_PARAMETER);

   /* get bucket size */
   psub=pfrag->Sub;
   pbucket=psub->Bucket;

   nalloc=pfrag->Number;
   if((psub->Bitmap&(1<<nalloc))==0)
      return __ErrorReturnFalse(ERROR_INVALID_PARAMETER);
   psub->NumberFree++;
   if(psub->NumberFree==pbucket->Number)
   {
      /* free suballoc */
      if(psub==pbucket->FirstFree)
         pbucket->FirstFree=psub->Next;
      if(psub->Prev)
         psub->Prev->Next=psub->Next;
      if(psub->Next)
         psub->Next->Prev=psub->Prev;
      if(!__HeapFree(pheap, flags, psub))
         return FALSE;
   }
   else
   {
      /* free fragment */
      psub->Bitmap&= ~(1<<nalloc);

      if(psub->FirstFree)
      {
         pfrag->FreeNext           = psub->FirstFree;
         pfrag->FreePrev           = NULL;
         psub->FirstFree->FreePrev = pfrag;
         psub->FirstFree           = pfrag;
      }
      else
      {
         psub->FirstFree=pfrag;
         pfrag->FreePrev=NULL;
         pfrag->FreeNext=NULL;
      }
   }
   return TRUE;
}

/*********************************************************************
*                     __HeapPrepare                                  *
*                                                                    *
* Fills in all the data structures of a heap                         *
*********************************************************************/
PHEAP __HeapPrepare(LPVOID base, ULONG minsize, ULONG maxsize,  ULONG flags)
{
   PHEAP pheap=(PHEAP) base;
   
   dprintf("__HeapPrepare(base %x, minsize %d, maxsize %d, flags %x)\n",
	   base,minsize,maxsize,flags);
   
   pheap->Magic=MAGIC_HEAP;
   pheap->End= ((LPVOID)pheap)+minsize;
   pheap->Flags=flags;
   pheap->LastBlock=(LPVOID)pheap + PAGESIZE;
   CopyMemory(pheap->Bucket,__HeapDefaultBuckets,sizeof(__HeapDefaultBuckets));
   if(__ProcessHeap)
   {
      pheap->NextHeap=__ProcessHeap->NextHeap;
      __ProcessHeap->NextHeap=pheap;
   }
   else
   {
      pheap->NextHeap=0;
      __ProcessHeap=pheap;
   }
   InitializeCriticalSection(&(pheap->Synchronize));
   pheap->Start.Size= (minsize-sizeof(HEAP))|HEAP_FREE_TAG;
   pheap->Start.PrevSize =0;

   return pheap;
}

/*********************************************************************
*                     __HeapInit                                     *
*                                                                    *
* Called by __VirtualInit to initialize the default process heap     *
*********************************************************************/

VOID WINAPI __HeapInit(LPVOID base, ULONG minsize, ULONG maxsize)
{
   VirtualAlloc(base,maxsize,MEM_RESERVE,PAGE_READWRITE);
   VirtualAlloc(base,PAGESIZE,MEM_COMMIT,PAGE_READWRITE);
   
   __HeapPrepare(base, minsize, maxsize, 0);
}


/*********************************************************************
*                     HeapCreate -- KERNEL32                         *
*********************************************************************/
HANDLE STDCALL HeapCreate(DWORD flags, DWORD minsize, DWORD maxsize)
{
   PHEAP pheap;

   aprintf("HeapCreate( 0x%lX, 0x%lX, 0x%lX )\n", flags, minsize, maxsize);

   pheap = VirtualAlloc(NULL, minsize, MEM_TOP_DOWN, PAGE_READWRITE);
   VirtualAlloc(pheap, PAGESIZE, MEM_COMMIT, PAGE_READWRITE);
   return (HANDLE) __HeapPrepare(pheap, minsize, maxsize, flags);
}

/*********************************************************************
*                     HeapDestroy -- KERNEL32                        *
*********************************************************************/
BOOL WINAPI HeapDestroy(HANDLE hheap)
{
   PHEAP pheap=(PHEAP) hheap;

   aprintf("HeapDestroy( 0x%lX )\n", (ULONG) hheap );

   if(pheap->Magic!=MAGIC_HEAP)
      return __ErrorReturnFalse(ERROR_INVALID_PARAMETER);

   DeleteCriticalSection(&(pheap->Synchronize));
   VirtualFree(pheap,0,MEM_RELEASE);
   
   return TRUE;
}

/*********************************************************************
*                     HeapAlloc -- KERNEL32                          *
*********************************************************************/
LPVOID STDCALL HeapAlloc(HANDLE hheap, DWORD flags, DWORD size)
{
   PHEAP    pheap=hheap;
   LPVOID   retval;

   aprintf("HeapAlloc( 0x%lX, 0x%lX, 0x%lX )\n",
           (ULONG) hheap, flags, (ULONG) size );
#ifdef NOT
   HeapValidate(hheap, 0, 0);
#endif
   if(( flags | pheap->Flags)  & HEAP_NO_SERIALIZE )
      EnterCriticalSection(&(pheap->Synchronize));

   if(size>HEAP_FRAGMENT_THRESHOLD)
      retval=__HeapAlloc(pheap, flags, size, HEAP_NORMAL_TAG);
   else
      retval=__HeapAllocFragment(pheap, flags, size);

   if( (flags | pheap->Flags) & HEAP_NO_SERIALIZE )
      LeaveCriticalSection(&(pheap->Synchronize));

   aprintf("HeapAlloc returns 0x%lX\n", (ULONG) retval);
   return retval;

}

/*********************************************************************
*                     HeapReAlloc -- KERNEL32                        *
*********************************************************************/
LPVOID STDCALL HeapReAlloc(HANDLE hheap, DWORD flags, LPVOID ptr, DWORD size)
{
   PHEAP            pheap=hheap;
   PHEAP_BLOCK      pfree=((PHEAP_BLOCK)ptr-1);
   LPVOID           retval;

   aprintf("HeapReAlloc( 0x%lX, 0x%lX, 0x%lX, 0x%lX )\n",
           (ULONG) hheap, flags, (ULONG) ptr, size );
#ifdef NOT
   HeapValidate(hheap, 0, 0);
#endif
   if(( flags | pheap->Flags)  & HEAP_NO_SERIALIZE )
      EnterCriticalSection(&(pheap->Synchronize));

   if(HEAP_ISNORMAL(pfree))
      retval=__HeapReAlloc(pheap, flags, ptr, size);
   else if(HEAP_ISFRAG(pfree))
      retval=__HeapReAllocFragment(pheap, flags, ptr, size);
   else
      retval=__ErrorReturnNull(ERROR_INVALID_PARAMETER);

   if( (flags| pheap->Flags) & HEAP_NO_SERIALIZE )
      LeaveCriticalSection(&(pheap->Synchronize));

   return retval;
}

/*********************************************************************
*                     HeapFree -- KERNEL32                           *
*********************************************************************/
WINBOOL STDCALL HeapFree(HANDLE hheap, DWORD flags, LPVOID ptr)
{
   PHEAP            pheap=hheap;
   PHEAP_BLOCK      pfree=(PHEAP_BLOCK)((LPVOID)ptr-HEAP_ADMIN_SIZE);
   BOOL             retval;

   aprintf("HeapFree( 0x%lX, 0x%lX, 0x%lX )\n",
           (ULONG) hheap, flags, (ULONG) ptr );
#ifdef NOT
   HeapValidate(hheap, 0, 0);
#endif
   if(( flags | pheap->Flags)  & HEAP_NO_SERIALIZE )
      EnterCriticalSection(&(pheap->Synchronize));

   if(HEAP_ISNORMAL(pfree))
      retval=__HeapFree(pheap, flags, ptr);
   else if(HEAP_ISFRAG(pfree))
      retval=__HeapFreeFragment(pheap, flags, ptr);
   else
      retval=__ErrorReturnFalse(ERROR_INVALID_PARAMETER);

   if( (flags| pheap->Flags) & HEAP_NO_SERIALIZE )
      LeaveCriticalSection(&(pheap->Synchronize));

   return retval;
}

/*********************************************************************
*                   GetProcessHeap  --  KERNEL32                     *
*********************************************************************/
HANDLE WINAPI GetProcessHeap(VOID)
{
   aprintf("GetProcessHeap()\n");
   return (HANDLE) __ProcessHeap;
}

/********************************************************************
*                   GetProcessHeaps  --  KERNEL32                   *
*                                                                   *
* NOTE in Win95 this function is not implemented and just returns   *
* ERROR_CALL_NOT_IMPLEMENTED                                        *
********************************************************************/
DWORD WINAPI GetProcessHeaps(DWORD maxheaps, PHANDLE phandles )
{
   DWORD retval;
   PHEAP pheap;

   aprintf("GetProcessHeaps( %u, 0x%lX )\n", maxheaps, (ULONG) phandles );

   pheap= __ProcessHeap;
   retval=0;
   while((pheap)&&(maxheaps))
   {
      *phandles=pheap;
      phandles++;
      maxheaps--;
      retval++;
      pheap=pheap->NextHeap;
   }
   while(pheap)
   {
      retval++;
      pheap=pheap->NextHeap;
   }


   return retval;
}

/*********************************************************************
*                    HeapLock  --  KERNEL32                          *
*********************************************************************/

BOOL WINAPI HeapLock(HANDLE hheap)
{
   PHEAP pheap=hheap;

   aprintf("HeapLock( 0x%lX )\n", (ULONG) hheap );

   EnterCriticalSection(&(pheap->Synchronize));
   return TRUE;
}

/*********************************************************************
*                    HeapUnlock  --  KERNEL32                        *
*********************************************************************/

BOOL WINAPI HeapUnlock(HANDLE hheap)
{
   PHEAP pheap=hheap;

   aprintf("HeapUnlock( 0x%lX )\n", (ULONG) hheap );

   LeaveCriticalSection(&(pheap->Synchronize));
   return TRUE;
}

/*********************************************************************
*                    HeapCompact  --  KERNEL32                       *
*                                                                    *
* NT uses this function to compact moveable blocks and other things  *
* Here it does not compact, but it finds the largest free region     *
*********************************************************************/

UINT HeapCompact(HANDLE hheap, DWORD flags)
{
   PHEAP	pheap=hheap;
   PHEAP_BLOCK	pfree;
   ULONG	freesize;
   ULONG	largestfree;

   if(( flags | pheap->Flags)  & HEAP_NO_SERIALIZE )
      EnterCriticalSection(&(pheap->Synchronize));

   pfree=&(pheap->Start);
   /* look for the largest free region of memory */
   largestfree=0;
   do
   {
      freesize=HEAP_SIZE(pfree);
      if(HEAP_ISFREE(pfree) && freesize>largestfree)
         largestfree=freesize;

      pfree=HEAP_NEXT(pfree);
   }
   while( (LPVOID)pfree < pheap->End );

   if( (flags| pheap->Flags) & HEAP_NO_SERIALIZE )
      LeaveCriticalSection(&(pheap->Synchronize));

   return largestfree;
}

/*********************************************************************
*                    HeapSize  --  KERNEL32                          *
*********************************************************************/
DWORD WINAPI HeapSize(HANDLE hheap, DWORD flags, LPCVOID pmem)
{
   PHEAP	pheap=(PHEAP) hheap;
   PHEAP_BLOCK	palloc=((PHEAP_BLOCK)pmem-1);
   DWORD	retval=0;

   aprintf("HeapSize( 0x%lX, 0x%lX, 0x%lX )\n",
           (ULONG) hheap, flags, (ULONG) pmem );

   if(pheap->Magic!=MAGIC_HEAP)
      { SetLastError(ERROR_INVALID_PARAMETER); return 0; }

   if(( flags | pheap->Flags)  & HEAP_NO_SERIALIZE )
      EnterCriticalSection(&(pheap->Synchronize));

   if((pmem> (LPVOID)pheap)&&(pmem < pheap->End))
   {
      if(HEAP_ISALLOC(palloc))
         retval=HEAP_SIZE(palloc);	/* normal allocation */
      else if(HEAP_ISFRAG(palloc))
         retval=HEAP_FRAG_SIZE(palloc); /* fragment */
      else
         { SetLastError(ERROR_INVALID_PARAMETER); retval = -1; }
   }

   if( (flags| pheap->Flags) & HEAP_NO_SERIALIZE )
       LeaveCriticalSection(&(pheap->Synchronize));

   return retval;
}

/*********************************************************************
*                    HeapValidate  --  KERNEL32                      *
*                                                                    *
* NOTE: only implemented in NT                                       *
*********************************************************************/
BOOL WINAPI HeapValidate(HANDLE hheap, DWORD flags, LPCVOID pmem)
{
   PHEAP		pheap=(PHEAP)hheap;
   PHEAP_BLOCK		pcheck;
   PHEAP_BLOCK		pprev;
   PHEAP_BLOCK		pnext;
   PHEAP_SUBALLOC	psub;
   PHEAP_FRAGMENT	pfrag;
   PHEAP_FRAGMENT	pnextfrag;
   PHEAP_FRAGMENT	pprevfrag;
   PHEAP_BUCKET		pbucket;
   INT			i;
   INT			number;
   INT			add;

   if(( flags | pheap->Flags)  & HEAP_NO_SERIALIZE )
      EnterCriticalSection(&(pheap->Synchronize));

   if(pmem==NULL)
   {
      pcheck=&(pheap->Start);
      pprev=NULL;
      /* verify all blocks */
      do
      {
         pnext=HEAP_NEXT(pcheck);
         if((pprev)&&(HEAP_PREV(pcheck)!=pprev))
         {
            dprintf("HeapValidate: linked list invalid, region 0x%lX,"
                    " previous region 0x%lX, list says 0x%lX\n",
                     (ULONG)pcheck, (ULONG)pprev, (ULONG) HEAP_PREV(pcheck));
            return FALSE;
         }
         if(HEAP_ISSUB(pcheck))
         {

            /* check fragments */
            psub=(PHEAP_SUBALLOC) ((PHEAP_BLOCK)pcheck+1);
            pbucket=psub->Bucket;
            pfrag=(PHEAP_FRAGMENT) ((LPVOID)psub + sizeof(HEAP_SUBALLOC));

            if(psub->NumberFree>pbucket->Number)
               return FALSE;

            add=pbucket->Size+HEAP_FRAG_ADMIN_SIZE;
            pprevfrag=NULL;
            number=0;
            for(i=0;i<pbucket->Number;i++)
            {
               pnextfrag=(PHEAP_FRAGMENT)((LPVOID)pfrag+add);
               if(pfrag->Magic!=HEAP_FRAG_MAGIC)
               {
                  dprintf("HeapValidate: fragment %d magic invalid, region 0x%lX,"
                          " previous region 0x%lX\n", i, (ULONG)pcheck, (ULONG)pprev);
                  return FALSE;
               }
               if(pfrag->Number!=i)
               {
                  dprintf("HeapValidate: fragment %d number invalid, region 0x%lX,"
                          " previous region 0x%lX\n", i, (ULONG)pcheck, (ULONG)pprev);
                  return FALSE;
               }
               if((psub->Bitmap&(1<<i))==0)
                  number++;
               if(pfrag->Sub!=psub)
               {
                  dprintf("HeapValidate: fragment %d suballoc invalid, region 0x%lX,"
                          " previous region 0x%lX\n", i, (ULONG)pcheck, (ULONG)pprev);
                  return FALSE;
               }
               pprevfrag=pfrag;
               pfrag=pnextfrag;
            }
            if(number!=psub->NumberFree)
            {
               dprintf("HeapValidate: invalid number of free fragments, region 0x%lX,"
                       " previous region 0x%lX\n", (ULONG)pcheck, (ULONG)pprev);
               return FALSE;
            }
            dprintf("HeapValidate: [0x%08lX-0x%08lX] suballocated,"
                    " bucket size=%d, bitmap=0x%08lX\n",
                    (ULONG) pcheck, (ULONG) pnext, pbucket->Size, psub->Bitmap);
         }
         else if(HEAP_ISFREE(pcheck))
         {
            if(HEAP_RSIZE(pcheck)!=HEAP_SIZE(pcheck))
            {
               dprintf("HeapValidate: invalid size of free region 0x%lX,"
                       " previous region 0x%lX\n",
                       (ULONG) pcheck, (ULONG) pprev);
               return FALSE;
            }
            dprintf("HeapValidate: [0x%08lX-0x%08lX] free\n",
                    (ULONG) pcheck, (ULONG) pnext );
         }
         else if(HEAP_ISNORMAL(pcheck))
         {
            dprintf("HeapValidate: [0x%08lX-0x%08lX] allocated\n",
                    (ULONG) pcheck, (ULONG) pnext );
         }
         else
         {
            dprintf("HeapValidate: invalid tag %x, region 0x%lX,"
                    " previous region 0x%lX\n", pcheck->Size>>28,
                     (ULONG)pcheck, (ULONG)pprev);
            return FALSE;
         }
         pprev=pcheck;
         pcheck=HEAP_NEXT(pcheck);
      }
      while( (LPVOID)pcheck < pheap->End );
   }

   if( (flags| pheap->Flags) & HEAP_NO_SERIALIZE )
      LeaveCriticalSection(&(pheap->Synchronize));

   return TRUE;
}

