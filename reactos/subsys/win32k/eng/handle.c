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
/* $Id: handle.c,v 1.13 2003/05/18 17:16:17 ea Exp $
 * 
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

static int LastHandle = MAX_GDI_HANDLES;

ULONG FASTCALL CreateGDIHandle(ULONG InternalSize, ULONG UserSize)
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

  for( i = (MAX_GDI_HANDLES - 1 <= LastHandle ? 1 : LastHandle + 1); i != LastHandle;
       i = (MAX_GDI_HANDLES - 1 <= i ? 1 : i + 1) ){
	if( GDIHandles[ i ].pEngObj == NULL ){
		pObj->hObj = i;
		GDIHandles[ i ].pEngObj = pObj;
		DPRINT("CreateGDIHandle: obj: %x, handle: %d, usersize: %d\n", pObj, i, UserSize );
		LastHandle = i;
		return i;
	}
  }
  DPRINT1("CreateGDIHandle: Out of available handles!!!\n");
  EngFreeMem( pObj );
  return 0;
}

VOID FASTCALL FreeGDIHandle(ULONG Handle)
{
  if( Handle == 0 || Handle >= MAX_GDI_HANDLES ){
	DPRINT1("FreeGDIHandle: invalid handle!!!!\n");
	return;
  }
  DPRINT("FreeGDIHandle: handle: %d\n", Handle);
  EngFreeMem( GDIHandles[Handle].pEngObj );
  GDIHandles[Handle].pEngObj = NULL;
}

PVOID FASTCALL AccessInternalObject(ULONG Handle)
{
  PENGOBJ pEngObj;

  if( Handle == 0 || Handle >= MAX_GDI_HANDLES ){
	DPRINT1("AccessInternalObject: invalid handle: %d!!!!\n", Handle);
	return NULL;
  }

  pEngObj = GDIHandles[Handle].pEngObj;
  return (PVOID)pEngObj;
}

PVOID FASTCALL AccessUserObject(ULONG Handle)
{
  PENGOBJ pEngObj;

  if( Handle == 0 || Handle >= MAX_GDI_HANDLES ){
	DPRINT1("AccessUserObject: invalid handle: %d!!!!\n", Handle);
	return NULL;
  }

  pEngObj = GDIHandles[Handle].pEngObj;
  return (PVOID)( (PCHAR)pEngObj + sizeof( ENGOBJ ) );
}

ULONG FASTCALL AccessHandleFromUserObject(PVOID UserObject)
{
  PENGOBJ pEngObj;
  ULONG Handle;

  if( !UserObject )
	return INVALID_HANDLE;

  pEngObj = (PENGOBJ)((PCHAR) UserObject - sizeof( ENGOBJ ));
  Handle = pEngObj->hObj;

  if( Handle == 0 || Handle >= MAX_GDI_HANDLES ){
	DPRINT1("AccessHandleFromUserObject: inv handle: %d, obj: %x!!!!\n", Handle, pEngObj);
  	return INVALID_HANDLE;
  }
  return Handle;
}

PVOID FASTCALL AccessInternalObjectFromUserObject(PVOID UserObject)
{

  return AccessInternalObject( AccessHandleFromUserObject( UserObject ) );
}

VOID FASTCALL InitEngHandleTable( void )
{
	ULONG i;
  	for( i=1; i < MAX_GDI_HANDLES; i++ ){
		GDIHandles[ i ].pEngObj = NULL;
	}
}
/* EOF */
