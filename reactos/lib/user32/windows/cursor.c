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
/* $Id: cursor.c,v 1.20 2004/04/09 20:03:14 navaraf Exp $
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/cursor.c
 * PURPOSE:         Cursor
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <user32.h>
#include <string.h>
#include <debug.h>
#define NTOS_MODE_USER
#include <ntos.h>
#undef CopyCursor

HBITMAP
CopyBitmap(HBITMAP bmp);

/* INTERNAL ******************************************************************/

/* This callback routine is called directly after switching to gui mode */
NTSTATUS STDCALL
User32SetupDefaultCursors(PVOID Arguments, ULONG ArgumentLength)
{
  BOOL *DefaultCursor = (BOOL*)Arguments;
  LRESULT Result = TRUE;
  
  if(*DefaultCursor)
  {
    /* set default cursor */
    SetCursor(LoadCursorW(0, (LPCWSTR)IDC_ARROW));
  }
  else
  {
    /* FIXME load system cursor scheme */
    SetCursor(0);
    SetCursor(LoadCursorW(0, (LPCWSTR)IDC_ARROW));
  }
  
  return(ZwCallbackReturn(&Result, sizeof(LRESULT), STATUS_SUCCESS));
}

/* FUNCTIONS *****************************************************************/


/*
 * @implemented
 */
HCURSOR STDCALL
CopyCursor(HCURSOR pcur)
{
  ICONINFO IconInfo;
  
  if(NtUserGetCursorIconInfo((HANDLE)pcur, &IconInfo))
  {
    return (HCURSOR)NtUserCreateCursorIconHandle(&IconInfo, FALSE);
  }
  return (HCURSOR)0;
}


/*
 * @implemented
 */
HCURSOR STDCALL
CreateCursor(HINSTANCE hInst,
	     int xHotSpot,
	     int yHotSpot,
	     int nWidth,
	     int nHeight,
	     CONST VOID *pvANDPlane,
	     CONST VOID *pvXORPlane)
{
  ICONINFO IconInfo;

  IconInfo.fIcon = FALSE;
  IconInfo.xHotspot = xHotSpot;
  IconInfo.yHotspot = yHotSpot;
  IconInfo.hbmMask = CreateBitmap(nWidth, nHeight, 1, 1, pvANDPlane);
  if(!IconInfo.hbmMask)
  {
    return (HICON)0;
  }
  IconInfo.hbmColor = CreateBitmap(nWidth, nHeight, 1, 1, pvXORPlane);
  if(!IconInfo.hbmColor)
  {
    DeleteObject(IconInfo.hbmMask);
    return (HICON)0;
  }
  
  return (HCURSOR)NtUserCreateCursorIconHandle(&IconInfo, FALSE);
}


/*
 * @implemented
 */
BOOL STDCALL
DestroyCursor(HCURSOR hCursor)
{
  return (BOOL)NtUserDestroyCursorIcon((HANDLE)hCursor, 0);
}


/*
 * @implemented
 */
BOOL STDCALL
GetClipCursor(LPRECT lpRect)
{
  return NtUserGetClipCursor(lpRect);
}


/*
 * @implemented
 */
HCURSOR STDCALL
GetCursor(VOID)
{
  CURSORINFO ci;
  ci.cbSize = sizeof(CURSORINFO);
  if(NtUserGetCursorInfo(&ci))
    return ci.hCursor;
  else
    return (HCURSOR)0;
}


/*
 * @implemented
 */
BOOL STDCALL
GetCursorInfo(PCURSORINFO pci)
{
  return (BOOL)NtUserGetCursorInfo(pci);
}


/*
 * @implemented
 */
BOOL STDCALL
GetCursorPos(LPPOINT lpPoint)
{
  BOOL res;
  /* Windows doesn't check if lpPoint == NULL, we do */
  if(!lpPoint)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  
  res = NtUserGetCursorPos(lpPoint);

  return res;
}


/*
 * @implemented
 */
HCURSOR STDCALL
LoadCursorA(HINSTANCE hInstance,
	    LPCSTR lpCursorName)
{
  return(LoadImageA(hInstance, lpCursorName, IMAGE_CURSOR, 0, 0,
         LR_SHARED | LR_DEFAULTSIZE));
}


/*
 * @implemented
 */
HCURSOR STDCALL
LoadCursorFromFileA(LPCSTR lpFileName)
{
  UNICODE_STRING FileName;
  HCURSOR Result;
  RtlCreateUnicodeStringFromAsciiz(&FileName, (LPSTR)lpFileName);
  Result = LoadImageW(0, FileName.Buffer, IMAGE_CURSOR, 0, 0, 
		      LR_LOADFROMFILE | LR_DEFAULTSIZE);
  RtlFreeUnicodeString(&FileName);
  return(Result);
}


/*
 * @implemented
 */
HCURSOR STDCALL
LoadCursorFromFileW(LPCWSTR lpFileName)
{
  return(LoadImageW(0, lpFileName, IMAGE_CURSOR, 0, 0, 
		    LR_LOADFROMFILE | LR_DEFAULTSIZE));
}


/*
 * @implemented
 */
HCURSOR STDCALL
LoadCursorW(HINSTANCE hInstance,
	    LPCWSTR lpCursorName)
{
  return(LoadImageW(hInstance, lpCursorName, IMAGE_CURSOR, 0, 0,
		 LR_SHARED | LR_DEFAULTSIZE));
}


/*
 * @implemented
 */
BOOL
STDCALL
ClipCursor(
  CONST RECT *lpRect)
{
  return NtUserClipCursor((RECT *)lpRect);
}


/*
 * @implemented
 */
HCURSOR STDCALL
SetCursor(HCURSOR hCursor)
{
  return NtUserSetCursor(hCursor);
}


/*
 * @implemented
 */
BOOL STDCALL
SetCursorPos(int X,
	     int Y)
{
  POINT pos;
  pos.x = (LONG)X;
  pos.y = (LONG)Y;
  return NtUserSetCursorPos(&pos);
}


/*
 * @unimplemented
 */
BOOL STDCALL
SetSystemCursor(HCURSOR hcur,
		DWORD id)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
int STDCALL
ShowCursor(BOOL bShow)
{
  UNIMPLEMENTED;
  return 0;
}

HCURSOR
CursorIconToCursor(HICON hIcon, BOOL SemiTransparent)
{
  UNIMPLEMENTED;
  return 0;
}
