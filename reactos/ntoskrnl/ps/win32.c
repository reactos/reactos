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
/* $Id: win32.c,v 1.7 2003/07/11 01:23:15 royce Exp $
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

#include <ddk/ntddk.h>
#include <internal/ps.h>
#include <napi/win32.h>

/* TYPES *******************************************************************/

/* GLOBALS ******************************************************************/

static PW32_PROCESS_CALLBACK PspWin32ProcessCallback = NULL;
static PW32_THREAD_CALLBACK PspWin32ThreadCallback = NULL;
static ULONG PspWin32ProcessSize = 0;
static ULONG PspWin32ThreadSize = 0;

/* FUNCTIONS ***************************************************************/

PW32THREAD STDCALL
PsGetWin32Thread(VOID)
{
  return(PsGetCurrentThread()->Win32Thread);
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


/*
 * @implemented
 */
VOID STDCALL
PsEstablishWin32Callouts (PW32_PROCESS_CALLBACK W32ProcessCallback,
			  PW32_THREAD_CALLBACK W32ThreadCallback,
			  PVOID Param3,
			  PVOID Param4,
			  ULONG W32ThreadSize,
			  ULONG W32ProcessSize)
{
  PspWin32ProcessCallback = W32ProcessCallback;
  PspWin32ThreadCallback = W32ThreadCallback;

  PspWin32ProcessSize = W32ProcessSize;
  PspWin32ThreadSize = W32ThreadSize;
}


NTSTATUS
PsInitWin32Thread (PETHREAD Thread)
{
  PEPROCESS Process;

  Process = Thread->ThreadsProcess;

  if (Process->Win32Process == NULL)
    {
      Process->Win32Process = ExAllocatePool (NonPagedPool,
					      PspWin32ProcessSize);
      if (Process->Win32Process == NULL)
	return STATUS_NO_MEMORY;

      RtlZeroMemory (Process->Win32Process,
		     PspWin32ProcessSize);

      if (PspWin32ProcessCallback != NULL)
	{
	  PspWin32ProcessCallback (Process, TRUE);
	}
    }

  if (Thread->Win32Thread == NULL)
    {
      Thread->Win32Thread = ExAllocatePool (NonPagedPool,
					    PspWin32ThreadSize);
      if (Thread->Win32Thread == NULL)
	return STATUS_NO_MEMORY;

      RtlZeroMemory (Thread->Win32Thread,
		     PspWin32ThreadSize);

      if (PspWin32ThreadCallback != NULL)
	{
	  PspWin32ThreadCallback (Thread, TRUE);
	}
    }

  return(STATUS_SUCCESS);
}


VOID
PsTerminateWin32Process (PEPROCESS Process)
{
  if (Process->Win32Process == NULL)
    return;

  if (PspWin32ProcessCallback != NULL)
    {
      PspWin32ProcessCallback (Process, FALSE);
    }

  ExFreePool (Process->Win32Process);
}


VOID
PsTerminateWin32Thread (PETHREAD Thread)
{
  if (Thread->Win32Thread == NULL)
    return;

  if (PspWin32ThreadCallback != NULL)
    {
      PspWin32ThreadCallback (Thread, FALSE);
    }

  ExFreePool (Thread->Win32Thread);
}

/* EOF */
