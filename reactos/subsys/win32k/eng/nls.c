/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 2003 ReactOS Team
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
/* $Id: nls.c,v 1.1 2003/06/21 18:04:49 ekohl Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          NLS functions
 * FILE:             subsys/win32k/eng/nls.c
 * PROGRAMER:        Eric Kohl
 * REVISION HISTORY:
 *        6/21/2003: Created
 */

/* INCLUDES *****************************************************************/

#include <ddk/winddi.h>
#include <ddk/ntddk.h>


/* FUNCTIONS ****************************************************************/

VOID STDCALL
EngGetCurrentCodePage(OUT PUSHORT OemCodePage,
		      OUT PUSHORT AnsiCodePage)
{
  RtlGetDefaultCodePage (OemCodePage,
			 AnsiCodePage);
}

VOID STDCALL
EngMultiByteToUnicodeN(OUT LPWSTR UnicodeString,
		       IN ULONG MaxBytesInUnicodeString,
		       OUT PULONG BytesInUnicodeString,
		       IN PCHAR MultiByteString,
		       IN ULONG BytesInMultiByteString)
{
  RtlMultiByteToUnicodeN(UnicodeString,
			 MaxBytesInUnicodeString,
			 BytesInUnicodeString,
			 MultiByteString,
			 BytesInMultiByteString);
}

VOID STDCALL
EngUnicodeToMultiByteN(OUT PCHAR MultiByteString,
		       IN ULONG MaxBytesInMultiByteString,
		       OUT PULONG BytesInMultiByteString,
		       IN PWSTR UnicodeString,
		       IN ULONG BytesInUnicodeString)
{
  RtlUnicodeToMultiByteN(MultiByteString,
			 MaxBytesInMultiByteString,
			 BytesInMultiByteString,
			 UnicodeString,
			 BytesInUnicodeString);
}

/* EOF */
