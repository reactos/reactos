/*
 * GLX Hardware Device Driver common code
 * Copyright (C) 1999 Keith Whitwell
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * KEITH WHITWELL, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef MM_INC
#define MM_INC

struct mem_block_t {
  struct mem_block_t *next;
  struct mem_block_t *heap;
  int ofs,size;
  int align;
  int free:1;
  int reserved:1;
};
typedef struct mem_block_t TMemBlock;
typedef struct mem_block_t *PMemBlock;

/* a heap is just the first block in a chain */
typedef struct mem_block_t memHeap_t;

static __inline__ int mmBlockSize(PMemBlock b)
{ return b->size; }

static __inline__ int mmOffset(PMemBlock b)
{ return b->ofs; }

/* 
 * input: total size in bytes
 * return: a heap pointer if OK, NULL if error
 */
memHeap_t *mmInit( int ofs, int size );

/*
 * Allocate 'size' bytes with 2^align2 bytes alignment,
 * restrict the search to free memory after 'startSearch'
 * depth and back buffers should be in different 4mb banks
 * to get better page hits if possible
 * input:	size = size of block
 *       	align2 = 2^align2 bytes alignment
 *		startSearch = linear offset from start of heap to begin search
 * return: pointer to the allocated block, 0 if error
 */
PMemBlock  mmAllocMem( memHeap_t *heap, int size, int align2, 
		       int startSearch );

/*
 * Free block starts at offset
 * input: pointer to a block
 * return: 0 if OK, -1 if error
 */
int  mmFreeMem( PMemBlock b );

/*
 * destroy MM
 */
void mmDestroy( memHeap_t *mmInit );

/* For debuging purpose. */
void mmDumpMemInfo( memHeap_t *mmInit );

#endif
