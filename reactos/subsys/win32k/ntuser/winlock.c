/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
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
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Windows++ locking
 * FILE:             subsys/win32k/ntuser/winlock.c
 * PROGRAMER:        Gunnar
 * REVISION HISTORY:
 *
 */
/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <win32k/win32k.h>
#include <include/object.h>
#include <include/guicheck.h>
#include <include/window.h>
#include <include/class.h>
#include <include/error.h>
#include <include/winsta.h>
#include <include/winpos.h>
#include <include/callback.h>
#include <include/msgqueue.h>
#include <include/rect.h>

#define NDEBUG
#include <win32k/debug1.h>
#include <debug.h>

/* GLOBALS *****************************************************************/

static ERESOURCE WinLock;

/* FUNCTIONS *****************************************************************/

BOOL FASTCALL IntVerifyWinLock(WINLOCK_TYPE Type)
{
  switch (Type)
  {
    case None:
      return !ExIsResourceAcquiredSharedLite(&WinLock);
    case Shared: /* NOTE: an exclusive lock is also a shared lock */
    case Any: 
      return ExIsResourceAcquiredSharedLite(&WinLock);
    case Exclusive:
      return ExIsResourceAcquiredExclusiveLite(&WinLock);
  }

  KEBUGCHECK(0);
  return FALSE;
}

WINLOCK_TYPE FASTCALL IntSuspendWinLock()
{
  ASSERT_WINLOCK(Any);

  if (ExIsResourceAcquiredExclusiveLite(&WinLock)) return Exclusive;
  
  return Shared;
}

VOID FASTCALL IntRestoreWinLock(WINLOCK_TYPE Type)
{
  switch (Type)
  {
    case Exclusive:
      return IntAcquireWinLockExclusive(&WinLock);
    case Shared:
      return IntAcquireWinLockShared(&WinLock);
    /* silence warnings */
    case None:
    case Any:
      break;
  }

  KEBUGCHECK(0);
}

inline VOID IntAcquireWinLockShared()
{
  ExAcquireResourceSharedLite(&WinLock, TRUE /*Wait*/ );
}

inline VOID IntAcquireWinLockExclusive()
{
  ExAcquireResourceExclusiveLite(&WinLock, TRUE /*Wait*/  );
}

inline VOID IntReleaseWinLock()
{
  ExReleaseResourceLite(&WinLock ); 
}

inline BOOL IntInitializeWinLock()
{
  ExInitializeResourceLite(&WinLock );
  return TRUE;
}

inline VOID IntDeleteWinLock()
{
  ExDeleteResourceLite(&WinLock );  
}

/* EOF */
