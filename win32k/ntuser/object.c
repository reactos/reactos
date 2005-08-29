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

/* return TRUE if its ok to destroy the object */
BOOLEAN FASTCALL UserSetCheckDestroy(PUSER_OBJECT_HDR hdr)
{
   if (hdr->flags & USER_OBJ_DESTROYED)
   {
      ASSERT(hdr->flags & USER_OBJ_DESTROYING);
      /* already destroyed */
      return FALSE;
   }
   
   hdr->flags |= USER_OBJ_DESTROYING;   
   if (hdr->refs)
   {
      /* will be destroyed when refcount reach zero */
      return FALSE;
   }
      
   hdr->flags |= USER_OBJ_DESTROYED;
   /* go ahead and destroy */
   return TRUE;
}


VOID FASTCALL UserDestroyObject(PUSER_OBJECT_HDR hdr)
{
   PUSER_HANDLE_ENTRY entry = handle_to_entry(&gHandleTable, hdr->hSelf);
   
   switch (entry->type)
   {
      case otWindow:
         //UserDestroyWindow(hdr);
         break;
         
      case otMenu:
         //UserDestroyMenuObject(hdr);
         break;
         
      case otAccel:
         //UserDestroyAccelObject(hdr);
         break;         
         
      case otCursor:
         //UserDestroyCursorObject(hdr);
         break;         
         
      case otHook:
         //UserDestroyHookObject(hdr);
         break;         
         
      case otMonitor:
         //UserDestroyMonitorObject(hdr);
         break;         
         
      default:
         ASSERT(FALSE);
   }
}
