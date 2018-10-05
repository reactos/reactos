/*
 * msvcrt.dll heap functions
 *
 * Copyright 2000 Jon Griffiths
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * Note: Win32 heap operations are MT safe. We only lock the new
 *       handler and non atomic heap operations
 */

#include <precomp.h>
#include <malloc.h>

#define MSVCRT_size_t size_t
#define MSVCRT_intptr_t intptr_t
#define MSVCRT_wchar_t wchar_t
#define MSVCRT__HEAPBADNODE _HEAPBADNODE
#define MSVCRT__HEAPOK _HEAPOK
#define MSVCRT__HEAPEND _HEAPEND
#define MSVCRT__FREEENTRY _FREEENTRY
#define MSVCRT__USEDENTRY _USEDENTRY
#define MSVCRT__HEAPBADBEGIN _HEAPBADBEGIN
#define MSVCRT_EINVAL EINVAL
#define MSVCRT_ENOSYS ENOSYS
#define MSVCRT_ENOMEM ENOMEM
#define MSVCRT_ERANGE ERANGE
#define MSVCRT__TRUNCATE _TRUNCATE
#define MSVCRT__heapinfo _heapinfo
#define MSVCRT__errno _errno
#define MSVCRT_calloc calloc
#define MSVCRT_malloc malloc
#define MSVCRT_realloc realloc
#define MSVCRT_free free
#define MSVCRT_memcpy_s memcpy_s
#define MSVCRT_memmove_s memmove_s
#define MSVCRT_strncpy_s strncpy_s
#define msvcrt_set_errno _dosmaperr

/* MT */
#define LOCK_HEAP   _mlock( _HEAP_LOCK )
#define UNLOCK_HEAP _munlock( _HEAP_LOCK )

/* _aligned */
#define SAVED_PTR(x) ((void *)((DWORD_PTR)((char *)x - sizeof(void *)) & \
                               ~(sizeof(void *) - 1)))
#define ALIGN_PTR(ptr, alignment, offset) ((void *) \
    ((((DWORD_PTR)((char *)ptr + alignment + sizeof(void *) + offset)) & \
      ~(alignment - 1)) - offset))

#define SB_HEAP_ALIGN 16

static HANDLE heap, sb_heap;

typedef int (CDECL *MSVCRT_new_handler_func)(MSVCRT_size_t size);

static MSVCRT_new_handler_func MSVCRT_new_handler;
static int MSVCRT_new_mode;

/* FIXME - According to documentation it should be 8*1024, at runtime it returns 16 */ 
static unsigned int MSVCRT_amblksiz = 16;
/* FIXME - According to documentation it should be 480 bytes, at runtime default is 0 */
static MSVCRT_size_t MSVCRT_sbh_threshold = 0;

static void* msvcrt_heap_alloc(DWORD flags, MSVCRT_size_t size)
{
    if(size < MSVCRT_sbh_threshold)
    {
        void *memblock, *temp, **saved;

        temp = HeapAlloc(sb_heap, flags, size+sizeof(void*)+SB_HEAP_ALIGN);
        if(!temp) return NULL;

        memblock = ALIGN_PTR(temp, SB_HEAP_ALIGN, 0);
        saved = SAVED_PTR(memblock);
        *saved = temp;
        return memblock;
    }

    return HeapAlloc(heap, flags, size);
}

static void* msvcrt_heap_realloc(DWORD flags, void *ptr, MSVCRT_size_t size)
{
    if(sb_heap && ptr && !HeapValidate(heap, 0, ptr))
    {
        /* TODO: move data to normal heap if it exceeds sbh_threshold limit */
        void *memblock, *temp, **saved;
        MSVCRT_size_t old_padding, new_padding, old_size;

        saved = SAVED_PTR(ptr);
        old_padding = (char*)ptr - (char*)*saved;
        old_size = HeapSize(sb_heap, 0, *saved);
        if(old_size == -1)
            return NULL;
        old_size -= old_padding;

        temp = HeapReAlloc(sb_heap, flags, *saved, size+sizeof(void*)+SB_HEAP_ALIGN);
        if(!temp) return NULL;

        memblock = ALIGN_PTR(temp, SB_HEAP_ALIGN, 0);
        saved = SAVED_PTR(memblock);
        new_padding = (char*)memblock - (char*)temp;

        if(new_padding != old_padding)
            memmove(memblock, (char*)temp+old_padding, old_size>size ? size : old_size);

        *saved = temp;
        return memblock;
    }

    return HeapReAlloc(heap, flags, ptr, size);
}

static BOOL msvcrt_heap_free(void *ptr)
{
    if(sb_heap && ptr && !HeapValidate(heap, 0, ptr))
    {
        void **saved = SAVED_PTR(ptr);
        return HeapFree(sb_heap, 0, *saved);
    }

    return HeapFree(heap, 0, ptr);
}

static MSVCRT_size_t msvcrt_heap_size(void *ptr)
{
    if(sb_heap && ptr && !HeapValidate(heap, 0, ptr))
    {
        void **saved = SAVED_PTR(ptr);
        return HeapSize(sb_heap, 0, *saved);
    }

    return HeapSize(heap, 0, ptr);
}

/*********************************************************************
 *		??2@YAPAXI@Z (MSVCRT.@)
 */
void* CDECL DECLSPEC_HOTPATCH MSVCRT_operator_new(MSVCRT_size_t size)
{
  void *retval;
  int freed;
  MSVCRT_new_handler_func handler;

  do
  {
    retval = msvcrt_heap_alloc(0, size);
    if(retval)
    {
      TRACE("(%ld) returning %p\n", size, retval);
      return retval;
    }

    LOCK_HEAP;
    handler = MSVCRT_new_handler;
    if(handler)
      freed = (*handler)(size);
    else
      freed = 0;
    UNLOCK_HEAP;
  } while(freed);

  TRACE("(%ld) out of memory\n", size);
#if _MSVCR_VER >= 80
  throw_exception(EXCEPTION_BAD_ALLOC, 0, "bad allocation");
#endif
  return NULL;
}


/*********************************************************************
 *		??2@YAPAXIHPBDH@Z (MSVCRT.@)
 */
void* CDECL MSVCRT_operator_new_dbg(MSVCRT_size_t size, int type, const char *file, int line)
{
    return MSVCRT_operator_new( size );
}


/*********************************************************************
 *		??3@YAXPAX@Z (MSVCRT.@)
 */
void CDECL DECLSPEC_HOTPATCH MSVCRT_operator_delete(void *mem)
{
  TRACE("(%p)\n", mem);
  msvcrt_heap_free(mem);
}


/*********************************************************************
 *		?_query_new_handler@@YAP6AHI@ZXZ (MSVCRT.@)
 */
MSVCRT_new_handler_func CDECL MSVCRT__query_new_handler(void)
{
  return MSVCRT_new_handler;
}


/*********************************************************************
 *		?_query_new_mode@@YAHXZ (MSVCRT.@)
 */
int CDECL MSVCRT__query_new_mode(void)
{
  return MSVCRT_new_mode;
}

/*********************************************************************
 *		?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z (MSVCRT.@)
 */
MSVCRT_new_handler_func CDECL MSVCRT__set_new_handler(MSVCRT_new_handler_func func)
{
  MSVCRT_new_handler_func old_handler;
  LOCK_HEAP;
  old_handler = MSVCRT_new_handler;
  MSVCRT_new_handler = func;
  UNLOCK_HEAP;
  return old_handler;
}

/*********************************************************************
 *		?set_new_handler@@YAP6AXXZP6AXXZ@Z (MSVCRT.@)
 */
MSVCRT_new_handler_func CDECL MSVCRT_set_new_handler(void *func)
{
  TRACE("(%p)\n",func);
  MSVCRT__set_new_handler(NULL);
  return NULL;
}

/*********************************************************************
 *		?_set_new_mode@@YAHH@Z (MSVCRT.@)
 */
int CDECL MSVCRT__set_new_mode(int mode)
{
  int old_mode;
  LOCK_HEAP;
  old_mode = MSVCRT_new_mode;
  MSVCRT_new_mode = mode;
  UNLOCK_HEAP;
  return old_mode;
}

/*********************************************************************
 *		_callnewh (MSVCRT.@)
 */
int CDECL _callnewh(MSVCRT_size_t size)
{
  int ret = 0;
  MSVCRT_new_handler_func handler = MSVCRT_new_handler;
  if(handler)
    ret = (*handler)(size) ? 1 : 0;
  return ret;
}

/*********************************************************************
 *		_expand (MSVCRT.@)
 */
void* CDECL _expand(void* mem, MSVCRT_size_t size)
{
  return msvcrt_heap_realloc(HEAP_REALLOC_IN_PLACE_ONLY, mem, size);
}

/*********************************************************************
 *		_heapchk (MSVCRT.@)
 */
int CDECL _heapchk(void)
{
  if (!HeapValidate(heap, 0, NULL) ||
          (sb_heap && !HeapValidate(sb_heap, 0, NULL)))
  {
    msvcrt_set_errno(GetLastError());
    return MSVCRT__HEAPBADNODE;
  }
  return MSVCRT__HEAPOK;
}

/*********************************************************************
 *		_heapmin (MSVCRT.@)
 */
int CDECL _heapmin(void)
{
  if (!HeapCompact( heap, 0 ) ||
          (sb_heap && !HeapCompact( sb_heap, 0 )))
  {
    if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
      msvcrt_set_errno(GetLastError());
    return -1;
  }
  return 0;
}

/*********************************************************************
 *		_heapwalk (MSVCRT.@)
 */
int CDECL _heapwalk(struct MSVCRT__heapinfo* next)
{
  PROCESS_HEAP_ENTRY phe;

  if (sb_heap)
      FIXME("small blocks heap not supported\n");

  LOCK_HEAP;
  phe.lpData = next->_pentry;
  phe.cbData = (DWORD)next->_size;
  phe.wFlags = next->_useflag == MSVCRT__USEDENTRY ? PROCESS_HEAP_ENTRY_BUSY : 0;

  if (phe.lpData && phe.wFlags & PROCESS_HEAP_ENTRY_BUSY &&
      !HeapValidate( heap, 0, phe.lpData ))
  {
    UNLOCK_HEAP;
    msvcrt_set_errno(GetLastError());
    return MSVCRT__HEAPBADNODE;
  }

  do
  {
    if (!HeapWalk( heap, &phe ))
    {
      UNLOCK_HEAP;
      if (GetLastError() == ERROR_NO_MORE_ITEMS)
         return MSVCRT__HEAPEND;
      msvcrt_set_errno(GetLastError());
      if (!phe.lpData)
        return MSVCRT__HEAPBADBEGIN;
      return MSVCRT__HEAPBADNODE;
    }
  } while (phe.wFlags & (PROCESS_HEAP_REGION|PROCESS_HEAP_UNCOMMITTED_RANGE));

  UNLOCK_HEAP;
  next->_pentry = phe.lpData;
  next->_size = phe.cbData;
  next->_useflag = phe.wFlags & PROCESS_HEAP_ENTRY_BUSY ? MSVCRT__USEDENTRY : MSVCRT__FREEENTRY;
  return MSVCRT__HEAPOK;
}

/*********************************************************************
 *		_heapset (MSVCRT.@)
 */
int CDECL _heapset(unsigned int value)
{
  int retval;
  struct MSVCRT__heapinfo heap;

  memset( &heap, 0, sizeof(heap) );
  LOCK_HEAP;
  while ((retval = _heapwalk(&heap)) == MSVCRT__HEAPOK)
  {
    if (heap._useflag == MSVCRT__FREEENTRY)
      memset(heap._pentry, value, heap._size);
  }
  UNLOCK_HEAP;
  return retval == MSVCRT__HEAPEND? MSVCRT__HEAPOK : retval;
}

/*********************************************************************
 *		_heapadd (MSVCRT.@)
 */
int CDECL _heapadd(void* mem, MSVCRT_size_t size)
{
  TRACE("(%p,%ld) unsupported in Win32\n", mem,size);
  *MSVCRT__errno() = MSVCRT_ENOSYS;
  return -1;
}

/*********************************************************************
 *		_get_heap_handle (MSVCRT.@)
 */
MSVCRT_intptr_t CDECL _get_heap_handle(void)
{
    return (MSVCRT_intptr_t)heap;
}

/*********************************************************************
 *		_msize (MSVCRT.@)
 */
MSVCRT_size_t CDECL _msize(void* mem)
{
  MSVCRT_size_t size = msvcrt_heap_size(mem);
  if (size == ~(MSVCRT_size_t)0)
  {
    WARN(":Probably called with non wine-allocated memory, ret = -1\n");
    /* At least the Win32 crtdll/msvcrt also return -1 in this case */
  }
  return size;
}

#if _MSVCR_VER>=80
/*********************************************************************
 * _aligned_msize (MSVCR80.@)
 */
size_t CDECL _aligned_msize(void *p, MSVCRT_size_t alignment, MSVCRT_size_t offset)
{
    void **alloc_ptr;

    if(!MSVCRT_CHECK_PMT(p)) return -1;

    if(alignment < sizeof(void*))
        alignment = sizeof(void*);

    alloc_ptr = SAVED_PTR(p);
    return _msize(*alloc_ptr)-alignment-sizeof(void*);
}
#endif

/*********************************************************************
 *		calloc (MSVCRT.@)
 */
void* CDECL DECLSPEC_HOTPATCH MSVCRT_calloc(MSVCRT_size_t count, MSVCRT_size_t size)
{
  MSVCRT_size_t bytes = count*size;

  if (size && bytes / size != count)
  {
    *MSVCRT__errno() = MSVCRT_ENOMEM;
    return NULL;
  }

  return msvcrt_heap_alloc(HEAP_ZERO_MEMORY, bytes);
}

#if _MSVCR_VER>=140
/*********************************************************************
 *		_calloc_base (UCRTBASE.@)
 */
void* CDECL _calloc_base(MSVCRT_size_t count, MSVCRT_size_t size)
{
  return MSVCRT_calloc(count, size);
}
#endif

/*********************************************************************
 *		free (MSVCRT.@)
 */
void CDECL DECLSPEC_HOTPATCH MSVCRT_free(void* ptr)
{
  msvcrt_heap_free(ptr);
}

#if _MSVCR_VER>=140
/*********************************************************************
 *		_free_base (UCRTBASE.@)
 */
void CDECL _free_base(void* ptr)
{
  msvcrt_heap_free(ptr);
}
#endif

/*********************************************************************
 *                  malloc (MSVCRT.@)
 */
void* CDECL MSVCRT_malloc(MSVCRT_size_t size)
{
  void *ret = msvcrt_heap_alloc(0, size);
  if (!ret)
      *MSVCRT__errno() = MSVCRT_ENOMEM;
  return ret;
}

#if _MSVCR_VER>=140
/*********************************************************************
 *                  _malloc_base (UCRTBASE.@)
 */
void* CDECL _malloc_base(MSVCRT_size_t size)
{
  return MSVCRT_malloc(size);
}
#endif

/*********************************************************************
 *		realloc (MSVCRT.@)
 */
void* CDECL DECLSPEC_HOTPATCH MSVCRT_realloc(void* ptr, MSVCRT_size_t size)
{
  if (!ptr) return MSVCRT_malloc(size);
  if (size) return msvcrt_heap_realloc(0, ptr, size);
  MSVCRT_free(ptr);
  return NULL;
}

#if _MSVCR_VER>=140
/*********************************************************************
 *		_realloc_base (UCRTBASE.@)
 */
void* CDECL _realloc_base(void* ptr, MSVCRT_size_t size)
{
  return MSVCRT_realloc(ptr, size);
}
#endif

#if _MSVCR_VER>=80
/*********************************************************************
 * _recalloc (MSVCR80.@)
 */
void* CDECL _recalloc(void *mem, MSVCRT_size_t num, MSVCRT_size_t size)
{
    MSVCRT_size_t old_size;
    void *ret;

    if(!mem)
        return MSVCRT_calloc(num, size);

    size = num*size;
    old_size = _msize(mem);

    ret = MSVCRT_realloc(mem, size);
    if(!ret) {
        *MSVCRT__errno() = MSVCRT_ENOMEM;
        return NULL;
    }

    if(size>old_size)
        memset((BYTE*)ret+old_size, 0, size-old_size);
    return ret;
}
#endif

/*********************************************************************
 *		__p__amblksiz (MSVCRT.@)
 */
unsigned int* CDECL __p__amblksiz(void)
{
  return &MSVCRT_amblksiz;
}

/*********************************************************************
 *		_get_sbh_threshold (MSVCRT.@)
 */
MSVCRT_size_t CDECL _get_sbh_threshold(void)
{
  return MSVCRT_sbh_threshold;
}

/*********************************************************************
 *		_set_sbh_threshold (MSVCRT.@)
 */
int CDECL _set_sbh_threshold(MSVCRT_size_t threshold)
{
#ifdef _WIN64
  return 0;
#else
  if(threshold > 1016)
     return 0;

  if(!sb_heap)
  {
      sb_heap = HeapCreate(0, 0, 0);
      if(!sb_heap)
          return 0;
  }

  MSVCRT_sbh_threshold = (threshold+0xf) & ~0xf;
  return 1;
#endif
}

/*********************************************************************
 *		_aligned_free (MSVCRT.@)
 */
void CDECL _aligned_free(void *memblock)
{
    TRACE("(%p)\n", memblock);

    if (memblock)
    {
        void **saved = SAVED_PTR(memblock);
        MSVCRT_free(*saved);
    }
}

/*********************************************************************
 *		_aligned_offset_malloc (MSVCRT.@)
 */
void * CDECL _aligned_offset_malloc(MSVCRT_size_t size, MSVCRT_size_t alignment, MSVCRT_size_t offset)
{
    void *memblock, *temp, **saved;
    TRACE("(%lu, %lu, %lu)\n", size, alignment, offset);

    /* alignment must be a power of 2 */
    if ((alignment & (alignment - 1)) != 0)
    {
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return NULL;
    }

    /* offset must be less than size */
    if (offset && offset >= size)
    {
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return NULL;
    }

    /* don't align to less than void pointer size */
    if (alignment < sizeof(void *))
        alignment = sizeof(void *);

    /* allocate enough space for void pointer and alignment */
    temp = MSVCRT_malloc(size + alignment + sizeof(void *));

    if (!temp)
        return NULL;

    /* adjust pointer for proper alignment and offset */
    memblock = ALIGN_PTR(temp, alignment, offset);

    /* Save the real allocation address below returned address */
    /* so it can be found later to free. */
    saved = SAVED_PTR(memblock);
    *saved = temp;

    return memblock;
}

/*********************************************************************
 *		_aligned_malloc (MSVCRT.@)
 */
void * CDECL _aligned_malloc(MSVCRT_size_t size, MSVCRT_size_t alignment)
{
    TRACE("(%lu, %lu)\n", size, alignment);
    return _aligned_offset_malloc(size, alignment, 0);
}

/*********************************************************************
 *		_aligned_offset_realloc (MSVCRT.@)
 */
void * CDECL _aligned_offset_realloc(void *memblock, MSVCRT_size_t size,
                                     MSVCRT_size_t alignment, MSVCRT_size_t offset)
{
    void * temp, **saved;
    MSVCRT_size_t old_padding, new_padding, old_size;
    TRACE("(%p, %lu, %lu, %lu)\n", memblock, size, alignment, offset);

    if (!memblock)
        return _aligned_offset_malloc(size, alignment, offset);

    /* alignment must be a power of 2 */
    if ((alignment & (alignment - 1)) != 0)
    {
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return NULL;
    }

    /* offset must be less than size */
    if (offset >= size)
    {
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return NULL;
    }

    if (size == 0)
    {
        _aligned_free(memblock);
        return NULL;
    }

    /* don't align to less than void pointer size */
    if (alignment < sizeof(void *))
        alignment = sizeof(void *);

    /* make sure alignment and offset didn't change */
    saved = SAVED_PTR(memblock);
    if (memblock != ALIGN_PTR(*saved, alignment, offset))
    {
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return NULL;
    }

    old_padding = (char *)memblock - (char *)*saved;

    /* Get previous size of block */
    old_size = _msize(*saved);
    if (old_size == -1)
    {
        /* It seems this function was called with an invalid pointer. Bail out. */
        return NULL;
    }

    /* Adjust old_size to get amount of actual data in old block. */
    if (old_size < old_padding)
    {
        /* Shouldn't happen. Something's weird, so bail out. */
        return NULL;
    }
    old_size -= old_padding;

    temp = MSVCRT_realloc(*saved, size + alignment + sizeof(void *));

    if (!temp)
        return NULL;

    /* adjust pointer for proper alignment and offset */
    memblock = ALIGN_PTR(temp, alignment, offset);

    /* Save the real allocation address below returned address */
    /* so it can be found later to free. */
    saved = SAVED_PTR(memblock);

    new_padding = (char *)memblock - (char *)temp;

/*
   Memory layout of old block is as follows:
   +-------+---------------------+-+--------------------------+-----------+
   |  ...  | "old_padding" bytes | | ... "old_size" bytes ... |    ...    |
   +-------+---------------------+-+--------------------------+-----------+
           ^                     ^ ^
           |                     | |
        *saved               saved memblock

   Memory layout of new block is as follows:
   +-------+-----------------------------+-+----------------------+-------+
   |  ...  |    "new_padding" bytes      | | ... "size" bytes ... |  ...  |
   +-------+-----------------------------+-+----------------------+-------+
           ^                             ^ ^
           |                             | |
          temp                       saved memblock

   However, in the new block, actual data is still written as follows
   (because it was copied by MSVCRT_realloc):
   +-------+---------------------+--------------------------------+-------+
   |  ...  | "old_padding" bytes |   ... "old_size" bytes ...     |  ...  |
   +-------+---------------------+--------------------------------+-------+
           ^                             ^ ^
           |                             | |
          temp                       saved memblock

   Therefore, min(old_size,size) bytes of actual data have to be moved
   from the offset they were at in the old block (temp + old_padding),
   to the offset they have to be in the new block (temp + new_padding == memblock).
*/
    if (new_padding != old_padding)
        memmove((char *)memblock, (char *)temp + old_padding, (old_size < size) ? old_size : size);

    *saved = temp;

    return memblock;
}

/*********************************************************************
 *		_aligned_realloc (MSVCRT.@)
 */
void * CDECL _aligned_realloc(void *memblock, MSVCRT_size_t size, MSVCRT_size_t alignment)
{
    TRACE("(%p, %lu, %lu)\n", memblock, size, alignment);
    return _aligned_offset_realloc(memblock, size, alignment, 0);
}

/*********************************************************************
 *		memmove_s (MSVCRT.@)
 */
int CDECL MSVCRT_memmove_s(void *dest, MSVCRT_size_t numberOfElements, const void *src, MSVCRT_size_t count)
{
    TRACE("(%p %lu %p %lu)\n", dest, numberOfElements, src, count);

    if(!count)
        return 0;

    if (!MSVCRT_CHECK_PMT(dest != NULL)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(src != NULL)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT_ERR( count <= numberOfElements, MSVCRT_ERANGE )) return MSVCRT_ERANGE;

    memmove(dest, src, count);
    return 0;
}

#if _MSVCR_VER>=100
/*********************************************************************
 *              wmemmove_s (MSVCR100.@)
 */
int CDECL wmemmove_s(MSVCRT_wchar_t *dest, MSVCRT_size_t numberOfElements,
        const MSVCRT_wchar_t *src, MSVCRT_size_t count)
{
    TRACE("(%p %lu %p %lu)\n", dest, numberOfElements, src, count);

    if (!count)
        return 0;

    /* Native does not seem to conform to 6.7.1.2.3 in
     * http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1225.pdf
     * in that it does not zero the output buffer on constraint violation.
     */
    if (!MSVCRT_CHECK_PMT(dest != NULL)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(src != NULL)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT_ERR(count <= numberOfElements, MSVCRT_ERANGE)) return MSVCRT_ERANGE;

    memmove(dest, src, sizeof(MSVCRT_wchar_t)*count);
    return 0;
}
#endif

/*********************************************************************
 *		memcpy_s (MSVCRT.@)
 */
int CDECL MSVCRT_memcpy_s(void *dest, MSVCRT_size_t numberOfElements, const void *src, MSVCRT_size_t count)
{
    TRACE("(%p %lu %p %lu)\n", dest, numberOfElements, src, count);

    if(!count)
        return 0;

    if (!MSVCRT_CHECK_PMT(dest != NULL)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(src != NULL))
    {
        memset(dest, 0, numberOfElements);
        return MSVCRT_EINVAL;
    }
    if (!MSVCRT_CHECK_PMT_ERR( count <= numberOfElements, MSVCRT_ERANGE ))
    {
        memset(dest, 0, numberOfElements);
        return MSVCRT_ERANGE;
    }

    memmove(dest, src, count);
    return 0;
}

#if _MSVCR_VER>=100
/*********************************************************************
 *              wmemcpy_s (MSVCR100.@)
 */
int CDECL wmemcpy_s(MSVCRT_wchar_t *dest, MSVCRT_size_t numberOfElements,
        const MSVCRT_wchar_t *src, MSVCRT_size_t count)
{
    TRACE("(%p %lu %p %lu)\n", dest, numberOfElements, src, count);

    if (!count)
        return 0;

    if (!MSVCRT_CHECK_PMT(dest != NULL)) return MSVCRT_EINVAL;

    if (!MSVCRT_CHECK_PMT(src != NULL)) {
        memset(dest, 0, numberOfElements*sizeof(MSVCRT_wchar_t));
        return MSVCRT_EINVAL;
    }
    if (!MSVCRT_CHECK_PMT_ERR(count <= numberOfElements, MSVCRT_ERANGE)) {
        memset(dest, 0, numberOfElements*sizeof(MSVCRT_wchar_t));
        return MSVCRT_ERANGE;
    }

    memmove(dest, src, sizeof(MSVCRT_wchar_t)*count);
    return 0;
}
#endif

/*********************************************************************
 *		strncpy_s (MSVCRT.@)
 */
int CDECL MSVCRT_strncpy_s(char *dest, MSVCRT_size_t numberOfElements,
        const char *src, MSVCRT_size_t count)
{
    MSVCRT_size_t i, end;

    TRACE("(%s %lu %s %lu)\n", dest, numberOfElements, src, count);

    if(!count) {
        if(dest && numberOfElements)
            *dest = 0;
        return 0;
    }

    if (!MSVCRT_CHECK_PMT(dest != NULL)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(src != NULL)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(numberOfElements != 0)) return MSVCRT_EINVAL;

    if(count!=MSVCRT__TRUNCATE && count<numberOfElements)
        end = count;
    else
        end = numberOfElements-1;

    for(i=0; i<end && src[i]; i++)
        dest[i] = src[i];

    if(!src[i] || end==count || count==MSVCRT__TRUNCATE) {
        dest[i] = '\0';
        return 0;
    }

    MSVCRT_INVALID_PMT("dest[numberOfElements] is too small", MSVCRT_EINVAL);
    dest[0] = '\0';
    return MSVCRT_EINVAL;
}

BOOL msvcrt_init_heap(void)
{
#ifdef __REACTOS__
    heap = GetProcessHeap();
#else
    heap = HeapCreate(0, 0, 0);
#endif
    return heap != NULL;
}

void msvcrt_destroy_heap(void)
{
    HeapDestroy(heap);
    if(sb_heap)
        HeapDestroy(sb_heap);
}
