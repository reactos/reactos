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
/* $Id: prop.c,v 1.4 2002/09/08 10:23:12 chorns Exp $
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
EnumPropsA(HWND hWnd, PROPENUMPROC lpEnumFunc)
{
  return 0;
}

int STDCALL
EnumPropsExA(HWND hWnd, PROPENUMPROCEX lpEnumFunc, LPARAM lParam)
{
  return 0;
}

int STDCALL
EnumPropsExW(HWND hWnd, PROPENUMPROCEX lpEnumFunc, LPARAM lParam)
{
  return 0;
}

int STDCALL
EnumPropsW(HWND hWnd, PROPENUMPROC lpEnumFunc)
{
  return 0;
}

HANDLE STDCALL
GetPropA(HWND hWnd, LPCSTR lpString)
{
  PWSTR lpWString;
  HANDLE Ret;
  if (HIWORD(lpString))
    {
      lpWString = User32ConvertString(lpString);
      if (lpWString == NULL)
	{
	  return(FALSE);
	}
      Ret = GetPropW(hWnd, lpWString);
      User32FreeString(lpWString);
    }
  else
    {
      Ret = GetPropW(hWnd, lpString);
    }  
  return(Ret);
}

HANDLE STDCALL
GetPropW(HWND hWnd, LPCWSTR lpString)
{
  ATOM Atom;
  if (HIWORD(lpString))
    {
      Atom = GlobalFindAtomW(lpString);
    }
  else
    {
      Atom = LOWORD(lpString);
    }
  return(NtUserGetProp(hWnd, Atom));
}

HANDLE STDCALL
RemovePropA(HWND hWnd, LPCSTR lpString)
{
  PWSTR lpWString;
  HANDLE Ret;

  if (HIWORD(lpString))
    {
      lpWString = User32ConvertString(lpString);
      if (lpWString == NULL)
	{
	  return(FALSE);
	}
      Ret = RemovePropW(hWnd, lpWString);
      User32FreeString(lpWString);
    }
  else
    {
      Ret = RemovePropW(hWnd, lpWString);
    }
  return(Ret);
}

HANDLE STDCALL
RemovePropW(HWND hWnd,
	    LPCWSTR lpString)
{
  ATOM Atom;
  if (HIWORD(lpString))
    {
      Atom = GlobalFindAtomW(lpString);
    }
  else
    {
      Atom = LOWORD(lpString);
    }
  return(NtUserRemoveProp(hWnd, Atom));
}

WINBOOL STDCALL
SetPropA(HWND hWnd, LPCSTR lpString, HANDLE hData)
{
  PWSTR lpWString;
  BOOL Ret;
  
  if (HIWORD(lpString))
    {
      lpWString = User32ConvertString(lpString);
      if (lpWString == NULL)
	{
	  return(FALSE);
	}
      Ret = SetPropW(hWnd, lpWString, hData);
      User32FreeString(lpWString);
    }
  else
    {
      Ret = SetPropW(hWnd, lpString, hData);
    }
  return(Ret);
}

WINBOOL STDCALL
SetPropW(HWND hWnd, LPCWSTR lpString, HANDLE hData)
{
  ATOM Atom;
  if (HIWORD(lpString))
    {
      Atom = GlobalFindAtomW(lpString);
    }
  else
    {
      Atom = LOWORD(lpString);
    }
  
  return(NtUserSetProp(hWnd, Atom, hData));
}
