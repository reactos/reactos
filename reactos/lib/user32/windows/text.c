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
/* $Id: text.c,v 1.3 2002/09/08 10:23:12 chorns Exp $
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

LPSTR
STDCALL
CharLowerA(
  LPSTR lpsz)
{
  return (LPSTR)NULL;
}

DWORD
STDCALL
CharLowerBuffA(
  LPSTR lpsz,
  DWORD cchLength)
{
  return 0;
}

DWORD
STDCALL
CharLowerBuffW(
  LPWSTR lpsz,
  DWORD cchLength)
{
  return 0;
}

LPWSTR
STDCALL
CharLowerW(
  LPWSTR lpsz)
{
  return (LPWSTR)NULL;
}

LPSTR
STDCALL
CharNextA(
  LPCSTR lpsz)
{
  return (LPSTR)NULL;
}

LPSTR
STDCALL
CharNextExA(
  WORD CodePage,
  LPCSTR lpCurrentChar,
  DWORD dwFlags)
{
}

LPWSTR
STDCALL
CharNextW(
  LPCWSTR lpsz)
{
  return (LPWSTR)NULL;
}

LPSTR
STDCALL
CharPrevA(
  LPCSTR lpszStart,
  LPCSTR lpszCurrent)
{
  return (LPSTR)NULL;
}

LPWSTR
STDCALL
CharPrevW(
  LPCWSTR lpszStart,
  LPCWSTR lpszCurrent)
{
  return (LPWSTR)NULL;
}

LPSTR
STDCALL
CharPrevExA(
  WORD CodePage,
  LPCSTR lpStart,
  LPCSTR lpCurrentChar,
  DWORD dwFlags)
{
  return (LPSTR)NULL;
}

WINBOOL
STDCALL
CharToOemA(
  LPCSTR lpszSrc,
  LPSTR lpszDst)
{
  return FALSE;
}

WINBOOL
STDCALL
CharToOemBuffA(
  LPCSTR lpszSrc,
  LPSTR lpszDst,
  DWORD cchDstLength)
{
  return FALSE;
}

WINBOOL
STDCALL
CharToOemBuffW(
  LPCWSTR lpszSrc,
  LPSTR lpszDst,
  DWORD cchDstLength)
{
  return FALSE;
}

WINBOOL
STDCALL
CharToOemW(
  LPCWSTR lpszSrc,
  LPSTR lpszDst)
{
  return FALSE;
}

LPSTR
STDCALL
CharUpperA(
  LPSTR lpsz)
{
  return (LPSTR)NULL;
}

DWORD
STDCALL
CharUpperBuffA(
  LPSTR lpsz,
  DWORD cchLength)
{
  return 0;
}

DWORD
STDCALL
CharUpperBuffW(
  LPWSTR lpsz,
  DWORD cchLength)
{
  return 0;
}

LPWSTR
STDCALL
CharUpperW(
  LPWSTR lpsz)
{
  return (LPWSTR)NULL;
}
WINBOOL
STDCALL
IsCharAlphaA(
  CHAR ch)
{
  return FALSE;
}

WINBOOL
STDCALL
IsCharAlphaNumericA(
  CHAR ch)
{
  return FALSE;
}

WINBOOL
STDCALL
IsCharAlphaNumericW(
  WCHAR ch)
{
  return FALSE;
}

WINBOOL
STDCALL
IsCharAlphaW(
  WCHAR ch)
{
  return FALSE;
}

WINBOOL
STDCALL
IsCharLowerA(
  CHAR ch)
{
  return FALSE;
}

WINBOOL
STDCALL
IsCharLowerW(
  WCHAR ch)
{
  return FALSE;
}

WINBOOL
STDCALL
IsCharUpperA(
  CHAR ch)
{
  return FALSE;
}

WINBOOL
STDCALL
IsCharUpperW(
  WCHAR ch)
{
  return FALSE;
}

WINBOOL
STDCALL
OemToCharA(
  LPCSTR lpszSrc,
  LPSTR lpszDst)
{
  return FALSE;
}

WINBOOL
STDCALL
OemToCharBuffA(
  LPCSTR lpszSrc,
  LPSTR lpszDst,
  DWORD cchDstLength)
{
  return FALSE;
}

WINBOOL
STDCALL
OemToCharBuffW(
  LPCSTR lpszSrc,
  LPWSTR lpszDst,
  DWORD cchDstLength)
{
  return FALSE;
}

WINBOOL
STDCALL
OemToCharW(
  LPCSTR lpszSrc,
  LPWSTR lpszDst)
{
  return FALSE;
}
