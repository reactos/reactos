/* $Id: brkpoint.c,v 1.5 2003/04/26 23:13:28 hyperion Exp $
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
#include <debug.h>

/* FUNCTIONS *****************************************************************/

#if 0
/*
 FIXME: DbgBreakPoint must not have a stack frame, but GCC doesn't support
 __declspec(naked) yet
*/
__declspec(naked) VOID STDCALL DbgBreakPoint(VOID)
{ __asm__(ASM_BREAKPOINT_STR); }

VOID STDCALL DbgUserBreakPoint(VOID)
{ __asm__(ASM_BREAKPOINT_STR); }
#else
#define DBG_BP_FUNC(__NAME__) \
__asm__ \
( \
 "\n" \
 ".global _" #__NAME__ "@0\n" \
 "_" #__NAME__ "@0:\n" \
 ASM_BREAKPOINT \
 "ret $0\n" \
)

DBG_BP_FUNC(DbgBreakPoint);
DBG_BP_FUNC(DbgUserBreakPoint);
#endif

/* EOF */
