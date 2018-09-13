/**************************** Module Header ********************************\
* Module Name: combodir.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Directory Combo Box Routines
*
* History:
* ??-???-???? ??????    Ported from Win 3.0 sources
* 01-Feb-1991 mikeke    Added Revalidation code
\***************************************************************************/

#define CTLMGR
#define LSTRING

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* xxxCBDir
*
* Supports the CB_DIR message which adds a list of files from the
* current directory to the combo box.
*
* History:
\***************************************************************************/

int xxxCBDir(
    PCBOX pcbox,
    UINT attrib,
    LPWSTR pFileName)
{
    PLBIV plb;
    int errorValue;
    TL tlpwnd;

    CheckLock(pcbox->spwnd);
    UserAssert(pcbox->spwndList);

    plb = ((PLBWND)pcbox->spwndList)->pLBIV;

    ThreadLock(plb->spwnd, &tlpwnd);
    errorValue = xxxLbDir(plb, attrib, pFileName);
    ThreadUnlock(&tlpwnd);

    switch (errorValue) {
    case LB_ERR:
        return CB_ERR;
        break;
    case LB_ERRSPACE:
        return CB_ERRSPACE;
        break;
    default:
        return errorValue;
        break;
    }
}

/***************************************************************************\
* DlgDirSelectComboBoxEx
*
* Retrieves the current selection from the listbox of a combobox.
* It assumes that the combo box was filled by xxxDlgDirListComboBox()
* and that the selection is a drive letter, a file, or a directory name.
*
* History:
* 12-05-90 IanJa    converted to internal version
\***************************************************************************/

int DlgDirSelectComboBoxExA(
    HWND hwndDlg,
    LPSTR pszOut,
    int cchOut,
    int idComboBox)
{
    LPWSTR lpwsz;
    BOOL fRet;

    lpwsz = (LPWSTR)UserLocalAlloc(HEAP_ZERO_MEMORY, cchOut * sizeof(WCHAR));
    if (!lpwsz) {
        return FALSE;
    }

    fRet = DlgDirSelectComboBoxExW(hwndDlg, lpwsz, cchOut, idComboBox);

    WCSToMB(lpwsz, -1, &pszOut, cchOut, FALSE);

    UserLocalFree(lpwsz);

    return fRet;
}

int DlgDirSelectComboBoxExW(
    HWND hwndDlg,
    LPWSTR pwszOut,
    int cchOut,
    int idComboBox)
{
    PWND pwndDlg;
    PWND pwndComboBox;
    PCBOX pcbox;

    pwndDlg = ValidateHwnd(hwndDlg);

    if (pwndDlg == NULL)
        return FALSE;

    pwndComboBox = _GetDlgItem(pwndDlg, idComboBox);
    if (pwndComboBox == NULL) {
        RIPERR0(ERROR_CONTROL_ID_NOT_FOUND, RIP_VERBOSE, "");
        return 0;
    }
    pcbox = ((PCOMBOWND)pwndComboBox)->pcbox;
    if (pcbox == NULL) {
        RIPERR0(ERROR_WINDOW_NOT_COMBOBOX, RIP_VERBOSE, "");
        return 0;
    }

    return DlgDirSelectHelper(pwszOut, cchOut, HWq(pcbox->spwndList));
}


/***************************************************************************\
* xxxDlgDirListComboBox
*
* History:
* 12-05-90 IanJa    converted to internal version
\***************************************************************************/

int DlgDirListComboBoxA(
    HWND hwndDlg,
    LPSTR lpszPathSpecClient,
    int idComboBox,
    int idStaticPath,
    UINT attrib)
{
    LPWSTR lpszPathSpec;
    TL tlpwndDlg;
    PWND pwndDlg;
    BOOL fRet;

    pwndDlg = ValidateHwnd(hwndDlg);

    if (pwndDlg == NULL)
        return FALSE;

    lpszPathSpec = NULL;
    if (lpszPathSpecClient) {
        if (!MBToWCS(lpszPathSpecClient, -1, &lpszPathSpec, -1, TRUE))
            return FALSE;
    }

    ThreadLock(pwndDlg, &tlpwndDlg);
    fRet = xxxDlgDirListHelper(pwndDlg, lpszPathSpec, lpszPathSpecClient,
            idComboBox, idStaticPath, attrib, FALSE);
    ThreadUnlock(&tlpwndDlg);

    if (lpszPathSpec) {
        if (fRet) {
            /*
             * Non-zero retval means some text to copy out.  Copy out up to
             * the nul terminator (buffer will be big enough).
             */
            WCSToMB(lpszPathSpec, -1, &lpszPathSpecClient, MAXLONG, FALSE);
        }
        UserLocalFree(lpszPathSpec);
    }

    return fRet;
}

int DlgDirListComboBoxW(
    HWND hwndDlg,
    LPWSTR lpszPathSpecClient,
    int idComboBox,
    int idStaticPath,
    UINT attrib)
{
    LPWSTR lpszPathSpec;
    PWND pwndDlg;
    TL tlpwndDlg;
    BOOL fRet;

    pwndDlg = ValidateHwnd(hwndDlg);

    if (pwndDlg == NULL)
        return FALSE;

    lpszPathSpec = lpszPathSpecClient;

    ThreadLock(pwndDlg, &tlpwndDlg);
    fRet = xxxDlgDirListHelper(pwndDlg, lpszPathSpec, (LPBYTE)lpszPathSpecClient,
            idComboBox, idStaticPath, attrib, FALSE);
    ThreadUnlock(&tlpwndDlg);

    return fRet;
}
