/* $Id: brkpoint.c,v 1.2 2000/03/03 00:39:38 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/dbg/brkpoint.c
 * PURPOSE:         Handles breakpoints
 * PROGRAMMER:      Eric Kohl
 * UPDATE HISTORY:
 *                  Created 28/12/1999
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>


/* FUNCTIONS *****************************************************************/

VOID STDCALL DbgBreakPoint(VOID)
{
   __asm__("int $3\n\t");
}

VOID STDCALL DbgUserBreakPoint(VOID)
{
   __asm__("int $3\n\t");
}

/* EOF */
