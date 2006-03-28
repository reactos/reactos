/*
 * Wordpad implementation
 *
 * Copyright 2004 by Krzysztof Foltman
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

#define WIN32_LEAN_AND_MEAN

#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>
#include <assert.h>

#include <windows.h>
#include <richedit.h>
#include <commctrl.h>

#include "resource.h"


/* use LoadString */
static const WCHAR xszAppTitle[] = {'W','i','n','e',' ','W','o','r','d','p','a','d',0};
static const WCHAR xszMainMenu[] = {'M','A','I','N','M','E','N','U',0};

static const WCHAR wszRichEditClass[] = {'R','I','C','H','E','D','I','T','2','0','W',0};
static const WCHAR wszMainWndClass[] = {'W','O','R','D','P','A','D','T','O','P',0};
static const WCHAR wszAppTitle[] = {'W','i','n','e',' ','W','o','r','d','p','a','d',0};

static HWND hMainWnd = NULL;

void AddButton(HWND hwndToolBar, int nImage, int nCommand)
{
    TBBUTTON button;

    ZeroMemory(&button, sizeof(button));
    button.iBitmap = nImage;
    button.idCommand = nCommand;
    button.fsState = TBSTATE_ENABLED;
    button.fsStyle = TBSTYLE_BUTTON;
    button.dwData = 0;
    button.iString = -1;
    SendMessage(hwndToolBar, TB_ADDBUTTONS, 1, (LPARAM)&button);
}

void AddSeparator(HWND hwndToolBar)
{
    TBBUTTON button;

    ZeroMemory(&button, sizeof(button));
    button.iBitmap = -1;
    button.idCommand = 0;
    button.fsState = 0;
    button.fsStyle = TBSTYLE_SEP;
    button.dwData = 0;
    button.iString = -1;
    SendMessage(hwndToolBar, TB_ADDBUTTONS, 1, (LPARAM)&button);
}

static LRESULT OnCreate( HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    HWND hEditorWnd, hToolBarWnd, hReBarWnd;
    HINSTANCE hInstance = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
    HANDLE hDLL;
    TBADDBITMAP ab;
    int nStdBitmaps = 0;
    REBARINFO rbi;
    REBARBANDINFO rbb;

    CreateStatusWindow(CCS_NODIVIDER|WS_CHILD|WS_VISIBLE, "RichEdit text", hWnd, IDC_STATUSBAR);

    hReBarWnd = CreateWindowEx(WS_EX_TOOLWINDOW, REBARCLASSNAME, NULL,
      CCS_NODIVIDER|WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|RBS_VARHEIGHT|CCS_TOP,
      CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, hWnd, (HMENU)IDC_REBAR, hInstance, NULL);

    rbi.cbSize = sizeof(rbi);
    rbi.fMask = 0;
    rbi.himl = NULL;
    if(!SendMessage(hReBarWnd, RB_SETBARINFO, 0, (LPARAM)&rbi))
        return -1;

    hToolBarWnd = CreateToolbarEx(hReBarWnd, CCS_NOPARENTALIGN|CCS_NODIVIDER|CCS_NOMOVEY|WS_VISIBLE|WS_CHILD|TBSTYLE_TOOLTIPS|TBSTYLE_FLAT,
      IDC_TOOLBAR,
      3, hInstance, IDB_TOOLBAR,
      NULL, 0,
      24, 24, 16, 16, sizeof(TBBUTTON));

    ab.hInst = HINST_COMMCTRL;
    ab.nID = IDB_STD_SMALL_COLOR;
    nStdBitmaps = SendMessage(hToolBarWnd, TB_ADDBITMAP, 3, (LPARAM)&ab);
    AddButton(hToolBarWnd, nStdBitmaps+STD_FILENEW, ID_FILE_NEW);
    AddButton(hToolBarWnd, nStdBitmaps+STD_FILEOPEN, ID_FILE_OPEN);
    AddButton(hToolBarWnd, nStdBitmaps+STD_FILESAVE, ID_FILE_SAVE);
    AddSeparator(hToolBarWnd);
    AddButton(hToolBarWnd, nStdBitmaps+STD_CUT, ID_EDIT_CUT);
    AddButton(hToolBarWnd, nStdBitmaps+STD_COPY, ID_EDIT_COPY);
    AddSeparator(hToolBarWnd);
    AddButton(hToolBarWnd, nStdBitmaps+STD_UNDO, ID_EDIT_UNDO);
    AddButton(hToolBarWnd, nStdBitmaps+STD_REDOW, ID_EDIT_REDO);
    AddSeparator(hToolBarWnd);
    AddButton(hToolBarWnd, 0, ID_FORMAT_BOLD);
    AddButton(hToolBarWnd, 1, ID_FORMAT_ITALIC);
    AddButton(hToolBarWnd, 2, ID_FORMAT_UNDERLINE);

    SendMessage(hToolBarWnd, TB_ADDSTRING, 0, (LPARAM)"Exit\0");
    SendMessage(hToolBarWnd, TB_AUTOSIZE, 0, 0);

    rbb.cbSize = sizeof(rbb);
    rbb.fMask = RBBIM_SIZE | RBBIM_CHILDSIZE | RBBIM_CHILD | RBBIM_STYLE;
    rbb.fStyle = RBBS_CHILDEDGE;
    rbb.cx = 500;
    rbb.hwndChild = hToolBarWnd;
    rbb.cxMinChild = 0;
    rbb.cyChild = rbb.cyMinChild = HIWORD(SendMessage(hToolBarWnd, TB_GETBUTTONSIZE, 0, 0));

    SendMessage(hReBarWnd, RB_INSERTBAND, -1, (LPARAM)&rbb);

    hDLL = LoadLibrary("RICHED20.DLL");
    assert(hDLL);

    hEditorWnd = CreateWindowExW(WS_EX_CLIENTEDGE, wszRichEditClass, NULL,
      WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN,
      0, 0, 1000, 100, hWnd, (HMENU)IDC_EDITOR, hInstance, NULL);
    if (!hEditorWnd)
    {
        fprintf(stderr, "Error code %lu\n", GetLastError());
        return -1;
    }
    assert(hEditorWnd);

    SetFocus(hEditorWnd);
    SendMessage(hEditorWnd, EM_SETEVENTMASK, 0, ENM_SELCHANGE);
    return 0;
}

static LRESULT OnUser( HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    HWND hwndEditor = GetDlgItem(hWnd, IDC_EDITOR);
    HWND hwndReBar = GetDlgItem(hWnd, IDC_REBAR);
    HWND hwndToolBar = GetDlgItem(hwndReBar, IDC_TOOLBAR);
    int from, to;
    CHARFORMAT2W fmt;

    ZeroMemory(&fmt, sizeof(fmt));
    fmt.cbSize = sizeof(fmt);

    SendMessage(hwndEditor, EM_GETCHARFORMAT, TRUE, (LPARAM)&fmt);

    SendMessage(hwndEditor, EM_GETSEL, (WPARAM)&from, (LPARAM)&to);
    SendMessage(hwndToolBar, TB_ENABLEBUTTON, ID_EDIT_UNDO,
      SendMessage(hwndEditor, EM_CANUNDO, 0, 0));
    SendMessage(hwndToolBar, TB_ENABLEBUTTON, ID_EDIT_REDO,
      SendMessage(hwndEditor, EM_CANREDO, 0, 0));
    SendMessage(hwndToolBar, TB_ENABLEBUTTON, ID_EDIT_CUT, from == to ? 0 : 1);
    SendMessage(hwndToolBar, TB_ENABLEBUTTON, ID_EDIT_COPY, from == to ? 0 : 1);
    SendMessage(hwndToolBar, TB_CHECKBUTTON, ID_FORMAT_BOLD, (fmt.dwMask & CFM_BOLD) && (fmt.dwEffects & CFE_BOLD));
    SendMessage(hwndToolBar, TB_INDETERMINATE, ID_FORMAT_BOLD, !(fmt.dwMask & CFM_BOLD));
    SendMessage(hwndToolBar, TB_CHECKBUTTON, ID_FORMAT_ITALIC, (fmt.dwMask & CFM_ITALIC) && (fmt.dwEffects & CFE_ITALIC));
    SendMessage(hwndToolBar, TB_INDETERMINATE, ID_FORMAT_ITALIC, !(fmt.dwMask & CFM_ITALIC));
    SendMessage(hwndToolBar, TB_CHECKBUTTON, ID_FORMAT_UNDERLINE, (fmt.dwMask & CFM_UNDERLINE) && (fmt.dwEffects & CFE_UNDERLINE));
    SendMessage(hwndToolBar, TB_INDETERMINATE, ID_FORMAT_UNDERLINE, !(fmt.dwMask & CFM_UNDERLINE));
    return 0;
}

static LRESULT OnNotify( HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    HWND hwndEditor = GetDlgItem(hWnd, IDC_EDITOR);
    NMHDR *pHdr = (NMHDR *)lParam;

    if (pHdr->hwndFrom != hwndEditor)
        return 0;

    if (pHdr->code == EN_SELCHANGE)
    {
        SELCHANGE *pSC = (SELCHANGE *)lParam;
        char buf[128];

        sprintf( buf,"selection = %ld..%ld, line count=%ld\n",
                 pSC->chrg.cpMin, pSC->chrg.cpMax,
        SendMessage(hwndEditor, EM_GETLINECOUNT, 0, 0));
        SetWindowText(GetDlgItem(hWnd, IDC_STATUSBAR), buf);
        SendMessage(hWnd, WM_USER, 0, 0);
        return 1;
    }
    return 0;
}

static LRESULT OnCommand( HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    HWND hwndEditor = GetDlgItem(hWnd, IDC_EDITOR);

    if ((HWND)lParam == hwndEditor)
        return 0;

    switch(LOWORD(wParam))
    {
    case ID_FILE_EXIT:
        PostMessage(hWnd, WM_CLOSE, 0, 0);
        break;

    case ID_FILE_NEW:
        SetWindowTextA(hwndEditor, "");
        /* FIXME: set default format too */
        break;

    case ID_FILE_OPEN:
    case ID_FILE_SAVE:
        MessageBox(hWnd, "Open/Save not implemented", "WordPad", MB_OK);
        break;

    case ID_FORMAT_BOLD:
    case ID_FORMAT_ITALIC:
    case ID_FORMAT_UNDERLINE:
        {
        CHARFORMAT2W fmt;
        int mask = CFM_BOLD;
        if (LOWORD(wParam) == ID_FORMAT_ITALIC) mask = CFM_ITALIC;
        if (LOWORD(wParam) == ID_FORMAT_UNDERLINE) mask = CFM_UNDERLINE;

        ZeroMemory(&fmt, sizeof(fmt));
        fmt.cbSize = sizeof(fmt);
        SendMessage(hwndEditor, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&fmt);
        if (!(fmt.dwMask&mask))
            fmt.dwEffects |= mask;
        else
            fmt.dwEffects ^= mask;
        fmt.dwMask = mask;
        SendMessage(hwndEditor, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&fmt);
        break;
        }

    case ID_EDIT_CUT:
        PostMessage(hwndEditor, WM_CUT, 0, 0);
        break;

    case ID_EDIT_COPY:
        PostMessage(hwndEditor, WM_COPY, 0, 0);
        break;

    case ID_EDIT_SELECTALL:
        {
        CHARRANGE range = {0, -1};
        SendMessage(hwndEditor, EM_EXSETSEL, 0, (LPARAM)&range);
        /* SendMessage(hwndEditor, EM_SETSEL, 0, -1); */
        return 0;
        }

    case ID_EDIT_GETTEXT:
        {
        int nLen = GetWindowTextLengthW(hwndEditor);
        LPWSTR data = HeapAlloc( GetProcessHeap(), 0, (nLen+1)*sizeof(WCHAR) );
        TEXTRANGEW tr;

        GetWindowTextW(hwndEditor, data, nLen+1);
        MessageBoxW(NULL, data, xszAppTitle, MB_OK);

        HeapFree( GetProcessHeap(), 0, data);
        data = HeapAlloc(GetProcessHeap(), 0, (nLen+1)*sizeof(WCHAR));
        tr.chrg.cpMin = 0;
        tr.chrg.cpMax = nLen;
        tr.lpstrText = data;
        SendMessage (hwndEditor, EM_GETTEXTRANGE, 0, (LPARAM)&tr);
        MessageBoxW(NULL, data, xszAppTitle, MB_OK);
        HeapFree( GetProcessHeap(), 0, data );

        /* SendMessage(hwndEditor, EM_SETSEL, 0, -1); */
        return 0;
        }

    case ID_EDIT_CHARFORMAT:
    case ID_EDIT_DEFCHARFORMAT:
        {
        CHARFORMAT2W cf;
        LRESULT i;
        ZeroMemory(&cf, sizeof(cf));
        cf.cbSize = sizeof(cf);
        cf.dwMask = 0;
        i = SendMessage(hwndEditor, EM_GETCHARFORMAT,
                        LOWORD(wParam) == ID_EDIT_CHARFORMAT, (LPARAM)&cf);
        return 0;
        }

    case ID_EDIT_PARAFORMAT:
        {
        PARAFORMAT2 pf;
        ZeroMemory(&pf, sizeof(pf));
        pf.cbSize = sizeof(pf);
        SendMessage(hwndEditor, EM_GETPARAFORMAT, 0, (LPARAM)&pf);
        return 0;
        }

    case ID_EDIT_SELECTIONINFO:
        {
        CHARRANGE range = {0, -1};
        char buf[128];
        WCHAR *data = NULL;

        SendMessage(hwndEditor, EM_EXGETSEL, 0, (LPARAM)&range);
        data = HeapAlloc(GetProcessHeap(), 0, sizeof(*data) * (range.cpMax-range.cpMin+1));
        SendMessage(hwndEditor, EM_GETSELTEXT, 0, (LPARAM)data);
        sprintf(buf, "Start = %ld, End = %ld", range.cpMin, range.cpMax);
        MessageBoxA(hWnd, buf, "Editor", MB_OK);
        MessageBoxW(hWnd, data, xszAppTitle, MB_OK);
        HeapFree( GetProcessHeap(), 0, data);
        /* SendMessage(hwndEditor, EM_SETSEL, 0, -1); */
        return 0;
        }

    case ID_EDIT_READONLY:
        {
        long nStyle = GetWindowLong(hwndEditor, GWL_STYLE);
        if (nStyle & ES_READONLY)
            SendMessage(hwndEditor, EM_SETREADONLY, 0, 0);
        else
            SendMessage(hwndEditor, EM_SETREADONLY, 1, 0);
        return 0;
        }

    case ID_EDIT_MODIFIED:
        if (SendMessage(hwndEditor, EM_GETMODIFY, 0, 0))
            SendMessage(hwndEditor, EM_SETMODIFY, 0, 0);
        else
            SendMessage(hwndEditor, EM_SETMODIFY, 1, 0);
        return 0;

    case ID_EDIT_UNDO:
        SendMessage(hwndEditor, EM_UNDO, 0, 0);
        return 0;

    case ID_EDIT_REDO:
        SendMessage(hwndEditor, EM_REDO, 0, 0);
        return 0;

    case ID_ALIGN_LEFT:
    case ID_ALIGN_CENTER:
    case ID_ALIGN_RIGHT:
        {
        PARAFORMAT2 pf;

        pf.cbSize = sizeof(pf);
        pf.dwMask = PFM_ALIGNMENT;
        switch(LOWORD(wParam)) {
        case ID_ALIGN_LEFT: pf.wAlignment = PFA_LEFT; break;
        case ID_ALIGN_CENTER: pf.wAlignment = PFA_CENTER; break;
        case ID_ALIGN_RIGHT: pf.wAlignment = PFA_RIGHT; break;
        }
        SendMessage(hwndEditor, EM_SETPARAFORMAT, 0, (LPARAM)&pf);
        break;
        }

    case ID_BACK_1:
        SendMessage(hwndEditor, EM_SETBKGNDCOLOR, 1, 0);
        break;

    case ID_BACK_2:
        SendMessage(hwndEditor, EM_SETBKGNDCOLOR, 0, RGB(255,255,192));
        break;

    default:
        SendMessage(hwndEditor, WM_COMMAND, wParam, lParam);
        break;
    }
    return 0;
}

static LRESULT OnInitPopupMenu( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
    HMENU hMenu = (HMENU)wParam;
    HWND hwndEditor = GetDlgItem(hWnd, IDC_EDITOR);
    PARAFORMAT pf;
    int nAlignment = -1;

    pf.cbSize = sizeof(PARAFORMAT);
    SendMessage(hwndEditor, EM_GETPARAFORMAT, 0, (LPARAM)&pf);
    CheckMenuItem(hMenu, ID_EDIT_READONLY,
      MF_BYCOMMAND|(GetWindowLong(hwndEditor, GWL_STYLE)&ES_READONLY ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hMenu, ID_EDIT_MODIFIED,
      MF_BYCOMMAND|(SendMessage(hwndEditor, EM_GETMODIFY, 0, 0) ? MF_CHECKED : MF_UNCHECKED));
    if (pf.dwMask & PFM_ALIGNMENT)
        nAlignment = pf.wAlignment;
    CheckMenuItem(hMenu, ID_ALIGN_LEFT, MF_BYCOMMAND|(nAlignment == PFA_LEFT) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_ALIGN_CENTER, MF_BYCOMMAND|(nAlignment == PFA_CENTER) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, ID_ALIGN_RIGHT, MF_BYCOMMAND|(nAlignment == PFA_RIGHT) ? MF_CHECKED : MF_UNCHECKED);
    EnableMenuItem(hMenu, ID_EDIT_UNDO, MF_BYCOMMAND|(SendMessage(hwndEditor, EM_CANUNDO, 0, 0)) ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(hMenu, ID_EDIT_REDO, MF_BYCOMMAND|(SendMessage(hwndEditor, EM_CANREDO, 0, 0)) ? MF_ENABLED : MF_GRAYED);
    return 0;
}

static LRESULT OnSize( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
    int nStatusSize = 0, nTBSize = 0;
    RECT rc;
    HWND hwndEditor = GetDlgItem(hWnd, IDC_EDITOR);
    HWND hwndStatusBar = GetDlgItem(hWnd, IDC_STATUSBAR);
    HWND hwndReBar = GetDlgItem(hWnd, IDC_REBAR);
    HWND hwndToolBar = GetDlgItem(hwndReBar, IDC_TOOLBAR);

    if (hwndStatusBar)
    {
        SendMessage(hwndStatusBar, WM_SIZE, 0, 0);
        GetClientRect(hwndStatusBar, &rc);
        nStatusSize = rc.bottom - rc.top;
    }
    if (hwndToolBar)
    {
        rc.left = rc.top = 0;
        rc.right = LOWORD(lParam);
        rc.bottom = HIWORD(lParam);
        SendMessage(hwndToolBar, TB_AUTOSIZE, 0, 0);
        SendMessage(hwndReBar, RB_SIZETORECT, 0, (LPARAM)&rc);
        nTBSize = SendMessage(hwndReBar, RB_GETBARHEIGHT, 0, 0);
        GetClientRect(hwndReBar, &rc);
        MoveWindow(hwndReBar, 0, 0, LOWORD(lParam), rc.right, FALSE);
    }
    if (hwndEditor)
    {
        GetClientRect(hWnd, &rc);
        MoveWindow(hwndEditor, 0, nTBSize, rc.right, rc.bottom-nStatusSize-nTBSize, TRUE);
    }

    return DefWindowProcW(hWnd, WM_SIZE, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
    case WM_CREATE:
        return OnCreate( hWnd, wParam, lParam );

    case WM_USER:
        return OnUser( hWnd, wParam, lParam );

    case WM_NOTIFY:
        return OnNotify( hWnd, wParam, lParam );

    case WM_COMMAND:
        return OnCommand( hWnd, wParam, lParam );

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_ACTIVATE:
        if (LOWORD(wParam))
            SetFocus(GetDlgItem(hWnd, IDC_EDITOR));
        return 0;

    case WM_INITMENUPOPUP:
        return OnInitPopupMenu( hWnd, wParam, lParam );

    case WM_SIZE:
        return OnSize( hWnd, wParam, lParam );

    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }

    return 0;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hOldInstance, LPSTR szCmdParagraph, int res)
{
    INITCOMMONCONTROLSEX classes = {8, ICC_BAR_CLASSES|ICC_COOL_CLASSES};
    HACCEL hAccel;
    WNDCLASSW wc;
    MSG msg;

    InitCommonControlsEx(&classes);

    hAccel = LoadAccelerators(hInstance, "MAINACCELTABLE");

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 4;
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_IBEAM);
    wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    wc.lpszMenuName = xszMainMenu;
    wc.lpszClassName = wszMainWndClass;
    RegisterClassW(&wc);

    hMainWnd = CreateWindowExW(0, wszMainWndClass, wszAppTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, 680, 260, NULL, NULL, hInstance, NULL);
    ShowWindow(hMainWnd, SW_SHOWMAXIMIZED);

    while(GetMessage(&msg,0,0,0))
    {
        if (TranslateAccelerator(hMainWnd, hAccel, &msg))
            continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (!PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE))
            SendMessage(hMainWnd, WM_USER, 0, 0);
    }

    return 0;
}
