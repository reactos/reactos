#ifndef _WIN32K_OBJECT_H
#define _WIN32K_OBJECT_H

/*
 * User Objects
 */

#define N_USER_HANDLES	0x4000

typedef enum {
  otUNKNOWN = 0,
  otWINDOW,
  otMENU,
  otACCEL,
  otCURSOR,
  otHOOK,
  otDWP
} USER_OBJECT_TYPE;

typedef struct _USER_OBJECT_HEADER *PUSER_OBJECT_HEADER;

typedef struct _USER_OBJECT_HEADER
{
  USER_OBJECT_TYPE Type;
  PUSER_OBJECT_HEADER *Slot;
  LONG RefCount;
} USER_OBJECT_HEADER;

typedef struct _USER_HANDLE_TABLE
{
  ULONG HandleCount;
  PUSER_OBJECT_HEADER Handles[N_USER_HANDLES];
} USER_HANDLE_TABLE, *PUSER_HANDLE_TABLE;

typedef BOOL (FASTCALL *PFNENUMHANDLESPROC)(PVOID ObjectBody, PVOID UserData);

VOID FASTCALL
ObmReferenceObject(
  PVOID ObjectBody);

VOID FASTCALL
ObmDereferenceObject(
  PVOID ObjectBody);

PVOID FASTCALL
ObmEnumHandles(
  PUSER_HANDLE_TABLE HandleTable,
  USER_OBJECT_TYPE ObjectType,
  PVOID UserData,
  PFNENUMHANDLESPROC EnumProc);

BOOL FASTCALL
ObmObjectDeleted(
  PVOID ObjectBody);

PVOID FASTCALL
ObmCreateObject(
  PUSER_HANDLE_TABLE HandleTable,
  PHANDLE Handle,
  USER_OBJECT_TYPE ObjectType,
  ULONG ObjectSize);

USER_OBJECT_TYPE FASTCALL
ObmGetObjectType(
  PUSER_HANDLE_TABLE HandleTable,
  PVOID ObjectBody);

BOOL FASTCALL
ObmDeleteObject(
  PUSER_HANDLE_TABLE HandleTable,
  PVOID ObjectBody);

PVOID FASTCALL
ObmGetObject(
  PUSER_HANDLE_TABLE HandleTable,
  HANDLE Handle,
  USER_OBJECT_TYPE ObjectType);

VOID FASTCALL
ObmInitializeHandleTable(
  PUSER_HANDLE_TABLE HandleTable);

VOID FASTCALL
ObmFreeHandleTable(
  PUSER_HANDLE_TABLE HandleTable);

PUSER_HANDLE_TABLE FASTCALL
ObmCreateHandleTable(VOID);

VOID FASTCALL
ObmDestroyHandleTable (PUSER_HANDLE_TABLE HandleTable);

#define IntGetUserObject(ObjectType, Handle) \
  ObmGetObject(PsGetWin32Process()->WindowStation->HandleTable, (Handle), ot##ObjectType )

/*
 * GDI
 */

VOID  FASTCALL InitGdiObjectHandleTable (VOID);

VOID  FASTCALL CreateStockObjects (VOID);
VOID  FASTCALL CreateSysColorObjects (VOID);

BOOL  FASTCALL CleanupForProcess (struct _EPROCESS *Process, INT Pid);

PPOINT FASTCALL GDI_Bezier (const POINT *Points, INT count, PINT nPtsOut);

#endif /* _WIN32K_OBJECT_H */

/* EOF */

