/* $Id: win32err.c,v 1.1 1999/05/16 07:27:35 ea Exp $
 *
 * win32err.c
 *
 * Copyright (c) 1998 Mark Russinovich
 *	Systems Internals
 *	http://www.sysinternals.com/
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
 * Print a Win32 error.
 *
 * 1999 February (Emanuele Aliberti)
 * 	Taken from chkdskx.c and formatx.c by Mark Russinovich
 * 	to be used in all sysutils.
 */ 	
#include <windows.h>
#include <stdio.h>

//----------------------------------------------------------------------
//
// PrintWin32Error
//
// Takes the win32 error code and prints the text version.
//
//----------------------------------------------------------------------
void
PrintWin32Error(
	PWCHAR	Message,
	DWORD	ErrorCode
	)
{
	LPVOID lpMsgBuf;
 
	FormatMessageW(
		(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM),
		NULL,
		ErrorCode, 
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(PWCHAR) & lpMsgBuf,
		0,
		NULL
		);
	wprintf(
		L"%s: %s\n",
		Message,
		lpMsgBuf
		);
	LocalFree( lpMsgBuf );
}


/* EOF */
