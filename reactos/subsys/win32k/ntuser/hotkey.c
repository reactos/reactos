/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 2003 ReactOS Team
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
/* $Id: hotkey.c,v 1.1 2003/11/02 16:33:33 ekohl Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          HotKey support
 * FILE:             subsys/win32k/ntuser/hotkey.c
 * PROGRAMER:        Eric Kohl
 * REVISION HISTORY:
 *       02-11-2003  EK  Created
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <win32k/win32k.h>
#include <win32k/userobj.h>
#include <include/msgqueue.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/


/* FUNCTIONS *****************************************************************/


BOOL STDCALL
NtUserRegisterHotKey(HWND hWnd,
		     int id,
		     UINT fsModifiers,
		     UINT vk)
{
  UNIMPLEMENTED

  return FALSE;
}


BOOL STDCALL
NtUserUnregisterHotKey(HWND hWnd,
		       int id)
{
  UNIMPLEMENTED

  return FALSE;
}

/* EOF */
