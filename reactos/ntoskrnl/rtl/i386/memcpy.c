/* $Id: memcpy.c,v 1.3 2002/09/08 10:23:43 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/i386/memcpy.c
 * PROGRAMMER:      Hartmut Birr
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
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

