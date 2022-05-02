/*
 *    Folder Options
 *
 * Copyright 2007 Johannes Anderwald <johannes.anderwald@reactos.org>
 * Copyright 2016-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL (fprop);

// Folder Options:
// CLASSKEY = HKEY_CLASSES_ROOT\CLSID\{6DFD7C5C-2451-11d3-A299-00C04F8EF6AF}

/////////////////////////////////////////////////////////////////////////////
// strings

// path to shell32
LPCWSTR g_pszShell32 = L"%SystemRoot%\\system32\\shell32.dll";

// the space characters
LPCWSTR g_pszSpace = L" \t\n\r\f\v";

/////////////////////////////////////////////////////////////////////////////
// utility functions

HBITMAP Create24BppBitmap(HDC hDC, INT cx, INT cy)
{
    BITMAPINFO bi;
    LPVOID pvBits;

    ZeroMemory(&bi, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = cx;
    bi.bmiHeader.biHeight = cy;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 24;
    bi.bmiHeader.biCompression = BI_RGB;

    HBITMAP hbm = CreateDIBSection(hDC, &bi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    return hbm;
}

HBITMAP BitmapFromIcon(HICON hIcon, INT cx, INT cy)
{
    HDC hDC = CreateCompatibleDC(NULL);
    if (!hDC)
        return NULL;

    HBITMAP hbm = Create24BppBitmap(hDC, cx, cy);
    if (!hbm)
    {
        DeleteDC(hDC);
        return NULL;
    }

    HGDIOBJ hbmOld = SelectObject(hDC, hbm);
    {
        RECT rc = { 0, 0, cx, cy };
        FillRect(hDC, &rc, HBRUSH(COLOR_3DFACE + 1));
        if (hIcon)
        {
            DrawIconEx(hDC, 0, 0, hIcon, cx, cy, 0, NULL, DI_NORMAL);
        }
    }
    SelectObject(hDC, hbmOld);
    DeleteDC(hDC);

    return hbm;
}

HBITMAP CreateCheckImage(HDC hDC, BOOL bCheck, BOOL bEnabled)
{
    INT cxSmallIcon = GetSystemMetrics(SM_CXSMICON);
    INT cySmallIcon = GetSystemMetrics(SM_CYSMICON);

    HBITMAP hbm = Create24BppBitmap(hDC, cxSmallIcon, cySmallIcon);
    if (hbm == NULL)
        return NULL;    // failure

    RECT Rect, BoxRect;
    SetRect(&Rect, 0, 0, cxSmallIcon, cySmallIcon);
    BoxRect = Rect;
    InflateRect(&BoxRect, -1, -1);

    HGDIOBJ hbmOld = SelectObject(hDC, hbm);
    {
        UINT uState = DFCS_BUTTONCHECK | DFCS_FLAT | DFCS_MONO;
        if (bCheck)
            uState |= DFCS_CHECKED;
        if (!bEnabled)
            uState |= DFCS_INACTIVE;
        DrawFrameControl(hDC, &BoxRect, DFC_BUTTON, uState);
    }
    SelectObject(hDC, hbmOld);

    return hbm;     // success
}

HBITMAP CreateCheckMask(HDC hDC)
{
    INT cxSmallIcon = GetSystemMetrics(SM_CXSMICON);
    INT cySmallIcon = GetSystemMetrics(SM_CYSMICON);

    HBITMAP hbm = CreateBitmap(cxSmallIcon, cySmallIcon, 1, 1, NULL);
    if (hbm == NULL)
        return NULL;    // failure

    RECT Rect, BoxRect;
    SetRect(&Rect, 0, 0, cxSmallIcon, cySmallIcon);
    BoxRect = Rect;
    InflateRect(&BoxRect, -1, -1);

    HGDIOBJ hbmOld = SelectObject(hDC, hbm);
    {
        FillRect(hDC, &Rect, HBRUSH(GetStockObject(WHITE_BRUSH)));
        FillRect(hDC, &BoxRect, HBRUSH(GetStockObject(BLACK_BRUSH)));
    }
    SelectObject(hDC, hbmOld);

    return hbm;     // success
}

HBITMAP CreateRadioImage(HDC hDC, BOOL bCheck, BOOL bEnabled)
{
    INT cxSmallIcon = GetSystemMetrics(SM_CXSMICON);
    INT cySmallIcon = GetSystemMetrics(SM_CYSMICON);

    HBITMAP hbm = Create24BppBitmap(hDC, cxSmallIcon, cySmallIcon);
    if (hbm == NULL)
        return NULL;    // failure

    RECT Rect, BoxRect;
    SetRect(&Rect, 0, 0, cxSmallIcon, cySmallIcon);
    BoxRect = Rect;
    InflateRect(&BoxRect, -1, -1);

    HGDIOBJ hbmOld = SelectObject(hDC, hbm);
    {
        UINT uState = DFCS_BUTTONRADIOIMAGE | DFCS_FLAT | DFCS_MONO;
        if (bCheck)
            uState |= DFCS_CHECKED;
        if (!bEnabled)
            uState |= DFCS_INACTIVE;
        DrawFrameControl(hDC, &BoxRect, DFC_BUTTON, uState);
    }
    SelectObject(hDC, hbmOld);

    return hbm;     // success
}

HBITMAP CreateRadioMask(HDC hDC)
{
    INT cxSmallIcon = GetSystemMetrics(SM_CXSMICON);
    INT cySmallIcon = GetSystemMetrics(SM_CYSMICON);

    HBITMAP hbm = CreateBitmap(cxSmallIcon, cySmallIcon, 1, 1, NULL);
    if (hbm == NULL)
        return NULL;    // failure

    RECT Rect, BoxRect;
    SetRect(&Rect, 0, 0, cxSmallIcon, cySmallIcon);
    BoxRect = Rect;
    InflateRect(&BoxRect, -1, -1);

    HGDIOBJ hbmOld = SelectObject(hDC, hbm);
    {
        FillRect(hDC, &Rect, HBRUSH(GetStockObject(WHITE_BRUSH)));
        UINT uState = DFCS_BUTTONRADIOMASK | DFCS_FLAT | DFCS_MONO;
        DrawFrameControl(hDC, &BoxRect, DFC_BUTTON, uState);
    }
    SelectObject(hDC, hbmOld);

    return hbm;     // success
}

/////////////////////////////////////////////////////////////////////////////

// CMSGlobalFolderOptionsStub --- The owner window of Folder Options.
// This window hides taskbar button of Folder Options.
class CMSGlobalFolderOptionsStub : public CWindowImpl<CMSGlobalFolderOptionsStub>
{
public:
    DECLARE_WND_CLASS_EX(_T("MSGlobalFolderOptionsStub"), 0, COLOR_WINDOWTEXT)

    BEGIN_MSG_MAP(CMSGlobalFolderOptionsStub)
    END_MSG_MAP()
};

/////////////////////////////////////////////////////////////////////////////

EXTERN_C HPSXA WINAPI SHCreatePropSheetExtArrayEx(HKEY hKey, LPCWSTR pszSubKey, UINT max_iface, IDataObject *pDataObj);

static int CALLBACK
PropSheetProc(HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
    // NOTE: This callback is needed to set large icon correctly.
    HICON hIcon;
    switch (uMsg)
    {
        case PSCB_INITIALIZED:
        {
            hIcon = LoadIconW(shell32_hInstance, MAKEINTRESOURCEW(IDI_SHELL_FOLDER_OPTIONS));
            SendMessageW(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            break;
        }
    }
    return 0;
}

static VOID
ShowFolderOptionsDialog(HWND hWnd, HINSTANCE hInst)
{
    PROPSHEETHEADERW pinfo;
    HPROPSHEETPAGE hppages[3];
    HPROPSHEETPAGE hpage;
    UINT num_pages = 0;
    WCHAR szOptions[100];

    hpage = SH_CreatePropertySheetPage(IDD_FOLDER_OPTIONS_GENERAL, FolderOptionsGeneralDlg, 0, NULL);
    if (hpage)
        hppages[num_pages++] = hpage;

    hpage = SH_CreatePropertySheetPage(IDD_FOLDER_OPTIONS_VIEW, FolderOptionsViewDlg, 0, NULL);
    if (hpage)
        hppages[num_pages++] = hpage;

    hpage = SH_CreatePropertySheetPage(IDD_FOLDER_OPTIONS_FILETYPES, FolderOptionsFileTypesDlg, 0, NULL);
    if (hpage)
        hppages[num_pages++] = hpage;

    szOptions[0] = 0;
    LoadStringW(shell32_hInstance, IDS_FOLDER_OPTIONS, szOptions, _countof(szOptions));
    szOptions[_countof(szOptions) - 1] = 0;

    // the stub window to hide taskbar button
    DWORD style = WS_DISABLED | WS_CLIPSIBLINGS | WS_CAPTION;
    DWORD exstyle = WS_EX_WINDOWEDGE | WS_EX_TOOLWINDOW;
    CMSGlobalFolderOptionsStub stub;
    if (!stub.Create(NULL, NULL, NULL, style, exstyle))
    {
        ERR("stub.Create failed\n");
        return;
    }

    memset(&pinfo, 0x0, sizeof(PROPSHEETHEADERW));
    pinfo.dwSize = sizeof(PROPSHEETHEADERW);
    pinfo.dwFlags = PSH_NOCONTEXTHELP | PSH_USEICONID | PSH_USECALLBACK;
    pinfo.hwndParent = stub;
    pinfo.nPages = num_pages;
    pinfo.phpage = hppages;
    pinfo.pszIcon = MAKEINTRESOURCEW(IDI_SHELL_FOLDER_OPTIONS);
    pinfo.pszCaption = szOptions;
    pinfo.pfnCallback = PropSheetProc;

    PropertySheetW(&pinfo);

    stub.DestroyWindow();
}

static VOID
Options_RunDLLCommon(HWND hWnd, HINSTANCE hInst, int fOptions, DWORD nCmdShow)
{
    switch(fOptions)
    {
        case 0:
            ShowFolderOptionsDialog(hWnd, hInst);
            break;

        case 1:
            // show taskbar options dialog
            FIXME("notify explorer to show taskbar options dialog\n");
            //PostMessage(GetShellWindow(), WM_USER+22, fOptions, 0);
            break;

        default:
            FIXME("unrecognized options id %d\n", fOptions);
    }
}

/*************************************************************************
 *              Options_RunDLL (SHELL32.@)
 */
EXTERN_C VOID WINAPI
Options_RunDLL(HWND hWnd, HINSTANCE hInst, LPCSTR cmd, DWORD nCmdShow)
{
    Options_RunDLLCommon(hWnd, hInst, StrToIntA(cmd), nCmdShow);
}

/*************************************************************************
 *              Options_RunDLLA (SHELL32.@)
 */
EXTERN_C VOID WINAPI
Options_RunDLLA(HWND hWnd, HINSTANCE hInst, LPCSTR cmd, DWORD nCmdShow)
{
    Options_RunDLLCommon(hWnd, hInst, StrToIntA(cmd), nCmdShow);
}

/*************************************************************************
 *              Options_RunDLLW (SHELL32.@)
 */
EXTERN_C VOID WINAPI
Options_RunDLLW(HWND hWnd, HINSTANCE hInst, LPCWSTR cmd, DWORD nCmdShow)
{
    Options_RunDLLCommon(hWnd, hInst, StrToIntW(cmd), nCmdShow);
}
