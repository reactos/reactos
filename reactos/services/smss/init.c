/* $Id: init.c,v 1.1 1999/05/30 20:40:18 ea Exp $
 *
 * smss.c - Session Manager
 * 
 * ReactOS Operating System
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
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.LIB. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 * 
 * 	19990530 (Emanuele Aliberti)
 * 		Compiled successfully with egcs 1.1.2
 */
#include <ddk/ntddk.h>
//#include <internal/lpc.h>

BOOL
InitSessionManager(
	HANDLE	Children[]
	)
{
	/* FIXME: Create the \SmApiPort object (LPC) */
	/* FIXME: Create two thread for \SmApiPort */
	/* FIXME: Create the system environment variables */
	/* FIXME: Define symbolic links to kernel devices (MS-DOS names) */
	/* FIXME: Create pagination files (if any) other than the first one */
	/* FIXME: Load the well known DLLs */
	/* FIXME: Load the kernel mode driver win32k.sys */
	/* FIXME: Start the Win32 subsystem (csrss.exe) */
	Children[0] = INVALID_HANDLE_VALUE;
	/* FIXME: Start winlogon.exe */
	Children[1] = INVALID_HANDLE_VALUE;
	/* FIXME: Create the \DbgSsApiPort object (LPC) */
	/* FIXME: Create the \DbgUiApiPort object (LPC) */
	return FALSE;
}


/* EOF */

