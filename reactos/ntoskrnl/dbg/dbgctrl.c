/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/dbg/dbgctrl.c
 * PURPOSE:         System debug control
 * PORTABILITY:     Checked
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL 
NtSystemDebugControl(DEBUG_CONTROL_CODE ControlCode,
		     PVOID InputBuffer,
		     ULONG InputBufferLength,
		     PVOID OutputBuffer,
		     ULONG OutputBufferLength,
		     PULONG ReturnLength)
{
  switch (ControlCode) {
    case DebugGetTraceInformation:
    case DebugSetInternalBreakpoint:
    case DebugClearSpecialCalls:
    case DebugQuerySpecialCalls:
    case DebugDbgBreakPoint:
      break;
#ifdef KDBG
    case DebugDbgLoadSymbols:
      KdbLdrLoadUserModuleSymbols((PLDR_MODULE) InputBuffer);
#endif /* KDBG */
      break;
    default:
      break;
  }
  return STATUS_SUCCESS;
}
