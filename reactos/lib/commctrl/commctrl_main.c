/*
 * ReactOS-CE Common Controls
 *
 * Copyright 2004 Steven Edwards
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * TODO:
 * ATM this is just a dummy forwarding dll for the WinCE API to Win32 API
 * When you exit a simple WinCE app on Windows or on ReactOS it causes
 * a program error or unhandled exception probl'y because we dont handle
 * threading and process attaching/detaching like we should here.
 *
 * There will be some parts of the WinCE API that cant just be forwarded to
 * the Win32 API as paramater names or types may differ. In that case you will
 * need to copy the Win32 implementation and make the needed changes.
 */

#include <debug.h>
#include "windows.h"

BOOL STDCALL
DllMain(HANDLE hDll,
	DWORD dwReason,
	LPVOID lpReserved)
{
   return TRUE;
}

BOOL STDCALL 
_CommandBar_Create()
{
DPRINT1("CommandBar_Create called\n");
   return TRUE;
}

BOOL STDCALL 
_CommandBar_Show()
{
DPRINT1("CommandBar_Show called\n");
   return TRUE;
}

BOOL STDCALL
_CommandBar_InsertMenubar()
{
DPRINT1("CommandBar_InsertMenubar called\n");
   return TRUE;
}

BOOL STDCALL 
_CommandBar_AddAdornments()
{
DPRINT1("CommandBar_AddAdornments called\n");
   return TRUE;
}
