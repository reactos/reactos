/* $Id: print.c,v 1.3 2001/08/14 12:57:16 ea Exp $
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

#include <csrss/csrss.h>


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
   ULONG i;
   
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
