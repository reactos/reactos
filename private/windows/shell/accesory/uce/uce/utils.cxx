/********************************************************************
 *
 *  Module Name : utils.c
 *
 *  Various utilities for UCE
 *
 *  Copyright (c) 1997-1999 Microsoft Corporation.
 ********************************************************************/

#include "windows.h"
#include "commctrl.h"

#include "UCE.h"
#include "stdlib.h"
#include "tchar.h"
#include "stdio.h"
#include "winuser.h"
#include "string.h"
#include "search.h"
#include "getuname.h"

#include "winnls.h"
#include "wingdi.h"

/********************************************************************

    Function : Set current selection in combobox.

********************************************************************/
BOOL LoadNeedMessage()
{
  TCHAR szVal[128] = TEXT("");
  DWORD dwRetVal;

  dwRetVal=GetProfileString(TEXT("MSUCE"),TEXT("DISPLAYFONTMSG"),NULL,(LPTSTR)szVal, sizeof(szVal));
  if (!dwRetVal)
    return true;
  else
    return (BOOL)(atoi((const char*)szVal));
}

void SaveNeedMessage(BOOL nMsg)
{
  if (nMsg)
    WriteProfileString(TEXT("MSUCE"), TEXT("DISPLAYFONTMSG"), TEXT("1"));
  else
    WriteProfileString(TEXT("MSUCE"), TEXT("DISPLAYFONTMSG"), TEXT("0"));
}


INT
LoadCurrentSelection(
    HWND   hWnd,
    UINT   uID,
    LPTSTR lpszKey,
    LPTSTR lpszDefault
    )
{
    TCHAR szValueName[128] = TEXT("");
    INT nIndex;
    DWORD dwRetVal;

    nIndex = CB_ERR;

    dwRetVal = GetProfileString(
                   TEXT("MSUCE"),
                   lpszKey,
                   NULL,
                   (LPTSTR)szValueName,
                   BTOC(sizeof(szValueName))
               );

    if (dwRetVal != 0)
    {
        nIndex = (INT) SendDlgItemMessage(
                           hWnd,
                           uID,
                           CB_SELECTSTRING,
                           (WPARAM)-1,
                           (LPARAM)(LPTSTR)szValueName
                       );
    }

    /*
     * If there was no profile or the selection failed then try selecting
     * the Basic Latin block, if that fails then select the first one.
     */

    if (nIndex == CB_ERR)
    {
        nIndex = (INT) SendDlgItemMessage(
                           hWnd,
                           uID,
                           CB_SELECTSTRING,
                           (WPARAM)-1,
                           (LPARAM) lpszDefault
                       );
    }

    nIndex = (INT)SendDlgItemMessage(
                 hWnd,
                 uID,
                 CB_SETCURSEL,
                 (WPARAM) (nIndex == CB_ERR) ? 0 : nIndex,
                 (LPARAM) 0L
             );

    return nIndex;
}

/********************************************************************

    Function : Used to save the current selection values in win.ini,
               so that it can be selected the next time UCE comes up.

********************************************************************/
BOOL
SaveCurrentSelection(
    HWND   hWnd,
    UINT   uID,
    LPTSTR lpszKey
    )
{
    TCHAR szValue[128] = TEXT("");
    INT nIndex;
    INT nRetVal;

    nIndex = (INT) SendDlgItemMessage(
                       hWnd,
                       uID,
                       CB_GETCURSEL,
                       (WPARAM) 0,
                       (LPARAM) 0L
                   );

    if (nIndex == CB_ERR)
    {
        return FALSE;
    }

    nRetVal = (INT)SendDlgItemMessage(
                  hWnd,
                  uID,
                  CB_GETLBTEXT,
                  (WPARAM) nIndex,
                  (LPARAM) szValue
              );

    if (nRetVal == CB_ERR)
    {
        return FALSE;
    }

    return WriteProfileString(TEXT("MSUCE"), lpszKey, szValue);
}

/********************************************************************

    Function : GetSystemPathName

********************************************************************/
VOID GetSystemPathName(
    PWSTR pwszPath,
    PWSTR pwszFileName,
    UINT  maxChar
    )
{
    UINT fnLen = wcslen(pwszFileName);
    UINT i = GetSystemDirectoryW(pwszPath, maxChar);

    // avoid error condition
    if (fnLen + 1 >= maxChar) {
        *pwszPath = L'\0';
        return;
    }
    if (i > 0 || i < maxChar - fnLen - 1) {
        pwszPath += i;
        if (pwszPath[-1] != L'\\')
            *pwszPath++ = L'\\';
    }
    wcscpy(pwszPath, pwszFileName);
}

/********************************************************************

    Function : Set current selection in combobox.

********************************************************************/
INT
LoadAdvancedSelection(
    HWND   hWnd,
    UINT   uID,
    LPTSTR lpszKey
    )
{
    TCHAR szValueName[128] = TEXT("");
    DWORD iCheckState;

    iCheckState = GetProfileInt(
                   TEXT("MSUCE"),
                   lpszKey,
                   0);

    fDisplayAdvControls = (iCheckState == 0)? FALSE: TRUE;

    return 0;
}

/********************************************************************

    Function : Used to save the current selection values in win.ini,
               so that it can be selected the next time UCE comes up.

********************************************************************/
BOOL
SaveAdvancedSelection(
    HWND   hWnd,
    UINT   uID,
    LPTSTR lpszKey
    )
{
    TCHAR szValue[128] = TEXT("");
    int   iCheckState;

    iCheckState = (fDisplayAdvControls == TRUE)? 1: 0;

    wsprintf(szValue, L"%d", iCheckState);

    return WriteProfileString(TEXT("MSUCE"), lpszKey, szValue);
}

