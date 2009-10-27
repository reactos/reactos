/* $Id$
 *
 * smss.c - Session Manager
 *
 * ReactOS Operating System
 *
 * --------------------------------------------------------------------
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * --------------------------------------------------------------------
 *
 * 	19990529 (Emanuele Aliberti)
 * 		Compiled successfully with egcs 1.1.2
 */

#include <csrss.h>

#define NDEBUG
#include <debug.h>

VOID WINAPI DisplayString(LPCWSTR lpwString)
{
   UNICODE_STRING us;

   RtlInitUnicodeString (&us, lpwString);
   ZwDisplayString (&us);
}

VOID WINAPI PrintString (char* fmt, ...)
{
   char buffer[512];
   va_list ap;
   UNICODE_STRING UnicodeString;
   ANSI_STRING AnsiString;

   va_start(ap, fmt);
   vsprintf(buffer, fmt, ap);
   va_end(ap);

   RtlInitAnsiString (&AnsiString, buffer);
   RtlAnsiStringToUnicodeString (&UnicodeString,
				 &AnsiString,
				 TRUE);
   NtDisplayString(&UnicodeString);
   RtlFreeUnicodeString (&UnicodeString);
}

/* EOF */
