#ifndef __WIN32K_OBJECT_H
#define __WIN32K_OBJECT_H

#include <windows.h>

typedef enum {
  otUnknown = 0,
  otClass,
  otWindow
} USER_OBJECT_TYPE;

typedef struct _USER_OBJECT_HEADER
/*
 * Header for user object
 */
{
  USER_OBJECT_TYPE Type;
  LONG HandleCount;
  LONG RefCount;
  CSHORT Size;
} USER_OBJECT_HEADER, *PUSER_OBJECT_HEADER;


typedef struct _USER_HANDLE
{
  PVOID ObjectBody;
} USER_HANDLE, *PUSER_HANDLE;

#define HANDLE_BLOCK_ENTRIES ((PAGESIZE-sizeof(LIST_ENTRY))/sizeof(USER_HANDLE))

typedef struct _USER_HANDLE_BLOCK
{
  LIST_ENTRY ListEntry;
  USER_HANDLE Handles[HANDLE_BLOCK_ENTRIES];
} USER_HANDLE_BLOCK, *PUSER_HANDLE_BLOCK;

typedef struct _USER_HANDLE_TABLE
{
   LIST_ENTRY ListHead;
   PFAST_MUTEX ListLock;
} USER_HANDLE_TABLE, *PUSER_HANDLE_TABLE;


ULONG
ObmGetReferenceCount(
  PVOID ObjectBody);

ULONG
ObmGetHandleCount(
  PVOID ObjectBody);

VOID
ObmReferenceObject(
  PVOID ObjectBody);

VOID
ObmDereferenceObject(
  PVOID ObjectBody);

NTSTATUS
ObmReferenceObjectByPointer(
  PVOID ObjectBody,
  USER_OBJECT_TYPE ObjectType);

PVOID
ObmCreateObject(
  PUSER_HANDLE_TABLE HandleTable,
  PHANDLE Handle,
	USER_OBJECT_TYPE ObjectType,
  ULONG ObjectSize);

NTSTATUS
ObmCreateHandle(
  PUSER_HANDLE_TABLE HandleTable,
  PVOID ObjectBody,
	PHANDLE HandleReturn);

NTSTATUS
ObmReferenceObjectByHandle(
  PUSER_HANDLE_TABLE HandleTable,
  HANDLE Handle,
	USER_OBJECT_TYPE ObjectType,
	PVOID* Object);

NTSTATUS
ObmCloseHandle(
  PUSER_HANDLE_TABLE HandleTable,
  HANDLE Handle);

VOID
ObmInitializeHandleTable(
  PUSER_HANDLE_TABLE HandleTable);

VOID
ObmFreeHandleTable(
  PUSER_HANDLE_TABLE HandleTable);

PUSER_HANDLE_TABLE
ObmCreateHandleTable(VOID);

VOID
ObmDestroyHandleTable(
  PUSER_HANDLE_TABLE HandleTable);

PVOID AccessInternalObjectFromUserObject(PVOID UserObject);
PVOID AccessUserObject(ULONG Handle);
ULONG CreateGDIHandle(PVOID InternalObject, PVOID UserObject);
VOID FreeGDIHandle(ULONG Handle);
ULONG AccessHandleFromUserObject(PVOID UserObject);
PVOID AccessInternalObject(ULONG Handle);

#endif /* __WIN32K_OBJECT_H */

/* EOF */
