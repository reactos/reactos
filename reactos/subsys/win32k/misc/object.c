/* $Id: object.c,v 1.1 2001/06/12 17:51:51 chorns Exp $
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
#include <ddk/ntddk.h>
#include <include/object.h>

#define NDEBUG
#include <debug.h>

PVOID
HEADER_TO_BODY(
  PUSER_OBJECT_HEADER ObjectHeader)
{
   return (((PUSER_OBJECT_HEADER)ObjectHeader) + 1);
}

PUSER_OBJECT_HEADER BODY_TO_HEADER(
  PVOID ObjectBody)
{
  return (((PUSER_OBJECT_HEADER)ObjectBody) - 1);
}

static VOID
ObmpLockHandleTable(
  PUSER_HANDLE_TABLE HandleTable)
{
//   ExAcquireFastMutex(HandleTable->ListLock);
}

static VOID
ObmpUnlockHandleTable(
  PUSER_HANDLE_TABLE HandleTable)
{
//   ExReleaseFastMutex(AtomTable->ListLock);
}

VOID
ObmpPerformRetentionChecks(
  PUSER_OBJECT_HEADER ObjectHeader)
{
  if (ObjectHeader->RefCount < 0)
  {
    DbgPrint("ObjectHeader 0x%X has invalid reference count (%d)\n",
		  ObjectHeader, ObjectHeader->RefCount);
  }

  if (ObjectHeader->HandleCount < 0)
  {
    DbgPrint("Object 0x%X has invalid handle count (%d)\n",
      ObjectHeader, ObjectHeader->HandleCount);
  }

  if ((ObjectHeader->RefCount == 0) && (ObjectHeader->HandleCount == 0))
  {
    ExFreePool(ObjectHeader);
  }
}

PUSER_HANDLE
ObmpGetObjectByHandle(
  PUSER_HANDLE_TABLE HandleTable,
  HANDLE Handle)
/*
 * FUNCTION: Get the data structure for a handle
 * ARGUMENTS:
 *   HandleTable = Table to search
 *   Handle      = Handle to get data structure for
 * RETURNS:
 *   Pointer to the data structure identified by the handle on success,
 *   NULL on failure
 */
{
  ULONG Count = ((ULONG)Handle / HANDLE_BLOCK_ENTRIES);
  ULONG Index = (((ULONG)Handle) - 1) >> 2;
  PUSER_HANDLE_BLOCK Block = NULL;
  PLIST_ENTRY Current;
  ULONG i;

  Current = HandleTable->ListHead.Flink;

  for (i = 0; i < Count; i++)
  {
    Current = Current->Flink;
    if (Current == &(HandleTable->ListHead))
    {
      return NULL;
    }
  }

  Block = CONTAINING_RECORD(Current, USER_HANDLE_BLOCK, ListEntry);
  return &(Block->Handles[Index % HANDLE_BLOCK_ENTRIES]);
}

VOID
ObmpCloseAllHandles(
  PUSER_HANDLE_TABLE HandleTable)
{
  PLIST_ENTRY CurrentEntry;
  PUSER_HANDLE_BLOCK Current;
  PVOID ObjectBody;
  ULONG i;

  ObmpLockHandleTable(HandleTable);

  CurrentEntry = HandleTable->ListHead.Flink;

  while (CurrentEntry != &HandleTable->ListHead)
  {
    Current = CONTAINING_RECORD(CurrentEntry, USER_HANDLE_BLOCK, ListEntry);

    for (i = 0; i < HANDLE_BLOCK_ENTRIES; i++)
    {
      ObjectBody = Current->Handles[i].ObjectBody;

      if (ObjectBody != NULL)
      {
        PUSER_OBJECT_HEADER ObjectHeader = BODY_TO_HEADER(ObjectBody);

        ObmReferenceObjectByPointer(ObjectBody, otUnknown);
        ObjectHeader->HandleCount--;
		    Current->Handles[i].ObjectBody = NULL;

		    ObmpUnlockHandleTable(HandleTable);

        ObmDereferenceObject(ObjectBody);

        ObmpLockHandleTable(HandleTable);
		    CurrentEntry = &HandleTable->ListHead;
		    break;
      }
    }

    CurrentEntry = CurrentEntry->Flink;
  }

  ObmpUnlockHandleTable(HandleTable);
}

VOID
ObmpDeleteHandleTable(
  PUSER_HANDLE_TABLE HandleTable)
{
  PUSER_HANDLE_BLOCK Current;
  PLIST_ENTRY CurrentEntry;

  ObmpCloseAllHandles(HandleTable);

  CurrentEntry = RemoveHeadList(&HandleTable->ListHead);

  while (CurrentEntry != &HandleTable->ListHead)
  {
    Current = CONTAINING_RECORD(CurrentEntry,
      USER_HANDLE_BLOCK,
    	ListEntry);

    ExFreePool(Current);

	  CurrentEntry = RemoveHeadList(&HandleTable->ListHead);
  }
}

PVOID
ObmpDeleteHandle(
  PUSER_HANDLE_TABLE HandleTable,
  HANDLE Handle)
{
  PUSER_OBJECT_HEADER ObjectHeader;
  PUSER_HANDLE Entry;
  PVOID ObjectBody;

  ObmpLockHandleTable(HandleTable);

  Entry = ObmpGetObjectByHandle(HandleTable, Handle);
  if (Entry == NULL)
  {
    ObmpUnlockHandleTable(HandleTable);
    return NULL;
  }

  ObjectBody = Entry->ObjectBody;

  if (ObjectBody != NULL)
  {
    ObjectHeader = BODY_TO_HEADER(ObjectBody);
	  ObjectHeader->HandleCount--;
	  ObmReferenceObjectByPointer(ObjectBody, otUnknown);
    Entry->ObjectBody = NULL;
  }

  ObmpUnlockHandleTable(HandleTable);

  return ObjectBody;
}

NTSTATUS
ObmpInitializeObject(
  PUSER_HANDLE_TABLE HandleTable,
  PUSER_OBJECT_HEADER ObjectHeader,
  PHANDLE Handle,
  USER_OBJECT_TYPE ObjectType,
  ULONG ObjectSize)
{
  DWORD Status = STATUS_SUCCESS;

  ObjectHeader->Type = ObjectType;
  ObjectHeader->HandleCount = 0;
  ObjectHeader->RefCount = 1;
  ObjectHeader->Size = ObjectSize;

  if (Handle != NULL)
  {
    Status = ObmCreateHandle(
      HandleTable,
      HEADER_TO_BODY(ObjectHeader),
	    Handle);
  }

  return Status;
}


ULONG
ObmGetReferenceCount(
  PVOID ObjectBody)
{
  PUSER_OBJECT_HEADER ObjectHeader = BODY_TO_HEADER(ObjectBody);

  return ObjectHeader->RefCount;
}

ULONG
ObmGetHandleCount(
  PVOID ObjectBody)
{
  PUSER_OBJECT_HEADER ObjectHeader = BODY_TO_HEADER(ObjectBody);

  return ObjectHeader->HandleCount;
}

VOID
ObmReferenceObject(
  PVOID ObjectBody)
/*
 * FUNCTION: Increments a given object's reference count and performs
 *           retention checks
 * ARGUMENTS:
 *   ObjectBody = Body of the object
 */
{
  PUSER_OBJECT_HEADER ObjectHeader;

  if (!ObjectBody)
  {
    return;
  }

  ObjectHeader = BODY_TO_HEADER(ObjectBody);

  ObjectHeader->RefCount++;

  ObmpPerformRetentionChecks(ObjectHeader);
}

VOID
ObmDereferenceObject(
  PVOID ObjectBody)
/*
 * FUNCTION: Decrements a given object's reference count and performs
 *           retention checks
 * ARGUMENTS:
 *   ObjectBody = Body of the object
 */
{
  PUSER_OBJECT_HEADER ObjectHeader;

  if (!ObjectBody)
  {
    return;
  }

  ObjectHeader = BODY_TO_HEADER(ObjectBody);

  ObjectHeader->RefCount--;

  ObmpPerformRetentionChecks(ObjectHeader);
}

NTSTATUS
ObmReferenceObjectByPointer(
  PVOID ObjectBody,
  USER_OBJECT_TYPE ObjectType)
/*
 * FUNCTION: Increments the pointer reference count for a given object
 * ARGUMENTS:
 *         ObjectBody = Object's body
 *         ObjectType = Object type
 * RETURNS: Status
 */
{
  PUSER_OBJECT_HEADER ObjectHeader;

  ObjectHeader = BODY_TO_HEADER(ObjectBody);

  if ((ObjectType != otUnknown) && (ObjectHeader->Type != ObjectType))
  {
    return STATUS_INVALID_PARAMETER;
  }

  ObjectHeader->RefCount++;

  return STATUS_SUCCESS;
}

PVOID
ObmCreateObject(
  PUSER_HANDLE_TABLE HandleTable,
  PHANDLE Handle,
	USER_OBJECT_TYPE ObjectType,
  ULONG ObjectSize)
{
  PUSER_OBJECT_HEADER ObjectHeader;
  PVOID ObjectBody;
  DWORD Status;

  ObjectHeader = (PUSER_OBJECT_HEADER)ExAllocatePool(
    NonPagedPool, ObjectSize + sizeof(USER_OBJECT_HEADER));
  if (!ObjectHeader)
  {
    return NULL;
  }

  ObjectBody = HEADER_TO_BODY(ObjectHeader);

  RtlZeroMemory(ObjectBody, ObjectSize);

  Status = ObmpInitializeObject(
    HandleTable,
    ObjectHeader,
    Handle,
		ObjectType,
    ObjectSize);

  if (!NT_SUCCESS(Status))
  {
    ExFreePool(ObjectHeader);
    return NULL;
  }

  return ObjectBody;
}

NTSTATUS
ObmCreateHandle(
  PUSER_HANDLE_TABLE HandleTable,
  PVOID ObjectBody,
	PHANDLE HandleReturn)
/*
 * FUNCTION: Add a handle referencing an object
 * ARGUMENTS:
 *   HandleTable = Table to put handle in
 *   ObjectBody  = Object body that the handle should refer to
 * RETURNS: The created handle
 */
{
  PUSER_HANDLE_BLOCK NewBlock;
  PLIST_ENTRY Current;
  ULONG Handle;
  ULONG i;

  if (ObjectBody != NULL) {
    BODY_TO_HEADER(ObjectBody)->HandleCount++;
  }

  ObmpLockHandleTable(HandleTable);

  Current = HandleTable->ListHead.Flink;
  /*
   * Scan through the currently allocated Handle blocks looking for a free
   * slot
   */
  while (Current != &(HandleTable->ListHead))
  {
    PUSER_HANDLE_BLOCK Block = CONTAINING_RECORD(
      Current, USER_HANDLE_BLOCK, ListEntry);

    Handle = 1;
    for (i = 0; i < HANDLE_BLOCK_ENTRIES; i++)
    {
      if (!Block->Handles[i].ObjectBody)
      {
        Block->Handles[i].ObjectBody = ObjectBody;
        ObmpUnlockHandleTable(HandleTable);
        *HandleReturn = (HANDLE)((Handle + i) << 2);
        return ERROR_SUCCESS;
      }
    }

    Handle = Handle + HANDLE_BLOCK_ENTRIES;
    Current = Current->Flink;
  }

  /*
   * Add a new Handle block to the end of the list
   */
  NewBlock = (PUSER_HANDLE_BLOCK)ExAllocatePool(
    NonPagedPool, sizeof(USER_HANDLE_BLOCK));
  if (!NewBlock)
  {
    *HandleReturn = (PHANDLE)NULL;
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  RtlZeroMemory(NewBlock, sizeof(USER_HANDLE_BLOCK));
  NewBlock->Handles[0].ObjectBody = ObjectBody;
  InsertTailList(&HandleTable->ListHead, &NewBlock->ListEntry);
  ObmpUnlockHandleTable(HandleTable);
  *HandleReturn = (HANDLE)(Handle << 2);

  return STATUS_SUCCESS;
}

NTSTATUS
ObmReferenceObjectByHandle(
  PUSER_HANDLE_TABLE HandleTable,
  HANDLE Handle,
	USER_OBJECT_TYPE ObjectType,
	PVOID* Object)
/*
 * FUNCTION: Increments the reference count for an object and returns a
 *           pointer to its body
 * ARGUMENTS:
 *         HandleTable = Table to search
 *         Handle = Handle for the object
 *         ObjectType = Type of object
 *         Object (OUT) = Points to the object body on return
 * RETURNS: Status
 */
{
  PUSER_OBJECT_HEADER ObjectHeader;
  PUSER_HANDLE UserHandle;
  PVOID ObjectBody;

  ObmpLockHandleTable(HandleTable);

  UserHandle = ObmpGetObjectByHandle(HandleTable, Handle);

  if ((UserHandle == NULL) || (UserHandle->ObjectBody == NULL))
  {
    ObmpUnlockHandleTable(HandleTable);
    return STATUS_UNSUCCESSFUL;
  }

  ObjectBody = UserHandle->ObjectBody;
  ObmReferenceObjectByPointer(ObjectBody, ObjectType);

  ObmpUnlockHandleTable(HandleTable);

  ObjectHeader = BODY_TO_HEADER(ObjectBody);

  if ((ObjectType != otUnknown) && (ObjectHeader->Type != ObjectType))
  {
	  return STATUS_UNSUCCESSFUL;
  }

  *Object = ObjectBody;

  return STATUS_SUCCESS;
}

NTSTATUS
ObmCloseHandle(
  PUSER_HANDLE_TABLE HandleTable,
  HANDLE Handle)
{
  PVOID ObjectBody;

  ObjectBody = ObmpDeleteHandle(HandleTable, Handle);
  if (ObjectBody == NULL)
  {
    return STATUS_UNSUCCESSFUL;
  }

  ObmDereferenceObject(ObjectBody);

  return STATUS_SUCCESS;
}


VOID
ObmInitializeHandleTable(
  PUSER_HANDLE_TABLE HandleTable)
{
  InitializeListHead(&HandleTable->ListHead);
  //ExInitializeFastMutex(HandleTable->ListLock);
}

VOID
ObmFreeHandleTable(
  PUSER_HANDLE_TABLE HandleTable)
{
  ObmpDeleteHandleTable(HandleTable);
}

PUSER_HANDLE_TABLE
ObmCreateHandleTable(VOID)
{
  PUSER_HANDLE_TABLE HandleTable;

  HandleTable = (PUSER_HANDLE_TABLE)ExAllocatePool(
    NonPagedPool, sizeof(USER_HANDLE_TABLE));
  if (!HandleTable)
  {
    return NULL;
  }

  ObmInitializeHandleTable(HandleTable);

  return HandleTable;
}

VOID
ObmDestroyHandleTable(
  PUSER_HANDLE_TABLE HandleTable)
{
  ObmFreeHandleTable(HandleTable);
  ExFreePool(HandleTable);
}

/* EOF */
