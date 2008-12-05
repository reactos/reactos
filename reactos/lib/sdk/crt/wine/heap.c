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

#include <precomp.h>

/* MT */
#define LOCK_HEAP   _mlock( _HEAP_LOCK )
#define UNLOCK_HEAP _munlock( _HEAP_LOCK )

/* _aligned */
#define SAVED_PTR(x) ((void *)((DWORD_PTR)((char *)x - sizeof(void *)) & \
                                ~(sizeof(void *) - 1)))
#define ALIGN_PTR(ptr, alignment, offset) ((void *) \
    ((((DWORD_PTR)((char *)ptr + alignment + sizeof(void *) + offset)) & \
      ~(alignment - 1)) - offset))

typedef void (*MSVCRT_new_handler_func)(unsigned long size);

static MSVCRT_new_handler_func MSVCRT_new_handler;
static int MSVCRT_new_mode;

/* FIXME - According to documentation it should be 480 bytes, at runtime default is 0 */
static size_t MSVCRT_sbh_threshold = 0;

/*********************************************************************
 *		??2@YAPAXI@Z (MSVCRT.@)
 */
void* MSVCRT_operator_new(unsigned long size)
{
  void *retval = malloc(size);
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
  free(mem);
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

int CDECL _callnewh(unsigned long size)
{
    if(MSVCRT_new_handler)
        (*MSVCRT_new_handler)(size);
    return 0;
}

/*********************************************************************
 *              _get_sbh_threshold (MSVCRT.@)
 */
size_t CDECL _get_sbh_threshold(void)
{
    return MSVCRT_sbh_threshold;
}

/*********************************************************************
 *              _set_sbh_threshold (MSVCRT.@)
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
 *    _heapadd (MSVCRT.@)
 */
int _heapadd(void* mem, size_t size)
{
  TRACE("(%p,%d) unsupported in Win32\n", mem,size);
  *_errno() = ENOSYS;
  return -1;
}

void CDECL _aligned_free(void *memblock)
{
    if (memblock)
    {
        void **saved = SAVED_PTR(memblock);
        free(*saved);
    }
}

void * CDECL _aligned_offset_malloc(size_t size, size_t alignment, size_t offset)
{
    void *memblock, *temp, **saved;

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

void * CDECL _aligned_malloc(size_t size, size_t alignment)
{
    return _aligned_offset_malloc(size, alignment, 0);
}

void * CDECL _aligned_offset_realloc(void *memblock, size_t size,
                                     size_t alignment, size_t offset)
{
    void * temp, **saved;
    size_t old_padding, new_padding, old_size;

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
    if (old_size == (size_t)-1)
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

void * CDECL _aligned_realloc(void *memblock, size_t size, size_t alignment)
{
    return _aligned_offset_realloc(memblock, size, alignment, 0);
}
