/* $Id: luid.c,v 1.2 2000/06/29 23:35:31 dwelch Exp $
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

VOID
STDCALL
RtlCopyLuid (
	PLUID LuidDest,
	PLUID LuidSrc
	)
{
	LuidDest->QuadPart = LuidSrc->QuadPart;
}

#if 0
RtlCopyLuidAndAttributesArray (
	)
{

}
#endif

BOOLEAN
STDCALL
RtlEqualLuid (
	PLUID	Luid1,
	PLUID	Luid2
	)
{
	return ((Luid1->QuadPart == Luid2->QuadPart) ? TRUE : FALSE);
}

/* EOF */
