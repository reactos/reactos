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
/* $Id: autochk.c,v 1.4 2002/10/25 22:08:20 chorns Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             apps/system/autochk/autochk.c
 * PURPOSE:          Filesystem checker
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <napi/shared_data.h>

/* FUNCTIONS ****************************************************************/

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

VOID STDCALL
NtProcessStartup(PPEB Peb)
{
  ULONG i;

  PrintString("Autochk 0.0.1\n");

  for (i = 0; i < 26; i++)
    {
      if ((SharedUserData->DosDeviceMap & (1 << i)) &&
	  (SharedUserData->DosDeviceDriveType[i] == DOSDEVICE_DRIVE_FIXED))
	{
	  PrintString("  Checking drive %c:", 'A'+i);
	  PrintString("      OK\n");
	}
    }
  PrintString("\n");

  NtTerminateProcess(NtCurrentProcess(), 0);
}

/* EOF */
