// See license at end of file

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * A  "smarter" malloc				William L. Sebok
 *						Sept. 24, 1984
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *	 If n = the size of an area rounded DOWN to the nearest power of two,
 *	all free areas of memory whose length is the same index n is organized
 *	into a chain with other free areas of index n. A request for memory
 *	takes the first item in the chain whose index is the size of the
 *	request rounded UP to the nearest power of two.  If this chain is
 *	empty the next higher chain is examined.  If no larger chain has memory
 *	then new memory is allocated.  Only the amount of new memory needed is
 *	allocated.  Any old free memory left after an allocation is returned
 *	to the free list.  Extra new memory returned because of rounding
 *	to page boundaries is returned to free list.
 *
 *	  All memory areas (free or busy) handled by malloc are also chained
 *	sequentially by increasing address.  When memory is freed it is
 *	merged with adjacent free areas, if any.  If a free area of memory
 *	ends at the end of memory (i.e. at the break),  the break is
 *	contracted, freeing the memory back to the system.
 *
 *	Notes:
 *		ov_length field includes sizeof(struct overhead)
 *		adjacency chain includes all memory, allocated plus free.
 */

#define MALLOC
#include "malloc.h"
#ifndef	NULL
#define NULL	0
#endif
#ifdef debug
# define ASSERT(p,q)	if (!(p)) fatal(q)
#else
# define ASSERT(p,q)
#endif

#define ALIGN(n, granule)  ((n + ((granule)-1)) & ~((granule)-1))
/*
// PowerPC page size = 4 KB; PC page size is the same???
*/

#define PAGE_SIZE (ULONG)0x1000
#define ULONG unsigned long

char *
malloc(nbytes)
	unsigned nbytes;
{
        extern ULONG OFClaim();
	register struct overhead *p, *q;
	register int surplus;
	register struct qelem *bucket;
	nbytes = ALIGN(nbytes, NALIGN) + sizeof(struct overhead);
	bucket = &buckets[_log2(nbytes-1) + 1];	/* log2 rounded up */
	for (p = NULL; bucket < &buckets[NBUCKETS]; bucket++) { 
		if (bucket->q_forw != bucket) {
			/*  remove from bucket chain */
			p = FROMBUK(bucket->q_forw);
			ASSERT(p->ov_magic == MAGIC_FREE, "\nmalloc: Entry \
not marked FREE found on Free List!\n");
			remque(TOBUK(p));
			surplus = p->ov_length - nbytes;
			break;
		}
	}
	if (p == NULL) {
#ifdef USE_SBRK
		register int i;
		p = (struct overhead *)CURBRK;
		if ((int)p == -1)
			return(NULL);
		if (i = (int)p&(NALIGN-1))
			sbrk(NALIGN-i);
		p = (struct overhead *)sbrk(nbytes);
		if ((int)p == -1)
			return(NULL);
		q = (struct overhead *)CURBRK;
		if ((int)q == -1)
			return(NULL);
		p->ov_length = (char *)q - (char *)p;
		surplus = p->ov_length - nbytes;
		/* add to end of adjacency chain */
		ASSERT((FROMADJ(adjhead.q_back)) < p, "\nmalloc: Entry in \
adjacency chain found with address lower than Chain head!\n" );
		insque(TOADJ(p),adjhead.q_back);
#else
		struct qelem *pp;
		int alloc_size = ALIGN(nbytes, PAGE_SIZE);

		p = (struct overhead *)OFClaim(0, alloc_size, NALIGN);
		if (p == (struct overhead *)-1)
			return(NULL);
		p->ov_length = alloc_size;
		surplus = p->ov_length - nbytes;

		/* add to adjacency chain in the correct place */
		for (pp = adjhead.q_forw;
		     pp != &adjhead;
		     pp = pp->q_forw) {
			if (p < FROMADJ(pp))
				break;
		}
		ASSERT(pp == &adjhead || (p < FROMADJ(pp)),
		       "\nmalloc: Bogus insertion in adjacency list\n");
		insque(TOADJ(p),pp->q_back);
#endif
	}
	if (surplus > sizeof(struct overhead)) {
		/* if big enough, split it up */
		q = (struct overhead *)( (char *)p + nbytes);
		q->ov_length = surplus;
		p->ov_length = nbytes;
		q->ov_magic = MAGIC_FREE;
		/* add surplus into adjacency chain */
		insque(TOADJ(q),TOADJ(p));
		/* add surplus into bucket chain */
		insque(TOBUK(q),&buckets[_log2(surplus)]);
	}
	p->ov_magic = MAGIC_BUSY;
	return((char*)p + sizeof(struct overhead));
}

void
free(mem)
register char *mem;
{
	register struct overhead *p, *q;
	if (mem == NULL)
		return;
	p = (struct overhead *)(mem - sizeof(struct overhead));
	if (p->ov_magic == MAGIC_FREE)
		return;
	if (p->ov_magic != MAGIC_BUSY) {
		fatal("attempt to free memory not allocated with malloc!\n");
	}
	q = FROMADJ((TOADJ(p))->q_back);
	if (q != FROMADJ(&adjhead)) {	/* q is not the first list item */
		ASSERT(q < p, "\nfree: While trying to merge a free area with \
a lower adjacent free area,\n addresses were found out of order!\n");
		/* If lower segment can be merged */
		if (   q->ov_magic == MAGIC_FREE
		   && (char *)q + q->ov_length == (char *)p
		) {
			/* remove lower address area from bucket chain */
			remque(TOBUK(q));
			/* remove upper address area from adjacency chain */
			remque(TOADJ(p));
			q->ov_length += p->ov_length;
			p->ov_magic = NULL;
			p = q;
		}
	}
	q = FROMADJ((TOADJ(p))->q_forw);
	if (q != FROMADJ(&adjhead)) {	/* q is not the last list item */
		/* upper segment can be merged */
		ASSERT(q > p, "\nfree: While trying to merge a free area with \
a higher adjacent free area,\n addresses were found out of order!\n");
		if ( 	q->ov_magic == MAGIC_FREE
		   &&	(char *)p + p->ov_length == (char *)q
		) {
			/* remove upper from bucket chain */
			remque(TOBUK(q));
			/* remove upper from adjacency chain */
			remque(TOADJ(q));
			p->ov_length += q->ov_length;
			q->ov_magic = NULL;
		}
	}
#ifdef USE_SBRK
	if (	/* freed area is at end of memory */
		endfree && adjhead.q_back == TOADJ(p)
	    &&	(char*)p + p->ov_length == (char *)CURBRK
	) {
		/* remove from end of adjacency chain */
		remque(adjhead.q_back);
		/* release memory to system */
		sbrk( -((int)(p->ov_length)));
		return;
	}
#endif
	p->ov_magic = MAGIC_FREE;
	/* place in bucket chain */
	insque(TOBUK(p),&buckets[_log2(p->ov_length)]);
	return;
}

char *
realloc(mem,nbytes)
register char *mem; unsigned nbytes;
{
	register char *newmem;
	register struct overhead *p, *q;
	register int surplus;
	if (mem == NULL)
		return(malloc(nbytes));
	if(mem > (char*)FROMADJ(adjhead.q_back) + sizeof(struct overhead))
		return(NULL);
	
	p = (struct overhead *)(mem - sizeof(struct overhead));
	nbytes = (nbytes + (NALIGN-1)) & (~(NALIGN-1));
	if (  p->ov_magic == MAGIC_BUSY
	   && (q = FROMADJ(adjhead.q_back)) != p
	   && (q->ov_magic != MAGIC_FREE || (FROMADJ(q->ov_adj.q_back) != p))
	)
		free(mem);
	if( (p->ov_magic == MAGIC_BUSY || p->ov_magic == MAGIC_FREE)
	 && (surplus = p->ov_length - nbytes - sizeof(struct overhead)) >= 0
	) {
		if (surplus > sizeof(struct overhead)) {
			/*  return surplus to free list */
			nbytes += sizeof(struct overhead);
#ifdef USE_SBRK
			if (	/* freed area is at end of memory */
				endfree && adjhead.q_back == TOADJ(p)
			  &&	(char*)p + p->ov_length == (char *)CURBRK
			) {
				/* release memory to system */
				sbrk(-surplus);
			} else
#endif
			{
				q = (struct overhead *)( (char *)p + nbytes);
				q->ov_length = surplus;
				q->ov_magic = MAGIC_FREE;
				insque(TOADJ(q),TOADJ(p));
				insque(TOBUK(q),&buckets[_log2(surplus)]);
			}
			p->ov_length = nbytes;
		}
		if (p->ov_magic == MAGIC_FREE) {
			remque(TOBUK(p));
			p->ov_magic = MAGIC_BUSY;
		}
		return(mem);
	}
	newmem = malloc(nbytes);
	if (newmem != mem && newmem != NULL) {
		register int n;
		if (p->ov_magic == MAGIC_BUSY || p->ov_magic == MAGIC_FREE) {
			n = p->ov_length - sizeof(struct overhead);
			nbytes = (nbytes < n) ? nbytes : n ;
		}
		memcpy(newmem,mem,nbytes);
	}
	if (p->ov_magic == MAGIC_BUSY)
		free(mem);
	return(newmem);
}

_log2(n)
register int n;
{
	register int i = 0;
	while ((n >>= 1) > 0)
		i++;
	return(i);
}

void
insque(item,queu)
register struct qelem *item, *queu;
{
	register struct qelem *pueu;
	pueu = queu->q_forw;
	item->q_forw = pueu;
	item->q_back = queu;
	queu->q_forw = item;
	pueu->q_back = item;
}

void
remque(item)
register struct qelem *item;
{
	register struct qelem *queu, *pueu;
	pueu = item->q_forw;
	queu = item->q_back;
	queu->q_forw = pueu;
	pueu->q_back = queu;
}

// LICENSE_BEGIN
// Copyright (c) 2006 FirmWorks
// 
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// LICENSE_END
