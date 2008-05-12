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

#include <malloc.h>
#include <stdlib.h>
#include <internal/mtdll.h>

#define NDEBUG
#include <debug.h>

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
  void *retval = malloc(size);
  DPRINT("(%ld) returning %p\n", size, retval);
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
  DPRINT("(%p)\n", mem);
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
  DPRINT("(%p)\n",func);
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

/*********************************************************************
 *    _heapadd (MSVCRT.@)
 */
int _heapadd(void* mem, size_t size)
{
  DPRINT("(%p,%d) unsupported in Win32\n", mem,size);
  *_errno() = ENOSYS;
  return -1;
}
