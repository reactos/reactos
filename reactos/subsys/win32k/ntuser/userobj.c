/* $Id: userobj.c,v 1.1 2001/07/06 00:05:05 rex Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          USER Object manager interface definition
 * FILE:             subsys/win32k/ntuser/userobj.c
 * PROGRAMER:        Rex Jolliff (rex@lvcablemodem.com)
 *
 */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#define NDEBUG
#include <debug.h>
#include <ddk/ntddk.h>
#include <win32k/userobj.h>

static  LIST_ENTRY  UserObjectList;

PUSER_OBJECT  USEROBJ_AllocObject (WORD size, WORD magic)
{
  PUSER_OBJECT_HEADER  newObject;

  newObject = ExAllocatePoolWithTag(PagedPool, 
                                    size + sizeof (USER_OBJECT_HEADER), 
                                    USER_OBJECT_TAG);
  if (newObject == 0)
  {
    return  0;
  }
  RtlZeroMemory(newObject, size + sizeof (USER_OBJECT_HEADER));

  newObject->magic = magic;
  ExInitializeFastMutex (&newObject->mutex);
  InsertTailList (&UserObjectList, &newObject->listEntry);

  return  UserObjectHeaderToBody (newObject);
}

BOOL  USEROBJ_FreeObject (PUSER_OBJECT object, WORD magic)
{
  PUSER_OBJECT_HEADER  objectHeader;

  if (object == NULL)
  {
    return FALSE;
  }
  objectHeader = UserObjectBodyToHeader (object);
  if (objectHeader->magic != magic)
  {
    return  FALSE;
  }
  RemoveEntryList (&objectHeader->listEntry);
  ExFreePool (objectHeader);

  return  TRUE;
}

HUSEROBJ  USEROBJ_PtrToHandle (PUSER_OBJECT  object, WORD  magic)
{
  PUSER_OBJECT_HEADER  objectHeader;
  
  if (object == 0)
  {
    return  0;
  }
  objectHeader = UserObjectBodyToHeader (object);
  if (objectHeader->magic != magic)
  {
    return  0;
  }
  
  return  UserObjectHeaderToHandle(objectHeader);
}

PUSER_OBJECT  USEROBJ_HandleToPtr (HUSEROBJ  handle, WORD  magic)
{
  PUSER_OBJECT_HEADER  objectHeader;

  if (handle == 0)
  {
    return  0;
  }
  objectHeader = UserObjectHandleToHeader (handle);
  if ((objectHeader->magic != magic) && 
      (magic != UO_MAGIC_DONTCARE))
  {
    return  0;
  }

  return  UserObjectHeaderToBody (objectHeader);
}

BOOL  USEROBJ_LockObject (HUSEROBJ  objectHandle)
{
  PUSER_OBJECT_HEADER  objectHeader;

  if (objectHandle == 0)
  {
    return  FALSE;
  }
  objectHeader = UserObjectHandleToHeader (objectHandle);

  ExAcquireFastMutexUnsafe (&objectHeader->mutex);

  return  TRUE;
}

BOOL  USEROBJ_UnlockObject (HUSEROBJ  objectHandle)
{
  PUSER_OBJECT_HEADER  objectHeader;

  if (objectHandle == 0)
  {
    return  FALSE;
  }
  objectHeader = UserObjectHandleToHeader (objectHandle);

  ExReleaseFastMutexUnsafe (&objectHeader->mutex);

  return  TRUE;
}



