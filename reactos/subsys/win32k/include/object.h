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

typedef BOOL (INTERNAL_CALL *PFNENUMHANDLESPROC)(PVOID ObjectBody, PVOID UserData);

VOID INTERNAL_CALL
ObmReferenceObject(
  PVOID ObjectBody);

VOID INTERNAL_CALL
ObmDereferenceObject(
  PVOID ObjectBody);

PVOID INTERNAL_CALL
ObmEnumHandles(
  PUSER_HANDLE_TABLE HandleTable,
  USER_OBJECT_TYPE ObjectType,
  PVOID UserData,
  PFNENUMHANDLESPROC EnumProc);

BOOL INTERNAL_CALL
ObmObjectDeleted(
  PVOID ObjectBody);

PVOID INTERNAL_CALL
ObmCreateObject(
  PUSER_HANDLE_TABLE HandleTable,
  PHANDLE Handle,
  USER_OBJECT_TYPE ObjectType,
  ULONG ObjectSize);

USER_OBJECT_TYPE INTERNAL_CALL
ObmGetObjectType(
  PUSER_HANDLE_TABLE HandleTable,
  PVOID ObjectBody);

BOOL INTERNAL_CALL
ObmDeleteObject(
  PUSER_HANDLE_TABLE HandleTable,
  PVOID ObjectBody);

PVOID INTERNAL_CALL
ObmGetObject(
  PUSER_HANDLE_TABLE HandleTable,
  HANDLE Handle,
  USER_OBJECT_TYPE ObjectType);

VOID INTERNAL_CALL
ObmInitializeHandleTable(
  PUSER_HANDLE_TABLE HandleTable);

VOID INTERNAL_CALL
ObmFreeHandleTable(
  PUSER_HANDLE_TABLE HandleTable);

PUSER_HANDLE_TABLE INTERNAL_CALL
ObmCreateHandleTable(VOID);

VOID INTERNAL_CALL
ObmDestroyHandleTable (PUSER_HANDLE_TABLE HandleTable);

#define IntGetUserObject(ObjectType, Handle) \
  ObmGetObject(PsGetWin32Process()->WindowStation->HandleTable, (Handle), ot##ObjectType )

/*
 * GDI
 */

VOID  INTERNAL_CALL InitGdiObjectHandleTable (VOID);

VOID  INTERNAL_CALL CreateStockObjects (VOID);
VOID  INTERNAL_CALL CreateSysColorObjects (VOID);

BOOL  INTERNAL_CALL CleanupForProcess (struct _EPROCESS *Process, INT Pid);

PPOINT INTERNAL_CALL GDI_Bezier (const POINT *Points, INT count, PINT nPtsOut);

#endif /* _WIN32K_OBJECT_H */

/* EOF */

