/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS User Manager Control Panel
 * FILE:            dll/cpl/usrmgr/misc.c
 * PURPOSE:         Miscellaneus functions
 *
 * PROGRAMMERS:     Eric Kohl
 */

#include "usrmgr.h"


VOID
DebugPrintf(LPTSTR szFormat, ...)
{
    TCHAR szOut[512];
    va_list arg_ptr;


    va_start (arg_ptr, szFormat);
    _vstprintf (szOut, szFormat, arg_ptr);
    va_end (arg_ptr);

    MessageBox(NULL, szOut, _T("Debug"), MB_OK);
}

BOOL
CheckAccountName(HWND hwndDlg,
                 INT nIdDlgItem,
                 LPTSTR lpAccountName)
{
    TCHAR szAccountName[256];
    UINT uLen;

    if (lpAccountName)
        uLen = _tcslen(lpAccountName);
    else
        uLen = GetDlgItemText(hwndDlg, nIdDlgItem, szAccountName, 256);

    /* Check the account name */
    if (uLen > 0 &&
        _tcspbrk((lpAccountName) ? lpAccountName : szAccountName, TEXT("\"*+,/\\:;<=>?[]|")) != NULL)
    {
        MessageBox(hwndDlg,
                   TEXT("The account name you entered is invalid! An account name must not contain the following charecters: *+,/:;<=>?[\\]|"),
                   TEXT("ERROR"),
                   MB_OK | MB_ICONERROR);
        return FALSE;
    }

    return TRUE;
}
