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

#define NDEBUG
#include <win32k/debug1.h>


ULONG CreateGDIHandle(ULONG InternalSize, ULONG UserSize)
{
  ULONG size;
  PENGOBJ pObj;
  int i;

  //size = sizeof( ENGOBJ ) + InternalSize + UserSize;
  size = InternalSize;      	//internal size includes header and user portions
  pObj = EngAllocMem( FL_ZERO_MEMORY, size, 0 );

  if( !pObj )
	return 0;

  pObj->InternalSize = InternalSize;
  pObj->UserSize = UserSize;

  for( i=1; i < MAX_GDI_HANDLES; i++ ){
	if( GDIHandles[ i ].pEngObj == NULL ){
		pObj->hObj = i;
		GDIHandles[ i ].pEngObj = pObj;
		DPRINT("CreateGDIHandle: obj: %x, handle: %d, usersize: %d\n", pObj, i, UserSize );
		return i;
	}
  }
  DbgPrint("CreateGDIHandle: Out of available handles!!!\n");
  EngFreeMem( pObj );
  return 0;
}

VOID FreeGDIHandle(ULONG Handle)
{
  if( Handle == 0 || Handle >= MAX_GDI_HANDLES ){
	DbgPrint("FreeGDIHandle: invalid handle!!!!\n");
	return;
  }
  DPRINT("FreeGDIHandle: handle: %d\n", Handle);
  EngFreeMem( GDIHandles[Handle].pEngObj );
  GDIHandles[Handle].pEngObj = NULL;
}

PVOID AccessInternalObject(ULONG Handle)
{
  PENGOBJ pEngObj;

  if( Handle == 0 || Handle >= MAX_GDI_HANDLES ){
	DbgPrint("AccessInternalObject: invalid handle: %d!!!!\n", Handle);
	return NULL;
  }

  pEngObj = GDIHandles[Handle].pEngObj;
  return (PVOID)pEngObj;
}

PVOID AccessUserObject(ULONG Handle)
{
  PENGOBJ pEngObj;

  if( Handle == 0 || Handle >= MAX_GDI_HANDLES ){
	DbgPrint("AccessUserObject: invalid handle: %d!!!!\n", Handle);
	return NULL;
  }

  pEngObj = GDIHandles[Handle].pEngObj;
  return (PVOID)( (PCHAR)pEngObj + sizeof( ENGOBJ ) );
}

ULONG AccessHandleFromUserObject(PVOID UserObject)
{
  PENGOBJ pEngObj;
  ULONG Handle;

  if( !UserObject )
	return INVALID_HANDLE;

  pEngObj = (PENGOBJ)((PCHAR) UserObject - sizeof( ENGOBJ ));
  Handle = pEngObj->hObj;

  if( Handle == 0 || Handle >= MAX_GDI_HANDLES ){
	DbgPrint("AccessHandleFromUserObject: inv handle: %d, obj: %x!!!!\n", Handle, pEngObj);
  	return INVALID_HANDLE;
  }
  return Handle;
}

PVOID AccessInternalObjectFromUserObject(PVOID UserObject)
{

  return AccessInternalObject( AccessHandleFromUserObject( UserObject ) );
}

VOID InitEngHandleTable( void )
{
	ULONG i;
  	for( i=1; i < MAX_GDI_HANDLES; i++ ){
		GDIHandles[ i ].pEngObj = NULL;
	}
}
