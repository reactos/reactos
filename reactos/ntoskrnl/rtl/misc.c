/* $Id$
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Various functions
 * FILE:              lib/ntoskrnl/rtl/misc.c
 * PROGRAMER:         Hartmut Birr
 * REVISION HISTORY:
 *                    01/03/2005: Created
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

extern ULONG NtGlobalFlag;

/* FUNCTIONS *****************************************************************/

/*
* @implemented
*/
ULONG
STDCALL
RtlGetNtGlobalFlags(VOID)
{
	return(NtGlobalFlag);
}
