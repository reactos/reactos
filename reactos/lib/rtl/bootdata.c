/*
 *  ReactOS kernel
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
/* $Id$
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Boot Data implementation
 * FILE:              lib/rtl/bootdata.c
 */

#include <ddk/ntddk.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
* @unimplemented
*/
NTSTATUS
STDCALL
RtlCreateSystemVolumeInformationFolder(
	IN PUNICODE_STRING VolumeRootPath
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
STDCALL
RtlGetSetBootStatusData(
	HANDLE Filehandle,
	BOOLEAN WriteMode,
	DWORD DataClass,
	PVOID Buffer,
	ULONG BufferSize,
	DWORD DataClass2
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
STDCALL
RtlLockBootStatusData(
	HANDLE Filehandle
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
STDCALL
RtlUnlockBootStatusData(
	HANDLE Filehandle
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
