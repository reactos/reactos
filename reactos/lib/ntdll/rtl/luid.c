/* $Id: luid.c,v 1.4 2002/09/07 15:12:40 chorns Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Locally unique identifier (LUID) helper functions
 * FILE:              lib/ntdll/rtl/luid.c
 * PROGRAMER:         Eric Kohl <ekohl@zr-online.de>
 * REVISION HISTORY:
 *                    15/04/2000: Created
 */

/* INCLUDES *****************************************************************/

#define NTOS_USER_MODE
#include <ntos.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

VOID STDCALL
RtlCopyLuid(PLUID LuidDest,
	    PLUID LuidSrc)
{
  LuidDest->QuadPart = LuidSrc->QuadPart;
}


VOID STDCALL
RtlCopyLuidAndAttributesArray(ULONG Count,
			      PLUID_AND_ATTRIBUTES Src,
			      PLUID_AND_ATTRIBUTES Dest)
{
  ULONG i;

  for (i = 0; i < Count; i++)
    {
      RtlCopyMemory(&Dest[i],
		    &Src[i],
		    sizeof(LUID_AND_ATTRIBUTES));
    }
}

#undef RtlEqualLuid

BOOLEAN STDCALL
RtlEqualLuid(PLUID Luid1,
	     PLUID Luid2)
{
  return((Luid1->QuadPart == Luid2->QuadPart) ? TRUE : FALSE);
}

/* EOF */
