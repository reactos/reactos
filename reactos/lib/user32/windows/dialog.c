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
/* $Id: dialog.c,v 1.5 2002/06/13 20:36:40 dwelch Exp $
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

HWND
STDCALL
CreateDialogIndirectParamA(
  HINSTANCE hInstance,
  LPCDLGTEMPLATE lpTemplate,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM lParamInit)
{
  return (HWND)0;
}

HWND
STDCALL
CreateDialogIndirectParamAorW(
  HINSTANCE hInstance,
  LPCDLGTEMPLATE lpTemplate,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM lParamInit)
{
  return (HWND)0;
}

HWND
STDCALL
CreateDialogIndirectParamW(
  HINSTANCE hInstance,
  LPCDLGTEMPLATE lpTemplate,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM lParamInit)
{
  return (HWND)0;
}

HWND
STDCALL
CreateDialogParamA(
  HINSTANCE hInstance,
  LPCSTR lpTemplateName,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
  return (HWND)0;
}

HWND
STDCALL
CreateDialogParamW(
  HINSTANCE hInstance,
  LPCWSTR lpTemplateName,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
  return (HWND)0;
}
LRESULT
STDCALL
DefDlgProcA(
  HWND hDlg,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return (LRESULT)0;
}

LRESULT
STDCALL
DefDlgProcW(
  HWND hDlg,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return (LRESULT)0;
}
INT_PTR
STDCALL
DialogBoxIndirectParamA(
  HINSTANCE hInstance,
  LPCDLGTEMPLATE hDialogTemplate,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
  return (INT_PTR)NULL;
}

INT_PTR
STDCALL
DialogBoxIndirectParamAorW(
  HINSTANCE hInstance,
  LPCDLGTEMPLATE hDialogTemplate,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
  return (INT_PTR)NULL;
}

INT_PTR
STDCALL
DialogBoxIndirectParamW(
  HINSTANCE hInstance,
  LPCDLGTEMPLATE hDialogTemplate,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
  return (INT_PTR)NULL;
}

INT_PTR
STDCALL
DialogBoxParamA(
  HINSTANCE hInstance,
  LPCSTR lpTemplateName,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
  return (INT_PTR)0;
}

INT_PTR
STDCALL
DialogBoxParamW(
  HINSTANCE hInstance,
  LPCWSTR lpTemplateName,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
  return (INT_PTR)0;
}

int
STDCALL
DlgDirListA(
  HWND hDlg,
  LPSTR lpPathSpec,
  int nIDListBox,
  int nIDStaticPath,
  UINT uFileType)
{
  return 0;
}

int
STDCALL
DlgDirListComboBoxA(
  HWND hDlg,
  LPSTR lpPathSpec,
  int nIDComboBox,
  int nIDStaticPath,
  UINT uFiletype)
{
  return 0;
}

int
STDCALL
DlgDirListComboBoxW(
  HWND hDlg,
  LPWSTR lpPathSpec,
  int nIDComboBox,
  int nIDStaticPath,
  UINT uFiletype)
{
  return 0;
}

int
STDCALL
DlgDirListW(
  HWND hDlg,
  LPWSTR lpPathSpec,
  int nIDListBox,
  int nIDStaticPath,
  UINT uFileType)
{
  return 0;
}

WINBOOL
STDCALL
DlgDirSelectComboBoxExA(
  HWND hDlg,
  LPSTR lpString,
  int nCount,
  int nIDComboBox)
{
  return FALSE;
}

WINBOOL
STDCALL
DlgDirSelectComboBoxExW(
  HWND hDlg,
  LPWSTR lpString,
  int nCount,
  int nIDComboBox)
{
  return FALSE;
}

WINBOOL
STDCALL
DlgDirSelectExA(
  HWND hDlg,
  LPSTR lpString,
  int nCount,
  int nIDListBox)
{
  return FALSE;
}

WINBOOL
STDCALL
DlgDirSelectExW(
  HWND hDlg,
  LPWSTR lpString,
  int nCount,
  int nIDListBox)
{
  return FALSE;
}
WINBOOL
STDCALL
EndDialog(
  HWND hDlg,
  INT_PTR nResult)
{
  return FALSE;
}
LONG
STDCALL
GetDialogBaseUnits(VOID)
{
  return 0;
}

int
STDCALL
GetDlgCtrlID(
  HWND hwndCtl)
{
  return 0;
}

HWND
STDCALL
GetDlgItem(
  HWND hDlg,
  int nIDDlgItem)
{
  return (HWND)0;
}

UINT
STDCALL
GetDlgItemInt(
  HWND hDlg,
  int nIDDlgItem,
  WINBOOL *lpTranslated,
  WINBOOL bSigned)
{
  return 0;
}

UINT
STDCALL
GetDlgItemTextA(
  HWND hDlg,
  int nIDDlgItem,
  LPSTR lpString,
  int nMaxCount)
{
  return 0;
}

UINT
STDCALL
GetDlgItemTextW(
  HWND hDlg,
  int nIDDlgItem,
  LPWSTR lpString,
  int nMaxCount)
{
  return 0;
}
HWND
STDCALL
GetNextDlgGroupItem(
  HWND hDlg,
  HWND hCtl,
  WINBOOL bPrevious)
{
  return (HWND)0;
}

HWND
STDCALL
GetNextDlgTabItem(
  HWND hDlg,
  HWND hCtl,
  WINBOOL bPrevious)
{
  return (HWND)0;
}
#if 0
WINBOOL
STDCALL
IsDialogMessage(
  HWND hDlg,
  LPMSG lpMsg)
{
  return FALSE;
}
#endif

WINBOOL
STDCALL
IsDialogMessageA(
  HWND hDlg,
  LPMSG lpMsg)
{
  return FALSE;
}

WINBOOL
STDCALL
IsDialogMessageW(
  HWND hDlg,
  LPMSG lpMsg)
{
  return FALSE;
}

UINT
STDCALL
IsDlgButtonChecked(
  HWND hDlg,
  int nIDButton)
{
  return 0;
}
WINBOOL
STDCALL
MapDialogRect(
  HWND hDlg,
  LPRECT lpRect)
{
  return FALSE;
}
LRESULT
STDCALL
SendDlgItemMessageA(
  HWND hDlg,
  int nIDDlgItem,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return (LRESULT)0;
}

LRESULT
STDCALL
SendDlgItemMessageW(
  HWND hDlg,
  int nIDDlgItem,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return (LRESULT)0;
}
WINBOOL
STDCALL
SetDlgItemInt(
  HWND hDlg,
  int nIDDlgItem,
  UINT uValue,
  WINBOOL bSigned)
{
  return FALSE;
}

WINBOOL
STDCALL
SetDlgItemTextA(
  HWND hDlg,
  int nIDDlgItem,
  LPCSTR lpString)
{
  return FALSE;
}

WINBOOL
STDCALL
SetDlgItemTextW(
  HWND hDlg,
  int nIDDlgItem,
  LPCWSTR lpString)
{
  return FALSE;
}
WINBOOL
STDCALL
CheckDlgButton(
  HWND hDlg,
  int nIDButton,
  UINT uCheck)
{
  return FALSE;
}

