/* $Id: help.c,v 1.1 1999/06/06 15:34:09 ea Exp $
 *
 * reactos/lib/user32/windows/help.c
 *
 * Copyright (c) 1999 Emanuele Aliberti
 *
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this software; see the file COPYING.LIB. If
 * not, write to the Free Software Foundation, Inc., 675 Mass Ave,
 * Cambridge, MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 *
 * ReactOS user32.dll window context help functions.
 *
 */
#include <windows.h>

DWORD 
STDCALL
GetMenuContextHelpId (
	HMENU	hmenu 	 
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


BOOL
STDCALL
SetMenuContextHelpId (
	HMENU	hmenu, 	
	DWORD	dwContextHelpId 	
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

DWORD
STDCALL
GetWindowContextHelpId (
	HWND	hwnd	 
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


BOOL
STDCALL
SetWindowContextHelpId (
	HWND	hwnd,	 
	DWORD	dwContextHelpId 	
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/* EOF */
