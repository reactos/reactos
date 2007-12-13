
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************

*/

#include "2bload.h"

void * memcpy(void *dest, const void *src,  size_t size) {

#if 0
	BYTE * pb=(BYTE *)src, *pbd=(BYTE *)dest;
	while(size--) *pbd++=*pb++;

#else
		__asm__ __volatile__ (
              "    push %%esi    \n"
              "    push %%edi    \n"
              "    push %%ecx    \n"
              "    cld    \n"
              "    mov %0, %%esi \n"
              "    mov %1, %%edi \n"
              "    mov %2, %%ecx \n"
              "    push %%ecx    \n"
              "    shr $2, %%ecx \n"
              "    rep movsl     \n"
              "    pop %%ecx     \n"
              "    and $3, %%ecx \n"
              "    rep movsb     \n"
              : :"S" (src), "D" (dest), "c" (size)
		);

		__asm__ __volatile__ (
	      "    pop %ecx     \n"
              "    pop %edi     \n"
              "    pop %esi     \n"
		);
#endif
}

void * memset(void *dest, int data,  size_t size)
{
  	char *p = dest;
	while (size -- > 0)
	{
		*p ++ = data;
	}
}

int _memcmp(const BYTE *pb, const BYTE *pb1, int n) {
	while(n--) { if(*pb++ != *pb1++) return 1; }
	return 0;
}

