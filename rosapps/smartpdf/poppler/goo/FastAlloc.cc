/* Written by: Krzysztof Kowalczyk (http://blog.kowalczyk.info)
   License: Public Domain (http://creativecommons.org/licenses/publicdomain/)
   Take all the code you want, I'll just write more.
*/

#include "FastAlloc.h"
#include <assert.h>
#include "FastFixedAllocator.h"
#include "GooMutex.h"
#include "gmem.h"
#include "gtypes.h"
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>

//#define DO_MEM_STATS 1
#ifdef DO_MEM_STATS

#define ALLOCS_AT_A_TIME 1024

typedef struct {
    size_t  size;
    void *  p;
} AllocRecord;

static AllocRecord gAllocs[ALLOCS_AT_A_TIME];
static int gAllocsCount = 0;

#define ITS_A_FREE (size_t)-1

#define MAX_BUF_FILENAME_SIZE    256
#define LOG_BUF_SIZE            2048

static char             gLogFileName[MAX_BUF_FILENAME_SIZE] = {0};
static char             gLogBuf[LOG_BUF_SIZE];
static const char *     gLogFileNamePattern = "fast_alloc_%d.txt";
static int              gLogBufUsed = 0;

#ifdef MULTITHREADED
GooMutex logMutex;
#endif

static GBool FileExists(const char *fileName)
{
  struct stat buf;
  int         res;
  
  res = stat(fileName, &buf);
  if (0 != res)
    return gFalse;
  return gTrue;
}

void LogSetFileNamePattern(const char *fileNamePattern)
{
  assert(!gLogFileNamePattern);
  gLogFileNamePattern = fileNamePattern;
}

static GBool LogGenUniqueFile(void)
{
  static GBool tried = gFalse;
  int          n = 0;
  
  assert(gLogFileNamePattern);
  if (gLogFileName[0])
    return gTrue; /* already generated */
  if (tried)
    return gFalse;
  tried = gTrue;
  while (n < 999) {
    sprintf(gLogFileName, gLogFileNamePattern, n);
    if (!FileExists(gLogFileName))
      return gTrue;
    ++n;
  }
  gLogFileName[0] = 0;
  assert(0);
  return gFalse;
}

static void LogAllocs(void)
{
  FILE *  fp;
  int     i;
  char    buf[256];
  int     len;

  assert(gAllocsCount <= ALLOCS_AT_A_TIME);
  if (0 == gAllocsCount)
    return;

  if (!LogGenUniqueFile())
    return;
  fp = fopen(gLogFileName, "a");
  if (!fp)
    return;

  for (i = 0; i < gAllocsCount; i++) {
    if (ITS_A_FREE == gAllocs[i].size) {
       len = sprintf(buf, "- 0x%p\n", gAllocs[i].p);
    } else {
       len = sprintf(buf, "+ 0x%p 0x%x\n", gAllocs[i].p, gAllocs[i].size);
    }
    assert(len >= 0);
    if (len > 0)
      fwrite(buf, 1, len, fp);
  }
  fclose(fp);
}

void RecordEntry(void *p, size_t size)
{
#ifdef MULTITHREADED
  gLockMutex(&logMutex);
#endif
  if (ALLOCS_AT_A_TIME == gAllocsCount) {
    LogAllocs();
    gAllocsCount = 0;
  }
  assert(gAllocsCount < ALLOCS_AT_A_TIME);
  gAllocs[gAllocsCount].size = size;
  gAllocs[gAllocsCount].p = p;
  ++gAllocsCount;
#ifdef MULTITHREADED
  gUnlockMutex(&logMutex);
#endif
}

void RecordMalloc(void *p, size_t size)
{
  assert(size != ITS_A_FREE);
  if (p)
    RecordEntry(p, size);
}

void RecordFree(void *p)
{
  if (p)
    RecordEntry(p, ITS_A_FREE);
}

void RecordRealloc(void *oldP, void *newP, size_t newSize)
{
    RecordFree(oldP);
    RecordMalloc(newP, newSize);
}

/* To make sure that we log everything without explicitly calling
   LogAllocs() at the end, this class' destructor should be called by C++
   runtime at the end of program */
class EnsureLastAllocsLogged {
public:
    EnsureLastAllocsLogged() {
    }
    ~EnsureLastAllocsLogged(void) { 
        LogAllocs(); 
    }
};
static EnsureLastAllocsLogged gEnsureLastAllocsLogged;

#ifdef MULTITHREADED
static MutexAutoInitDestroy gLogMutexInit(&logMutex);
#endif

#else
#define RecordMalloc(p, size)
#define RecordFree(p)
#define RecordRealloc(oldP, newP, newSize)
#endif

#ifdef USE_FAST_ALLOC
void malloc_hook(void *p, size_t size)
{
  RecordMalloc(p, size);
}

void free_hook(void *p)
{
  RecordFree(p);
}

void realloc_hook(void *oldP, void *newP, size_t newSize)
{
  RecordRealloc(oldP, newP, newSize);
}

#define MAGIC_COOKIE_FOR_0_16 (FreeListNode*)0xa1b2c3d4
#define MAGIC_COOKIE_FOR_32 (FreeListNode*)0x11326374
#define MAGIC_COOKIE_FOR_88 (FreeListNode*)0x91a2c8d9
#define MAGIC_COOKIE_FOR_20_24 (FreeListNode*)0x51783c4d

/* Acording to my profiles, covering allocations in ranges <0-16>, <20-24>,
   32 and 88 covers 89% allocations (in scenario I've measured which was
   loading a PDF document PDFReference16.pdf (available at adobe website).
   They're also ordered by frequence (sizes most frequently allocated first) */
#define ALLOCATORS_COUNT 4
static FastFixedAllocator gAllocators[ALLOCATORS_COUNT] = {
    FastFixedAllocator(0,16,4096,MAGIC_COOKIE_FOR_0_16),
    FastFixedAllocator(32,32,1024,MAGIC_COOKIE_FOR_32),
    FastFixedAllocator(88,88,1024,MAGIC_COOKIE_FOR_88),
    FastFixedAllocator(20,24,1024,MAGIC_COOKIE_FOR_20_24),
};

/* Return an allocator for a given size or NULL if doesn't exist.
   Speed of this function is critical so it was inlined */
static inline FastFixedAllocator *GetAllocatorForSize(size_t size)
{
  if ((size > 0) && (size <= 16))
    return &gAllocators[0];
  else if (32 == size)
    return &gAllocators[1];
  else if (88 == size)
    return &gAllocators[2];
  else if ((size >= 20) && (size <= 24))
    return &gAllocators[3];
  else
    return NULL;
}

/* Return an allocator that allocated this pointer or NULL if comes
   from system alloc.
   Speed of this function is critical so it was inlined */
static inline FastFixedAllocator *GetAllocatorForPointer(void *p)
{
  FreeListNode *    freeListNode;
  if (!p)
    return NULL;
  freeListNode = (FreeListNode*)((char*)p - sizeof(FreeListNode));
  if (MAGIC_COOKIE_FOR_0_16 == freeListNode->next) {
    if (gAllocators[0].AllocatedPointerFast(p)) {
        return &gAllocators[0];
    }
  } else if (MAGIC_COOKIE_FOR_32 == freeListNode->next) {
    if (gAllocators[1].AllocatedPointerFast(p)) {
        return &gAllocators[1];
    }
  } else if (MAGIC_COOKIE_FOR_88 == freeListNode->next) {
    if (gAllocators[2].AllocatedPointerFast(p)) {
        return &gAllocators[2];
    }
  } else if (MAGIC_COOKIE_FOR_20_24 == freeListNode->next) {
    if (gAllocators[3].AllocatedPointerFast(p)) {
        return &gAllocators[3];
    }
  }
  return NULL;
}

void *fast_malloc(size_t size)
{
  FastFixedAllocator *allocator;
  void *              p = NULL;

  allocator = GetAllocatorForSize(size);
  if (allocator) {
    p = allocator->Alloc(size);
  }

  if (!p)
    p = malloc(size);
  malloc_hook(p, size);
  return p;
}

void fast_free(void *p)
{
  FastFixedAllocator *allocator;
  free_hook(p);
  allocator = GetAllocatorForPointer(p);
  if (allocator) {
    allocator->Free(p);
  } else
    free(p);
}

void *fast_realloc(void *oldP, size_t size)
{
  FastFixedAllocator *  allocator;
  void *                newP = NULL;
  size_t                oldSize;

  if (!oldP)
    return fast_malloc(size);

  allocator = GetAllocatorForPointer(oldP);
  if (allocator) {
    if (size > 0) {
      oldSize = allocator->AllocationSize();
      if (size > oldSize) {
        newP = fast_malloc(size);
        memcpy(newP, oldP, oldSize);
      } else
        newP = oldP;
    }
    allocator->Free(oldP);
  }
  else
    newP = realloc(oldP, size);
  realloc_hook(oldP, newP, size);
  return newP;
}
#endif

#ifndef DEBUG_MEM
void *operator new(size_t size) {
  return gmalloc(size);
}

void *operator new[](size_t size) {
  return gmalloc(size);
}

void operator delete(void *p) {
  gfree(p);
}

void operator delete[](void *p) {
  gfree(p);
}
#endif
