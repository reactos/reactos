/* $Id: brkpoint.c,v 1.1 1999/12/29 17:12:28 ekohl Exp $
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

VOID DbgBreakPoint(VOID)
{
   __asm__("int $3\n\t");
}

VOID DbgUserBreakPoint(VOID)
{
   __asm__("int $3\n\t");
}

/* EOF */
