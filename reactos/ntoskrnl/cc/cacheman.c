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
/*
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cc/cacheman.c
 * PURPOSE:         Cache manager
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * PORTABILITY:     Checked
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/cc.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID
CcInit(VOID)
{
  CcInitView();
}

/*
 * @unimplemented
 */
LARGE_INTEGER
STDCALL
CcGetFlushedValidData (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
    IN BOOLEAN BcbListHeld
    )
{
	UNIMPLEMENTED;

	LARGE_INTEGER i; 
	i.QuadPart = 0;
	return i;
}

/*
 * @unimplemented
 */
VOID
STDCALL
CcRepinBcb (
	IN	PVOID	Bcb
	)
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
CcRemapBcb (
    IN PVOID Bcb
    )
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
CcScheduleReadAhead (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length
	)
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
CcSetAdditionalCacheAttributes (
	IN	PFILE_OBJECT	FileObject,
	IN	BOOLEAN		DisableReadAhead,
	IN	BOOLEAN		DisableWriteBehind
	)
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
CcSetBcbOwnerPointer (
	IN	PVOID	Bcb,
	IN	PVOID	Owner
	)
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
CcSetDirtyPageThreshold (
	IN	PFILE_OBJECT	FileObject,
	IN	ULONG		DirtyPageThreshold
	)
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
CcSetReadAheadGranularity (
	IN	PFILE_OBJECT	FileObject,
	IN	ULONG		Granularity
	)
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
CcUnpinRepinnedBcb (
	IN	PVOID			Bcb,
	IN	BOOLEAN			WriteThrough,
	IN	PIO_STATUS_BLOCK	IoStatus
	)
{
	UNIMPLEMENTED;
}
