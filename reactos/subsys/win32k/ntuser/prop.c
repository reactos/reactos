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
/* $Id: prop.c,v 1.4 2003/08/11 19:05:27 gdalsnes Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window properties
 * FILE:             subsys/win32k/ntuser/prop.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
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

//#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

PPROPERTY FASTCALL
W32kGetProp(PWINDOW_OBJECT WindowObject, ATOM Atom)
{
  PLIST_ENTRY ListEntry;
  PPROPERTY Property;
  
  ListEntry = WindowObject->PropListHead.Flink;
  while (ListEntry != &WindowObject->PropListHead)
    {
      Property = CONTAINING_RECORD(ListEntry, PROPERTY, PropListEntry);
      if (Property->Atom == Atom)
	{
	  return(Property);
	}
      ListEntry = ListEntry->Flink;
    }
  return(NULL);
}

DWORD STDCALL
NtUserBuildPropList(DWORD Unknown0,
		    DWORD Unknown1,
		    DWORD Unknown2,
		    DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

HANDLE STDCALL
NtUserRemoveProp(HWND hWnd, ATOM Atom)
{
  PWINDOW_OBJECT WindowObject;
  PPROPERTY Prop;
  HANDLE Data;

  if (!(WindowObject = W32kGetWindowObject(hWnd)))
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return NULL;
  }

  Prop = W32kGetProp(WindowObject, Atom);
  if (Prop == NULL)
    {
      W32kReleaseWindowObject(WindowObject);
      return(NULL);
    }
  Data = Prop->Data;
  RemoveEntryList(&Prop->PropListEntry);
  ExFreePool(Prop);
  W32kReleaseWindowObject(WindowObject);
  return(Data);
}

HANDLE STDCALL
NtUserGetProp(HWND hWnd, ATOM Atom)
{
  PWINDOW_OBJECT WindowObject;
  PPROPERTY Prop;
  HANDLE Data = NULL;

  W32kAcquireWinLockShared();

  if (!(WindowObject = W32kGetWindowObject(hWnd)))
  {
    W32kReleaseWinLock();
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return FALSE;
  }

  Prop = W32kGetProp(WindowObject, Atom);
  if (Prop != NULL)
  {
    Data = Prop->Data;
  }
  
  W32kReleaseWinLock();

  return(Data);
}

BOOL FASTCALL
W32kSetProp(PWINDOW_OBJECT Wnd, ATOM Atom, HANDLE Data)
{
  PPROPERTY Prop;

  Prop = W32kGetProp(Wnd, Atom);

  if (Prop == NULL)
  {
    Prop = ExAllocatePool(PagedPool, sizeof(PROPERTY));
    if (Prop == NULL) return FALSE;
    Prop->Atom = Atom;
    InsertTailList(&Wnd->PropListHead, &Prop->PropListEntry);
  }

  Prop->Data = Data;
  return TRUE;
}


BOOL STDCALL
NtUserSetProp(HWND hWnd, ATOM Atom, HANDLE Data)
{
  PWINDOW_OBJECT Wnd;
  BOOL ret;

  W32kAcquireWinLockExclusive();

  if (!(Wnd = W32kGetWindowObject(hWnd)))
  {
    W32kReleaseWinLock();
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return FALSE;
  }

  ret = W32kSetProp(Wnd, Atom, Data);

  W32kReleaseWinLock();
  return ret;
}

/* EOF */
