#ifndef __WIN32K_OBJECT_H
#define __WIN32K_OBJECT_H

#include <windows.h>
#include <win32k/gdiobj.h>
#include <win32k/bitmaps.h>
#include <win32k/pen.h>

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

#define HANDLE_BLOCK_ENTRIES ((PAGE_SIZE-sizeof(LIST_ENTRY))/sizeof(USER_HANDLE))

typedef struct _USER_HANDLE_BLOCK
{
  LIST_ENTRY ListEntry;
  USER_HANDLE Handles[HANDLE_BLOCK_ENTRIES];
} USER_HANDLE_BLOCK, *PUSER_HANDLE_BLOCK;

typedef struct _USER_HANDLE_TABLE
{
   LIST_ENTRY ListHead;
   FAST_MUTEX ListLock;
} USER_HANDLE_TABLE, *PUSER_HANDLE_TABLE;


ULONG FASTCALL
ObmGetReferenceCount(
  PVOID ObjectBody);

ULONG FASTCALL
ObmGetHandleCount(
  PVOID ObjectBody);

VOID FASTCALL
ObmReferenceObject(
  PVOID ObjectBody);

VOID FASTCALL
ObmDereferenceObject(
  PVOID ObjectBody);

NTSTATUS FASTCALL
ObmReferenceObjectByPointer(
  PVOID ObjectBody,
  USER_OBJECT_TYPE ObjectType);

PVOID FASTCALL
ObmCreateObject(
  PUSER_HANDLE_TABLE HandleTable,
  PHANDLE Handle,
	USER_OBJECT_TYPE ObjectType,
  ULONG ObjectSize);

NTSTATUS FASTCALL
ObmCreateHandle(
  PUSER_HANDLE_TABLE HandleTable,
  PVOID ObjectBody,
	PHANDLE HandleReturn);

NTSTATUS FASTCALL
ObmReferenceObjectByHandle(
  PUSER_HANDLE_TABLE HandleTable,
  HANDLE Handle,
	USER_OBJECT_TYPE ObjectType,
	PVOID* Object);

NTSTATUS FASTCALL
ObmCloseHandle(
  PUSER_HANDLE_TABLE HandleTable,
  HANDLE Handle);

VOID FASTCALL
ObmInitializeHandleTable(
  PUSER_HANDLE_TABLE HandleTable);

VOID FASTCALL
ObmFreeHandleTable(
  PUSER_HANDLE_TABLE HandleTable);

PUSER_HANDLE_TABLE FASTCALL
ObmCreateHandleTable(VOID);

VOID  FASTCALL ObmDestroyHandleTable (PUSER_HANDLE_TABLE HandleTable);

ULONG FASTCALL CreateGDIHandle (ULONG InternalSize, ULONG UserSize);
VOID  FASTCALL FreeGDIHandle (ULONG Handle);

PVOID FASTCALL AccessUserObject (ULONG Handle);
PVOID FASTCALL AccessInternalObject (ULONG Handle);

PVOID FASTCALL AccessInternalObjectFromUserObject (PVOID UserObject);
ULONG FASTCALL AccessHandleFromUserObject (PVOID UserObject);

VOID  FASTCALL InitEngHandleTable (VOID);
VOID  FASTCALL InitGdiObjectHandleTable (VOID);

VOID  FASTCALL CreateStockObjects (VOID);

BOOL  FASTCALL CleanupForProcess (struct _EPROCESS *Process, INT Pid);

VOID  FASTCALL GDIOBJ_MarkObjectGlobal (HGDIOBJ ObjectHandle);
BOOL  FASTCALL GDIOBJ_LockMultipleObj (PGDIMULTILOCK pList, INT nObj);

PPOINT FASTCALL GDI_Bezier (const POINT *Points, INT count, PINT nPtsOut);

/* objects/objconv.c */

PBRUSHOBJ FASTCALL PenToBrushObj (PDC dc, PENOBJ *pen);
HBITMAP FASTCALL BitmapToSurf (PBITMAPOBJ BitmapObj);

#endif /* __WIN32K_OBJECT_H */

/* EOF */
