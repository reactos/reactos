#include "stdafx.h"
#pragma hdrstop
//#include <shellids.h>
//#include "pattern.h"
//#include "deskstat.h"
//#include "dutil.h"
//#include "editpat.h"
//#include "resource.h"

#include <mluisupp.h>

#define THISCLASS CPattern

#define SetDefaultDialogFont SHSetDefaultDialogFont
#define RemoveDefaultDialogFont SHRemoveDefaultDialogFont

#define c_szHelpFile    TEXT("Update.hlp")
const static DWORD aPatternHelpIDs[] = {  // Context Help IDs
    IDC_PAT_PATTERN,    IDH_DISPLAY_PATTERN,
    IDC_PAT_LIST,       IDH_DISPLAY_PATTERN,
    IDC_PAT_PREVIEW,    IDH_DISPLAY_PATTERN,
    IDC_PAT_SAMPLE,     IDH_DISPLAY_PATTERN,
    IDC_PAT_EDIT,       IDH_EDIT_PATTERN,
    0, 0
};

THISCLASS::CPattern(void)
{
}

LPTSTR GetSection(LPCTSTR pszIniFile, LPCTSTR pszSection)
{
    BOOL fDone = FALSE;
    int cchBuf = 4096;
    LPTSTR pszBuf = (LPTSTR)LocalAlloc(LPTR, cchBuf * SIZEOF(TCHAR));

    while (pszBuf && !fDone)
    {
        int cchRead = GetPrivateProfileString(pszSection, NULL, c_szNULL, pszBuf, cchBuf, pszIniFile);

        if (cchRead > cchBuf-2)
        {
            //
            // Need to grow the buffer.
            //
            cchBuf += 2048;
            LPTSTR pszTemp = pszBuf;
            pszBuf = (LPTSTR)LocalReAlloc((HANDLE)pszBuf, cchBuf * SIZEOF(TCHAR), LMEM_MOVEABLE);
            if (pszBuf == NULL)
            {
                LocalFree(pszTemp);
            }
        }
        else
        {
            fDone = TRUE;
        }
    }

    return pszBuf;
}

void THISCLASS::_OnInitDialog(HWND hwnd)
{
    //Set the font that can display the strings in the native language.
    SetDefaultDialogFont(hwnd, IDC_PAT_LIST);
    _hwnd = hwnd;
    _hwndLB = GetDlgItem(_hwnd, IDC_PAT_LIST);
    _hwndSample = GetDlgItem(_hwnd, IDC_PAT_SAMPLE);

    _szCurPattern[0] = TEXT('\0');

    WCHAR   wszCurPatBits[MAX_PATH];
    LPTSTR  pszCurPatBits;

    g_pActiveDesk->GetPattern(wszCurPatBits, ARRAYSIZE(wszCurPatBits), 0);
#ifndef UNICODE
    CHAR    szCurPatBits[MAX_PATH];

    SHUnicodeToAnsi(wszCurPatBits, szCurPatBits, ARRAYSIZE(szCurPatBits));
    pszCurPatBits = szCurPatBits;
#else
    pszCurPatBits = wszCurPatBits;
#endif

    //
    // Populate the listbox.
    //
    LPTSTR pszPatterns = GetSection(c_szControlIni, c_szPatterns);
    if (pszPatterns)
    {
        BOOL fAddedNone = FALSE;

        for (; *pszPatterns; pszPatterns+=lstrlen(pszPatterns)+1)
        {
            TCHAR szBuf[MAX_PATH];
            if (GetPrivateProfileString(c_szPatterns, pszPatterns, c_szNULL, szBuf, ARRAYSIZE(szBuf), c_szControlIni))
            {
                BOOL fIsNone = !fAddedNone && (lstrcmpi(g_szNone, szBuf) == 0);

                //
                // If there's a right-hand side, add it to the list box.
                //
                if (fIsNone || IsValidPattern(szBuf))
                {
                    if (fIsNone)
                    {
                        fAddedNone = TRUE;
                    }

                    SendMessage(_hwndLB, LB_ADDSTRING, 0, (LPARAM)pszPatterns);

                    //
                    // If we haven't found current pattern name, maybe this is it.
                    //
                    if ((_szCurPattern[0] == TEXT('\0')) && (lstrcmpi(szBuf, pszCurPatBits) == 0))
                    {
                        //
                        // Same pattern bits.  We have a name.
                        //
                        lstrcpy(_szCurPattern, pszPatterns);
                    }
                }
            }
        }
        LocalFree((HANDLE)pszPatterns);
    }

    //
    // If our pattern's bits weren't in the list, use a fake name.
    //
    if (_szCurPattern[0] == TEXT('\0'))
    {
        MLLoadString(IDS_PAT_UNLISTED, _szCurPattern, ARRAYSIZE(_szCurPattern));
    }

    //
    // Select the current pattern.
    //
    SendMessage(_hwndLB, LB_SELECTSTRING, (WPARAM)-1, (LPARAM)_szCurPattern);

    //
    // Enable all necessary UI
    //
    _EnableControls();
}

void THISCLASS::_GetPattern(LPTSTR pszPattern, int cchPattern)
{
    int iSel = ListBox_GetCurSel(_hwndLB);

    if (iSel != LB_ERR)
    {
        TCHAR szPatternName[MAX_PATH];
        ListBox_GetText(_hwndLB, iSel, szPatternName);

        GetPrivateProfileString(c_szPatterns, szPatternName, c_szNULL, pszPattern, cchPattern, c_szControlIni);

        if (IsValidPattern(pszPattern) == FALSE)
        {
            pszPattern[0] = TEXT('\0');
        }
    }
    else
    {
        pszPattern[0] = TEXT('\0');
    }
}

void THISCLASS::_EnableControls(void)
{
    //
    // Pattern button enabled only when a non-none pattern is selected.
    //
    EnableWindow(GetDlgItem(_hwnd, IDC_PAT_EDIT), ListBox_GetCurSel(_hwndLB) > 0);
}

void THISCLASS::_OnCommand(WORD wNotifyCode, WORD wID, HWND hwndCtl)
{
    int iSel;

    switch (wID)
    {
    case IDC_PAT_LIST:
        switch (wNotifyCode)
        {
        case LBN_SELCHANGE:
            RECT rectSample;
            GetWindowRect(_hwndSample, &rectSample);
            rectSample.left++; rectSample.top++; rectSample.right--; rectSample.bottom--;
            // Use MapWindowPoints instead of ScreenToClient 
            // because it works on mirrored windows and on non mirrored windows.
            MapWindowPoints(NULL, _hwnd, (LPPOINT) &rectSample, 2);
            InvalidateRect(_hwnd, &rectSample, FALSE);
            _EnableControls();
            break;
        }
        break;

    case IDOK:
        TCHAR szPattern[MAX_PATH];
        LPWSTR pwszPattern;
        iSel = ListBox_GetCurSel(_hwndLB);
        if (iSel != LB_ERR)
        {
            _GetPattern(szPattern, ARRAYSIZE(szPattern));
#ifndef UNICODE
            WCHAR   wszPattern[MAX_PATH];
            SHAnsiToUnicode(szPattern, wszPattern, ARRAYSIZE(wszPattern));
            pwszPattern = wszPattern;
#else
            pwszPattern = (LPWSTR)szPattern;
#endif

            g_pActiveDesk->SetPattern(pwszPattern, 0);
            EndDialog(_hwnd, 0);
        }
        break;

    case IDCANCEL:
        EndDialog(_hwnd, -1);
        break;

    case IDC_PAT_EDIT:
        iSel = ListBox_GetCurSel(_hwndLB);
        if (iSel > 0)
        {
            DialogBoxParam(MLGetHinst(), MAKEINTRESOURCE(IDD_EDITPAT),
                _hwnd, EditPatDlgProc, (LPARAM)_hwndLB);
        }
        _OnCommand(LBN_SELCHANGE, IDC_PAT_LIST, _hwndLB);
        break;
    }
}

HBRUSH THISCLASS::_WordsToBrush(WORD *pwBits)
{
    HBRUSH hbrushRet = NULL;
    HBITMAP hbmDesktop = CreateBitmap(CXYDESKPATTERN, CXYDESKPATTERN, 1, 1, pwBits);

    if (hbmDesktop)
    {
        HDC hdcScreen = GetDC(_hwnd);
        HDC hdcMemSrc = CreateCompatibleDC(hdcScreen);
        if (hdcMemSrc)
        {
            SelectObject(hdcMemSrc, hbmDesktop);

            HBITMAP hbmMem = CreateCompatibleBitmap(hdcScreen, CXYDESKPATTERN, CXYDESKPATTERN);
            if (hbmMem)
            {
                HDC hdcMemDest = CreateCompatibleDC(hdcScreen);
                if (hdcMemDest)
                {
                    SelectObject(hdcMemDest, hbmMem);
                    SetTextColor(hdcMemDest, GetSysColor(COLOR_BACKGROUND));
                    SetBkColor(hdcMemDest, GetSysColor(COLOR_WINDOWTEXT));
                    BitBlt(hdcMemDest, 0, 0, CXYDESKPATTERN, CXYDESKPATTERN,
                            hdcMemSrc, 0, 0, SRCCOPY);

                    hbrushRet = CreatePatternBrush(hbmMem);

                    DeleteDC(hdcMemDest);
                }
                DeleteObject(hbmMem);
            }
            DeleteDC(hdcMemSrc);
        }

        ReleaseDC(_hwnd, hdcScreen);
        DeleteObject(hbmDesktop);
    }

    return hbrushRet;
}

void THISCLASS::_OnPaint(void)
{
    PAINTSTRUCT ps;

    BeginPaint(_hwnd, &ps);
    int iOldBkMode = SetBkMode(ps.hdc, TRANSPARENT);

    RECT rectSample;
    GetWindowRect(_hwndSample, &rectSample);
    rectSample.left++; rectSample.top++; rectSample.right--; rectSample.bottom--;
    // Use MapWindowPoints instead of ScreenToClient 
    // because it works on mirrored windows and on non mirrored windows.
    MapWindowPoints(NULL, _hwnd, (LPPOINT) &rectSample, 2);

    RECT rectPaint;
    if (IntersectRect(&rectPaint, &ps.rcPaint, &rectSample))
    {
        TCHAR szPattern[MAX_PATH];
        _GetPattern(szPattern, ARRAYSIZE(szPattern));

        WORD awPattern[8];
        PatternToWords(szPattern, awPattern);

        HBRUSH hbrPattern = _WordsToBrush(awPattern);
        if (hbrPattern)
        {
            FillRect(ps.hdc, &rectPaint, hbrPattern);
            DeleteObject(hbrPattern);
        }
    }

    SetBkMode(ps.hdc, iOldBkMode);
    EndPaint(_hwnd, &ps);
}

BOOL CALLBACK PatternDlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fRet = FALSE;
    CPattern *ppat = (CPattern *)GetWindowLong(hdlg, DWL_USER);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        ppat = new CPattern();
        if (ppat)
        {
            SetWindowLong(hdlg, DWL_USER, (LONG)ppat);
        }
        else
        {
            EndDialog(hdlg, -1);
        }
        ppat->_OnInitDialog(hdlg);
        break;

    case WM_COMMAND:
        ppat->_OnCommand(HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
        break;

    case WM_PAINT:
        ppat->_OnPaint();
        break;

    case WM_HELP:
        SHWinHelpOnDemandWrap((HWND)((LPHELPINFO) lParam)->hItemHandle, c_szHelpFile,
                HELP_WM_HELP, (DWORD)aPatternHelpIDs);
        break;

    case WM_CONTEXTMENU:
        SHWinHelpOnDemandWrap((HWND) wParam, c_szHelpFile, HELP_CONTEXTMENU,
                (DWORD)(LPVOID) aPatternHelpIDs);
        break;

    case WM_DESTROY:
        RemoveDefaultDialogFont(hdlg);
        break;
    }

    return fRet;
}

