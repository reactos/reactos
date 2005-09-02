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
 *
 *  $Id: painting.c 16320 2005-06-29 07:09:25Z navaraf $
 *
 *  COPYRIGHT:        See COPYING in the top level directory
 *  PROJECT:          ReactOS kernel
 *  PURPOSE:          ntuser init. and main funcs.
 *  FILE:             subsys/win32k/ntuser/ntuser.c
 *  REVISION HISTORY:
 *       16 July 2005   Created (hardon)
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

#if 0
VOID FASTCALL UserLinkVolatileObject2(PUSER_OBJECT_HDR hdr, PSINGLE_LIST_ENTRY list, PVOLATILE_OBJECT_ENTRY e)
{
//   if (obj){

   e->hdr = hdr;
   e->recycle_count = HIWORD(hdr->hSelf);
   e->objType = hdr->handle_entry->type;
   e->handle_entry = hdr->handle_entry;
   PushEntryList(list, &e->link);
}

BOOLEAN FASTCALL UserValidateVolatileObjects2(PSINGLE_LIST_ENTRY list)
{
   PSINGLE_LIST_ENTRY p = list->Next;
   while (p)
   {
      PVOLATILE_OBJECT_ENTRY e = CONTAINING_RECORD(p, VOLATILE_OBJECT_ENTRY, link);

      if 
      (
         (PUSER_OBJECT_HDR)e->handle_entry->ptr != e->hdr || 
         e->recycle_count != HIWORD(e->hdr->hSelf) || 
         e->objType != e->hdr->handle_entry->type
      )
         return FALSE;

      p = p->Next;
   }
   
   return TRUE;
}

inline OBJECT_ENTRY FASTCALL UserGetEntry2(PUSER_OBJECT_HDR hdr)
{
   OBJECT_ENTRY e;
   
   e.generation = hdr->handle_entry->generation;
   e.handle_entry = hdr->handle_entry;
   e.type = hdr->handle_entry->type;
   e.hdr = hdr;
   
   return e;
}


inline BOOLEAN FASTCALL UserValidateEntry2(POBJECT_ENTRY e)
{
   if 
   (
      (PUSER_OBJECT_HDR)e->handle_entry->ptr != e->hdr || 
         e->generation != HIWORD(e->hdr->hSelf) || 
         e->type != e->hdr->handle_entry->type
   )
      return FALSE;
      
   return TRUE;
}
#endif
