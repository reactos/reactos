/* $Id: memset.c,v 1.1 2002/08/28 07:03:10 hbirr Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/i386/memcpy.c
 * PROGRAMMER:      Hartmut Birr
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include <string.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

void * memset(void *src, int val, size_t count)
{
	__asm__( \
		"or	%%ecx,%%ecx\n\t"\
		"jz	.L1\n\t"	\
		"cld\t\n"		\
		"rep\t\n"		\
		"stosb\t\n"		\
		".L1:\n\t"
		: 
		: "D" (src), "c" (count), "a" (val));
	return src;
}
