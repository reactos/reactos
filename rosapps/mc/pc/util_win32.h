/* util.Win32.h - Header
   Utilities - Win32 utilities (Windows NT and Windows '95)
   Copyright (C) 1994, 1995, 1996 the Free Software Foundation.

   Written 1996 by Juan Grigera<grigera@isis.unlp.edu.ar>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  
*/

/* Prototypes */
int win32_GetPlatform ();
int win32_GetEXEType (const char* a_szFileName);
int win32_GetVersionEx ();


/* Constants */
enum {
	OS_WinNT = VER_PLATFORM_WIN32_NT,				/* windows.h values */
	OS_Win95 = VER_PLATFORM_WIN32_WINDOWS,
};

enum {
	EXE_Unknown,
	EXE_win16,
	EXE_win32CUI,
	EXE_win32GUI,
	EXE_otherCUI,
	EXE_Error
};

