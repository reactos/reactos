/* $Id: memcpy.c,v 1.2 2002/09/07 15:13:06 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/i386/memcpy.c
 * PROGRAMMER:      Hartmut Birr
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <string.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

#undef memcpy
void *memcpy (void *to, const void *from, size_t count)
{
  __asm__( \
	"or	%%ecx,%%ecx\n\t"\
	"jz	.L1\n\t"	\
	"cld\n\t"		\
	"rep\n\t"		\
	"movsb\n\t"		\
	".L1:\n\t"
	  :
	  : "D" (to), "S" (from), "c" (count));
  return to;
}

