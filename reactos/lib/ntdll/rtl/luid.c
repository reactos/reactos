/* $Id: luid.c,v 1.1 2000/04/15 23:10:41 ekohl Exp $
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

#include <internal/debug.h>

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
