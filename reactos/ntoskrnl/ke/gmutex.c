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
/* $Id$
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/gmutex.c
 * PURPOSE:         Implements guarded mutex (w2k3+/64)
 * PROGRAMMER:      
 * UPDATE HISTORY:
 *                  Created 2004-10-31 based on Steve Dispensa's blog
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
KeAcquireGuardedMutex
KeAcquireGuardedMutexUnsafe
KeEnterGuardedRegion
KeInitializeGuardedMutex
KeReleaseGuardedMutexUnsafe
KeTryToAcquireGuardedMutex
KeReleaseGuardedMutex
KeLeaveGuardedRegion
*/

/* EOF */
