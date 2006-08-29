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
#include <stdlib.h>
#include <malloc.h>


/* round to 16 bytes + alloc at minimum 16 bytes */
#define ROUND_SIZE(size) (max(16, ROUND_UP(size, 16)))

extern HANDLE hHeap;

/*
 * @implemented
 */
void* malloc(size_t _size)
{
   size_t nSize = ROUND_SIZE(_size);
   
   if (nSize<_size) 
       return NULL;     
       
   return HeapAlloc(hHeap, 0, nSize);
}

/*
 * @implemented
 */
void free(void* _ptr)
{
   HeapFree(hHeap,0,_ptr);
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
               
   return HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cSize );
}

/*
 * @implemented
 */
void* realloc(void* _ptr, size_t _size)
{
   size_t nSize;
       
   if (( _size == 0) && (_ptr !=NULL))
       return NULL;
   
   nSize = ROUND_SIZE(_size);
   
   if (nSize<_size)
       return NULL;
               
   if (!_ptr) return malloc(_size);
   if (_size) return HeapReAlloc(hHeap, 0, _ptr, nSize);
   free(_ptr);
   return NULL;
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
          
   return HeapReAlloc(hHeap, HEAP_REALLOC_IN_PLACE_ONLY, _ptr, nSize);
}

/*
 * @implemented
 */
size_t _msize(void* _ptr)
{
   return HeapSize(hHeap, 0, _ptr);
}

/*
 * @implemented
 */
int	_heapchk(void)
{
	if (!HeapValidate(hHeap, 0, NULL))
		return -1;
	return 0;
}

/*
 * @implemented
 */
int	_heapmin(void)
{
	if (!HeapCompact(hHeap, 0))
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

