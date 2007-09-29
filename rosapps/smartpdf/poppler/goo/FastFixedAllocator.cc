/* Written by: Krzysztof Kowalczyk (http://blog.kowalczyk.info)
   License: Public Domain (http://creativecommons.org/licenses/publicdomain/)
   Take all the code you want, I'll just write more.
*/

#include "FastFixedAllocator.h"
#include <stdlib.h>
#include <assert.h>

void *FastFixedAllocator::Alloc(size_t size) {
  char *                p;
  void *                toReturn = NULL;
  ChunkBlockNode *      currChunk;
  int                   totalBlockSize;
  bool                  needsToAllocateNewBlock;
  FreeListNode *        freeListNode;

#ifdef MULTITHREADED
  gLockMutex(&allocMutex);
#endif

  /* can we satisfy the allocation from the free list ? */
  if (freeListRoot) {
    freeListNode = freeListRoot;
    freeListRoot = freeListRoot->next;
    freeListNode->next = magicCookie; /* mark as being used */
    p = (char*)freeListNode;
    toReturn = (void*) (p + sizeof(FreeListNode));
#ifdef FAST_ALLOC_STATS
    totalAllocs++;
    allocsFromFreeList++;
#endif
    goto Exit;
  }

  /* can we satisfy the allocation from the last allocated block?
     We only have to look at the last allocated chunk since
     the lack of free list entries means all data in chunks is currently used */
  currChunk = chunkBlockListRoot;
  needsToAllocateNewBlock = true;

#ifdef FAST_ALLOC_STATS
  totalAllocs++;
#endif

  if (currChunk) {
    assert(currChunk->chunksUsed >= 0);
    assert(currChunk->chunksUsed <= chunksAtATime);
    if (currChunk->chunksUsed < chunksAtATime) {
        /* we still have space in the last allocated block */
        needsToAllocateNewBlock = false;
    }
  }

  if (needsToAllocateNewBlock) {
    totalBlockSize = GetBlockSize();
    currChunk = (ChunkBlockNode*)malloc(totalBlockSize);
    if (!currChunk)
        goto Exit;
    currChunk->chunksUsed = 0;
    currChunk->next = chunkBlockListRoot;
    chunkBlockListRoot = currChunk;
  }
#ifdef FAST_ALLOC_STATS
  else {
    allocsFromBlock++;
  }
#endif
    
  p = (char*)currChunk;
  p += sizeof(ChunkBlockNode);
  p += (GetTotalChunkSize() * currChunk->chunksUsed);
  freeListNode = (FreeListNode*)p;
  freeListNode->next = magicCookie; /* mark as being used */
  toReturn = (void*)(p + sizeof(FreeListNode));
  currChunk->chunksUsed++;
  assert(currChunk->chunksUsed <= chunksAtATime);

Exit:
#ifdef MULTITHREADED
    gUnlockMutex(&allocMutex);
#endif
  return toReturn;
}

void FastFixedAllocator::Free(void *p) {
  FreeListNode *    freeListNode;

  assert(AllocatedPointerSlow(p));

#ifdef MULTITHREADED
  gLockMutex(&allocMutex);
#endif
  freeListNode = (FreeListNode*)((char*)p - sizeof(FreeListNode));
  assert(magicCookie == freeListNode->next);
  freeListNode->next = freeListRoot;
  freeListRoot = freeListNode;
#ifdef MULTITHREADED
  gUnlockMutex(&allocMutex);
#endif
}

/* Return true if <pA> is a pointer that belongs to memory that we allocated
   (and is currently used; doesn't work for pointers in our free list
   This check must be fast. */
bool FastFixedAllocator::AllocatedPointerSlow(void *pA) {
  FreeListNode *    freeListNode;
  ChunkBlockNode *  currChunk;
  char *            p, *blockStart, *blockEnd;

  if (!pA) 
    return false;

  /* note: this might actually fail on some systems because we're looking
     at a memory of unknown origin. If the memory before the pointer is not
     accessible, we'll crash. Some debugging memory allocators use a trick
     of protecting memory right before and after the allocated chunk, to
     catch accesses beyond allocated chunks.
     This shouldn't be a problem when running on regular Unix or Windows system.
     I don't have a better idea on how to make this check fast. */
  freeListNode = (FreeListNode*)((char*)pA - sizeof(FreeListNode));
  if (magicCookie != freeListNode->next) {
    /* no magic value so it's not a pointer we allocated */
    return false;
  }

  /* It's possible (although highly unlikely) that a random pointer will have
     our magic value as well, so we still have to traverse all blocks to see
     if a pointer lies within memory allocated for blocks */
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
