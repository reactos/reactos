/* $Id: smss.c,v 1.11 2002/05/24 07:49:41 ekohl Exp $
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
 * 
 * 	19990529 (Emanuele Aliberti)
 * 		Compiled successfully with egcs 1.1.2
 */
#include <ddk/ntddk.h>

#include "smss.h"


void
DisplayString(LPCWSTR lpwString)
{
  UNICODE_STRING us;

  RtlInitUnicodeString(&us, lpwString);
  NtDisplayString(&us);
}


void
PrintString(char* fmt,...)
{
  char buffer[512];
  va_list ap;
  UNICODE_STRING UnicodeString;
  ANSI_STRING AnsiString;

  va_start(ap, fmt);
  vsprintf(buffer, fmt, ap);
  va_end(ap);

  RtlInitAnsiString(&AnsiString, buffer);
  RtlAnsiStringToUnicodeString(&UnicodeString,
			       &AnsiString,
			       TRUE);
  NtDisplayString(&UnicodeString);
  RtlFreeUnicodeString(&UnicodeString);
}


/* Native image's entry point */

VOID
NtProcessStartup(PPEB Peb)
{
  HANDLE Children[2]; /* csrss, winlogon */
  NTSTATUS Status;

  Status = InitSessionManager(Children);
  if (!NT_SUCCESS(Status))
    {
      PrintString("SM: Initialization failed!\n");
      goto ByeBye;
    }

  Status = NtWaitForMultipleObjects(((LONG) sizeof(Children) / sizeof(HANDLE)),
				    Children,
				    WaitAny,
				    TRUE,	/* alertable */
				    NULL);	/* NULL for infinite */
  if (!NT_SUCCESS(Status))
    {
      PrintString("SM: NtWaitForMultipleObjects failed!\n");
    }
  else
    {
      PrintString("SM: Process terminated!\n");
    }

ByeBye:
  /* Raise a hard error (crash the system/BSOD) */
  NtRaiseHardError(STATUS_SYSTEM_PROCESS_TERMINATED,
		   0,0,0,0,0);

//   NtTerminateProcess(NtCurrentProcess(), 0);
}

/* EOF */
