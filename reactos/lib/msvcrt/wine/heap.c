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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Note: Win32 heap operations are MT safe. We only lock the new
 *       handler and non atomic heap operations
 */

#include "msvcrt.h"
#include "msvcrt/errno.h"

#include "msvcrt/malloc.h"
#include "msvcrt/stdlib.h"
#include "mtdll.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/* MT */
#define LOCK_HEAP   _mlock( _HEAP_LOCK )
#define UNLOCK_HEAP _munlock( _HEAP_LOCK )


typedef void (*MSVCRT_new_handler_func)(unsigned long size);

static MSVCRT_new_handler_func MSVCRT_new_handler;
static int MSVCRT_new_mode;


/*********************************************************************
 *		??2@YAPAXI@Z (MSVCRT.@)
 */
void* MSVCRT_operator_new(unsigned long size)
{
  void *retval = HeapAlloc(GetProcessHeap(), 0, size);
  TRACE("(%ld) returning %p\n", size, retval);
  LOCK_HEAP;
  if(!retval && MSVCRT_new_handler)
    (*MSVCRT_new_handler)(size);
  UNLOCK_HEAP;
  return retval;
}

/*********************************************************************
 *		??3@YAXPAX@Z (MSVCRT.@)
 */
void MSVCRT_operator_delete(void *mem)
{
  TRACE("(%p)\n", mem);
  HeapFree(GetProcessHeap(), 0, mem);
}


/*********************************************************************
 *		?_query_new_handler@@YAP6AHI@ZXZ (MSVCRT.@)
 */
MSVCRT_new_handler_func MSVCRT__query_new_handler(void)
{
  return MSVCRT_new_handler;
}


/*********************************************************************
 *		?_query_new_mode@@YAHXZ (MSVCRT.@)
 */
int MSVCRT__query_new_mode(void)
{
  return MSVCRT_new_mode;
}

/*********************************************************************
 *		?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z (MSVCRT.@)
 */
MSVCRT_new_handler_func MSVCRT__set_new_handler(MSVCRT_new_handler_func func)
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
MSVCRT_new_handler_func MSVCRT_set_new_handler(void *func)
{
  TRACE("(%p)\n",func);
  MSVCRT__set_new_handler(NULL);
  return NULL;
}

/*********************************************************************
 *		?_set_new_mode@@YAHH@Z (MSVCRT.@)
 */
int MSVCRT__set_new_mode(int mode)
{
  int old_mode;
  LOCK_HEAP;
  old_mode = MSVCRT_new_mode;
  MSVCRT_new_mode = mode;
  UNLOCK_HEAP;
  return old_mode;
}

#if 0 /* __REACTOS__ */
/*********************************************************************
 *		_callnewh (MSVCRT.@)
 */
int _callnewh(unsigned long size)
{
  if(MSVCRT_new_handler)
    (*MSVCRT_new_handler)(size);
  return 0;
}

/*********************************************************************
 *		_expand (MSVCRT.@)
 */
void* _expand(void* mem, MSVCRT_size_t size)
{
  return HeapReAlloc(GetProcessHeap(), HEAP_REALLOC_IN_PLACE_ONLY, mem, size);
}

/*********************************************************************
 *		_heapchk (MSVCRT.@)
 */
int _heapchk(void)
{
  if (!HeapValidate( GetProcessHeap(), 0, NULL))
  {
    MSVCRT__set_errno(GetLastError());
    return _HEAPBADNODE;
  }
  return _HEAPOK;
}

/*********************************************************************
 *		_heapmin (MSVCRT.@)
 */
int _heapmin(void)
{
  if (!HeapCompact( GetProcessHeap(), 0 ))
  {
    if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
      MSVCRT__set_errno(GetLastError());
    return -1;
  }
  return 0;
}

/*********************************************************************
 *		_heapwalk (MSVCRT.@)
 */
int _heapwalk(_HEAPINFO* next)
{
  PROCESS_HEAP_ENTRY phe;

  LOCK_HEAP;
  phe.lpData = next->_pentry;
  phe.cbData = next->_size;
  phe.wFlags = next->_useflag == _USEDENTRY ? PROCESS_HEAP_ENTRY_BUSY : 0;

  if (phe.lpData && phe.wFlags & PROCESS_HEAP_ENTRY_BUSY &&
      !HeapValidate( GetProcessHeap(), 0, phe.lpData ))
  {
    UNLOCK_HEAP;
    MSVCRT__set_errno(GetLastError());
    return _HEAPBADNODE;
  }

  do
  {
    if (!HeapWalk( GetProcessHeap(), &phe ))
    {
      UNLOCK_HEAP;
      if (GetLastError() == ERROR_NO_MORE_ITEMS)
         return _HEAPEND;
      MSVCRT__set_errno(GetLastError());
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
int _heapset(unsigned int value)
{
  int retval;
  _HEAPINFO heap;

  memset( &heap, 0, sizeof(_HEAPINFO) );
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
int _heapadd(void* mem, MSVCRT_size_t size)
{
  TRACE("(%p,%d) unsupported in Win32\n", mem,size);
  *MSVCRT__errno() = MSVCRT_ENOSYS;
  return -1;
}

/*********************************************************************
 *		_msize (MSVCRT.@)
 */
MSVCRT_size_t _msize(void* mem)
{
  long size = HeapSize(GetProcessHeap(),0,mem);
  if (size == -1)
  {
    WARN(":Probably called with non wine-allocated memory, ret = -1\n");
    /* At least the Win32 crtdll/msvcrt also return -1 in this case */
  }
  return size;
}

/*********************************************************************
 *		calloc (MSVCRT.@)
 */
void* MSVCRT_calloc(MSVCRT_size_t size, MSVCRT_size_t count)
{
  return HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size * count );
}

/*********************************************************************
 *		free (MSVCRT.@)
 */
void MSVCRT_free(void* ptr)
{
  HeapFree(GetProcessHeap(),0,ptr);
}

/*********************************************************************
 *                  malloc (MSVCRT.@)
 */
void* MSVCRT_malloc(MSVCRT_size_t size)
{
  void *ret = HeapAlloc(GetProcessHeap(),0,size);
  if (!ret)
    MSVCRT__set_errno(GetLastError());
  return ret;
}

/*********************************************************************
 *		realloc (MSVCRT.@)
 */
void* MSVCRT_realloc(void* ptr, MSVCRT_size_t size)
{
  if (!ptr) return MSVCRT_malloc(size);
  if (size) return HeapReAlloc(GetProcessHeap(), 0, ptr, size);
  MSVCRT_free(ptr);
  return NULL;
}

#endif /* __REACTOS__ */
