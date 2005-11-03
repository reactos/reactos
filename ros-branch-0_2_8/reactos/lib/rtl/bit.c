/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/bit.c
 * PROGRAMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
CCHAR STDCALL
RtlFindLeastSignificantBit(IN ULONGLONG Set)
{
  int i;

  if (Set == 0ULL)
    return -1;

  for (i = 0; i < 64; i++)
  {
    if (Set & (1 << i))
      return (CCHAR)i;
  }

  return -1;
}


/*
 * @implemented
 */
CCHAR STDCALL
RtlFindMostSignificantBit(IN ULONGLONG Set)
{
  int i;

  if (Set == 0ULL)
    return -1;

  for (i = 63; i >= 0; i--)
  {
    if (Set & (1 << i))
      return (CCHAR)i;
  }

  return -1;
}

/* EOF */
