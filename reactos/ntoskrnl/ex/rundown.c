/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
 * FILE:            ntoskrnl/ex/rundown.c
 * PURPOSE:         Rundown Functions
 * PORTABILITY:     Checked
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <roscfg.h>
#include <internal/ldr.h>
#include <internal/kd.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
BOOLEAN
FASTCALL
ExAcquireRundownProtection (
	PVOID		ProcessRundownProtect
	)
{
	UNIMPLEMENTED;
	return FALSE;
}
/*
 * @unimplemented
 */
BOOLEAN
FASTCALL
ExAcquireRundownProtectionEx (
	IN PVOID	ProcessRundownProtect,
	IN PVOID	Unknown
	)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @unimplemented
 */
VOID
FASTCALL
ExInitializeRundownProtection (
	IN PVOID	ProcessRundownProtect
	)
{
	UNIMPLEMENTED;
}


/*
 * @unimplemented
 */
VOID
FASTCALL
ExReInitializeRundownProtection (
	IN PVOID	ProcessRundownProtect
	)
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
BOOLEAN
FASTCALL
ExReleaseRundownProtection (
	IN PVOID	ProcessRundownProtect
	)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @unimplemented
 */
BOOLEAN
FASTCALL
ExReleaseRundownProtectionEx (
	IN PVOID	ProcessRundownProtect,
	IN PVOID	Unknown
	)
{
	UNIMPLEMENTED;
	return FALSE;
}


/*
 * @unimplemented
 */
VOID
FASTCALL
ExRundownCompleted (
	IN PVOID	ProcessRundownProtect
	)
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
PVOID
FASTCALL
ExWaitForRundownProtectionRelease (
	PVOID		ProcessRundownProtect
	)
{
	UNIMPLEMENTED;
	return FALSE;
}

/* EOF */
