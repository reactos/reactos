/* $Id$
 *
 * print.c - Print on the blue screen
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

#define NDEBUG
#include <debug.h>

VOID STDCALL DisplayString(LPCWSTR lpwString)
{
   UNICODE_STRING us;

   RtlInitUnicodeString (&us, lpwString);
   NtDisplayString (&us);
}

VOID STDCALL PrintString (char* fmt, ...)
{
   char buffer[512];
   va_list ap;
   UNICODE_STRING UnicodeString;
   ANSI_STRING AnsiString;

   va_start(ap, fmt);
   vsprintf(buffer, fmt, ap);
   va_end(ap);
   DPRINT1("%s", buffer);

   RtlInitAnsiString (&AnsiString, buffer);
   RtlAnsiStringToUnicodeString (&UnicodeString,
				 &AnsiString,
				 TRUE);
   NtDisplayString(&UnicodeString);
   RtlFreeUnicodeString (&UnicodeString);
}

/* EOF */
