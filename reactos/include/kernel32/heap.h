/*
 * kernel/heap.c
 * Copyright (C) 1996, Onno Hovers, All rights reserved
 * Adapted for the ReactOS system libraries by David Welch (welch@mcmail.com)
 * todo: __processheap should be part of peb.
 */

#ifndef __INCLUDE_KERNEL32_HEAP_H
#define __INCLUDE_KERNEL32_HEAP_H

/* System wide includes ****************************************************/
#include <windows.h>

/* System library's private includes ***************************************/

#include <ntdll/pagesize.h>

/* definitions */
#define HEAP_ADMIN_SIZE		(sizeof(HEAP_BLOCK))
#define HEAP_FRAG_ADMIN_SIZE	(sizeof(HEAP_FRAGMENT))
#define HEAP_ROUNDVAL     (2*(HEAP_ADMIN_SIZE)-1)
#define HEAP_FRAGMENT_THRESHOLD 256

#define SIZE_TOTAL(s)     ROUNDUP((s)+HEAP_ADMIN_SIZE,8)
#define SIZE_ROUND(s)     ROUNDUP((s),8)

#define HEAP_FRAG_MAGIC   0x10
#define HEAP_ALLOC_MASK   0xF0000000
#define HEAP_FREE_MASK    0x80000000
#define HEAP_SIZE_MASK    0x0FFFFFFF
#define HEAP_FREE_TAG     0x80000000    /* free region */
#define HEAP_NORMAL_TAG   0x30000000    /* normal allocation */
#define HEAP_MOVEABLE_TAG 0x50000000    /* moveable handle */
#define HEAP_SUB_TAG      0x70000000    /* suballocated for fragments */

#define HEAP_ISFREE(p)    ((((PHEAP_BLOCK)p)->Size) & HEAP_FREE_MASK)
#define HEAP_ISALLOC(p)   (((((PHEAP_BLOCK)p)->Size) & HEAP_FREE_MASK)==0)
#define HEAP_ISFRAG(p)    ((((PHEAP_FRAGMENT)p)->Magic)==HEAP_FRAG_MAGIC)
#define HEAP_ISNORMAL(p)  (((((PHEAP_BLOCK)p)->Size) & HEAP_ALLOC_MASK)\
                          ==HEAP_NORMAL_TAG)
#define HEAP_ISSUB(p)     (((((PHEAP_BLOCK)p)->Size) & HEAP_ALLOC_MASK)\
                          ==HEAP_SUB_TAG)
#define HEAP_ISOLD(p)     (((((PHEAP_BLOCK)p)->Size) & HEAP_ALLOC_MASK)\
                          ==HEAP_MOVEABLE_TAG)

#define HEAP_SIZE(p)      ((((PHEAP_BLOCK)p)->Size) & HEAP_SIZE_MASK )
#define HEAP_RSIZE(p)     SIZE_ROUND(HEAP_SIZE(p))
#define HEAP_TSIZE(p)     SIZE_TOTAL(HEAP_SIZE(p))
#define HEAP_PREVSIZE(p)  ((((PHEAP_BLOCK)p)->PrevSize) & HEAP_SIZE_MASK )
#define HEAP_FRAG_SIZE(p) (((PHEAP_FRAGMENT)p)->Size)

#define HEAP_PREV(p)      ((PHEAP_BLOCK)(((LPVOID)(p))-HEAP_PREVSIZE(p)))
#define HEAP_NEXT(p)      ((PHEAP_BLOCK)(((LPVOID)(p))+HEAP_TSIZE(p)))

typedef struct __HEAP_BLOCK
{
   ULONG  Size;            /* this is relative to Data */
   ULONG  PrevSize;        /* p - p->PrevSize is the previous block */
} HEAP_BLOCK, *PHEAP_BLOCK;

struct __HEAP_SUBALLOC;

typedef struct __HEAP_FRAGMENT
{
   UCHAR                         Magic;
   UCHAR                         Number;
   ULONG                         Size;
   struct __HEAP_SUBALLOC 	*Sub;

   /* this is only used in free blocks */
   struct __HEAP_FRAGMENT	*FreeNext;
   struct __HEAP_FRAGMENT	*FreePrev;
} HEAP_FRAGMENT, *PHEAP_FRAGMENT, *LPHEAP_FRAGMENT;

typedef struct __HEAP_SUBALLOC
{
   ULONG                         Magic;
   ULONG                         NumberFree;
   struct __HEAP_SUBALLOC	*Next;

   struct __HEAP_SUBALLOC	*Prev;
   struct __HEAP_FRAGMENT	*FirstFree;
   struct __HEAP_BUCKET		*Bucket;
   ULONG			 Bitmap;
} HEAP_SUBALLOC, *PHEAP_SUBALLOC, *LPHEAP_SUBALLOC;

typedef struct __HEAP_BUCKET
{
   struct __HEAP_SUBALLOC	*FirstFree;
   ULONG                         Size;
   ULONG                         Number;
   ULONG                         TotalSize;
} HEAP_BUCKET, *PHEAP_BUCKET, *LPHEAP_BUCKET;

typedef struct __HEAP
{
   ULONG 		Magic;
   LPVOID		End;
   ULONG       		Flags;
   CRITICAL_SECTION	Synchronize;
   HEAP_BUCKET		Bucket[8];
   struct __HEAP	*NextHeap;
   LPVOID      		LastBlock;
   /* this has to aligned on an 8 byte boundary */
   HEAP_BLOCK		Start  __attribute__((aligned (8)));
} HEAP, *PHEAP;

#endif /* __INCLUDE_KERNEL32_HEAP_H */
