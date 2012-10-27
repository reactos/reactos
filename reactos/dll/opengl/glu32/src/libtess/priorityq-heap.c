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

#include <stddef.h>
#include <assert.h>
#include "priorityq-heap.h"
#include "memalloc.h"

#define INIT_SIZE	32

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifdef FOR_TRITE_TEST_PROGRAM
#define LEQ(x,y)	(*pq->leq)(x,y)
#else
/* Violates modularity, but a little faster */
#include "geom.h"
#define LEQ(x,y)	VertLeq((GLUvertex *)x, (GLUvertex *)y)
#endif

/* really __gl_pqHeapNewPriorityQ */
PriorityQ *pqNewPriorityQ( int (*leq)(PQkey key1, PQkey key2) )
{
  PriorityQ *pq = (PriorityQ *)memAlloc( sizeof( PriorityQ ));
  if (pq == NULL) return NULL;

  pq->size = 0;
  pq->max = INIT_SIZE;
  pq->nodes = (PQnode *)memAlloc( (INIT_SIZE + 1) * sizeof(pq->nodes[0]) );
  if (pq->nodes == NULL) {
     memFree(pq);
     return NULL;
  }

  pq->handles = (PQhandleElem *)memAlloc( (INIT_SIZE + 1) * sizeof(pq->handles[0]) );
  if (pq->handles == NULL) {
     memFree(pq->nodes);
     memFree(pq);
     return NULL;
  }

  pq->initialized = FALSE;
  pq->freeList = 0;
  pq->leq = leq;

  pq->nodes[1].handle = 1;	/* so that Minimum() returns NULL */
  pq->handles[1].key = NULL;
  return pq;
}

/* really __gl_pqHeapDeletePriorityQ */
void pqDeletePriorityQ( PriorityQ *pq )
{
  memFree( pq->handles );
  memFree( pq->nodes );
  memFree( pq );
}


static void FloatDown( PriorityQ *pq, long curr )
{
  PQnode *n = pq->nodes;
  PQhandleElem *h = pq->handles;
  PQhandle hCurr, hChild;
  long child;

  hCurr = n[curr].handle;
  for( ;; ) {
    child = curr << 1;
    if( child < pq->size && LEQ( h[n[child+1].handle].key,
				 h[n[child].handle].key )) {
      ++child;
    }

    assert(child <= pq->max);

    hChild = n[child].handle;
    if( child > pq->size || LEQ( h[hCurr].key, h[hChild].key )) {
      n[curr].handle = hCurr;
      h[hCurr].node = curr;
      break;
    }
    n[curr].handle = hChild;
    h[hChild].node = curr;
    curr = child;
  }
}


static void FloatUp( PriorityQ *pq, long curr )
{
  PQnode *n = pq->nodes;
  PQhandleElem *h = pq->handles;
  PQhandle hCurr, hParent;
  long parent;

  hCurr = n[curr].handle;
  for( ;; ) {
    parent = curr >> 1;
    hParent = n[parent].handle;
    if( parent == 0 || LEQ( h[hParent].key, h[hCurr].key )) {
      n[curr].handle = hCurr;
      h[hCurr].node = curr;
      break;
    }
    n[curr].handle = hParent;
    h[hParent].node = curr;
    curr = parent;
  }
}

/* really __gl_pqHeapInit */
void pqInit( PriorityQ *pq )
{
  long i;

  /* This method of building a heap is O(n), rather than O(n lg n). */

  for( i = pq->size; i >= 1; --i ) {
    FloatDown( pq, i );
  }
  pq->initialized = TRUE;
}

/* really __gl_pqHeapInsert */
/* returns LONG_MAX iff out of memory */
PQhandle pqInsert( PriorityQ *pq, PQkey keyNew )
{
  long curr;
  PQhandle free_handle;

  curr = ++ pq->size;
  if( (curr*2) > pq->max ) {
    PQnode *saveNodes= pq->nodes;
    PQhandleElem *saveHandles= pq->handles;

    /* If the heap overflows, double its size. */
    pq->max <<= 1;
    pq->nodes = (PQnode *)memRealloc( pq->nodes, 
				     (size_t) 
				     ((pq->max + 1) * sizeof( pq->nodes[0] )));
    if (pq->nodes == NULL) {
       pq->nodes = saveNodes;	/* restore ptr to free upon return */
       return LONG_MAX;
    }
    pq->handles = (PQhandleElem *)memRealloc( pq->handles,
			                     (size_t)
			                      ((pq->max + 1) * 
					       sizeof( pq->handles[0] )));
    if (pq->handles == NULL) {
       pq->handles = saveHandles; /* restore ptr to free upon return */
       return LONG_MAX;
    }
  }

  if( pq->freeList == 0 ) {
    free_handle = curr;
  } else {
    free_handle = pq->freeList;
    pq->freeList = pq->handles[free_handle].node;
  }

  pq->nodes[curr].handle = free_handle;
  pq->handles[free_handle].node = curr;
  pq->handles[free_handle].key = keyNew;

  if( pq->initialized ) {
    FloatUp( pq, curr );
  }
  assert(free_handle != LONG_MAX);
  return free_handle;
}

/* really __gl_pqHeapExtractMin */
PQkey pqExtractMin( PriorityQ *pq )
{
  PQnode *n = pq->nodes;
  PQhandleElem *h = pq->handles;
  PQhandle hMin = n[1].handle;
  PQkey min = h[hMin].key;

  if( pq->size > 0 ) {
    n[1].handle = n[pq->size].handle;
    h[n[1].handle].node = 1;

    h[hMin].key = NULL;
    h[hMin].node = pq->freeList;
    pq->freeList = hMin;

    if( -- pq->size > 0 ) {
      FloatDown( pq, 1 );
    }
  }
  return min;
}

/* really __gl_pqHeapDelete */
void pqDelete( PriorityQ *pq, PQhandle hCurr )
{
  PQnode *n = pq->nodes;
  PQhandleElem *h = pq->handles;
  long curr;

  assert( hCurr >= 1 && hCurr <= pq->max && h[hCurr].key != NULL );

  curr = h[hCurr].node;
  n[curr].handle = n[pq->size].handle;
  h[n[curr].handle].node = curr;

  if( curr <= -- pq->size ) {
    if( curr <= 1 || LEQ( h[n[curr>>1].handle].key, h[n[curr].handle].key )) {
      FloatDown( pq, curr );
    } else {
      FloatUp( pq, curr );
    }
  }
  h[hCurr].key = NULL;
  h[hCurr].node = pq->freeList;
  pq->freeList = hCurr;
}
