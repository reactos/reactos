/* $Id$
 *
 * smss.c - Session Manager
 * 
 * ReactOS Operating System
 * 
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.LIB. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 */
#include "smss.h"
//#include <ntdll/rtl.h>
#include <rosrtl/string.h>

#define NDEBUG
#include <debug.h>

/* Native image's entry point */

VOID STDCALL
NtProcessStartup(PPEB Peb)
{
  NTSTATUS Status;

  Status = InitSessionManager(Children);
  if (!NT_SUCCESS(Status))
    {
      int i;
      for (i=0; i < (sizeof Children / sizeof Children[0]); i++)
      {
        if (Children[i])
        {
          NtTerminateProcess(Children[i],0);
        }
      }
      DPRINT1("SM: Initialization failed!\n");
      goto ByeBye;
    }

  Status = NtWaitForMultipleObjects(((LONG) sizeof(Children) / sizeof(HANDLE)),
				    Children,
				    WaitAny,
				    TRUE,	/* alertable */
				    NULL);	/* NULL for infinite */
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("SM: NtWaitForMultipleObjects failed!\n");
    }
  else
    {
      DPRINT1("SM: Process terminated!\n");
    }

ByeBye:
  /* Raise a hard error (crash the system/BSOD) */
  NtRaiseHardError(STATUS_SYSTEM_PROCESS_TERMINATED,
		   0,0,0,0,0);

//   NtTerminateProcess(NtCurrentProcess(), 0);
}

/* EOF */
