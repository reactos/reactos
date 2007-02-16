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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <notepad.h>

static const WCHAR helpfileW[] = { 'n','o','t','e','p','a','d','.','h','l','p',0 };

static INT_PTR WINAPI DIALOG_PAGESETUP_DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

VOID ShowLastError(void)
{
    DWORD error = GetLastError();
    if (error != NO_ERROR)
    {
        LPWSTR lpMsgBuf;
        WCHAR szTitle[MAX_STRING_LEN];

        LoadString(Globals.hInstance, STRING_ERROR, szTitle, SIZEOF(szTitle));
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, error, 0,
            (LPTSTR) &lpMsgBuf, 0, NULL);
        MessageBox(NULL, lpMsgBuf, szTitle, MB_OK | MB_ICONERROR);
        LocalFree(lpMsgBuf);
    }
}

/**
 * Sets the caption of the main window according to Globals.szFileTitle:
 *    Notepad - (untitled)      if no file is open
 *    Notepad - [filename]      if a file is given
 */
static void UpdateWindowCaption(void)
{
  WCHAR szCaption[MAX_STRING_LEN];
  WCHAR szUntitled[MAX_STRING_LEN];

  LoadString(Globals.hInstance, STRING_NOTEPAD, szCaption, SIZEOF(szCaption));

  if (Globals.szFileTitle[0] != '\0') {
      static const WCHAR bracket_lW[] = { ' ','-',' ','[',0 };
      static const WCHAR bracket_rW[] = { ']',0 };
      lstrcat(szCaption, bracket_lW);
      lstrcat(szCaption, Globals.szFileTitle);
      lstrcat(szCaption, bracket_rW);
  }
  else
  {
      static const WCHAR hyphenW[] = { ' ','-',' ',0 };
      LoadString(Globals.hInstance, STRING_UNTITLED, szUntitled, SIZEOF(szUntitled));
      lstrcat(szCaption, hyphenW);
      lstrcat(szCaption, szUntitled);
  }

  SetWindowText(Globals.hMainWnd, szCaption);
}

static void AlertFileNotFound(LPCWSTR szFileName)
{
   WCHAR szMessage[MAX_STRING_LEN];
   WCHAR szResource[MAX_STRING_LEN];

   /* Load and format szMessage */
   LoadString(Globals.hInstance, STRING_NOTFOUND, szResource, SIZEOF(szResource));
   wsprintf(szMessage, szResource, szFileName);

   /* Load szCaption */
   LoadString(Globals.hInstance, STRING_NOTEPAD,  szResource, SIZEOF(szResource));

   /* Display Modal Dialog */
   MessageBox(Globals.hMainWnd, szMessage, szResource, MB_ICONEXCLAMATION);
}

static int AlertFileNotSaved(LPCWSTR szFileName)
{
   WCHAR szMessage[MAX_STRING_LEN];
   WCHAR szResource[MAX_STRING_LEN];
   WCHAR szUntitled[MAX_STRING_LEN];

   LoadString(Globals.hInstance, STRING_UNTITLED, szUntitled, SIZEOF(szUntitled));

   /* Load and format Message */
   LoadString(Globals.hInstance, STRING_NOTSAVED, szResource, SIZEOF(szResource));
   wsprintf(szMessage, szResource, szFileName[0] ? szFileName : szUntitled);

   /* Load Caption */
   LoadString(Globals.hInstance, STRING_NOTEPAD, szResource, SIZEOF(szResource));

   /* Display modal */
   return MessageBox(Globals.hMainWnd, szMessage, szResource, MB_ICONEXCLAMATION|MB_YESNOCANCEL);
}

/**
 * Returns:
 *   TRUE  - if file exists
 *   FALSE - if file does not exist
 */
BOOL FileExists(LPCWSTR szFilename)
{
   WIN32_FIND_DATA entry;
   HANDLE hFile;

   hFile = FindFirstFile(szFilename, &entry);
   FindClose(hFile);

   return (hFile != INVALID_HANDLE_VALUE);
}


BOOL HasFileExtension(LPCWSTR szFilename)
{
    LPCWSTR s;

    s = wcsrchr(szFilename, '\\');
    if (s)
        szFilename = s;
    return wcsrchr(szFilename, '.') != NULL;
}


static VOID DoSaveFile(VOID)
{
    HANDLE hFile;
    LPWSTR pTemp;
    DWORD size;

    hFile = CreateFile(Globals.szFileName, GENERIC_WRITE, FILE_SHARE_WRITE,
                       NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if(hFile == INVALID_HANDLE_VALUE)
    {
        ShowLastError();
        return;
    }

    size = GetWindowTextLengthW(Globals.hEdit) + 1;
    pTemp = HeapAlloc(GetProcessHeap(), 0, size * sizeof(*pTemp));
    if (!pTemp)
    {
        CloseHandle(hFile);
        ShowLastError();
        return;
    }
    size = GetWindowTextW(Globals.hEdit, pTemp, size);

    if (!WriteText(hFile, pTemp, size, Globals.iEncoding, Globals.iEoln))
        ShowLastError();
    else
        SendMessage(Globals.hEdit, EM_SETMODIFY, FALSE, 0);

    CloseHandle(hFile);
    HeapFree(GetProcessHeap(), 0, pTemp);
}

/**
 * Returns:
 *   TRUE  - User agreed to close (both save/don't save)
 *   FALSE - User cancelled close by selecting "Cancel"
 */
BOOL DoCloseFile(void)
{
    int nResult;
    static const WCHAR empty_strW[] = { 0 };

    if (SendMessage(Globals.hEdit, EM_GETMODIFY, 0, 0))
    {
        /* prompt user to save changes */
        nResult = AlertFileNotSaved(Globals.szFileName);
        switch (nResult) {
            case IDYES:     DIALOG_FileSave();
                            break;

            case IDNO:      break;

            case IDCANCEL:  return(FALSE);
                            break;

            default:        return(FALSE);
                            break;
        } /* switch */
    } /* if */

    SetFileName(empty_strW);

    UpdateWindowCaption();
    return(TRUE);
}


void DoOpenFile(LPCWSTR szFileName)
{
    static const WCHAR dotlog[] = { '.','L','O','G',0 };
    HANDLE hFile;
    LPWSTR pszText;
    DWORD dwTextLen;
    WCHAR log[5];

    /* Close any files and prompt to save changes */
    if (!DoCloseFile())
        return;

    hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        ShowLastError();
        goto done;
    }

    if (!ReadText(hFile, &pszText, &dwTextLen, &Globals.iEncoding, &Globals.iEoln))
    {
        ShowLastError();
        goto done;
    }

    SetWindowTextW(Globals.hEdit, pszText);

    SendMessage(Globals.hEdit, EM_SETMODIFY, FALSE, 0);
    SendMessage(Globals.hEdit, EM_EMPTYUNDOBUFFER, 0, 0);
    SetFocus(Globals.hEdit);

    /*  If the file starts with .LOG, add a time/date at the end and set cursor after
     *  See http://support.microsoft.com/?kbid=260563
     */
    if (GetWindowTextW(Globals.hEdit, log, sizeof(log)/sizeof(log[0])) && !lstrcmp(log, dotlog))
    {
	static const WCHAR lfW[] = { '\r','\n',0 };
	SendMessage(Globals.hEdit, EM_SETSEL, GetWindowTextLength(Globals.hEdit), -1);
	SendMessage(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM)lfW);
	DIALOG_EditTimeDate();
	SendMessage(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM)lfW);
    }

    SetFileName(szFileName);
    UpdateWindowCaption();

done:
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    if (pszText)
        HeapFree(GetProcessHeap(), 0, pszText);
}

VOID DIALOG_FileNew(VOID)
{
    static const WCHAR empty_strW[] = { 0 };

    /* Close any files and prompt to save changes */
    if (DoCloseFile()) {
        SetWindowText(Globals.hEdit, empty_strW);
        SendMessage(Globals.hEdit, EM_EMPTYUNDOBUFFER, 0, 0);
        SetFocus(Globals.hEdit);
    }
}

VOID DIALOG_FileOpen(VOID)
{
    OPENFILENAME openfilename;
    WCHAR szDir[MAX_PATH];
    WCHAR szPath[MAX_PATH];
    static const WCHAR szDefaultExt[] = { 't','x','t',0 };
    static const WCHAR txt_files[] = { '*','.','t','x','t',0 };

    ZeroMemory(&openfilename, sizeof(openfilename));

    GetCurrentDirectory(SIZEOF(szDir), szDir);
    if (Globals.szFileName[0] == 0)
        lstrcpy(szPath, txt_files);
    else
        lstrcpy(szPath, Globals.szFileName);

    openfilename.lStructSize       = sizeof(openfilename);
    openfilename.hwndOwner         = Globals.hMainWnd;
    openfilename.hInstance         = Globals.hInstance;
    openfilename.lpstrFilter       = Globals.szFilter;
    openfilename.lpstrFile         = szPath;
    openfilename.nMaxFile          = SIZEOF(szPath);
    openfilename.lpstrInitialDir   = szDir;
    openfilename.Flags             = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST |
        OFN_HIDEREADONLY;
    openfilename.lpstrDefExt       = szDefaultExt;


    if (GetOpenFileName(&openfilename)) {
        if (FileExists(openfilename.lpstrFile))
            DoOpenFile(openfilename.lpstrFile);
        else
            AlertFileNotFound(openfilename.lpstrFile);
    }
}


VOID DIALOG_FileSave(VOID)
{
    if (Globals.szFileName[0] == '\0')
        DIALOG_FileSaveAs();
    else
        DoSaveFile();
}

static UINT_PTR CALLBACK DIALOG_FileSaveAs_Hook(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WCHAR szText[128];
    HWND hCombo;
    OFNOTIFY *pNotify;

    UNREFERENCED_PARAMETER(wParam);

    switch(msg)
    {
        case WM_INITDIALOG:
            hCombo = GetDlgItem(hDlg, ID_ENCODING);

            LoadString(Globals.hInstance, STRING_ANSI, szText, SIZEOF(szText));
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM) szText);

            LoadString(Globals.hInstance, STRING_UNICODE, szText, SIZEOF(szText));
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM) szText);

            LoadString(Globals.hInstance, STRING_UNICODE_BE, szText, SIZEOF(szText));
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM) szText);

            LoadString(Globals.hInstance, STRING_UTF8, szText, SIZEOF(szText));
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM) szText);

            SendMessage(hCombo, CB_SETCURSEL, Globals.iEncoding, 0);

            hCombo = GetDlgItem(hDlg, ID_EOLN);

            LoadString(Globals.hInstance, STRING_CRLF, szText, SIZEOF(szText));
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM) szText);

            LoadString(Globals.hInstance, STRING_LF, szText, SIZEOF(szText));
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM) szText);

            LoadString(Globals.hInstance, STRING_CR, szText, SIZEOF(szText));
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM) szText);

            SendMessage(hCombo, CB_SETCURSEL, Globals.iEoln, 0);
            break;

        case WM_NOTIFY:
            if (((NMHDR *) lParam)->code == CDN_FILEOK)
            {
                pNotify = (OFNOTIFY *) lParam;

                hCombo = GetDlgItem(hDlg, ID_ENCODING);
				if (hCombo)
	                Globals.iEncoding = (int) SendMessage(hCombo, CB_GETCURSEL, 0, 0);

                hCombo = GetDlgItem(hDlg, ID_EOLN);
				if (hCombo)
	                Globals.iEoln = (int) SendMessage(hCombo, CB_GETCURSEL, 0, 0);
            }
            break;
    }
    return 0;
}

VOID DIALOG_FileSaveAs(VOID)
{
    OPENFILENAME saveas;
    WCHAR szDir[MAX_PATH];
    WCHAR szPath[MAX_PATH];
    static const WCHAR szDefaultExt[] = { 't','x','t',0 };
    static const WCHAR txt_files[] = { '*','.','t','x','t',0 };

    ZeroMemory(&saveas, sizeof(saveas));

    GetCurrentDirectory(SIZEOF(szDir), szDir);
    if (Globals.szFileName[0] == 0)
        lstrcpy(szPath, txt_files);
    else
        lstrcpy(szPath, Globals.szFileName);

    saveas.lStructSize       = sizeof(OPENFILENAME);
    saveas.hwndOwner         = Globals.hMainWnd;
    saveas.hInstance         = Globals.hInstance;
    saveas.lpstrFilter       = Globals.szFilter;
    saveas.lpstrFile         = szPath;
    saveas.nMaxFile          = SIZEOF(szPath);
    saveas.lpstrInitialDir   = szDir;
    saveas.Flags             = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT |
        OFN_HIDEREADONLY | OFN_EXPLORER | OFN_ENABLETEMPLATE | OFN_ENABLEHOOK;
    saveas.lpstrDefExt       = szDefaultExt;
    saveas.lpTemplateName    = MAKEINTRESOURCE(DIALOG_ENCODING);
    saveas.lpfnHook          = DIALOG_FileSaveAs_Hook;

    if (GetSaveFileName(&saveas)) {
        SetFileName(szPath);
        UpdateWindowCaption();
        DoSaveFile();
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
    LPWSTR pTemp;
    static const WCHAR times_new_romanW[] = { 'T','i','m','e','s',' ','N','e','w',' ','R','o','m','a','n',0 };

    /* Get a small font and print some header info on each page */
    hdrFont.lfHeight = 100;
    hdrFont.lfWidth = 0;
    hdrFont.lfEscapement = 0;
    hdrFont.lfOrientation = 0;
    hdrFont.lfWeight = FW_BOLD;
    hdrFont.lfItalic = 0;
    hdrFont.lfUnderline = 0;
    hdrFont.lfStrikeOut = 0;
    hdrFont.lfCharSet = ANSI_CHARSET;
    hdrFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
    hdrFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    hdrFont.lfQuality = PROOF_QUALITY;
    hdrFont.lfPitchAndFamily = VARIABLE_PITCH | FF_ROMAN;
    lstrcpy(hdrFont.lfFaceName, times_new_romanW);

    font = CreateFontIndirect(&hdrFont);

    /* Get Current Settings */
    ZeroMemory(&printer, sizeof(printer));
    printer.lStructSize           = sizeof(printer);
    printer.hwndOwner             = Globals.hMainWnd;
    printer.hInstance             = Globals.hInstance;

    /* Set some default flags */
    printer.Flags                 = PD_RETURNDC;
    printer.nFromPage             = 0;
    printer.nMinPage              = 1;
    /* we really need to calculate number of pages to set nMaxPage and nToPage */
    printer.nToPage               = 0;
    printer.nMaxPage              = (WORD) -1;

    /* Let commdlg manage copy settings */
    printer.nCopies               = (WORD)PD_USEDEVMODECOPIES;

    if (!PrintDlg(&printer)) return;

    assert(printer.hDC != 0);

    /* initialize DOCINFO */
    di.cbSize = sizeof(DOCINFO);
    di.lpszDocName = Globals.szFileTitle;
    di.lpszOutput = NULL;
    di.lpszDatatype = NULL;
    di.fwType = 0;

    if (StartDoc(printer.hDC, &di) <= 0) return;

    /* Get the page dimensions in pixels. */
    cWidthPels = GetDeviceCaps(printer.hDC, HORZRES);
    cHeightPels = GetDeviceCaps(printer.hDC, VERTRES);

    /* Get the file text */
    size = GetWindowTextLengthW(Globals.hEdit) + 1;
    pTemp = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
    if (!pTemp)
    {
        ShowLastError();
        return;
    }
    size = GetWindowTextW(Globals.hEdit, pTemp, size);

    border = 150;
    for (copycount=1; copycount <= printer.nCopies; copycount++) {
        i = 0;
        pagecount = 1;
        do {
            static const WCHAR letterM[] = { 'M',0 };

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
                    static const WCHAR failedW[] = { 'S','t','a','r','t','P','a','g','e',' ','f','a','i','l','e','d',0 };
                    static const WCHAR errorW[] = { 'P','r','i','n','t',' ','E','r','r','o','r',0 };
                    MessageBox(Globals.hMainWnd, failedW, errorW, MB_ICONEXCLAMATION);
                    return;
                }
                /* Write a rectangle and header at the top of each page */
                Rectangle(printer.hDC, border, border, cWidthPels-border, border+szMetric.cy*2);
                /* I don't know what's up with this TextOut command. This comes out
                kind of mangled.
                */
                TextOut(printer.hDC, border*2, border+szMetric.cy/2, Globals.szFileTitle, lstrlen(Globals.szFileTitle));
            }

            /* The starting point for the main text */
            xLeft = border*2;
            yTop = border+szMetric.cy*4;

            SelectObject(printer.hDC, old_font);
            GetTextExtentPoint32(printer.hDC, letterM, 1, &szMetric);

            /* Since outputting strings is giving me problems, output the main
            text one character at a time.
            */
            do {
                if (pTemp[i] == '\n') {
                    xLeft = border*2;
                    yTop += szMetric.cy;
                }
                else if (pTemp[i] != '\r') {
                    if (dopage)
                        TextOut(printer.hDC, xLeft, yTop, &pTemp[i], 1);
                    xLeft += szMetric.cx;
                }
            } while (i++<size && yTop<(cHeightPels-border*2));

            if (dopage)
                EndPage(printer.hDC);
            pagecount++;
        } while (i<size);
    }

    EndDoc(printer.hDC);
    DeleteDC(printer.hDC);
    HeapFree(GetProcessHeap(), 0, pTemp);
}

VOID DIALOG_FilePrinterSetup(VOID)
{
    PRINTDLG printer;

    ZeroMemory(&printer, sizeof(printer));
    printer.lStructSize         = sizeof(printer);
    printer.hwndOwner           = Globals.hMainWnd;
    printer.hInstance           = Globals.hInstance;
    printer.Flags               = PD_PRINTSETUP;
    printer.nCopies             = 1;

    PrintDlg(&printer);
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
    SYSTEMTIME   st;
    WCHAR        szDate[MAX_STRING_LEN];
    static const WCHAR spaceW[] = { ' ',0 };

    GetLocalTime(&st);

    GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, szDate, MAX_STRING_LEN);
    SendMessage(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM)szDate);

    SendMessage(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM)spaceW);

    GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL, szDate, MAX_STRING_LEN);
    SendMessage(Globals.hEdit, EM_REPLACESEL, TRUE, (LPARAM)szDate);
}

VOID DIALOG_EditWrap(VOID)
{
    static const WCHAR editW[] = { 'e','d','i','t',0 };
    DWORD dwStyle;
    RECT rc, rcstatus;
    DWORD size;
    LPWSTR pTemp;

    Globals.bWrapLongLines = !Globals.bWrapLongLines;

    size = GetWindowTextLength(Globals.hEdit) + 1;
    pTemp = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
    if (!pTemp)
    {
        ShowLastError();
        return;
    }
    GetWindowText(Globals.hEdit, pTemp, size);
    DestroyWindow(Globals.hEdit);
    GetClientRect(Globals.hMainWnd, &rc);
    dwStyle = Globals.bWrapLongLines ? EDIT_STYLE_WRAP : EDIT_STYLE;
    EnableMenuItem(GetMenu(Globals.hMainWnd), CMD_STATUSBAR,
        MF_BYCOMMAND | (Globals.bWrapLongLines ? MF_DISABLED | MF_GRAYED : MF_ENABLED));
    if ( Globals.hStatusBar )
    {
       if ( Globals.bWrapLongLines )
          ShowWindow(Globals.hStatusBar, SW_HIDE);
       else if ( Globals.bShowStatusBar )
       {
          GetClientRect(Globals.hStatusBar, &rcstatus);
          rc.bottom -= (rcstatus.bottom - rcstatus.top);
          ShowWindow(Globals.hStatusBar, SW_SHOW);
       }
    }
    Globals.hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, editW, NULL, dwStyle,
                         0, 0, rc.right, rc.bottom, Globals.hMainWnd,
                         NULL, Globals.hInstance, NULL);
    SendMessage(Globals.hEdit, WM_SETFONT, (WPARAM)Globals.hFont, (LPARAM)FALSE);
    SendMessage(Globals.hEdit, EM_LIMITTEXT, 0, 0);
    SetWindowTextW(Globals.hEdit, pTemp);
    SetFocus(Globals.hEdit);
    HeapFree(GetProcessHeap(), 0, pTemp);
    DrawMenuBar(Globals.hMainWnd);
}

VOID DIALOG_SelectFont(VOID)
{
    CHOOSEFONT cf;
    LOGFONT lf=Globals.lfFont;

    ZeroMemory( &cf, sizeof(cf) );
    cf.lStructSize=sizeof(cf);
    cf.hwndOwner=Globals.hMainWnd;
    cf.lpLogFont=&lf;
    cf.Flags=CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;

    if( ChooseFont(&cf) )
    {
        HFONT currfont=Globals.hFont;

        Globals.hFont=CreateFontIndirect( &lf );
        Globals.lfFont=lf;
        SendMessage( Globals.hEdit, WM_SETFONT, (WPARAM)Globals.hFont, (LPARAM)TRUE );
        if( currfont!=NULL )
            DeleteObject( currfont );
    }
}

typedef HWND (WINAPI *FINDPROC)(LPFINDREPLACE lpfr);

static VOID DIALOG_SearchDialog(FINDPROC pfnProc)
{
    ZeroMemory(&Globals.find, sizeof(Globals.find));
    Globals.find.lStructSize      = sizeof(Globals.find);
    Globals.find.hwndOwner        = Globals.hMainWnd;
    Globals.find.hInstance        = Globals.hInstance;
    Globals.find.lpstrFindWhat    = Globals.szFindText;
    Globals.find.wFindWhatLen     = SIZEOF(Globals.szFindText);
    Globals.find.lpstrReplaceWith = Globals.szReplaceText;
    Globals.find.wReplaceWithLen  = SIZEOF(Globals.szReplaceText);
    Globals.find.Flags            = FR_DOWN;

    /* We only need to create the modal FindReplace dialog which will */
    /* notify us of incoming events using hMainWnd Window Messages    */

    Globals.hFindReplaceDlg = pfnProc(&Globals.find);
    assert(Globals.hFindReplaceDlg !=0);
}

VOID DIALOG_Search(VOID)
{
    DIALOG_SearchDialog(FindText);
}

VOID DIALOG_SearchNext(VOID)
{
    if (Globals.find.lpstrFindWhat != NULL)
      NOTEPAD_FindNext(&Globals.find, FALSE, TRUE);
}

VOID DIALOG_Replace(VOID)
{
    DIALOG_SearchDialog(ReplaceText);
}

static INT_PTR CALLBACK DIALOG_GoTo_DialogProc(HWND hwndDialog, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL bResult = FALSE;
    HWND hTextBox;
    TCHAR szText[32];

    switch(uMsg) {
	case WM_INITDIALOG:
        hTextBox = GetDlgItem(hwndDialog, ID_LINENUMBER);
		_sntprintf(szText, sizeof(szText) / sizeof(szText[0]), _T("%d"), lParam);
        SetWindowText(hTextBox, szText);
		break;
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            if (LOWORD(wParam) == IDOK)
            {
                hTextBox = GetDlgItem(hwndDialog, ID_LINENUMBER);
                GetWindowText(hTextBox, szText, sizeof(szText) / sizeof(szText[0]));
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

    nLine = DialogBoxParam(Globals.hInstance, MAKEINTRESOURCE(DIALOG_GOTO),
        Globals.hMainWnd, DIALOG_GoTo_DialogProc, nLine);

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
    int line;
    int col;
    int ccol;
    POINT point;
    TCHAR buff[MAX_PATH];

    GetCaretPos(&point);
    line = (int) SendMessage(Globals.hEdit, EM_LINEFROMCHAR, (WPARAM)-1, (LPARAM)0);
    ccol = (int) SendMessage(Globals.hEdit, EM_CHARFROMPOS, (WPARAM)0, (LPARAM)MAKELPARAM(point.x, point.y));
    ccol = LOWORD(ccol);
    col = ccol - (int) SendMessage(Globals.hEdit, EM_LINEINDEX, (WPARAM)line, (LPARAM)0);

    _stprintf(buff, TEXT("%S %d, %S %d"), Globals.szStatusBarLine, line+1, Globals.szStatusBarCol, col+1);
    SendMessage(Globals.hStatusBar, SB_SETTEXT, (WPARAM) SB_SIMPLEID, (LPARAM)buff);
}

VOID DIALOG_ViewStatusBar(VOID)
{
   RECT rc;
   RECT rcstatus;

   Globals.bShowStatusBar = !Globals.bShowStatusBar;
   if ( !Globals.hStatusBar )
   {
       Globals.hStatusBar = CreateStatusWindow(WS_CHILD | WS_VISIBLE | WS_EX_STATICEDGE, TEXT("test"), Globals.hMainWnd, CMD_STATUSBAR_WND_ID );
       LoadString(Globals.hInstance, STRING_LINE, Globals.szStatusBarLine, MAX_PATH-1);
       LoadString(Globals.hInstance, STRING_COLUMN, Globals.szStatusBarCol, MAX_PATH-1);
       SendMessage(Globals.hStatusBar, SB_SIMPLE, (WPARAM)TRUE, (LPARAM)0);
   }
    CheckMenuItem(GetMenu(Globals.hMainWnd), CMD_STATUSBAR,
        MF_BYCOMMAND | (Globals.bShowStatusBar ? MF_CHECKED : MF_UNCHECKED));
    DrawMenuBar(Globals.hMainWnd);
    GetClientRect(Globals.hMainWnd, &rc);
    GetClientRect(Globals.hStatusBar, &rcstatus);
    if ( Globals.bShowStatusBar )
        rc.bottom -= (rcstatus.bottom - rcstatus.top);

    MoveWindow(Globals.hEdit, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
    ShowWindow(Globals.hStatusBar, Globals.bShowStatusBar);
    DIALOG_StatusBarUpdateCaretPos();
}

VOID DIALOG_HelpContents(VOID)
{
    WinHelp(Globals.hMainWnd, helpfileW, HELP_INDEX, 0);
}

VOID DIALOG_HelpSearch(VOID)
{
        /* Search Help */
}

VOID DIALOG_HelpHelp(VOID)
{
    WinHelp(Globals.hMainWnd, helpfileW, HELP_HELPONHELP, 0);
}

#ifdef _MSC_VER
#pragma warning(disable : 4100)
#endif
BOOL CALLBACK
AboutDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND    hLicenseEditWnd;
    TCHAR    strLicense[0x1000];

    switch (message)
    {
    case WM_INITDIALOG:

        hLicenseEditWnd = GetDlgItem(hDlg, IDC_LICENSE);

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



VOID DIALOG_HelpAboutWine(VOID)
{
    static const WCHAR notepadW[] = { 'N','o','t','e','p','a','d','\n',0 };
    WCHAR szNotepad[MAX_STRING_LEN];

    LoadString(Globals.hInstance, STRING_NOTEPAD, szNotepad, SIZEOF(szNotepad));
    ShellAbout(Globals.hMainWnd, szNotepad, notepadW, 0);
}


/***********************************************************************
 *
 *           DIALOG_FilePageSetup
 */
VOID DIALOG_FilePageSetup(void)
{
  DialogBox(Globals.hInstance, MAKEINTRESOURCE(DIALOG_PAGESETUP),
            Globals.hMainWnd, DIALOG_PAGESETUP_DlgProc);
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *           DIALOG_PAGESETUP_DlgProc
 */

static INT_PTR WINAPI DIALOG_PAGESETUP_DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
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
                GetDlgItemText(hDlg, 0x141, Globals.szHeader, SIZEOF(Globals.szHeader));
                GetDlgItemText(hDlg, 0x143, Globals.szFooter, SIZEOF(Globals.szFooter));
                GetDlgItemText(hDlg, 0x14A, Globals.szMarginTop, SIZEOF(Globals.szMarginTop));
                GetDlgItemText(hDlg, 0x150, Globals.szMarginBottom, SIZEOF(Globals.szMarginBottom));
                GetDlgItemText(hDlg, 0x147, Globals.szMarginLeft, SIZEOF(Globals.szMarginLeft));
                GetDlgItemText(hDlg, 0x14D, Globals.szMarginRight, SIZEOF(Globals.szMarginRight));
                EndDialog(hDlg, IDOK);
                return TRUE;

            case IDCANCEL:
                /* discard user input and close dialog */
                EndDialog(hDlg, IDCANCEL);
                return TRUE;

            case IDHELP:
                {
                    /* FIXME: Bring this to work */
                    static const WCHAR sorryW[] = { 'S','o','r','r','y',',',' ','n','o',' ','h','e','l','p',' ','a','v','a','i','l','a','b','l','e',0 };
                    static const WCHAR helpW[] = { 'H','e','l','p',0 };
                    MessageBox(Globals.hMainWnd, sorryW, helpW, MB_ICONEXCLAMATION);
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
        SetDlgItemText(hDlg, 0x14A, Globals.szMarginTop);
        SetDlgItemText(hDlg, 0x150, Globals.szMarginBottom);
        SetDlgItemText(hDlg, 0x147, Globals.szMarginLeft);
        SetDlgItemText(hDlg, 0x14D, Globals.szMarginRight);
        break;
    }

    return FALSE;
}
