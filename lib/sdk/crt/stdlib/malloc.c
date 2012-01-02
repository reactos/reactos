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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Note: Win32 heap operations are MT safe. We only lock the new
 *       handler and non atomic heap operations
 */

#include <precomp.h>
#include <stdlib.h>
#include <malloc.h>

#define ROUND_DOWN(n, align) \
    (((ULONG)n) & ~((align) - 1l))

#define ROUND_UP(n, align) \
    ROUND_DOWN(((ULONG)n) + (align) - 1, (align))

/* round to 16 bytes + alloc at minimum 16 bytes */
#define ROUND_SIZE(size) (max(16, ROUND_UP(size, 16)))

/*
 * @implemented
 */
void* malloc(size_t _size)
{
   size_t nSize = ROUND_SIZE(_size);

   if (nSize<_size)
       return NULL;

   return HeapAlloc(GetProcessHeap(), 0, nSize);
}

/*
 * @implemented
 */
void free(void* _ptr)
{
   HeapFree(GetProcessHeap(),0,_ptr);
}

/*
 * @implemented
 */
void* calloc(size_t _nmemb, size_t _size)
{
   size_t nSize = _nmemb * _size;
   size_t cSize = ROUND_SIZE(nSize);

   if ( (_nmemb > ((size_t)-1 / _size))  || (cSize<nSize))
      return NULL;

   return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cSize );
}

/*
 * @implemented
 */
void* realloc(void* _ptr, size_t _size)
{
   size_t nSize;

   if (_ptr == NULL)
      return malloc(_size);

   if (_size == 0)
   {
   	   free(_ptr);
       return NULL;
   }

   nSize = ROUND_SIZE(_size);

   /* check for integer overflow */
   if (nSize<_size)
       return NULL;

   return HeapReAlloc(GetProcessHeap(), 0, _ptr, nSize);
}

/*
 * @implemented
 */
void* _expand(void* _ptr, size_t _size)
{
   size_t nSize;

   nSize = ROUND_SIZE(_size);

   if (nSize<_size)
       return NULL;

   return HeapReAlloc(GetProcessHeap(), HEAP_REALLOC_IN_PLACE_ONLY, _ptr, nSize);
}

/*
 * @implemented
 */
size_t _msize(void* _ptr)
{
   return HeapSize(GetProcessHeap(), 0, _ptr);
}

/*
 * @implemented
 */
int	_heapchk(void)
{
	if (!HeapValidate(GetProcessHeap(), 0, NULL))
		return -1;
	return 0;
}

/*
 * @implemented
 */
int	_heapmin(void)
{
	if (!HeapCompact(GetProcessHeap(), 0))
		return -1;
	return 0;
}

/*
 * @implemented
 */
int	_heapset(unsigned int unFill)
{
	if (_heapchk() == -1)
		return -1;
	return 0;

}

/*
 * @implemented
 */
int _heapwalk(struct _heapinfo* entry)
{
	return 0;
}

