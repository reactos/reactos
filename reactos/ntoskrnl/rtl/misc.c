/* $Id:$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/misc.c
 * PURPOSE:         Various functions
 * 
 * PROGRAMMERS:     Hartmut Birr
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
