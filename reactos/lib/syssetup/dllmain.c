/*
 *  ReactOS kernel
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
/* $Id: dllmain.c,v 1.1 2003/05/02 18:07:55 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Main file
 * FILE:              lib/syssetup/dllmain.c
 * PROGRAMER:         Eric Kohl (ekohl@rz-online.de)
 */

/* INCLUDES *****************************************************************/

#include <windows.h>


/* GLOBALS *******************************************************************/


/* FUNCTIONS *****************************************************************/

BOOL STDCALL
DllMain (HINSTANCE hInstance,
	 DWORD dwReason,
	 LPVOID lpReserved)
{

  return TRUE;
}

/* EOF */
