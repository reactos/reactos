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
 * FILE:            ntoskrnl/ex/synch.c
 * PURPOSE:         Synchronization Functions (Pushlocks)
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
PVOID
FASTCALL
ExfAcquirePushLockExclusive (
	PVOID		Lock
	)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @unimplemented
 */
PVOID
FASTCALL
ExfAcquirePushLockShared (
	PVOID		Lock
	)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @unimplemented
 */
PVOID
FASTCALL
ExfReleasePushLock (
	PVOID		Lock
	)
{
	UNIMPLEMENTED;
}

/* EOF */
