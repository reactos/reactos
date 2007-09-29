/*
 * gmem.c
 *
 * Memory routines with out-of-memory checking.
 *
 * Copyright 1996-2003 Glyph & Cog, LLC
 */

#include <assert.h>
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include "gmem.h"
#include "FastAlloc.h"
#ifdef USE_KAZLIB_EXCEPT
#include "except.h"
#endif

#ifdef DEBUG_MEM

typedef struct _GMemHdr {
  int size;
  int index;
  struct _GMemHdr *next;
} GMemHdr;

#define gMemHdrSize ((sizeof(GMemHdr) + 7) & ~7)
#define gMemTrlSize (sizeof(long))

#if gmemTrlSize==8
#define gMemDeadVal 0xdeadbeefdeadbeefUL
#else
#define gMemDeadVal 0xdeadbeefUL
#endif

/* round data size so trailer will be aligned */
#define gMemDataSize(size) \
  ((((size) + gMemTrlSize - 1) / gMemTrlSize) * gMemTrlSize)

#define gMemNLists    64
#define gMemListShift  4
#define gMemListMask  (gMemNLists - 1)
static GMemHdr *gMemList[gMemNLists] = {
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

static int gMemIndex = 0;
static int gMemAlloc = 0;
static int gMemInUse = 0;

void *gmalloc(size_t size) {
  size_t size1;
  char *mem;
  GMemHdr *hdr;
  void *data;
  int lst;
  unsigned long *trl, *p;

  if (size <= 0)
    return NULL;
  size1 = gMemDataSize(size);
  if (!(mem = (char *)malloc(size1 + gMemHdrSize + gMemTrlSize))) {
    fprintf(stderr, "Out of memory\n");
    exit(1);
  }
  hdr = (GMemHdr *)mem;
  data = (void *)(mem + gMemHdrSize);
  trl = (unsigned long *)(mem + gMemHdrSize + size1);
  hdr->size = size;
  hdr->index = gMemIndex++;
  lst = ((int)hdr >> gMemListShift) & gMemListMask;
  hdr->next = gMemList[lst];
  gMemList[lst] = hdr;
  ++gMemAlloc;
  gMemInUse += size;
  for (p = (unsigned long *)data; p <= trl; ++p)
    *p = gMemDeadVal;
  return data;
}

void *grealloc(void *p, size_t size) {
  GMemHdr *hdr;
  void *q;
  size_t oldSize;

  if (size <= 0) {
    if (p)
      gfree(p);
    return NULL;
  }
  if (p) {
    hdr = (GMemHdr *)((char *)p - gMemHdrSize);
    oldSize = hdr->size;
    q = gmalloc(size);
    memcpy(q, p, size < oldSize ? size : oldSize);
    gfree(p);
  } else {
    q = gmalloc(size);
  }
  return q;
}

void gfree(void *p) {
  size_t size;
  GMemHdr *hdr;
  GMemHdr *prevHdr, *q;
  int lst;
  unsigned long *trl, *clr;

  if (p) {
    hdr = (GMemHdr *)((char *)p - gMemHdrSize);
    lst = ((int)hdr >> gMemListShift) & gMemListMask;
    for (prevHdr = NULL, q = gMemList[lst]; q; prevHdr = q, q = q->next) {
      if (q == hdr)
	break;
    }
    if (q) {
      if (prevHdr)
	prevHdr->next = hdr->next;
      else
	gMemList[lst] = hdr->next;
      --gMemAlloc;
      gMemInUse -= hdr->size;
      size = gMemDataSize(hdr->size);
      trl = (unsigned long *)((char *)hdr + gMemHdrSize + size);
      if (*trl != gMemDeadVal) {
	fprintf(stderr, "Overwrite past end of block %d at address %p\n",
		hdr->index, p);
      }
      for (clr = (unsigned long *)hdr; clr <= trl; ++clr)
	*clr = gMemDeadVal;
      free(hdr);
    } else {
      fprintf(stderr, "Attempted to free bad address %p\n", p);
    }
  }
}

void gMemReport(FILE *f) {
  GMemHdr *p;
  int lst;

  fprintf(f, "%d memory allocations in all\n", gMemIndex);
  if (gMemAlloc > 0) {
    fprintf(f, "%d memory blocks left allocated:\n", gMemAlloc);
    fprintf(f, " index     size\n");
    fprintf(f, "-------- --------\n");
    for (lst = 0; lst < gMemNLists; ++lst) {
      for (p = gMemList[lst]; p; p = p->next)
	fprintf(f, "%8d %8d\n", p->index, p->size);
    }
  } else {
    fprintf(f, "No memory blocks left allocated\n");
  }
}
#else /* DEBUG_MEM */

#ifdef DEBUG
/* You can simiulate low-memory under simulator by limiting the max size of
   allocation in debug build by setting env variable PDF_ALLOC_LIMIT */
#define NO_ALLOC_LIMIT (size_t)-1
#define LIMIT_NOT_CHECKED (size_t)-2
static size_t gAllocLimit = LIMIT_NOT_CHECKED;

static int OverLimit(size_t size) {
  if (LIMIT_NOT_CHECKED == gAllocLimit) {
    char *limit = getenv("PDF_ALLOC_LIMIT");
    if (limit) {
      gAllocLimit = atoi(limit);
      printf("Max alloc is %d\n", (int)gAllocLimit);
    } else {
      gAllocLimit = NO_ALLOC_LIMIT;
    }
  }
  if ((NO_ALLOC_LIMIT != gAllocLimit) && (size > gAllocLimit))
      return 1;
  return 0;
}
#else
#define OverLimit(n) 0
#endif

void gfree(void *p) {
#ifdef USE_FAST_ALLOC
  fast_free(p);
#else
  free(p);
#endif
}

void *gmalloc(size_t size) {
  void *p = NULL;
  if (!OverLimit(size)) {
  #ifdef USE_FAST_ALLOC
    p = fast_malloc(size);
  #else
    if (size <= 0)
      return NULL;
    p = malloc(size);
  #endif
  }
#ifdef USE_KAZLIB_EXCEPT
  if ((0 != size) && (NULL == p))
      except_throw(1, 1, "out of memory");
#endif
  return p;
}

void *grealloc(void *p, size_t size) {
  void *q = NULL;
  if (!OverLimit(size)) {
  #ifdef USE_FAST_ALLOC
    q = fast_realloc(p, size);
  #else
    if (size <= 0) {
      if (p)
        free(p);
      return NULL;
    }
    if (p)
      q = realloc(p, size);
    else
      q = malloc(size);
#endif
  }
#ifdef USE_KAZLIB_EXCEPT
  if ((0 != size) && (NULL == q))
      except_throw(1, 1, "out of memory");
#endif
  return q;
}
#endif /* !DEBUG_MEM */

void *gmallocn(int nObjs, int objSize) {
  int n;

  n = nObjs * objSize;
  assert((objSize != 0) && (n / objSize == nObjs));
  if (objSize == 0 || n / objSize != nObjs) {
    fprintf(stderr, "Bogus memory allocation size\n");
    exit(1);
  }
  return gmalloc(n);
}

void *greallocn(void *p, int nObjs, int objSize) {
  int n;

  n = nObjs * objSize;
  assert((objSize != 0) && (n / objSize == nObjs));
  if (objSize == 0 || n / objSize != nObjs) {
    fprintf(stderr, "Bogus memory allocation size\n");
    exit(1);
  }
  return grealloc(p, n);
}

char *copyString(char *s) {
  char *s1;

  int len = strlen(s) + 1;
  s1 = (char *)gmalloc(len);
  strcpy(s1, s);
  return s1;
}
