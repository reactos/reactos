/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998 - 2004 ReactOS Team
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
/* $Id: userlock.c,v 1.1.4.2 2004/09/01 22:14:50 weiden Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          User locks
 * FILE:             subsys/win32k/ntuser/userlock.c
 * PROGRAMER:        Thomas Weidenmueller <w3seek@reactos.com>
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */
#include <w32k.h>

static ERESOURCE UserLock;


VOID FASTCALL
IntInitUserResourceLocks(VOID)
{
  ExInitializeResourceLite(&UserLock);
}

VOID FASTCALL
IntCleanupUserResourceLocks(VOID)
{
  ExDeleteResourceLite(&UserLock);
}

inline VOID
IntUserEnterCritical(VOID)
{
  ExAcquireResourceExclusiveLite(&UserLock, TRUE);
}

inline VOID
IntUserEnterCriticalShared(VOID)
{
  ExAcquireResourceSharedLite(&UserLock, TRUE);
}

inline VOID
IntUserLeaveCritical(VOID)
{
  ExReleaseResourceLite(&UserLock);
}

inline BOOL
IntUserIsInCriticalShared(VOID)
{
  /* Exclusive locks are shared locks at the same time */
  return ExIsResourceAcquiredSharedLite(&UserLock);
}

inline BOOL
IntUserIsInCritical(VOID)
{
  return ExIsResourceAcquiredExclusiveLite(&UserLock);
}

