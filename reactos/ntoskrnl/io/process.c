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
/* $Id: process.c,v 1.13 2002/09/07 15:12:53 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/process.c
 * PURPOSE:         Process functions that, bizarrely, are in the iomgr
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


/* FUNCTIONS *****************************************************************/

PVOID STDCALL
IoGetInitialStack(VOID)
{
  return(PsGetCurrentThread()->Tcb.InitialStack);
}


VOID STDCALL
IoGetStackLimits(OUT PULONG LowLimit,
		 OUT PULONG HighLimit)
{
  *LowLimit = (ULONG)NtCurrentTeb()->Tib.StackLimit;
  *HighLimit = (ULONG)NtCurrentTeb()->Tib.StackBase;
}


PEPROCESS STDCALL
IoThreadToProcess(IN PETHREAD Thread)
{
  return(Thread->ThreadsProcess);
}


PEPROCESS STDCALL
IoGetRequestorProcess(IN PIRP Irp)
{
  return(Irp->Tail.Overlay.Thread->ThreadsProcess);
}


/**********************************************************************
 * NAME							EXPORTED
 * 	IoSetThreadHardErrorMode@4
 *
 * ARGUMENTS
 * 	HardErrorEnabled
 * 		TRUE : enable hard errors processing;
 * 		FALSE: do NOT process hard errors.
 *
 * RETURN VALUE
 * 	Previous value for the current thread's hard errors
 * 	processing policy.
 */
BOOLEAN STDCALL
IoSetThreadHardErrorMode(IN BOOLEAN HardErrorEnabled)
{
  BOOLEAN PreviousHEM = NtCurrentTeb()->HardErrorDisabled;

  NtCurrentTeb()->HardErrorDisabled = ((TRUE == HardErrorEnabled) ? FALSE : TRUE);

  return((TRUE == PreviousHEM) ? FALSE : TRUE);
}

/* EOF */
