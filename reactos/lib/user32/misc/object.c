/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
/* $Id: object.c,v 1.6 2004/01/23 23:38:26 ekohl Exp $
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/misc/dde.c
 * PURPOSE:         DDE
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <user32.h>
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
BOOL
STDCALL
SetUserObjectInformationA(
  HANDLE hObj,
  int nIndex,
  PVOID pvInfo,
  DWORD nLength)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
SetUserObjectInformationW(
  HANDLE hObj,
  int nIndex,
  PVOID pvInfo,
  DWORD nLength)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
UserHandleGrantAccess(
  HANDLE hUserHandle,
  HANDLE hJob,
  BOOL bGrant)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
GetUserObjectInformationA(
  HANDLE hObj,
  int nIndex,
  PVOID pvInfo,
  DWORD nLength,
  LPDWORD lpnLengthNeeded)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
GetUserObjectInformationW(
  HANDLE hObj,
  int nIndex,
  PVOID pvInfo,
  DWORD nLength,
  LPDWORD lpnLengthNeeded)
{
  UNIMPLEMENTED;
  return FALSE;
}
