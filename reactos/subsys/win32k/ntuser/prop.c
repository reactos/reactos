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
/* $Id: prop.c,v 1.11.12.2 2004/09/01 14:14:26 weiden Exp $
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
  while(ListEntry != &WindowObject->PropListHead)
  {
    Property = CONTAINING_RECORD(ListEntry, PROPERTY, PropListEntry);
    if(Property->Atom == Atom)
    {
      return Property;
    }
    ListEntry = ListEntry->Flink;
  }
  
  return NULL;
}

BOOL FASTCALL
IntRemoveProp(PWINDOW_OBJECT WindowObject, ATOM Atom, HANDLE *Data)
{
  PLIST_ENTRY ListEntry;
  PPROPERTY Property;
  
  ASSERT(Data);

  ListEntry = WindowObject->PropListHead.Flink;
  while(ListEntry != &WindowObject->PropListHead)
  {
    Property = CONTAINING_RECORD(ListEntry, PROPERTY, PropListEntry);
    if(Property->Atom == Atom)
    {
      *Data = Property->Data;
      RemoveEntryList(&Property->PropListEntry);
      ExFreePool(Property);
      return TRUE;
    }
    ListEntry = ListEntry->Flink;
  }
  
  return FALSE;
}

BOOL FASTCALL
IntSetProp(PWINDOW_OBJECT WindowObject, ATOM Atom, HANDLE Data)
{
  PPROPERTY Prop = IntGetProp(WindowObject, Atom);
  if(Prop == NULL)
  {
    Prop = ExAllocatePoolWithTag(PagedPool, sizeof(PROPERTY), TAG_WNDPROP);
    if(Prop != NULL)
    {
      Prop->Atom = Atom;
      Prop->Data = Data;
      InsertTailList(&WindowObject->PropListHead, &Prop->PropListEntry);
      WindowObject->PropListItems++;

      return TRUE;
    }
  }

  return FALSE;
}

/* EOF */
