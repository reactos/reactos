/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
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
/* $Id: win32.c,v 1.4 2002/09/07 15:13:05 chorns Exp $
 *
 * COPYRIGHT:              See COPYING in the top level directory
 * PROJECT:                ReactOS kernel
 * FILE:                   ntoskrnl/ps/win32.c
 * PURPOSE:                win32k support
 * PROGRAMMER:             Eric Kohl (ekohl@rz-online.de)
 * REVISION HISTORY: 
 *               04/01/2002: Created
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


/* TYPES *******************************************************************/

/* GLOBALS ******************************************************************/

static ULONG PspWin32ProcessSize = 0;
static ULONG PspWin32ThreadSize = 0;

/* FUNCTIONS ***************************************************************/

PW32THREAD STDCALL
PsGetWin32Thread(VOID)
{
  return(PsGetCurrentThread()->Win32Thread);
}

NTSTATUS STDCALL
PsCreateWin32Thread(PETHREAD Thread)
{
  if (Thread->Win32Thread != NULL)
    return(STATUS_SUCCESS);

  Thread->Win32Thread = ExAllocatePool(NonPagedPool,
					PspWin32ThreadSize);
  if (Thread->Win32Thread == NULL)
    return(STATUS_NO_MEMORY);

  RtlZeroMemory(Thread->Win32Thread,
		PspWin32ThreadSize);

  return(STATUS_SUCCESS);
}

PW32PROCESS STDCALL
PsGetWin32Process(VOID)
{
  return(PsGetCurrentProcess()->Win32Process);
}

NTSTATUS STDCALL
PsCreateWin32Process(PEPROCESS Process)
{
  if (Process->Win32Process != NULL)
    return(STATUS_SUCCESS);

  Process->Win32Process = ExAllocatePool(NonPagedPool,
					 PspWin32ProcessSize);
  if (Process->Win32Process == NULL)
    return(STATUS_NO_MEMORY);

  RtlZeroMemory(Process->Win32Process,
		PspWin32ProcessSize);

  return(STATUS_SUCCESS);
}


VOID STDCALL
PsEstablishWin32Callouts(PVOID Param1,
			 PVOID Param2,
			 PVOID Param3,
			 PVOID Param4,
			 ULONG W32ThreadSize,
			 ULONG W32ProcessSize)
{
  PspWin32ProcessSize = W32ProcessSize;
  PspWin32ThreadSize = W32ThreadSize;
}

/* EOF */
