/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Manage GDI Handles
 * FILE:              subsys/win32k/eng/handle.c
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 29/8/1999: Created
 */

#include <ddk/winddi.h>
#include "handle.h"

// FIXME: Total rewrite..
// Place User and GDI objects in ONE object with the user items shown first
// See ..\objects\gdiobj
// To switch between user and gdi objects, just -\+ the size of all the user items

ULONG CreateGDIHandle(PVOID InternalObject, PVOID UserObject)
{
   ULONG NewHandle = HandleCounter++;

   GDIHandles[NewHandle].InternalObject = InternalObject;
   GDIHandles[NewHandle].UserObject     = UserObject;

   return NewHandle;
}

VOID FreeGDIHandle(ULONG Handle)
{
   GDIHandles[Handle].InternalObject = NULL;
   GDIHandles[Handle].UserObject     = NULL;
}

PVOID AccessInternalObject(ULONG Handle)
{
   return GDIHandles[Handle].InternalObject;
}

PVOID AccessUserObject(ULONG Handle)
{
   return GDIHandles[Handle].UserObject;
}

PVOID AccessInternalObjectFromUserObject(PVOID UserObject)
{
   ULONG i;

   for(i=0; i<MAX_GDI_HANDLES; i++)
   {
      if(GDIHandles[i].UserObject == UserObject)
         return GDIHandles[i].InternalObject;
   }

   return NULL;
}

ULONG AccessHandleFromUserObject(PVOID UserObject)
{
   ULONG i;

   for(i=0; i<MAX_GDI_HANDLES; i++)
   {
      if(GDIHandles[i].UserObject == UserObject)
         return i;
   }

   return INVALID_HANDLE;
}
