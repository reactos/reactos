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
/* $Id: object.c,v 1.12.8.3 2004/09/14 01:00:43 weiden Exp $
 *
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * PURPOSE:        User object manager
 * FILE:           subsys/win32k/misc/object.c
 * PROGRAMMERS:    David Welch (welch@cwcom.net)
 *                 Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *     06-06-2001  CSH  Ported kernel object manager
 */
/* INCLUDES ******************************************************************/
#include <w32k.h>

#define NDEBUG
#include <debug.h>

#define HEADER_TO_BODY(ObjectHeader) \
  ((PVOID)(((PUSER_OBJECT_HEADER)ObjectHeader) + 1))

#define BODY_TO_HEADER(ObjectBody) \
  ((PUSER_OBJECT_HEADER)(((PUSER_OBJECT_HEADER)ObjectBody) - 1))

/* FUNCTIONS *****************************************************************/



VOID INTERNAL_CALL
ObmReferenceObject(PVOID ObjectBody)
/*
 * FUNCTION: Increments a given object's reference count and performs
 *           retention checks
 * ARGUMENTS:
 *   ObjectBody = Body of the object
 */
{
  PUSER_OBJECT_HEADER ObjectHeader;
  
  ASSERT(ObjectBody);
  
  ObjectHeader = BODY_TO_HEADER(ObjectBody);

  InterlockedIncrement(&ObjectHeader->RefCount);
}

VOID INTERNAL_CALL
ObmDereferenceObject(PVOID ObjectBody)
/*
 * FUNCTION: Decrements a given object's reference count and performs
 *           retention checks
 * ARGUMENTS:
 *   ObjectBody = Body of the object
 */
{
  PUSER_OBJECT_HEADER ObjectHeader;
  
  ASSERT(ObjectBody);
  
  ObjectHeader = BODY_TO_HEADER(ObjectBody);
  
  if(InterlockedDecrement(&ObjectHeader->RefCount) == 0)
  {
    /* free the object when 0 references reached */
    ExFreePool(ObjectHeader);
  }
}

PVOID INTERNAL_CALL
ObmEnumHandles(PUSER_HANDLE_TABLE HandleTable,
               USER_OBJECT_TYPE ObjectType,
               PVOID UserData,
               PFNENUMHANDLESPROC EnumProc)
{
  PUSER_OBJECT_HEADER ObjectHeader, *Slot, *LastSlot;
  
  ASSERT(EnumProc);
  
  /* enumerate all handles */
  LastSlot = HandleTable->Handles + N_USER_HANDLES;
  for(Slot = HandleTable->Handles; Slot < LastSlot; Slot++)
  {
    if((ObjectHeader = *Slot) && (ObjectType == ObjectHeader->Type || ObjectType == otUNKNOWN))
    {
      PVOID ObjectBody = HEADER_TO_BODY(ObjectHeader);
      if(EnumProc(ObjectBody, UserData))
      {
        return ObjectBody;
      }
    }
  }
  
  return NULL;
}

PVOID INTERNAL_CALL
ObmCreateObject(PUSER_HANDLE_TABLE HandleTable,
		PHANDLE Handle,
		USER_OBJECT_TYPE ObjectType,
		ULONG ObjectSize)
{
  PUSER_OBJECT_HEADER ObjectHeader, *Slot, *LastSlot;
  PVOID ObjectBody;
  
  ASSERT(HandleTable);
  ASSERT(Handle);
  ASSERT(ObjectSize);
  
  if(HandleTable->HandleCount == N_USER_HANDLES)
  {
    DPRINT1("No more free user handles!\n");
    return NULL;
  }
  
  ObjectHeader = (PUSER_OBJECT_HEADER)ExAllocatePool(PagedPool, 
                                                     sizeof(USER_OBJECT_HEADER) + ObjectSize);
  if (!ObjectHeader)
  {
    DPRINT1("Not enough memory to create a user object\n");
    return NULL;
  }
  
  ObjectHeader->Type = ObjectType;
  ObjectHeader->RefCount = 1;
  
  ObjectBody = HEADER_TO_BODY(ObjectHeader);
  RtlZeroMemory(ObjectBody, ObjectSize);
  
  /* search for a free handle slot */
  LastSlot = HandleTable->Handles + N_USER_HANDLES;
  for(Slot = HandleTable->Handles; Slot < LastSlot; Slot++)
  {
    if(InterlockedCompareExchange((LONG*)Slot, (LONG)ObjectHeader, 0) == 0)
    {
      /* found and assigned a free handle */
      InterlockedIncrement((LONG*)&HandleTable->HandleCount);
      
      ObjectHeader->Slot = Slot; 
      *Handle = (HANDLE)((((ULONG)Slot - (ULONG)HandleTable->Handles) / sizeof(HANDLE)) + 1);

      return ObjectBody;
    }
  }
  
  ExFreePool(ObjectHeader);
  
  return NULL;
}

BOOL INTERNAL_CALL
ObmObjectDeleted(PVOID ObjectBody)
{
  PUSER_OBJECT_HEADER ObjectHeader;
  
  ASSERT(ObjectBody);
  
  ObjectHeader = BODY_TO_HEADER(ObjectBody);
  return ((ObjectHeader->Slot == NULL) || (*(ObjectHeader->Slot) != ObjectHeader));
}

USER_OBJECT_TYPE INTERNAL_CALL
ObmGetObjectType(PUSER_HANDLE_TABLE HandleTable,
                 PVOID ObjectBody)
{
  PUSER_OBJECT_HEADER ObjectHeader;
  
  ASSERT(ObjectBody);
  
  ObjectHeader = BODY_TO_HEADER(ObjectBody);
  return ObjectHeader->Type;
}

BOOL INTERNAL_CALL
ObmDeleteObject(PUSER_HANDLE_TABLE HandleTable,
                PVOID ObjectBody)
{
  PUSER_OBJECT_HEADER ObjectHeader;
  
  ASSERT(ObjectBody);
  
  ObjectHeader = BODY_TO_HEADER(ObjectBody);
  
  if(ObjectHeader->Slot == NULL)
  {
    DPRINT1("Object 0x%x has been deleted already!\n");
    return FALSE;
  }

  /* remove the object from the handle table */
  InterlockedCompareExchange((LONG*)ObjectHeader->Slot, 0, (LONG)ObjectHeader);
  InterlockedDecrement((LONG*)&HandleTable->HandleCount);
  ObjectHeader->Slot = NULL;
  
  ObmDereferenceObject(ObjectBody);
  
  return TRUE;
}

PVOID INTERNAL_CALL
ObmGetObject(PUSER_HANDLE_TABLE HandleTable,
             HANDLE Handle,
	     USER_OBJECT_TYPE ObjectType)
{
  PUSER_OBJECT_HEADER *Slot, ObjectHeader;
  
  if(Handle == NULL || (ULONG)Handle > N_USER_HANDLES)
  {
    return FALSE;
  }
  
  Slot = HandleTable->Handles + (ULONG)Handle - 1;
  if((ObjectHeader = (*Slot)) && (ObjectType == ObjectHeader->Type))
  {
    return HEADER_TO_BODY(ObjectHeader);
  }
  
  return NULL;
}

VOID INTERNAL_CALL
ObmInitializeHandleTable(PUSER_HANDLE_TABLE HandleTable)
{
  PUSER_OBJECT_HEADER *Slot, *LastSlot;
  
  HandleTable->HandleCount = 0;
  /* Clear the handle table */
  LastSlot = HandleTable->Handles + N_USER_HANDLES;
  for(Slot = HandleTable->Handles; Slot < LastSlot; Slot++)
  {
    *Slot = NULL;
  }
}

VOID INTERNAL_CALL
ObmFreeHandleTable(PUSER_HANDLE_TABLE HandleTable)
{
  /* FIXME - delete all handles */
}

PUSER_HANDLE_TABLE INTERNAL_CALL
ObmCreateHandleTable(VOID)
{
  PUSER_HANDLE_TABLE HandleTable;

  HandleTable = (PUSER_HANDLE_TABLE)ExAllocatePool(PagedPool, 
						   sizeof(USER_HANDLE_TABLE));
  if (!HandleTable)
    {
      DPRINT1("Unable to create handle table\n");
      return NULL;
    }
  
  ObmInitializeHandleTable(HandleTable);
  
  return HandleTable;
}

VOID INTERNAL_CALL
ObmDestroyHandleTable(PUSER_HANDLE_TABLE HandleTable)
{
  ObmFreeHandleTable(HandleTable);
  ExFreePool(HandleTable);
}

/* EOF */
