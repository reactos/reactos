/* $Id:
 */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        subsys/psx/lib/psxdll/stdlib/malloc.c
 * PURPOSE:     Memory allocator
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              27/12/2001: Created
 */

#include <errno.h>
#include <psx/stdlib.h>

void * malloc(size_t size)
{
 void * pTemp = __malloc(size);

 if(!pTemp)
  errno = ENOMEM;

 return (pTemp);
}

void free(void * ptr)
{
 __free(ptr);
}

void * calloc(size_t nelem, size_t elsize)
{
 return (__malloc(nelem * elsize));
}

void * realloc(void * ptr, size_t size)
{
 void * pTemp;

 if(size == 0)
  __free(ptr);

 if(ptr == 0)
  return __malloc(size);

 pTemp = __realloc(ptr, size);

 if(pTemp == 0)
  errno = ENOMEM;

 return (pTemp);
}

/* EOF */

