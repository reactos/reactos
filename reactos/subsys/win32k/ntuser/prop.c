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
/* $Id$
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

#include <w32k.h>

#define NDEBUG
#include <debug.h>

typedef struct _PROPLISTITEM
{
  ATOM Atom;
  HANDLE Data;
} PROPLISTITEM, *PPROPLISTITEM;

/* FUNCTIONS *****************************************************************/

PPROPERTY FASTCALL
IntGetProp(PWINDOW_OBJECT WindowObject, ATOM Atom)
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

NTSTATUS STDCALL
NtUserBuildPropList(HWND hWnd,
		    LPVOID Buffer,
		    DWORD BufferSize,
		    DWORD *Count)
{
  PWINDOW_OBJECT WindowObject;
  PPROPERTY Property;
  PLIST_ENTRY ListEntry;
  PROPLISTITEM listitem, *li;
  NTSTATUS Status;
  DWORD Cnt = 0;
  DECLARE_RETURN(NTSTATUS);
  
  DPRINT("Enter NtUserBuildPropList\n");
  UserEnterShared();

  if (!(WindowObject = IntGetWindowObject(hWnd)))
  {
    RETURN( STATUS_INVALID_HANDLE);
  }

  if(Buffer)
  {
    if(!BufferSize || (BufferSize % sizeof(PROPLISTITEM) != 0))
    {
      IntReleaseWindowObject(WindowObject);
      RETURN( STATUS_INVALID_PARAMETER);
    }

    /* copy list */
    li = (PROPLISTITEM *)Buffer;
    ListEntry = WindowObject->PropListHead.Flink;
    while((BufferSize >= sizeof(PROPLISTITEM)) && (ListEntry != &WindowObject->PropListHead))
    {
      Property = CONTAINING_RECORD(ListEntry, PROPERTY, PropListEntry);
      listitem.Atom = Property->Atom;
      listitem.Data = Property->Data;

      Status = MmCopyToCaller(li, &listitem, sizeof(PROPLISTITEM));
      if(!NT_SUCCESS(Status))
      {
        IntReleaseWindowObject(WindowObject);
        RETURN( Status);
      }

      BufferSize -= sizeof(PROPLISTITEM);
      Cnt++;
      li++;
      ListEntry = ListEntry->Flink;
    }

  }
  else
  {
    Cnt = WindowObject->PropListItems * sizeof(PROPLISTITEM);
  }

  IntReleaseWindowObject(WindowObject);

  if(Count)
  {
    Status = MmCopyToCaller(Count, &Cnt, sizeof(DWORD));
    if(!NT_SUCCESS(Status))
    {
      RETURN( Status);
    }
  }

  RETURN( STATUS_SUCCESS);
  
CLEANUP:
  DPRINT("Leave NtUserBuildPropList, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
}

HANDLE STDCALL
NtUserRemoveProp(HWND hWnd, ATOM Atom)
{
  PWINDOW_OBJECT WindowObject;
  PPROPERTY Prop;
  HANDLE Data;
  DECLARE_RETURN(HANDLE);
  
  DPRINT("Enter NtUserRemoveProp\n");
  UserEnterExclusive();

  if (!(WindowObject = IntGetWindowObject(hWnd)))
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    RETURN( NULL);
  }

  Prop = IntGetProp(WindowObject, Atom);

  if (Prop == NULL)
    {
      IntReleaseWindowObject(WindowObject);
      RETURN(NULL);
    }
  Data = Prop->Data;
  RemoveEntryList(&Prop->PropListEntry);
  ExFreePool(Prop);
  WindowObject->PropListItems--;
  IntReleaseWindowObject(WindowObject);
  RETURN(Data);
  
CLEANUP:
  DPRINT("Leave NtUserRemoveProp, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;  
}

HANDLE STDCALL
NtUserGetProp(HWND hWnd, ATOM Atom)
{
  PWINDOW_OBJECT WindowObject;
  PPROPERTY Prop;
  HANDLE Data = NULL;
  DECLARE_RETURN(HANDLE);
  
  DPRINT("Enter NtUserGetProp\n");
  UserEnterShared();

  if (!(WindowObject = IntGetWindowObject(hWnd)))
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    RETURN( FALSE);
  }

  Prop = IntGetProp(WindowObject, Atom);
  if (Prop != NULL)
  {
    Data = Prop->Data;
  }
  IntReleaseWindowObject(WindowObject);
  RETURN(Data);
  
CLEANUP:
  DPRINT("Leave NtUserGetProp, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
}

BOOL FASTCALL
IntSetProp(PWINDOW_OBJECT Wnd, ATOM Atom, HANDLE Data)
{
  PPROPERTY Prop;

  Prop = IntGetProp(Wnd, Atom);

  if (Prop == NULL)
  {
    Prop = ExAllocatePoolWithTag(PagedPool, sizeof(PROPERTY), TAG_WNDPROP);
    if (Prop == NULL)
    {
      return FALSE;
    }
    Prop->Atom = Atom;
    InsertTailList(&Wnd->PropListHead, &Prop->PropListEntry);
    Wnd->PropListItems++;
  }

  Prop->Data = Data;
  return TRUE;
}


BOOL STDCALL
NtUserSetProp(HWND hWnd, ATOM Atom, HANDLE Data)
{
  PWINDOW_OBJECT WindowObject;
  BOOL ret;
  DECLARE_RETURN(BOOL);
  
  DPRINT("Enter NtUserSetProp\n");
  UserEnterExclusive();

  if (!(WindowObject = IntGetWindowObject(hWnd)))
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    RETURN( FALSE);
  }

  ret = IntSetProp(WindowObject, Atom, Data);

  IntReleaseWindowObject(WindowObject);
  RETURN( ret);
  
CLEANUP:
  DPRINT("Leave NtUserSetProp, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
}

/* EOF */
