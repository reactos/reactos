/* $Id: brkpoint.c,v 1.3 2002/09/07 15:12:39 chorns Exp $
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

#define NTOS_USER_MODE
#include <ntos.h>

#define NDEBUG
#include <debug.h>

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
