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

/* MT */
#define LOCK_HEAP   _mlock( _HEAP_LOCK )
#define UNLOCK_HEAP _munlock( _HEAP_LOCK )

/* _aligned */
#define SAVED_PTR(x) ((void *)((DWORD_PTR)((char *)x - sizeof(void *)) & \
                               ~(sizeof(void *) - 1)))
#define ALIGN_PTR(ptr, alignment, offset) ((void *) \
    ((((DWORD_PTR)((char *)ptr + alignment + sizeof(void *) + offset)) & \
      ~(alignment - 1)) - offset))


typedef int (CDECL *MSVCRT_new_handler_func)(size_t size);

static MSVCRT_new_handler_func MSVCRT_new_handler;
static int MSVCRT_new_mode;

/* FIXME - According to documentation it should be 8*1024, at runtime it returns 16 */
static unsigned int MSVCRT_amblksiz = 16;
/* FIXME - According to documentation it should be 480 bytes, at runtime default is 0 */
static size_t MSVCRT_sbh_threshold = 0;

/*********************************************************************
 *		??2@YAPAXI@Z (MSVCRT.@)
 */
void* CDECL MSVCRT_operator_new(size_t size)
{
  void *retval;
  int freed;

  do
  {
    retval = HeapAlloc(GetProcessHeap(), 0, size);
    if(retval)
    {
      TRACE("(%ld) returning %p\n", size, retval);
      return retval;
    }

    LOCK_HEAP;
    if(MSVCRT_new_handler)
      freed = (*MSVCRT_new_handler)(size);
    else
      freed = 0;
    UNLOCK_HEAP;
  } while(freed);

  TRACE("(%ld) out of memory\n", size);
  return NULL;
}


/*********************************************************************
 *		??2@YAPAXIHPBDH@Z (MSVCRT.@)
 */
void* CDECL MSVCRT_operator_new_dbg(size_t size, int type, const char *file, int line)
{
    return MSVCRT_operator_new( size );
}


/*********************************************************************
 *		??3@YAXPAX@Z (MSVCRT.@)
 */
void CDECL MSVCRT_operator_delete(void *mem)
{
  TRACE("(%p)\n", mem);
  HeapFree(GetProcessHeap(), 0, mem);
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
int CDECL _callnewh(size_t size)
{
  if(MSVCRT_new_handler)
    (*MSVCRT_new_handler)(size);
  return 0;
}

/*********************************************************************
 *		_expand (MSVCRT.@)
 */
void* CDECL _expand(void* mem, size_t size)
{
  return HeapReAlloc(GetProcessHeap(), HEAP_REALLOC_IN_PLACE_ONLY, mem, size);
}

/*********************************************************************
 *		_heapchk (MSVCRT.@)
 */
int CDECL _heapchk(void)
{
  if (!HeapValidate( GetProcessHeap(), 0, NULL))
  {
    _dosmaperr(GetLastError());
    return _HEAPBADNODE;
  }
  return _HEAPOK;
}

/*********************************************************************
 *		_heapmin (MSVCRT.@)
 */
int CDECL _heapmin(void)
{
  if (!HeapCompact( GetProcessHeap(), 0 ))
  {
    if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
      _dosmaperr(GetLastError());
    return -1;
  }
  return 0;
}

/*********************************************************************
 *		_heapwalk (MSVCRT.@)
 */
int CDECL _heapwalk(_HEAPINFO* next)
{
  PROCESS_HEAP_ENTRY phe;

  LOCK_HEAP;
  phe.lpData = next->_pentry;
  phe.cbData = (DWORD)next->_size;
  phe.wFlags = next->_useflag == _USEDENTRY ? PROCESS_HEAP_ENTRY_BUSY : 0;

  if (phe.lpData && phe.wFlags & PROCESS_HEAP_ENTRY_BUSY &&
      !HeapValidate( GetProcessHeap(), 0, phe.lpData ))
  {
    UNLOCK_HEAP;
    _dosmaperr(GetLastError());
    return _HEAPBADNODE;
  }

  do
  {
    if (!HeapWalk( GetProcessHeap(), &phe ))
    {
      UNLOCK_HEAP;
      if (GetLastError() == ERROR_NO_MORE_ITEMS)
         return _HEAPEND;
      _dosmaperr(GetLastError());
      if (!phe.lpData)
        return _HEAPBADBEGIN;
      return _HEAPBADNODE;
    }
  } while (phe.wFlags & (PROCESS_HEAP_REGION|PROCESS_HEAP_UNCOMMITTED_RANGE));

  UNLOCK_HEAP;
  next->_pentry = phe.lpData;
  next->_size = phe.cbData;
  next->_useflag = phe.wFlags & PROCESS_HEAP_ENTRY_BUSY ? _USEDENTRY : _FREEENTRY;
  return _HEAPOK;
}

/*********************************************************************
 *		_heapset (MSVCRT.@)
 */
int CDECL _heapset(unsigned int value)
{
  int retval;
  _HEAPINFO heap;

  memset( &heap, 0, sizeof(heap) );
  LOCK_HEAP;
  while ((retval = _heapwalk(&heap)) == _HEAPOK)
  {
    if (heap._useflag == _FREEENTRY)
      memset(heap._pentry, value, heap._size);
  }
  UNLOCK_HEAP;
  return retval == _HEAPEND? _HEAPOK : retval;
}

/*********************************************************************
 *		_heapadd (MSVCRT.@)
 */
int CDECL _heapadd(void* mem, size_t size)
{
  TRACE("(%p,%ld) unsupported in Win32\n", mem,size);
  *_errno() = ENOSYS;
  return -1;
}

/*********************************************************************
 *		_heapadd (MSVCRT.@)
 */
intptr_t CDECL _get_heap_handle(void)
{
    return (intptr_t)GetProcessHeap();
}

/*********************************************************************
 *		_msize (MSVCRT.@)
 */
size_t CDECL _msize(void* mem)
{
  size_t size = HeapSize(GetProcessHeap(),0,mem);
  if (size == ~(size_t)0)
  {
    WARN(":Probably called with non wine-allocated memory, ret = -1\n");
    /* At least the Win32 crtdll/msvcrt also return -1 in this case */
  }
  return size;
}

/*********************************************************************
 *		calloc (MSVCRT.@)
 */
void* CDECL calloc(size_t count, size_t size)
{
  return HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, count * size );
}

/*********************************************************************
 *		free (MSVCRT.@)
 */
void CDECL free(void* ptr)
{
  if(ptr == NULL) return;
  HeapFree(GetProcessHeap(),0,ptr);
}

/*********************************************************************
 *                  malloc (MSVCRT.@)
 */
void* CDECL malloc(size_t size)
{
  void *ret = HeapAlloc(GetProcessHeap(),0,size);
  if (!ret)
      *_errno() = ENOMEM;
  return ret;
}

/*********************************************************************
 *		realloc (MSVCRT.@)
 */
void* CDECL realloc(void* ptr, size_t size)
{
  if (!ptr) return malloc(size);
  if (size) return HeapReAlloc(GetProcessHeap(), 0, ptr, size);
  free(ptr);
  return NULL;
}

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
size_t CDECL _get_sbh_threshold(void)
{
  return MSVCRT_sbh_threshold;
}

/*********************************************************************
 *		_set_sbh_threshold (MSVCRT.@)
 */
int CDECL _set_sbh_threshold(size_t threshold)
{
  if(threshold > 1016)
     return 0;
  else
     MSVCRT_sbh_threshold = threshold;
  return 1;
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
        free(*saved);
    }
}

/*********************************************************************
 *		_aligned_offset_malloc (MSVCRT.@)
 */
void * CDECL _aligned_offset_malloc(size_t size, size_t alignment, size_t offset)
{
    void *memblock, *temp, **saved;
    TRACE("(%lu, %lu, %lu)\n", size, alignment, offset);

    /* alignment must be a power of 2 */
    if ((alignment & (alignment - 1)) != 0)
    {
        *_errno() = EINVAL;
        return NULL;
    }

    /* offset must be less than size */
    if (offset >= size)
    {
        *_errno() = EINVAL;
        return NULL;
    }

    /* don't align to less than void pointer size */
    if (alignment < sizeof(void *))
        alignment = sizeof(void *);

    /* allocate enough space for void pointer and alignment */
    temp = malloc(size + alignment + sizeof(void *));

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
void * CDECL _aligned_malloc(size_t size, size_t alignment)
{
    TRACE("(%lu, %lu)\n", size, alignment);
    return _aligned_offset_malloc(size, alignment, 0);
}

/*********************************************************************
 *		_aligned_offset_realloc (MSVCRT.@)
 */
void * CDECL _aligned_offset_realloc(void *memblock, size_t size,
                                     size_t alignment, size_t offset)
{
    void * temp, **saved;
    size_t old_padding, new_padding, old_size;
    TRACE("(%p, %lu, %lu, %lu)\n", memblock, size, alignment, offset);

    if (!memblock)
        return _aligned_offset_malloc(size, alignment, offset);

    /* alignment must be a power of 2 */
    if ((alignment & (alignment - 1)) != 0)
    {
        *_errno() = EINVAL;
        return NULL;
    }

    /* offset must be less than size */
    if (offset >= size)
    {
        *_errno() = EINVAL;
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
        *_errno() = EINVAL;
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

    temp = realloc(*saved, size + alignment + sizeof(void *));

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
void * CDECL _aligned_realloc(void *memblock, size_t size, size_t alignment)
{
    TRACE("(%p, %lu, %lu)\n", memblock, size, alignment);
    return _aligned_offset_realloc(memblock, size, alignment, 0);
}

/*********************************************************************
 *		memmove_s (MSVCRT.@)
 */
int CDECL memmove_s(void *dest, size_t numberOfElements, const void *src, size_t count)
{
    TRACE("(%p %lu %p %lu)\n", dest, numberOfElements, src, count);

    if(!count)
        return 0;

    if(!dest || !src) {
        if(dest)
            memset(dest, 0, numberOfElements);

        *_errno() = EINVAL;
        return EINVAL;
    }

    if(count > numberOfElements) {
        memset(dest, 0, numberOfElements);

        *_errno() = ERANGE;
        return ERANGE;
    }

    memmove(dest, src, count);
    return 0;
}

/*********************************************************************
 *		strncpy_s (MSVCRT.@)
 */
int CDECL strncpy_s(char *dest, size_t numberOfElements,
        const char *src, size_t count)
{
    size_t i, end;

    TRACE("(%s %lu %s %lu)\n", dest, numberOfElements, src, count);

    if(!count)
        return 0;

    if (!MSVCRT_CHECK_PMT(dest != NULL) || !MSVCRT_CHECK_PMT(src != NULL) ||
        !MSVCRT_CHECK_PMT(numberOfElements != 0)) {
        *_errno() = EINVAL;
        return EINVAL;
    }

    if(count!=_TRUNCATE && count<numberOfElements)
        end = count;
    else
        end = numberOfElements-1;

    for(i=0; i<end && src[i]; i++)
        dest[i] = src[i];

    if(!src[i] || end==count || count==_TRUNCATE) {
        dest[i] = '\0';
        return 0;
    }

    MSVCRT_INVALID_PMT("dest[numberOfElements] is too small", EINVAL);
    dest[0] = '\0';
    return EINVAL;
}
