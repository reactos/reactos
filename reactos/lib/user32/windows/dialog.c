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
/* $Id: dialog.c,v 1.9 2003/07/10 21:04:31 chorns Exp $
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

/*
 * @unimplemented
 */
HWND
STDCALL
CreateDialogIndirectParamA(
  HINSTANCE hInstance,
  LPCDLGTEMPLATE lpTemplate,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM lParamInit)
{
  UNIMPLEMENTED;
  return (HWND)0;
}


/*
 * @unimplemented
 */
HWND
STDCALL
CreateDialogIndirectParamAorW(
  HINSTANCE hInstance,
  LPCDLGTEMPLATE lpTemplate,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM lParamInit)
{
  UNIMPLEMENTED;
  return (HWND)0;
}


/*
 * @unimplemented
 */
HWND
STDCALL
CreateDialogIndirectParamW(
  HINSTANCE hInstance,
  LPCDLGTEMPLATE lpTemplate,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM lParamInit)
{
  UNIMPLEMENTED;
  return (HWND)0;
}


/*
 * @unimplemented
 */
HWND
STDCALL
CreateDialogParamA(
  HINSTANCE hInstance,
  LPCSTR lpTemplateName,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
  UNIMPLEMENTED;
  return (HWND)0;
}


/*
 * @unimplemented
 */
HWND
STDCALL
CreateDialogParamW(
  HINSTANCE hInstance,
  LPCWSTR lpTemplateName,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
  UNIMPLEMENTED;
  return (HWND)0;
}


/*
 * @unimplemented
 */
LRESULT
STDCALL
DefDlgProcA(
  HWND hDlg,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  UNIMPLEMENTED;
  return (LRESULT)0;
}


/*
 * @unimplemented
 */
LRESULT
STDCALL
DefDlgProcW(
  HWND hDlg,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  UNIMPLEMENTED;
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
  UNIMPLEMENTED;
  return (INT_PTR)NULL;
}


/*
 * @unimplemented
 */
INT_PTR
STDCALL
DialogBoxIndirectParamAorW(
  HINSTANCE hInstance,
  LPCDLGTEMPLATE hDialogTemplate,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
  UNIMPLEMENTED;
  return (INT_PTR)NULL;
}


/*
 * @unimplemented
 */
INT_PTR
STDCALL
DialogBoxIndirectParamW(
  HINSTANCE hInstance,
  LPCDLGTEMPLATE hDialogTemplate,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
  UNIMPLEMENTED;
  return (INT_PTR)NULL;
}


/*
 * @unimplemented
 */
INT_PTR
STDCALL
DialogBoxParamA(
  HINSTANCE hInstance,
  LPCSTR lpTemplateName,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
  UNIMPLEMENTED;
  return (INT_PTR)0;
}


/*
 * @unimplemented
 */
INT_PTR
STDCALL
DialogBoxParamW(
  HINSTANCE hInstance,
  LPCWSTR lpTemplateName,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
  UNIMPLEMENTED;
  return (INT_PTR)0;
}


/*
 * @unimplemented
 */
int
STDCALL
DlgDirListA(
  HWND hDlg,
  LPSTR lpPathSpec,
  int nIDListBox,
  int nIDStaticPath,
  UINT uFileType)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
DlgDirListComboBoxA(
  HWND hDlg,
  LPSTR lpPathSpec,
  int nIDComboBox,
  int nIDStaticPath,
  UINT uFiletype)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
DlgDirListComboBoxW(
  HWND hDlg,
  LPWSTR lpPathSpec,
  int nIDComboBox,
  int nIDStaticPath,
  UINT uFiletype)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
DlgDirListW(
  HWND hDlg,
  LPWSTR lpPathSpec,
  int nIDListBox,
  int nIDStaticPath,
  UINT uFileType)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
DlgDirSelectComboBoxExA(
  HWND hDlg,
  LPSTR lpString,
  int nCount,
  int nIDComboBox)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
DlgDirSelectComboBoxExW(
  HWND hDlg,
  LPWSTR lpString,
  int nCount,
  int nIDComboBox)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
DlgDirSelectExA(
  HWND hDlg,
  LPSTR lpString,
  int nCount,
  int nIDListBox)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
DlgDirSelectExW(
  HWND hDlg,
  LPWSTR lpString,
  int nCount,
  int nIDListBox)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
EndDialog(
  HWND hDlg,
  INT_PTR nResult)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
LONG
STDCALL
GetDialogBaseUnits(VOID)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
GetDlgCtrlID(
  HWND hwndCtl)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
HWND
STDCALL
GetDlgItem(
  HWND hDlg,
  int nIDDlgItem)
{
  UNIMPLEMENTED;
  return (HWND)0;
}


/*
 * @unimplemented
 */
UINT
STDCALL
GetDlgItemInt(
  HWND hDlg,
  int nIDDlgItem,
  WINBOOL *lpTranslated,
  WINBOOL bSigned)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
UINT
STDCALL
GetDlgItemTextA(
  HWND hDlg,
  int nIDDlgItem,
  LPSTR lpString,
  int nMaxCount)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
UINT
STDCALL
GetDlgItemTextW(
  HWND hDlg,
  int nIDDlgItem,
  LPWSTR lpString,
  int nMaxCount)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
HWND
STDCALL
GetNextDlgGroupItem(
  HWND hDlg,
  HWND hCtl,
  WINBOOL bPrevious)
{
  UNIMPLEMENTED;
  return (HWND)0;
}


/*
 * @unimplemented
 */
HWND
STDCALL
GetNextDlgTabItem(
  HWND hDlg,
  HWND hCtl,
  WINBOOL bPrevious)
{
  UNIMPLEMENTED;
  return (HWND)0;
}
#if 0
WINBOOL
STDCALL
IsDialogMessage(
  HWND hDlg,
  LPMSG lpMsg)
{
  UNIMPLEMENTED;
  return FALSE;
}
#endif


/*
 * @unimplemented
 */
WINBOOL
STDCALL
IsDialogMessageA(
  HWND hDlg,
  LPMSG lpMsg)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
IsDialogMessageW(
  HWND hDlg,
  LPMSG lpMsg)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
UINT
STDCALL
IsDlgButtonChecked(
  HWND hDlg,
  int nIDButton)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
MapDialogRect(
  HWND hDlg,
  LPRECT lpRect)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
LRESULT
STDCALL
SendDlgItemMessageA(
  HWND hDlg,
  int nIDDlgItem,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  UNIMPLEMENTED;
  return (LRESULT)0;
}


/*
 * @unimplemented
 */
LRESULT
STDCALL
SendDlgItemMessageW(
  HWND hDlg,
  int nIDDlgItem,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  UNIMPLEMENTED;
  return (LRESULT)0;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
SetDlgItemInt(
  HWND hDlg,
  int nIDDlgItem,
  UINT uValue,
  WINBOOL bSigned)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
SetDlgItemTextA(
  HWND hDlg,
  int nIDDlgItem,
  LPCSTR lpString)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
SetDlgItemTextW(
  HWND hDlg,
  int nIDDlgItem,
  LPCWSTR lpString)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
CheckDlgButton(
  HWND hDlg,
  int nIDButton,
  UINT uCheck)
{
  UNIMPLEMENTED;
  return FALSE;
}
