/*
 * SHLWAPI message box functions
 *
 * Copyright 2004 Jon Griffiths
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "shlwapi.h"
#include "wine/debug.h"
#include "resource.h"


WINE_DEFAULT_DEBUG_CHANNEL(shell);

extern HINSTANCE shlwapi_hInstance; /* in shlwapi_main.c */

static const WCHAR szDontShowKey[] = { 'S','o','f','t','w','a','r','e','\\',
  'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\',
  'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
  'E','x','p','l','o','r','e','r','\\','D','o','n','t','S','h','o','w',
  'M','e','T','h','i','s','D','i','a','l','o','g','A','g','a','i','n','\0'
};

INT_PTR WINAPI SHMessageBoxCheckExW(HWND,HINSTANCE,LPCWSTR,DLGPROC,LPARAM,INT_PTR,LPCWSTR);
INT_PTR WINAPI SHMessageBoxCheckW(HWND,LPCWSTR,LPCWSTR,DWORD,INT_PTR,LPCWSTR);

/* Data held by each general message boxes */
typedef struct tagDLGDATAEX
{
  DLGPROC dlgProc;   /* User supplied DlgProc */
  LPARAM  lParam;    /* User supplied LPARAM for dlgProc */
  LPCWSTR lpszId;    /* Name of reg key holding whether to skip */
} DLGDATAEX;

/* Dialogue procedure for general message boxes */
static INT_PTR CALLBACK SHDlgProcEx(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  DLGDATAEX *d = (DLGDATAEX *)GetWindowLongPtrW(hDlg, DWLP_USER);

  TRACE("(%p,%u,%Id,%Id) data %p\n", hDlg, uMsg, wParam, lParam, d);

  switch (uMsg)
  {
  case WM_INITDIALOG:
  {
    /* FIXME: Not sure where native stores its lParam */
    SetWindowLongPtrW(hDlg, DWLP_USER, lParam);
    d = (DLGDATAEX *)lParam;
    TRACE("WM_INITDIALOG: %p, %s,%p,%p\n", hDlg, debugstr_w(d->lpszId),
          d->dlgProc, (void*)d->lParam);
    if (d->dlgProc)
      return d->dlgProc(hDlg, uMsg, wParam, d->lParam);
    return TRUE;
  }

  case WM_COMMAND:
    switch (LOWORD(wParam))
    {
      case IDYES:
        wParam = MAKELONG(IDOK, HIWORD(wParam));
        /* Fall through ... */
      case IDNO:
        if (LOWORD(wParam) == IDNO)
          wParam = MAKELONG(IDCANCEL, HIWORD(wParam));
        /* Fall through ... */
      case IDOK:
      case IDCANCEL:

        TRACE("WM_COMMAND: id=%s data=%p\n",
              LOWORD(wParam) == IDOK ? "IDOK" : "IDCANCEL", d);

        if (SendMessageW(GetDlgItem(hDlg, IDC_ERR_DONT_SHOW), BM_GETCHECK, 0L, 0L))
        {
          DWORD dwZero = 0;

          /* The user clicked 'don't show again', so set the key */
          SHRegSetUSValueW(szDontShowKey, d->lpszId, REG_DWORD, &dwZero,
                           sizeof(dwZero), SHREGSET_DEFAULT);
        }
        if (!d->dlgProc || !d->dlgProc(hDlg, uMsg, wParam, lParam))
          EndDialog(hDlg, wParam);
        return TRUE;
    }
    break;

  default:
    break;
  }

  if (d && d->dlgProc)
    return d->dlgProc(hDlg, uMsg, wParam, lParam);
  return FALSE;
}

/*************************************************************************
 * @ [SHLWAPI.291]
 *
 * Pop up a 'Don't show this message again' dialogue box.
 *
 * PARAMS
 *  hWnd     [I] Window to be the dialogues' parent
 *  hInst    [I] Instance of the module holding the dialogue resource
 *  lpszName [I] Resource Id of the dialogue resource
 *  dlgProc  [I] Dialog procedure, or NULL for default handling
 *  lParam   [I] LPARAM to pass to dlgProc
 *  iRet     [I] Value to return if dialogue is not shown
 *  lpszId   [I] Name of registry subkey which determines whether to show the dialog
 *
 * RETURNS
 *  Success: The value returned from the dialogue procedure.
 *  Failure: iRet, if the dialogue resource could not be loaded or the dialogue
 *           should not be shown.
 *
 * NOTES
 *  Both lpszName and lpszId must be less than MAX_PATH in length.
 */
INT_PTR WINAPI SHMessageBoxCheckExA(HWND hWnd, HINSTANCE hInst, LPCSTR lpszName,
                                    DLGPROC dlgProc, LPARAM lParam, INT_PTR iRet,
                                    LPCSTR lpszId)
{
  WCHAR szNameBuff[MAX_PATH], szIdBuff[MAX_PATH];
  LPCWSTR szName = szNameBuff;

  if (IS_INTRESOURCE(lpszName))
    szName = (LPCWSTR)lpszName; /* Resource Id or NULL */
  else
    MultiByteToWideChar(CP_ACP, 0, lpszName, -1, szNameBuff, MAX_PATH);

  MultiByteToWideChar(CP_ACP, 0, lpszId, -1, szIdBuff, MAX_PATH);

  return SHMessageBoxCheckExW(hWnd, hInst, szName, dlgProc, lParam, iRet, szIdBuff);
}

/*************************************************************************
 * @ [SHLWAPI.292]
 *
 * Unicode version of SHMessageBoxCheckExW.
 */
INT_PTR WINAPI SHMessageBoxCheckExW(HWND hWnd, HINSTANCE hInst, LPCWSTR lpszName,
                                    DLGPROC dlgProc, LPARAM lParam, INT_PTR iRet, LPCWSTR lpszId)
{
  DLGDATAEX d;

  if (!SHRegGetBoolUSValueW(szDontShowKey, lpszId, FALSE, TRUE))
    return iRet;

  d.dlgProc = dlgProc;
  d.lParam = lParam;
  d.lpszId = lpszId;
  return DialogBoxParamW(hInst, lpszName, hWnd, SHDlgProcEx, (LPARAM)&d);
}

/* Data held by each shlwapi message box */
typedef struct tagDLGDATA
{
  LPCWSTR lpszTitle; /* User supplied message title */
  LPCWSTR lpszText;  /* User supplied message text */
  DWORD   dwType;    /* Message box type */
} DLGDATA;

/* Dialogue procedure for shlwapi message boxes */
static INT_PTR CALLBACK SHDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  TRACE("(%p,%u,%Id,%Id)\n", hDlg, uMsg, wParam, lParam);

  switch (uMsg)
  {
  case WM_INITDIALOG:
  {
    DLGDATA *d = (DLGDATA *)lParam;
    TRACE("WM_INITDIALOG: %p, %s,%s,%ld\n", hDlg, debugstr_w(d->lpszTitle),
          debugstr_w(d->lpszText), d->dwType);

    SetWindowTextW(hDlg, d->lpszTitle);
    SetWindowTextW(GetDlgItem(hDlg, IDS_ERR_USER_MSG), d->lpszText);

    /* Set buttons according to dwType */
    switch (d->dwType)
    {
    case 0:
      ShowWindow(GetDlgItem(hDlg, IDCANCEL), SW_HIDE);
      /* FIXME: Move OK button to position of the Cancel button (cosmetic) */
    case 1:
      ShowWindow(GetDlgItem(hDlg, IDYES), SW_HIDE);
      ShowWindow(GetDlgItem(hDlg, IDNO), SW_HIDE);
      break;
    default:
      ShowWindow(GetDlgItem(hDlg, IDOK), SW_HIDE);
      ShowWindow(GetDlgItem(hDlg, IDCANCEL), SW_HIDE);
      break;
    }
    return TRUE;
  }
  default:
    break;
  }
  return FALSE;
}

/*************************************************************************
 * @ [SHLWAPI.185]
 *
 * Pop up a 'Don't show this message again' dialogue box.
 *
 * PARAMS
 *  hWnd      [I] Window to be the dialogues' parent
 *  lpszText  [I] Text of the message to show
 *  lpszTitle [I] Title of the dialogue box
 *  dwType    [I] Type of dialogue buttons (See below)
 *  iRet      [I] Value to return if dialogue is not shown
 *  lpszId    [I] Name of registry subkey which determines whether to show the dialog
 *
 * RETURNS
 *  Success: The value returned from the dialogue procedure (e.g. IDOK).
 *  Failure: iRet, if the default dialogue resource could not be loaded or the
 *           dialogue should not be shown.
 *
 * NOTES
 *  - Both lpszTitle and lpszId must be less than MAX_PATH in length.
 *  - Possible values for dwType are:
 *| Value     Buttons
 *| -----     -------
 *|   0       OK
 *|   1       OK/Cancel
 *|   2       Yes/No
 */
INT_PTR WINAPI SHMessageBoxCheckA(HWND hWnd, LPCSTR lpszText, LPCSTR lpszTitle,
                                  DWORD dwType, INT_PTR iRet, LPCSTR lpszId)
{
  WCHAR szTitleBuff[MAX_PATH], szIdBuff[MAX_PATH];
  WCHAR *szTextBuff = NULL;
  int iLen;
  INT_PTR iRetVal;

  if (lpszTitle)
    MultiByteToWideChar(CP_ACP, 0, lpszTitle, -1, szTitleBuff, MAX_PATH);

  if (lpszText)
  {
    iLen = MultiByteToWideChar(CP_ACP, 0, lpszText, -1, NULL, 0);
    szTextBuff = malloc(iLen * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, lpszText, -1, szTextBuff, iLen);
  }

  MultiByteToWideChar(CP_ACP, 0, lpszId, -1, szIdBuff, MAX_PATH);

  iRetVal = SHMessageBoxCheckW(hWnd, szTextBuff, lpszTitle ? szTitleBuff : NULL,
                               dwType, iRet, szIdBuff);
  free(szTextBuff);
  return iRetVal;
}

/*************************************************************************
 * @ [SHLWAPI.191]
 *
 * Unicode version of SHMessageBoxCheckA.
 */
INT_PTR WINAPI SHMessageBoxCheckW(HWND hWnd, LPCWSTR lpszText, LPCWSTR lpszTitle,
                                  DWORD dwType, INT_PTR iRet, LPCWSTR lpszId)
{
  DLGDATA d;

  d.lpszTitle = lpszTitle;
  d.lpszText = lpszText;
  d.dwType = dwType;

  return SHMessageBoxCheckExW(hWnd, shlwapi_hInstance, (LPCWSTR)IDD_ERR_DIALOG,
                               SHDlgProc, (LPARAM)&d, iRet, lpszId);
}
