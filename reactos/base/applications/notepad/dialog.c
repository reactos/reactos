/*
 *  Notepad (dialog.c)
 *
 *  Copyright 1998,99 Marcel Baur <mbaur@g26.ethz.ch>
 *  Copyright 2002 Sylvain Petreolle <spetreolle@yahoo.fr>
 *  Copyright 2002 Andriy Palamarchuk
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "notepad.h"

#include <assert.h>
#include <commctrl.h>
#include <strsafe.h>

LRESULT CALLBACK EDIT_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static const TCHAR helpfile[] = _T("notepad.hlp");
static const TCHAR empty_str[] = _T("");
static const TCHAR szDefaultExt[] = _T("txt");
static const TCHAR txt_files[] = _T("*.txt");

static UINT_PTR CALLBACK DIALOG_PAGESETUP_Hook(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

VOID ShowLastError(VOID)
{
    DWORD error = GetLastError();
    if (error != NO_ERROR)
    {
        LPTSTR lpMsgBuf = NULL;
        TCHAR szTitle[MAX_STRING_LEN];

        LoadString(Globals.hInstance, STRING_ERROR, szTitle, ARRAY_SIZE(szTitle));

        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                      NULL,
                      error,
                      0,
                      (LPTSTR) &lpMsgBuf,
                      0,
                      NULL);

        MessageBox(NULL, lpMsgBuf, szTitle, MB_OK | MB_ICONERROR);
        LocalFree(lpMsgBuf);
    }
}

/**
 * Sets the caption of the main window according to Globals.szFileTitle:
 *    (untitled) - Notepad      if no file is open
 *    [filename] - Notepad      if a file is given
 */
static void UpdateWindowCaption(void)
{
    TCHAR szCaption[MAX_STRING_LEN];
    TCHAR szNotepad[MAX_STRING_LEN];

    LoadString(Globals.hInstance, STRING_NOTEPAD, szNotepad, ARRAY_SIZE(szNotepad));

    if (Globals.szFileTitle[0] != 0)
    {
        StringCchCopy(szCaption, ARRAY_SIZE(szCaption), Globals.szFileTitle);
    }
    else
    {
        LoadString(Globals.hInstance, STRING_UNTITLED, szCaption, ARRAY_SIZE(szCaption));
    }

    StringCchCat(szCaption, ARRAY_SIZE(szCaption), _T(" - "));
    StringCchCat(szCaption, ARRAY_SIZE(szCaption), szNotepad);
    SetWindowText(Globals.hMainWnd, szCaption);
}

int DIALOG_StringMsgBox(HWND hParent, int formatId, LPCTSTR szString, DWORD dwFlags)
{
    TCHAR szMessage[MAX_STRING_LEN];
    TCHAR szResource[MAX_STRING_LEN];

    /* Load and format szMessage */
    LoadString(Globals.hInstance, formatId, szResource, ARRAY_SIZE(szResource));
    _sntprintf(szMessage, ARRAY_SIZE(szMessage), szResource, szString);

    /* Load szCaption */
    if ((dwFlags & MB_ICONMASK) == MB_ICONEXCLAMATION)
        LoadString(Globals.hInstance, STRING_ERROR, szResource, ARRAY_SIZE(szResource));
    else
        LoadString(Globals.hInstance, STRING_NOTEPAD, szResource, ARRAY_SIZE(szResource));

    /* Display Modal Dialog */
    // if (hParent == NULL)
        // hParent = Globals.hMainWnd;
    return MessageBox(hParent, szMessage, szResource, dwFlags);
}

static void AlertFileNotFound(LPCTSTR szFileName)
{
    DIALOG_StringMsgBox(Globals.hMainWnd, STRING_NOTFOUND, szFileName, MB_ICONEXCLAMATION | MB_OK);
}

static int AlertFileNotSaved(LPCTSTR szFileName)
{
    TCHAR szUntitled[MAX_STRING_LEN];

    LoadString(Globals.hInstance, STRING_UNTITLED, szUntitled, ARRAY_SIZE(szUntitled));

    return DIALOG_StringMsgBox(Globals.hMainWnd, STRING_NOTSAVED,
                               szFileName[0] ? szFileName : szUntitled,
                               MB_ICONQUESTION | MB_YESNOCANCEL);
}

static void AlertPrintError(void)
{
    TCHAR szUntitled[MAX_STRING_LEN];

    LoadString(Globals.hInstance, STRING_UNTITLED, szUntitled, ARRAY_SIZE(szUntitled));

    DIALOG_StringMsgBox(Globals.hMainWnd, STRING_NOTSAVED,
                        Globals.szFileName[0] ? Globals.szFileName : szUntitled,
                        MB_ICONEXCLAMATION | MB_OK);
}

/**
 * Returns:
 *   TRUE  - if file exists
 *   FALSE - if file does not exist
 */
BOOL FileExists(LPCTSTR szFilename)
{
    WIN32_FIND_DATA entry;
    HANDLE hFile;

    hFile = FindFirstFile(szFilename, &entry);
    FindClose(hFile);

    return (hFile != INVALID_HANDLE_VALUE);
}

BOOL HasFileExtension(LPCTSTR szFilename)
{
    LPCTSTR s;

    s = _tcsrchr(szFilename, _T('\\'));
    if (s)
        szFilename = s;
    return _tcsrchr(szFilename, _T('.')) != NULL;
}

int GetSelectionTextLength(HWND hWnd)
{
    DWORD dwStart = 0;
    DWORD dwEnd = 0;

    SendMessage(hWnd, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);

    return dwEnd - dwStart;
}

int GetSelectionText(HWND hWnd, LPTSTR lpString, int nMaxCount)
{
    DWORD dwStart = 0;
    DWORD dwEnd = 0;
    DWORD dwSize;
    HRESULT hResult;
    LPTSTR lpTemp;

    if (!lpString)
    {
        return 0;
    }

    SendMessage(hWnd, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);

    if (dwStart == dwEnd)
    {
        return 0;
    }

    dwSize = GetWindowTextLength(hWnd) + 1;
    lpTemp = HeapAlloc(GetProcessHeap(), 0, dwSize * sizeof(TCHAR));
    if (!lpTemp)
    {
        return 0;
    }

    dwSize = GetWindowText(hWnd, lpTemp, dwSize);

    if (!dwSize)
    {
        HeapFree(GetProcessHeap(), 0, lpTemp);
        return 0;
    }

    hResult = StringCchCopyN(lpString, nMaxCount, lpTemp + dwStart, dwEnd - dwStart);
    HeapFree(GetProcessHeap(), 0, lpTemp);

    switch (hResult)
    {
        case S_OK:
        {
            return dwEnd - dwStart;
        }

        case STRSAFE_E_INSUFFICIENT_BUFFER:
        {
            return nMaxCount - 1;
        }

        default:
        {
            return 0;
        }
    }
}

static BOOL DoSaveFile(VOID)
{
    BOOL bRet = TRUE;
    HANDLE hFile;
    LPTSTR pTemp;
    DWORD size;

    hFile = CreateFile(Globals.szFileName, GENERIC_WRITE, FILE_SHARE_WRITE,
                       NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if(hFile == INVALID_HANDLE_VALUE)
    {
        ShowLastError();
        return FALSE;
    }

    size = GetWindowTextLength(Globals.hEdit) + 1;
    pTemp = HeapAlloc(GetProcessHeap(), 0, size * sizeof(*pTemp));
    if (!pTemp)
    {
        CloseHandle(hFile);
        ShowLastError();
        return FALSE;
    }
    size = GetWindowText(Globals.hEdit, pTemp, size);

    if (size)
    {
        if (!WriteText(hFile, (LPWSTR)pTemp, size, Globals.encFile, Globals.iEoln))
        {
            ShowLastError();
            bRet = FALSE;
        }
        else
        {
            SendMessage(Globals.hEdit, EM_SETMODIFY, FALSE, 0);
            bRet = TRUE;
        }
    }

    CloseHandle(hFile);
    HeapFree(GetProcessHeap(), 0, pTemp);
    return bRet;
}

/**
 * Returns:
 *   TRUE  - User agreed to close (both save/don't save)
 *   FALSE - User cancelled close by selecting "Cancel"
 */
BOOL DoCloseFile(VOID)
{
    int nResult;

    if (SendMessage(Globals.hEdit, EM_GETMODIFY, 0, 0))
    {
        /* prompt user to save changes */
        nResult = AlertFileNotSaved(Globals.szFileName);
        switch (nResult)
        {
            case IDYES:
                if(!DIALOG_FileSave())
                    return FALSE;
                break;

            case IDNO:
                break;

            case IDCANCEL:
                return FALSE;

            default:
                return FALSE;
        }
    }

    SetFileName(empty_str);
    UpdateWindowCaption();

    return TRUE;
}

VOID DoOpenFile(LPCTSTR szFileName)
{
    static const TCHAR dotlog[] = _T(".LOG");
    HANDLE hFile;
    LPTSTR pszText = NULL;
    DWORD dwTextLen;
    TCHAR log[5];

    /* Close any files and prompt to save changes */
    if (!DoCloseFile())
        return;

    hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                       OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        ShowLastError();
        goto done;
    }

    if (!ReadText(hFile, (LPWSTR *)&pszText, &dwTextLen, &Globals.encFile, &Globals.iEoln))
    {
        ShowLastError();
        goto done;
    }
    SetWindowText(Globals.hEdit, pszText);

    SendMessage(Globals.hEdit, EM_SETMODIFY, FALSE, 0);
    SendMessage(Globals.hEdit, EM_EMPTYUNDOBUFFER, 0, 0);
    SetFocus(Globals.hEdit);

    /*  If the file starts with .LOG, add a time/date at the end and set cursor after
     *  See http://support.microsoft.com/?kbid=260563
     */
    if (GetWindowText(Globals.hEdit, log, ARRAY_SIZE(log)) && !_tcscmp(log, dotlog))
    {
        static const TCHAR lf[] = _T("\r\n");
        SendMessage(Globals.hEdit, EM_SETSEL, GetWindowTextLength(Globals.hEdit), -1);
        SendMessage(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM)lf);
        DIALOG_EditTimeDate();
        SendMessage(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM)lf);
    }

    SetFileName(szFileName);
    UpdateWindowCaption();
    NOTEPAD_EnableSearchMenu();
done:
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    if (pszText)
        HeapFree(GetProcessHeap(), 0, pszText);
}

VOID DIALOG_FileNew(VOID)
{
    /* Close any files and prompt to save changes */
    if (DoCloseFile()) {
        SetWindowText(Globals.hEdit, empty_str);
        SendMessage(Globals.hEdit, EM_EMPTYUNDOBUFFER, 0, 0);
        SetFocus(Globals.hEdit);
        NOTEPAD_EnableSearchMenu();
    }
}

VOID DIALOG_FileOpen(VOID)
{
    OPENFILENAME openfilename;
    TCHAR szDir[MAX_PATH];
    TCHAR szPath[MAX_PATH];

    ZeroMemory(&openfilename, sizeof(openfilename));

    GetCurrentDirectory(ARRAY_SIZE(szDir), szDir);
    if (Globals.szFileName[0] == 0)
        _tcscpy(szPath, txt_files);
    else
        _tcscpy(szPath, Globals.szFileName);

    openfilename.lStructSize = sizeof(openfilename);
    openfilename.hwndOwner = Globals.hMainWnd;
    openfilename.hInstance = Globals.hInstance;
    openfilename.lpstrFilter = Globals.szFilter;
    openfilename.lpstrFile = szPath;
    openfilename.nMaxFile = ARRAY_SIZE(szPath);
    openfilename.lpstrInitialDir = szDir;
    openfilename.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    openfilename.lpstrDefExt = szDefaultExt;

    if (GetOpenFileName(&openfilename)) {
        if (FileExists(openfilename.lpstrFile))
            DoOpenFile(openfilename.lpstrFile);
        else
            AlertFileNotFound(openfilename.lpstrFile);
    }
}

BOOL DIALOG_FileSave(VOID)
{
    if (Globals.szFileName[0] == 0)
        return DIALOG_FileSaveAs();
    else
        return DoSaveFile();
}

static UINT_PTR
CALLBACK
DIALOG_FileSaveAs_Hook(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    TCHAR szText[128];
    HWND hCombo;

    UNREFERENCED_PARAMETER(wParam);

    switch(msg)
    {
        case WM_INITDIALOG:
            hCombo = GetDlgItem(hDlg, ID_ENCODING);

            LoadString(Globals.hInstance, STRING_ANSI, szText, ARRAY_SIZE(szText));
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM) szText);

            LoadString(Globals.hInstance, STRING_UNICODE, szText, ARRAY_SIZE(szText));
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM) szText);

            LoadString(Globals.hInstance, STRING_UNICODE_BE, szText, ARRAY_SIZE(szText));
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM) szText);

            LoadString(Globals.hInstance, STRING_UTF8, szText, ARRAY_SIZE(szText));
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM) szText);

            SendMessage(hCombo, CB_SETCURSEL, Globals.encFile, 0);

            hCombo = GetDlgItem(hDlg, ID_EOLN);

            LoadString(Globals.hInstance, STRING_CRLF, szText, ARRAY_SIZE(szText));
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM) szText);

            LoadString(Globals.hInstance, STRING_LF, szText, ARRAY_SIZE(szText));
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM) szText);

            LoadString(Globals.hInstance, STRING_CR, szText, ARRAY_SIZE(szText));
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM) szText);

            SendMessage(hCombo, CB_SETCURSEL, Globals.iEoln, 0);
            break;

        case WM_NOTIFY:
            if (((NMHDR *) lParam)->code == CDN_FILEOK)
            {
                hCombo = GetDlgItem(hDlg, ID_ENCODING);
                if (hCombo)
                    Globals.encFile = (int) SendMessage(hCombo, CB_GETCURSEL, 0, 0);

                hCombo = GetDlgItem(hDlg, ID_EOLN);
                if (hCombo)
                    Globals.iEoln = (int) SendMessage(hCombo, CB_GETCURSEL, 0, 0);
            }
            break;
    }
    return 0;
}

BOOL DIALOG_FileSaveAs(VOID)
{
    OPENFILENAME saveas;
    TCHAR szDir[MAX_PATH];
    TCHAR szPath[MAX_PATH];

    ZeroMemory(&saveas, sizeof(saveas));

    GetCurrentDirectory(ARRAY_SIZE(szDir), szDir);
    if (Globals.szFileName[0] == 0)
        _tcscpy(szPath, txt_files);
    else
        _tcscpy(szPath, Globals.szFileName);

    saveas.lStructSize = sizeof(OPENFILENAME);
    saveas.hwndOwner = Globals.hMainWnd;
    saveas.hInstance = Globals.hInstance;
    saveas.lpstrFilter = Globals.szFilter;
    saveas.lpstrFile = szPath;
    saveas.nMaxFile = ARRAY_SIZE(szPath);
    saveas.lpstrInitialDir = szDir;
    saveas.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY |
                   OFN_EXPLORER | OFN_ENABLETEMPLATE | OFN_ENABLEHOOK;
    saveas.lpstrDefExt = szDefaultExt;
    saveas.lpTemplateName = MAKEINTRESOURCE(DIALOG_ENCODING);
    saveas.lpfnHook = DIALOG_FileSaveAs_Hook;

    if (GetSaveFileName(&saveas))
    {
        /* HACK: Because in ROS, Save-As boxes don't check the validity
         * of file names and thus, here, szPath can be invalid !! We only
         * see its validity when we call DoSaveFile()... */
        SetFileName(szPath);
        if (DoSaveFile())
        {
            UpdateWindowCaption();
            return TRUE;
        }
        else
        {
            SetFileName(_T(""));
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
}

VOID DIALOG_FilePrint(VOID)
{
    DOCINFO di;
    PRINTDLG printer;
    SIZE szMetric;
    int cWidthPels, cHeightPels, border;
    int xLeft, yTop, pagecount, dopage, copycount;
    unsigned int i;
    LOGFONT hdrFont;
    HFONT font, old_font=0;
    DWORD size;
    LPTSTR pTemp;
    static const TCHAR times_new_roman[] = _T("Times New Roman");

    /* Get a small font and print some header info on each page */
    ZeroMemory(&hdrFont, sizeof(hdrFont));
    hdrFont.lfHeight = 100;
    hdrFont.lfWeight = FW_BOLD;
    hdrFont.lfCharSet = ANSI_CHARSET;
    hdrFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
    hdrFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    hdrFont.lfQuality = PROOF_QUALITY;
    hdrFont.lfPitchAndFamily = VARIABLE_PITCH | FF_ROMAN;
    _tcscpy(hdrFont.lfFaceName, times_new_roman);

    font = CreateFontIndirect(&hdrFont);

    /* Get Current Settings */
    ZeroMemory(&printer, sizeof(printer));
    printer.lStructSize = sizeof(printer);
    printer.hwndOwner = Globals.hMainWnd;
    printer.hInstance = Globals.hInstance;

    /* Set some default flags */
    printer.Flags = PD_RETURNDC | PD_SELECTION;

    /* Disable the selection radio button if there is no text selected */
    if (!GetSelectionTextLength(Globals.hEdit))
    {
        printer.Flags = printer.Flags | PD_NOSELECTION;
    }

    printer.nFromPage = 0;
    printer.nMinPage = 1;
    /* we really need to calculate number of pages to set nMaxPage and nToPage */
    printer.nToPage = 0;
    printer.nMaxPage = (WORD)-1;

    /* Let commdlg manage copy settings */
    printer.nCopies = (WORD)PD_USEDEVMODECOPIES;

    printer.hDevMode = Globals.hDevMode;
    printer.hDevNames = Globals.hDevNames;

    if (!PrintDlg(&printer))
    {
        DeleteObject(font);
        return;
    }

    Globals.hDevMode = printer.hDevMode;
    Globals.hDevNames = printer.hDevNames;

    assert(printer.hDC != 0);

    /* initialize DOCINFO */
    di.cbSize = sizeof(DOCINFO);
    di.lpszDocName = Globals.szFileTitle;
    di.lpszOutput = NULL;
    di.lpszDatatype = NULL;
    di.fwType = 0;

    if (StartDoc(printer.hDC, &di) <= 0)
    {
        DeleteObject(font);
        return;
    }

    /* Get the page dimensions in pixels. */
    cWidthPels = GetDeviceCaps(printer.hDC, HORZRES);
    cHeightPels = GetDeviceCaps(printer.hDC, VERTRES);

    /* Get the file text */
    if (printer.Flags & PD_SELECTION)
    {
        size = GetSelectionTextLength(Globals.hEdit) + 1;
    }
    else
    {
        size = GetWindowTextLength(Globals.hEdit) + 1;
    }

    pTemp = HeapAlloc(GetProcessHeap(), 0, size * sizeof(TCHAR));
    if (!pTemp)
    {
        EndDoc(printer.hDC);
        DeleteObject(font);
        ShowLastError();
        return;
    }

    if (printer.Flags & PD_SELECTION)
    {
        size = GetSelectionText(Globals.hEdit, pTemp, size);
    }
    else
    {
        size = GetWindowText(Globals.hEdit, pTemp, size);
    }

    border = 150;
    for (copycount=1; copycount <= printer.nCopies; copycount++) {
        i = 0;
        pagecount = 1;
        do {
            static const TCHAR letterM[] = _T("M");

            if (pagecount >= printer.nFromPage &&
    /*          ((printer.Flags & PD_PAGENUMS) == 0 ||  pagecount <= printer.nToPage))*/
            pagecount <= printer.nToPage)
                dopage = 1;
            else
                dopage = 0;

            old_font = SelectObject(printer.hDC, font);
            GetTextExtentPoint32(printer.hDC, letterM, 1, &szMetric);

            if (dopage) {
                if (StartPage(printer.hDC) <= 0) {
                    SelectObject(printer.hDC, old_font);
                    EndDoc(printer.hDC);
                    DeleteDC(printer.hDC);
                    HeapFree(GetProcessHeap(), 0, pTemp);
                    DeleteObject(font);
                    AlertPrintError();
                    return;
                }
                /* Write a rectangle and header at the top of each page */
                Rectangle(printer.hDC, border, border, cWidthPels-border, border+szMetric.cy*2);
                /* I don't know what's up with this TextOut command. This comes out
                kind of mangled.
                */
                TextOut(printer.hDC,
                        border * 2,
                        border + szMetric.cy / 2,
                        Globals.szFileTitle,
                        lstrlen(Globals.szFileTitle));
            }

            /* The starting point for the main text */
            xLeft = border * 2;
            yTop = border + szMetric.cy * 4;

            SelectObject(printer.hDC, old_font);
            GetTextExtentPoint32(printer.hDC, letterM, 1, &szMetric);

            /* Since outputting strings is giving me problems, output the main
             * text one character at a time. */
            do {
                if (pTemp[i] == '\n') {
                    xLeft = border * 2;
                    yTop += szMetric.cy;
                }
                else if (pTemp[i] != '\r') {
                    if (dopage)
                        TextOut(printer.hDC, xLeft, yTop, &pTemp[i], 1);
                    xLeft += szMetric.cx;
                }
            } while (i++ < size && yTop < (cHeightPels - border * 2));

            if (dopage)
                EndPage(printer.hDC);
            pagecount++;
        } while (i < size);
    }

    if (old_font != 0)
        SelectObject(printer.hDC, old_font);
    EndDoc(printer.hDC);
    DeleteDC(printer.hDC);
    HeapFree(GetProcessHeap(), 0, pTemp);
    DeleteObject(font);
}

VOID DIALOG_FileExit(VOID)
{
    PostMessage(Globals.hMainWnd, WM_CLOSE, 0, 0l);
}

VOID DIALOG_EditUndo(VOID)
{
    SendMessage(Globals.hEdit, EM_UNDO, 0, 0);
}

VOID DIALOG_EditCut(VOID)
{
    SendMessage(Globals.hEdit, WM_CUT, 0, 0);
}

VOID DIALOG_EditCopy(VOID)
{
    SendMessage(Globals.hEdit, WM_COPY, 0, 0);
}

VOID DIALOG_EditPaste(VOID)
{
    SendMessage(Globals.hEdit, WM_PASTE, 0, 0);
}

VOID DIALOG_EditDelete(VOID)
{
    SendMessage(Globals.hEdit, WM_CLEAR, 0, 0);
}

VOID DIALOG_EditSelectAll(VOID)
{
    SendMessage(Globals.hEdit, EM_SETSEL, 0, (LPARAM)-1);
}

VOID DIALOG_EditTimeDate(VOID)
{
    SYSTEMTIME st;
    TCHAR szDate[MAX_STRING_LEN];
    TCHAR szText[MAX_STRING_LEN * 2 + 2];

    GetLocalTime(&st);

    GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, szDate, MAX_STRING_LEN);
    _tcscpy(szText, szDate);
    _tcscat(szText, _T(" "));
    GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL, szDate, MAX_STRING_LEN);
    _tcscat(szText, szDate);
    SendMessage(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM)szText);
}

VOID DoCreateStatusBar(VOID)
{
    RECT rc;
    RECT rcstatus;
    BOOL bStatusBarVisible;

    /* Check if status bar object already exists. */
    if (Globals.hStatusBar == NULL)
    {
        /* Try to create the status bar */
        Globals.hStatusBar = CreateStatusWindow(WS_CHILD | WS_VISIBLE | WS_EX_STATICEDGE,
                                                NULL,
                                                Globals.hMainWnd,
                                                CMD_STATUSBAR_WND_ID);

        if (Globals.hStatusBar == NULL)
        {
            ShowLastError();
            return;
        }

        /* Load the string for formatting column/row text output */
        LoadString(Globals.hInstance, STRING_LINE_COLUMN, Globals.szStatusBarLineCol, MAX_PATH - 1);

        /* Set the status bar for single-text output */
        SendMessage(Globals.hStatusBar, SB_SIMPLE, (WPARAM)TRUE, (LPARAM)0);
    }

    /* Set status bar visiblity according to the settings. */
    if (Globals.bWrapLongLines == TRUE || Globals.bShowStatusBar == FALSE)
    {
        bStatusBarVisible = FALSE;
        ShowWindow(Globals.hStatusBar, SW_HIDE);
    }
    else
    {
        bStatusBarVisible = TRUE;
        ShowWindow(Globals.hStatusBar, SW_SHOW);
        SendMessage(Globals.hStatusBar, WM_SIZE, 0, 0);
    }

    /* Set check state in show status bar item. */
    if (bStatusBarVisible)
    {
        CheckMenuItem(Globals.hMenu, CMD_STATUSBAR, MF_BYCOMMAND | MF_CHECKED);
    }
    else
    {
        CheckMenuItem(Globals.hMenu, CMD_STATUSBAR, MF_BYCOMMAND | MF_UNCHECKED);
    }

    /* Update menu mar with the previous changes */
    DrawMenuBar(Globals.hMainWnd);

    /* Sefety test is edit control exists */
    if (Globals.hEdit != NULL)
    {
        /* Retrieve the sizes of the controls */
        GetClientRect(Globals.hMainWnd, &rc);
        GetClientRect(Globals.hStatusBar, &rcstatus);

        /* If status bar is currently visible, update dimensions of edit control */
        if (bStatusBarVisible)
            rc.bottom -= (rcstatus.bottom - rcstatus.top);

        /* Resize edit control to right size. */
        MoveWindow(Globals.hEdit,
                   rc.left,
                   rc.top,
                   rc.right - rc.left,
                   rc.bottom - rc.top,
                   TRUE);
    }

    /* Update content with current row/column text */
    DIALOG_StatusBarUpdateCaretPos();
}

VOID DoCreateEditWindow(VOID)
{
    DWORD dwStyle;
    int iSize;
    LPTSTR pTemp = NULL;
    BOOL bModified = FALSE;

    iSize = 0;

    /* If the edit control already exists, try to save its content */
    if (Globals.hEdit != NULL)
    {
        /* number of chars currently written into the editor. */
        iSize = GetWindowTextLength(Globals.hEdit);
        if (iSize)
        {
            /* Allocates temporary buffer. */
            pTemp = HeapAlloc(GetProcessHeap(), 0, (iSize + 1) * sizeof(TCHAR));
            if (!pTemp)
            {
                ShowLastError();
                return;
            }

            /* Recover the text into the control. */
            GetWindowText(Globals.hEdit, pTemp, iSize + 1);

            if (SendMessage(Globals.hEdit, EM_GETMODIFY, 0, 0))
                bModified = TRUE;
        }

        /* Restore original window procedure */
        SetWindowLongPtr(Globals.hEdit, GWLP_WNDPROC, (LONG_PTR)Globals.EditProc);

        /* Destroy the edit control */
        DestroyWindow(Globals.hEdit);
    }

    /* Update wrap status into the main menu and recover style flags */
    if (Globals.bWrapLongLines)
    {
        dwStyle = EDIT_STYLE_WRAP;
        EnableMenuItem(Globals.hMenu, CMD_STATUSBAR, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
    } else {
        dwStyle = EDIT_STYLE;
        EnableMenuItem(Globals.hMenu, CMD_STATUSBAR, MF_BYCOMMAND | MF_ENABLED);
    }

    /* Update previous changes */
    DrawMenuBar(Globals.hMainWnd);

    /* Create the new edit control */
    Globals.hEdit = CreateWindowEx(WS_EX_CLIENTEDGE,
                                   EDIT_CLASS,
                                   NULL,
                                   dwStyle,
                                   CW_USEDEFAULT,
                                   CW_USEDEFAULT,
                                   CW_USEDEFAULT,
                                   CW_USEDEFAULT,
                                   Globals.hMainWnd,
                                   NULL,
                                   Globals.hInstance,
                                   NULL);

    if (Globals.hEdit == NULL)
    {
        if (pTemp)
        {
            HeapFree(GetProcessHeap(), 0, pTemp);
        }

        ShowLastError();
        return;
    }

    SendMessage(Globals.hEdit, WM_SETFONT, (WPARAM)Globals.hFont, FALSE);
    SendMessage(Globals.hEdit, EM_LIMITTEXT, 0, 0);

    /* If some text was previously saved, restore it. */
    if (iSize != 0)
    {
        SetWindowText(Globals.hEdit, pTemp);
        HeapFree(GetProcessHeap(), 0, pTemp);

        if (bModified)
            SendMessage(Globals.hEdit, EM_SETMODIFY, TRUE, 0);
    }

    /* Sub-class a new window callback for row/column detection. */
    Globals.EditProc = (WNDPROC)SetWindowLongPtr(Globals.hEdit,
                                                 GWLP_WNDPROC,
                                                 (LONG_PTR)EDIT_WndProc);

    /* Create/update status bar */
    DoCreateStatusBar();

    /* Finally shows new edit control and set focus into it. */
    ShowWindow(Globals.hEdit, SW_SHOW);
    SetFocus(Globals.hEdit);
}

VOID DIALOG_EditWrap(VOID)
{
    Globals.bWrapLongLines = !Globals.bWrapLongLines;
    DoCreateEditWindow();
}

VOID DIALOG_SelectFont(VOID)
{
    CHOOSEFONT cf;
    LOGFONT lf = Globals.lfFont;

    ZeroMemory( &cf, sizeof(cf) );
    cf.lStructSize = sizeof(cf);
    cf.hwndOwner = Globals.hMainWnd;
    cf.lpLogFont = &lf;
    cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;

    if (ChooseFont(&cf))
    {
        HFONT currfont = Globals.hFont;

        Globals.hFont = CreateFontIndirect(&lf);
        Globals.lfFont = lf;
        SendMessage(Globals.hEdit, WM_SETFONT, (WPARAM)Globals.hFont, (LPARAM)TRUE);
        if (currfont != NULL)
            DeleteObject(currfont);
    }
}

typedef HWND (WINAPI *FINDPROC)(LPFINDREPLACE lpfr);

static VOID DIALOG_SearchDialog(FINDPROC pfnProc)
{
    ZeroMemory(&Globals.find, sizeof(Globals.find));
    Globals.find.lStructSize = sizeof(Globals.find);
    Globals.find.hwndOwner = Globals.hMainWnd;
    Globals.find.hInstance = Globals.hInstance;
    Globals.find.lpstrFindWhat = Globals.szFindText;
    Globals.find.wFindWhatLen = ARRAY_SIZE(Globals.szFindText);
    Globals.find.lpstrReplaceWith = Globals.szReplaceText;
    Globals.find.wReplaceWithLen = ARRAY_SIZE(Globals.szReplaceText);
    Globals.find.Flags = FR_DOWN;

    /* We only need to create the modal FindReplace dialog which will */
    /* notify us of incoming events using hMainWnd Window Messages    */

    Globals.hFindReplaceDlg = pfnProc(&Globals.find);
    assert(Globals.hFindReplaceDlg != 0);
}

VOID DIALOG_Search(VOID)
{
    DIALOG_SearchDialog(FindText);
}

VOID DIALOG_SearchNext(VOID)
{
    if (Globals.find.lpstrFindWhat != NULL)
        NOTEPAD_FindNext(&Globals.find, FALSE, TRUE);
    else
        DIALOG_Search();
}

VOID DIALOG_Replace(VOID)
{
    DIALOG_SearchDialog(ReplaceText);
}

static INT_PTR
CALLBACK
DIALOG_GoTo_DialogProc(HWND hwndDialog, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL bResult = FALSE;
    HWND hTextBox;
    TCHAR szText[32];

    switch(uMsg) {
    case WM_INITDIALOG:
        hTextBox = GetDlgItem(hwndDialog, ID_LINENUMBER);
        _sntprintf(szText, ARRAY_SIZE(szText), _T("%ld"), lParam);
        SetWindowText(hTextBox, szText);
        break;
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            if (LOWORD(wParam) == IDOK)
            {
                hTextBox = GetDlgItem(hwndDialog, ID_LINENUMBER);
                GetWindowText(hTextBox, szText, ARRAY_SIZE(szText));
                EndDialog(hwndDialog, _ttoi(szText));
                bResult = TRUE;
            }
            else if (LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hwndDialog, 0);
                bResult = TRUE;
            }
        }
        break;
    }

    return bResult;
}

VOID DIALOG_GoTo(VOID)
{
    INT_PTR nLine;
    LPTSTR pszText;
    int nLength, i;
    DWORD dwStart, dwEnd;

    nLength = GetWindowTextLength(Globals.hEdit);
    pszText = (LPTSTR) HeapAlloc(GetProcessHeap(), 0, (nLength + 1) * sizeof(*pszText));
    if (!pszText)
        return;

    /* Retrieve current text */
    GetWindowText(Globals.hEdit, pszText, nLength + 1);
    SendMessage(Globals.hEdit, EM_GETSEL, (WPARAM) &dwStart, (LPARAM) &dwEnd);

    nLine = 1;
    for (i = 0; pszText[i] && (i < (int) dwStart); i++)
    {
        if (pszText[i] == '\n')
            nLine++;
    }

    nLine = DialogBoxParam(Globals.hInstance,
                           MAKEINTRESOURCE(DIALOG_GOTO),
                           Globals.hMainWnd,
                           DIALOG_GoTo_DialogProc,
                           nLine);

    if (nLine >= 1)
    {
        for (i = 0; pszText[i] && (nLine > 1) && (i < nLength - 1); i++)
        {
            if (pszText[i] == '\n')
                nLine--;
        }
        SendMessage(Globals.hEdit, EM_SETSEL, i, i);
        SendMessage(Globals.hEdit, EM_SCROLLCARET, 0, 0);
    }
    HeapFree(GetProcessHeap(), 0, pszText);
}

VOID DIALOG_StatusBarUpdateCaretPos(VOID)
{
    int line, col;
    TCHAR buff[MAX_PATH];
    DWORD dwStart, dwSize;

    SendMessage(Globals.hEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwSize);
    line = SendMessage(Globals.hEdit, EM_LINEFROMCHAR, (WPARAM)dwStart, 0);
    col = dwStart - SendMessage(Globals.hEdit, EM_LINEINDEX, (WPARAM)line, 0);

    _stprintf(buff, Globals.szStatusBarLineCol, line + 1, col + 1);
    SendMessage(Globals.hStatusBar, SB_SETTEXT, SB_SIMPLEID, (LPARAM)buff);
}

VOID DIALOG_ViewStatusBar(VOID)
{
    Globals.bShowStatusBar = !Globals.bShowStatusBar;

    DoCreateStatusBar();
}

VOID DIALOG_HelpContents(VOID)
{
    WinHelp(Globals.hMainWnd, helpfile, HELP_INDEX, 0);
}

VOID DIALOG_HelpSearch(VOID)
{
    /* Search Help */
}

VOID DIALOG_HelpHelp(VOID)
{
    WinHelp(Globals.hMainWnd, helpfile, HELP_HELPONHELP, 0);
}

VOID DIALOG_HelpAboutNotepad(VOID)
{
    TCHAR szNotepad[MAX_STRING_LEN];
    HICON notepadIcon = LoadIcon(Globals.hInstance, MAKEINTRESOURCE(IDI_NPICON));

    LoadString(Globals.hInstance, STRING_NOTEPAD, szNotepad, ARRAY_SIZE(szNotepad));
    ShellAbout(Globals.hMainWnd, szNotepad, 0, notepadIcon);
    DeleteObject(notepadIcon);
}

INT_PTR
CALLBACK
AboutDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hLicenseEditWnd;
    TCHAR *strLicense;

    switch (message)
    {
    case WM_INITDIALOG:

        hLicenseEditWnd = GetDlgItem(hDlg, IDC_LICENSE);

        /* 0x1000 should be enough */
        strLicense = (TCHAR *)_alloca(0x1000);
        LoadString(GetModuleHandle(NULL), STRING_LICENSE, strLicense, 0x1000);

        SetWindowText(hLicenseEditWnd, strLicense);

        return TRUE;

    case WM_COMMAND:

        if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL))
        {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }

        break;
    }

    return 0;
}

/***********************************************************************
 *
 *           DIALOG_FilePageSetup
 */
VOID DIALOG_FilePageSetup(void)
{
    PAGESETUPDLG page;

    ZeroMemory(&page, sizeof(page));
    page.lStructSize = sizeof(page);
    page.hwndOwner = Globals.hMainWnd;
    page.Flags = PSD_ENABLEPAGESETUPTEMPLATE | PSD_ENABLEPAGESETUPHOOK | PSD_MARGINS;
    page.hInstance = Globals.hInstance;
    page.rtMargin.left = Globals.lMarginLeft;
    page.rtMargin.top = Globals.lMarginTop;
    page.rtMargin.right = Globals.lMarginRight;
    page.rtMargin.bottom = Globals.lMarginBottom;
    page.hDevMode = Globals.hDevMode;
    page.hDevNames = Globals.hDevNames;
    page.lpPageSetupTemplateName = MAKEINTRESOURCE(DIALOG_PAGESETUP);
    page.lpfnPageSetupHook = DIALOG_PAGESETUP_Hook;

    PageSetupDlg(&page);

    Globals.hDevMode = page.hDevMode;
    Globals.hDevNames = page.hDevNames;
    Globals.lMarginLeft = page.rtMargin.left;
    Globals.lMarginTop = page.rtMargin.top;
    Globals.lMarginRight = page.rtMargin.right;
    Globals.lMarginBottom = page.rtMargin.bottom;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *           DIALOG_PAGESETUP_Hook
 */

static UINT_PTR CALLBACK DIALOG_PAGESETUP_Hook(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            switch (LOWORD(wParam))
            {
            case IDOK:
                /* save user input and close dialog */
                GetDlgItemText(hDlg, 0x141, Globals.szHeader, ARRAY_SIZE(Globals.szHeader));
                GetDlgItemText(hDlg, 0x143, Globals.szFooter, ARRAY_SIZE(Globals.szFooter));
                return FALSE;

            case IDCANCEL:
                /* discard user input and close dialog */
                return FALSE;

            case IDHELP:
                {
                    /* FIXME: Bring this to work */
                    static const TCHAR sorry[] = _T("Sorry, no help available");
                    static const TCHAR help[] = _T("Help");
                    MessageBox(Globals.hMainWnd, sorry, help, MB_ICONEXCLAMATION);
                    return TRUE;
                }

            default:
                break;
            }
        }
        break;

    case WM_INITDIALOG:
        /* fetch last user input prior to display dialog */
        SetDlgItemText(hDlg, 0x141, Globals.szHeader);
        SetDlgItemText(hDlg, 0x143, Globals.szFooter);
        break;
    }

    return FALSE;
}
