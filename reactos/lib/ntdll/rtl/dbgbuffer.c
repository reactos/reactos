/*
 *  ReactOS kernel
 *  Copyright (C) 2004 ReactOS Team
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
/* $Id:
 *
 * PROJECT:           ReactOS kernel
 * PURPOSE:           User-mode Debug Buffer support
 * FILE:              lib/ntdll/rtl/dbgbuffer.c
 * PROGRAMER:         James Tabor
 */

/* INCLUDES *****************************************************************/

#include <ntos/types.h>
#include <napi/teb.h>
#include <ntdll/rtl.h>
#include <ddk/ntddk.h>

/* FUNCTIONS ***************************************************************/

PDEBUG_BUFFER STDCALL
RtlCreateQueryDebugBuffer(IN ULONG Size,
                          IN BOOLEAN EventPair)
{
	return(0);
}

NTSTATUS STDCALL
RtlDestroyQueryDebugBuffer(IN PDEBUG_BUFFER Buf)
{
	return(0);
}

NTSTATUS STDCALL 
RtlQueryProcessDebugInformation(IN ULONG ProcessId, 
                                IN ULONG DebugInfoMask, 
                                IN OUT PDEBUG_BUFFER Buf)
{
	return (0);
}


