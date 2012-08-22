/*
 * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
 * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice including the dates of first publication and
 * either this permission notice or a reference to
 * http://oss.sgi.com/projects/FreeB/
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * SILICON GRAPHICS, INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Silicon Graphics, Inc.
 * shall not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Silicon Graphics, Inc.
 */
/*
** Author: Eric Veach, July 1994.
**
*/

#include "gluos.h"
#include <stddef.h>
#include <assert.h>
#include <limits.h>		/* LONG_MAX */
#include "memalloc.h"

/* Include all the code for the regular heap-based queue here. */

#include "priorityq-heap.c"

/* Now redefine all the function names to map to their "Sort" versions. */

#include "priorityq-sort.h"

/* really __gl_pqSortNewPriorityQ */
PriorityQ *pqNewPriorityQ( int (*leq)(PQkey key1, PQkey key2) )
{
  PriorityQ *pq = (PriorityQ *)memAlloc( sizeof( PriorityQ ));
  if (pq == NULL) return NULL;

  pq->heap = __gl_pqHeapNewPriorityQ( leq );
  if (pq->heap == NULL) {
     memFree(pq);
     return NULL;
  }

  pq->keys = (PQHeapKey *)memAlloc( INIT_SIZE * sizeof(pq->keys[0]) );
  if (pq->keys == NULL) {
     __gl_pqHeapDeletePriorityQ(pq->heap);
     memFree(pq);
     return NULL;
  }

  pq->size = 0;
  pq->max = INIT_SIZE;
  pq->initialized = FALSE;
  pq->leq = leq;
  return pq;
}

/* really __gl_pqSortDeletePriorityQ */
void pqDeletePriorityQ( PriorityQ *pq )
{
  assert(pq != NULL); 
  if (pq->heap != NULL) __gl_pqHeapDeletePriorityQ( pq->heap );
  if (pq->order != NULL) memFree( pq->order );
  if (pq->keys != NULL) memFree( pq->keys );
  memFree( pq );
}


#define LT(x,y)		(! LEQ(y,x))
#define GT(x,y)		(! LEQ(x,y))
#define Swap(a,b)	do{PQkey *tmp = *a; *a = *b; *b = tmp;}while(0)

/* really __gl_pqSortInit */
int pqInit( PriorityQ *pq )
{
  PQkey **p, **r, **i, **j, *piv;
  struct { PQkey **p, **r; } Stack[50], *top = Stack;
  unsigned long seed = 2016473283;

  /* Create an array of indirect pointers to the keys, so that we
   * the handles we have returned are still valid.
   */
/*
  pq->order = (PQHeapKey **)memAlloc( (size_t)
                                  (pq->size * sizeof(pq->order[0])) );
*/
  pq->order = (PQHeapKey **)memAlloc( (size_t)
                                  ((pq->size+1) * sizeof(pq->order[0])) );
/* the previous line is a patch to compensate for the fact that IBM */
/* machines return a null on a malloc of zero bytes (unlike SGI),   */
/* so we have to put in this defense to guard against a memory      */
/* fault four lines down. from fossum@austin.ibm.com.               */
  if (pq->order == NULL) return 0;

  p = pq->order;
  r = p + pq->size - 1;
  for( piv = pq->keys, i = p; i <= r; ++piv, ++i ) {
    *i = piv;
  }

  /* Sort the indirect pointers in descending order,
   * using randomized Quicksort
   */
  top->p = p; top->r = r; ++top;
  while( --top >= Stack ) {
    p = top->p;
    r = top->r;
    while( r > p + 10 ) {
      seed = seed * 1539415821 + 1;
      i = p + seed % (r - p + 1);
      piv = *i;
      *i = *p;
      *p = piv;
      i = p - 1;
      j = r + 1;
      do {
	do { ++i; } while( GT( **i, *piv ));
	do { --j; } while( LT( **j, *piv ));
	Swap( i, j );
      } while( i < j );
      Swap( i, j );	/* Undo last swap */
      if( i - p < r - j ) {
	top->p = j+1; top->r = r; ++top;
	r = i-1;
      } else {
	top->p = p; top->r = i-1; ++top;
	p = j+1;
      }
    }
    /* Insertion sort small lists */
    for( i = p+1; i <= r; ++i ) {
      piv = *i;
      for( j = i; j > p && LT( **(j-1), *piv ); --j ) {
	*j = *(j-1);
      }
      *j = piv;
    }
  }
  pq->max = pq->size;
  pq->initialized = TRUE;
  __gl_pqHeapInit( pq->heap );	/* always succeeds */

#ifndef NDEBUG
  p = pq->order;
  r = p + pq->size - 1;
  for( i = p; i < r; ++i ) {
    assert( LEQ( **(i+1), **i ));
  }
#endif

  return 1;
}

/* really __gl_pqSortInsert */
/* returns LONG_MAX iff out of memory */ 
PQhandle pqInsert( PriorityQ *pq, PQkey keyNew )
{
  long curr;

  if( pq->initialized ) {
    return __gl_pqHeapInsert( pq->heap, keyNew );
  }
  curr = pq->size;
  if( ++ pq->size >= pq->max ) {
    PQkey *saveKey= pq->keys;

    /* If the heap overflows, double its size. */
    pq->max <<= 1;
    pq->keys = (PQHeapKey *)memRealloc( pq->keys, 
	 	                        (size_t)
	                                 (pq->max * sizeof( pq->keys[0] )));
    if (pq->keys == NULL) {	
       pq->keys = saveKey;	/* restore ptr to free upon return */
       return LONG_MAX;
    }
  }
  assert(curr != LONG_MAX);	
  pq->keys[curr] = keyNew;

  /* Negative handles index the sorted array. */
  return -(curr+1);
}

/* really __gl_pqSortExtractMin */
PQkey pqExtractMin( PriorityQ *pq )
{
  PQkey sortMin, heapMin;

  if( pq->size == 0 ) {
    return __gl_pqHeapExtractMin( pq->heap );
  }
  sortMin = *(pq->order[pq->size-1]);
  if( ! __gl_pqHeapIsEmpty( pq->heap )) {
    heapMin = __gl_pqHeapMinimum( pq->heap );
    if( LEQ( heapMin, sortMin )) {
      return __gl_pqHeapExtractMin( pq->heap );
    }
  }
  do {
    -- pq->size;
  } while( pq->size > 0 && *(pq->order[pq->size-1]) == NULL );
  return sortMin;
}

/* really __gl_pqSortMinimum */
PQkey pqMinimum( PriorityQ *pq )
{
  PQkey sortMin, heapMin;

  if( pq->size == 0 ) {
    return __gl_pqHeapMinimum( pq->heap );
  }
  sortMin = *(pq->order[pq->size-1]);
  if( ! __gl_pqHeapIsEmpty( pq->heap )) {
    heapMin = __gl_pqHeapMinimum( pq->heap );
    if( LEQ( heapMin, sortMin )) {
      return heapMin;
    }
  }
  return sortMin;
}

/* really __gl_pqSortIsEmpty */
int pqIsEmpty( PriorityQ *pq )
{
  return (pq->size == 0) && __gl_pqHeapIsEmpty( pq->heap );
}

/* really __gl_pqSortDelete */
void pqDelete( PriorityQ *pq, PQhandle curr )
{
  if( curr >= 0 ) {
    __gl_pqHeapDelete( pq->heap, curr );
    return;
  }
  curr = -(curr+1);
  assert( curr < pq->max && pq->keys[curr] != NULL );

  pq->keys[curr] = NULL;
  while( pq->size > 0 && *(pq->order[pq->size-1]) == NULL ) {
    -- pq->size;
  }
}
