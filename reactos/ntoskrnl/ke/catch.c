/*
 *  ReactOS kernel
 *  Copyright (C) 2000  ReactOS Team
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
/* $Id: catch.c,v 1.11 2001/03/16 23:04:59 dwelch Exp $
 *
 * PROJECT:              ReactOS kernel
 * FILE:                 ntoskrnl/ke/catch.c
 * PURPOSE:              Exception handling
 * PROGRAMMER:           David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS ****************************************************************/

VOID STDCALL
ExRaiseAccessViolation (VOID)
{
  ExRaiseStatus (STATUS_ACCESS_VIOLATION);
}

VOID STDCALL
ExRaiseDatatypeMisalignment (VOID)
{
  ExRaiseStatus (STATUS_DATATYPE_MISALIGNMENT);
}

VOID STDCALL
ExRaiseStatus (IN	NTSTATUS	Status)
{
  DbgPrint("ExRaiseStatus(%x)\n",Status);
  for(;;);
}



NTSTATUS STDCALL
NtRaiseException (IN	PEXCEPTION_RECORD	ExceptionRecord,
		  IN	PCONTEXT		Context,
		  IN	BOOL			IsDebugger	OPTIONAL)
{
  UNIMPLEMENTED;
}


VOID STDCALL
RtlRaiseException(PEXCEPTION_RECORD ExceptionRecord)
{

}


VOID STDCALL
RtlUnwind(ULONG Unknown1,
	  ULONG Unknown2,
	  ULONG Unknown3,
	  ULONG Unknown4)
{

}

/* EOF */
