/* $Id: luid.c,v 1.7 2003/07/11 13:50:23 royce Exp $
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

#include <ddk/ntddk.h>


/* FUNCTIONS *****************************************************************/

VOID STDCALL
RtlCopyLuid(PLUID LuidDest,
	    PLUID LuidSrc)
{
  LuidDest->LowPart = LuidSrc->LowPart;
  LuidDest->HighPart = LuidSrc->HighPart;
}


/*
 * @implemented
 */
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


/*
 * @implemented
 */
BOOLEAN STDCALL
RtlEqualLuid(PLUID Luid1,
	     PLUID Luid2)
{
  return (Luid1->LowPart == Luid2->LowPart &&
	  Luid1->HighPart == Luid2->HighPart);
}

/* EOF */
