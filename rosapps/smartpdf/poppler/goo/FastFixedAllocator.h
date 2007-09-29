#ifndef FAST_FIXED_ALLOCATOR_H_
#define FAST_FIXED_ALLOCATOR_H_

/* Written by: Krzysztof Kowalczyk (http://blog.kowalczyk.info)
   License: Public Domain (http://creativecommons.org/licenses/publicdomain/)
   Take all the code you want, I'll just write more.
*/

#include <stdlib.h>
#include <assert.h>
#include "GooMutex.h"

/*
This is a a specialized allocator design to allocate large numbers of small
objects (chunks) quickly.

It does that by employing two techniques:
- allocating blocks of objects (chunks) at a time
- a free list of all freed allocations

By allocating memory in blocks it makes less trips to system memory allocator.

By using free list it can satisfy new allocations from memory that has been
previously allocated and freed. This helps especially if there's a lot of
trashing (allocating/freeing).

It adds sizeof(char*) overhead for each allocation i.e. on 32-bit machine
allocating 4 byte chunks uses 8 bytes. The overhead is for storing a pointer
to the next free chunk if the chunk is on free list.

This is ok, because we actually use less memory than most OS allocators, since:
- OS allocators also have overhead for keeping track of allocation, which is
  likely to be bigger than sizeof(char*)
- OS allocators round allocation (e.g. to 16-byte boundary on Ubuntu 6 libc)
*/

/* Define if you want statistics about how many allocations there were and
   what is our hit ratio (number of allocations served from free list and
   from existing block of memory) */
#define FAST_ALLOC_STATS 1

/* We prepend this to every allocated chunk */
typedef struct FreeListNode {
    struct FreeListNode *next;
} FreeListNode;

/* Keeps info about a bunch of chunks allocated as one block */
typedef struct ChunkBlockNode {
    struct ChunkBlockNode * next;
    int                     chunksUsed;
    /* data comes inline here */
} ChunkBlockNode;

class FastFixedAllocator {

public:
  FastFixedAllocator(size_t chunkSizeLowA, size_t chunkSizeHighA, 
                     int chunksAtATimeA, FreeListNode *magicCookieA) : 
    chunkSizeLow(chunkSizeLowA), 
    chunkSizeHigh(chunkSizeHighA),
    magicCookie(magicCookieA),
    chunksAtATime(chunksAtATimeA),
    freeListRoot(NULL), chunkBlockListRoot(NULL)
#ifdef FAST_ALLOC_STATS
    , totalAllocs(0), allocsFromFreeList(0), allocsFromBlock(0)
#endif
    {
#ifdef MULTITHREADED
    gInitMutex(&allocMutex);
#endif
  }
  ~FastFixedAllocator(void) {
#ifdef MULTITHREADED
    gDestroyMutex(&allocMutex);
#endif
  }

  bool    HandlesSize(size_t size) {
    if ((size <= chunkSizeHigh) && (size >= chunkSizeLow))
        return true;
    return false;
  }

  size_t  AllocationSize(void) { return chunkSizeHigh; }
  void *  Alloc(size_t size);
  void    Free(void* p);
  bool    AllocatedPointerSlow(void *pA);

  /* like AllcoatedPointerSlow but assumes that NULL check and magicCookie
     check has already been performed. Also, forced inline by being defined
     in a header file. Speed of this function is critical to the allocator */
  bool AllocatedPointerFast(void *pA) {
    ChunkBlockNode *  currChunk;
    char *            p, *blockStart, *blockEnd;

#if DEBUG  
    assert(pA);
    FreeListNode *    freeListNode;
    freeListNode = (FreeListNode*)((char*)pA - sizeof(FreeListNode));
    assert(magicCookie == freeListNode->next);
#endif

    p = (char*)pA - sizeof(FreeListNode);
    currChunk = chunkBlockListRoot;
    while (currChunk) {
      blockStart = (char*)currChunk;
      blockEnd = blockStart + GetBlockSize();
      blockStart += sizeof(ChunkBlockNode);
      if ((p >= blockStart) && (p < blockEnd)) {
  #ifdef DEBUG
        /* something's wrong if it looks like allocated by us but not on a proper
           boundary */
        unsigned long pos = (unsigned long)(p - blockStart);
        assert(0 == pos % GetTotalChunkSize());
  #endif
        return true;
      }
      currChunk = currChunk->next;
    }
    return false;
}

private:
  int     GetTotalChunkSize(void) {
    return sizeof(FreeListNode) + chunkSizeHigh;
  }
  int     GetBlockSize(void) {
    return sizeof(ChunkBlockNode) + (GetTotalChunkSize() * chunksAtATime);
  }

  /* This allocator handles allocations for allocation sizes 
     in range <chunkSizeLow, chunkSizeHigh> */
  size_t            chunkSizeLow;
  size_t            chunkSizeHigh;

  /* We need a relly fast check for deciding whether a given pointer was allocated
     by us or not, so that we know we can put it on a free list when it's being
     freed.
     We do that by putting a magic value (<magicCookie> in the place used to store 
     pointer to next free chunk when this value is on a free list. That way
     if the magic value is not there, we know it's not pointer to memory we
     allocated (should be majority of cases).
     <magicCookie> for each instance should be unique (otherwise it'll be slow)
  */
  FreeListNode *    magicCookie;

  int               chunksAtATime;
  FreeListNode *    freeListRoot;
  ChunkBlockNode *  chunkBlockListRoot;

#ifdef FAST_ALLOC_STATS
  /* gather statistics about allocations */
  int               totalAllocs;
  int               allocsFromFreeList;
  int               allocsFromBlock;
#endif
#ifdef MULTITHREADED
  GooMutex          allocMutex;
#endif
};

#endif
