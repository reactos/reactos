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
/* $Id: accel.c,v 1.3 2002/09/08 10:23:11 chorns Exp $
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/input.c
 * PURPOSE:         Input
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <user32.h>
#include <debug.h>

/* FUNCTIONS *****************************************************************/

int STDCALL
CopyAcceleratorTableA(HACCEL hAccelSrc,
		      LPACCEL lpAccelDst,
		      int cAccelEntries)
{
  return 0;
}

int STDCALL
CopyAcceleratorTableW(HACCEL hAccelSrc,
		      LPACCEL lpAccelDst,
		      int cAccelEntries)
{
  return 0;
}

HACCEL STDCALL
CreateAcceleratorTableA(LPACCEL lpaccl,
			int cEntries)
{
  return (HACCEL)0;
}

HACCEL STDCALL
CreateAcceleratorTableW(LPACCEL lpaccl,
			int cEntries)
{
  return (HACCEL)0;
}

WINBOOL STDCALL
DestroyAcceleratorTable(HACCEL hAccel)
{
  User32FreeHeap(hAccel);
  return(TRUE);
}

HACCEL STDCALL
LoadAcceleratorsA(HINSTANCE hInstance,
		  LPCSTR lpTableName)
{
  LPWSTR lpTableNameW;
  HACCEL Res;
  lpTableNameW = User32ConvertString(lpTableName);
  Res = LoadAcceleratorsW(hInstance, lpTableNameW);
  User32FreeString(lpTableName);
  return(Res);
}

HACCEL STDCALL
LoadAcceleratorsW(HINSTANCE hInstance,
		  LPCWSTR lpTableName)
{
  HRSRC Rsrc;
  HGLOBAL Mem;
  PVOID AccelTableRsrc;
  PVOID AccelTable;
  ULONG Size;

  Rsrc = FindResourceW(hInstance, lpTableName, RT_ACCELERATOR);
  if (Rsrc == NULL)
    {
      return(NULL);
    }
  else
    {
      Mem = LoadResource(hInstance, Rsrc);
      Size = SizeofResource(hInstance, Rsrc);
      AccelTableRsrc = LockResource(Mem);
      AccelTable = User32AllocHeap(Size);
      memcpy(AccelTable, AccelTableRsrc, Size);
      return((HACCEL)AccelTable);
    }
}

int STDCALL
TranslateAcceleratorA(HWND hWnd,
		      HACCEL hAccTable,
		      LPMSG lpMsg)
{
  return 0;
}

int STDCALL
TranslateAcceleratorW(HWND hWnd,
		      HACCEL hAccTable,
		      LPMSG lpMsg)
{
  return 0;
}
