/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/memcpy.c
 * PROGRAMMER:      David Welch (welch@cwcom.net)
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
  const char *f = from;
  char *t = to;
  int i = count;

  while (i-- > 0)
    *t++ = *f++;

  return to;
}

