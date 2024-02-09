/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    The main window and wWinMain etc.
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 *             Copyright 2017-2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 *             Copyright 2018 Stanislav Motylkov <x86corez@gmail.com>
 */

#include "precomp.h"

#include <dlgs.h>
#include <mapi.h>
#include <assert.h>

BOOL g_askBeforeEnlarging = FALSE;  // TODO: initialize from registry
HINSTANCE g_hinstExe = NULL;
WCHAR g_szFileName[MAX_LONG_PATH] = { 0 };
WCHAR g_szMailTempFile[MAX_LONG_PATH] = { 0 };
BOOL g_isAFile = FALSE;
BOOL g_imageSaved = FALSE;
BOOL g_showGrid = FALSE;
HWND g_hStatusBar = NULL;

CMainWindow mainWindow;

typedef HWND (WINAPI *FN_HtmlHelpW)(HWND, LPCWSTR, UINT, DWORD_PTR);

static HINSTANCE s_hHHCTRL_OCX = NULL; // HtmlHelpW needs "hhctrl.ocx"
static FN_HtmlHelpW s_pHtmlHelpW = NULL;

/* FUNCTIONS ********************************************************/

void ShowOutOfMemory(void)
{
    WCHAR szText[256];
    ::FormatMessageW(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
                     NULL,
                     ERROR_OUTOFMEMORY,
                     0,
                     szText, _countof(szText),
                     NULL);
    mainWindow.MessageBox(szText, NULL, MB_ICONERROR);
}

// get file name extension from filter string
static BOOL
FileExtFromFilter(LPWSTR pExt, OPENFILENAME *pOFN)
{
    LPWSTR pchExt = pExt;
    *pchExt = 0;

    DWORD nIndex = 1;
    for (LPCWSTR pch = pOFN->lpstrFilter; *pch; ++nIndex)
    {
        pch += lstrlen(pch) + 1;
        if (pOFN->nFilterIndex == nIndex)
        {
            for (++pch; *pch && *pch != L';'; ++pch)
            {
                *pchExt++ = *pch;
            }
            *pchExt = 0;
            CharLower(pExt);
            return TRUE;
        }
        pch += wcslen(pch) + 1;
    }
    return FALSE;
}

// Hook procedure for OPENFILENAME to change the file name extension
static UINT_PTR APIENTRY
OFNHookProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hParent;
    OFNOTIFYW *pon;
    WCHAR Path[MAX_PATH];
    switch (uMsg)
    {
    case WM_NOTIFY:
        pon = (OFNOTIFYW *)lParam;
        if (pon->hdr.code == CDN_TYPECHANGE)
        {
            hParent = GetParent(hwnd);
            SendMessageW(hParent, CDM_GETFILEPATH, _countof(Path), (LPARAM)Path);
            FileExtFromFilter(PathFindExtensionW(Path), pon->lpOFN);
            SendMessageW(hParent, CDM_SETCONTROLTEXT, cmb13, (LPARAM)PathFindFileNameW(Path));
            StringCchCopyW(pon->lpOFN->lpstrFile, pon->lpOFN->nMaxFile, Path);
        }
        break;
    }
    return 0;
}

typedef ULONG (WINAPI *FN_MAPISendMail)(LHANDLE, ULONG_PTR, lpMapiMessage, FLAGS, ULONG);
typedef ULONG (WINAPI *FN_MAPISendMailW)(LHANDLE, ULONG_PTR, lpMapiMessageW, FLAGS, ULONG);

BOOL OpenMailer(HWND hWnd, LPCWSTR pszPathName)
{
    // Delete the temporary file if any
    if (g_szMailTempFile[0])
    {
        ::DeleteFileW(g_szMailTempFile);
        g_szMailTempFile[0] = UNICODE_NULL;
    }

    CStringW strFileTitle;
    if (PathFileExistsW(pszPathName) && imageModel.IsImageSaved())
    {
        strFileTitle = PathFindFileNameW(pszPathName);
    }
    else // Not existing or not saved
    {
        // Get the name of a temporary file
        WCHAR szTempDir[MAX_PATH];
        ::GetTempPathW(_countof(szTempDir), szTempDir);
        if (!::GetTempFileNameW(szTempDir, L"afx", 0, g_szMailTempFile))
            return FALSE; // Failure

        if (PathFileExistsW(g_szFileName))
        {
            // Set file title
            strFileTitle = PathFindFileNameW(g_szFileName);

            // Copy to the temporary file
            if (!::CopyFileW(g_szFileName, g_szMailTempFile, FALSE))
            {
                g_szMailTempFile[0] = UNICODE_NULL;
                return FALSE; // Failure
            }
        }
        else
        {
            // Set file title
            strFileTitle.LoadString(IDS_DEFAULTFILENAME);
            strFileTitle += L".png";

            // Save it to the temporary file
            HBITMAP hbmLocked = imageModel.LockBitmap();
            BOOL ret = SaveDIBToFile(hbmLocked, g_szMailTempFile, FALSE, Gdiplus::ImageFormatPNG);
            imageModel.UnlockBitmap(hbmLocked);
            if (!ret)
            {
                g_szMailTempFile[0] = UNICODE_NULL;
                return FALSE; // Failure
            }
        }

        // Use the temporary file 
        pszPathName = g_szMailTempFile;
    }

    // Load "mapi32.dll"
    HINSTANCE hMAPI = LoadLibraryW(L"mapi32.dll");
    if (!hMAPI)
        return FALSE; // Failure

    // Attachment
    MapiFileDescW attachmentW = { 0 };
    attachmentW.nPosition = (ULONG)-1;
    attachmentW.lpszPathName = (LPWSTR)pszPathName;
    attachmentW.lpszFileName = (LPWSTR)(LPCWSTR)strFileTitle;

    // Message with attachment
    MapiMessageW messageW = { 0 };
    messageW.lpszSubject = NULL;
    messageW.nFileCount = 1;
    messageW.lpFiles = &attachmentW;

    // First, try to open the mailer by the function of Unicode version
    FN_MAPISendMailW pMAPISendMailW = (FN_MAPISendMailW)::GetProcAddress(hMAPI, "MAPISendMailW");
    if (pMAPISendMailW)
    {
        pMAPISendMailW(0, (ULONG_PTR)hWnd, &messageW, MAPI_DIALOG | MAPI_LOGON_UI, 0);
        ::FreeLibrary(hMAPI);
        return TRUE; // MAPISendMailW will show an error message on failure
    }

    // Convert to ANSI strings
    CStringA szPathNameA(pszPathName), szFileTitleA(strFileTitle);

    MapiFileDesc attachment = { 0 };
    attachment.nPosition = (ULONG)-1;
    attachment.lpszPathName = (LPSTR)(LPCSTR)szPathNameA;
    attachment.lpszFileName = (LPSTR)(LPCSTR)szFileTitleA;

    MapiMessage message = { 0 };
    message.lpszSubject = NULL;
    message.nFileCount = 1;
    message.lpFiles = &attachment;

    // Try again but in ANSI version
    FN_MAPISendMail pMAPISendMail = (FN_MAPISendMail)::GetProcAddress(hMAPI, "MAPISendMail");
    if (pMAPISendMail)
    {
        pMAPISendMail(0, (ULONG_PTR)hWnd, &message, MAPI_DIALOG | MAPI_LOGON_UI, 0);
        ::FreeLibrary(hMAPI);
        return TRUE; // MAPISendMail will show an error message on failure
    }

    ::FreeLibrary(hMAPI);
    return FALSE; // Failure
}

BOOL CMainWindow::GetOpenFileName(IN OUT LPWSTR pszFile, INT cchMaxFile)
{
    static OPENFILENAMEW ofn = { 0 };
    static CStringW strFilter;

    if (ofn.lStructSize == 0)
    {
        // The "All Files" item text
        CStringW strAllPictureFiles;
        strAllPictureFiles.LoadString(g_hinstExe, IDS_ALLPICTUREFILES);

        // Get the import filter
        CSimpleArray<GUID> aguidFileTypesI;
        CImage::GetImporterFilterString(strFilter, aguidFileTypesI, strAllPictureFiles,
                                        CImage::excludeDefaultLoad, L'|');
        strFilter.Replace(L'|', UNICODE_NULL);

        // Initializing the OPENFILENAME structure for GetOpenFileName
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner   = m_hWnd;
        ofn.hInstance   = g_hinstExe;
        ofn.lpstrFilter = strFilter;
        ofn.Flags       = OFN_EXPLORER | OFN_HIDEREADONLY;
        ofn.lpstrDefExt = L"png";
    }

    ofn.lpstrFile = pszFile;
    ofn.nMaxFile  = cchMaxFile;
    return ::GetOpenFileNameW(&ofn);
}

BOOL CMainWindow::GetSaveFileName(IN OUT LPWSTR pszFile, INT cchMaxFile)
{
    static OPENFILENAMEW sfn = { 0 };
    static CStringW strFilter;

    if (sfn.lStructSize == 0)
    {
        // Get the export filter
        CSimpleArray<GUID> aguidFileTypesE;
        CImage::GetExporterFilterString(strFilter, aguidFileTypesE, NULL,
                                        CImage::excludeDefaultSave, L'|');
        strFilter.Replace(L'|', UNICODE_NULL);

        // Initializing the OPENFILENAME structure for GetSaveFileName
        ZeroMemory(&sfn, sizeof(sfn));
        sfn.lStructSize = sizeof(sfn);
        sfn.hwndOwner   = m_hWnd;
        sfn.hInstance   = g_hinstExe;
        sfn.lpstrFilter = strFilter;
        sfn.Flags       = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_ENABLEHOOK;
        sfn.lpfnHook    = OFNHookProc;
        sfn.lpstrDefExt = L"png";

        LPWSTR pchDotExt = PathFindExtensionW(pszFile);
        if (*pchDotExt == UNICODE_NULL)
        {
            // Choose PNG
            StringCchCatW(pszFile, cchMaxFile, L".png");
            for (INT i = 0; i < aguidFileTypesE.GetSize(); ++i)
            {
                if (aguidFileTypesE[i] == Gdiplus::ImageFormatPNG)
                {
                    sfn.nFilterIndex = i + 1;
                    break;
                }
            }
        }
    }

    sfn.lpstrFile = pszFile;
    sfn.nMaxFile  = cchMaxFile;
    return ::GetSaveFileNameW(&sfn);
}

BOOL CMainWindow::ChooseColor(IN OUT COLORREF *prgbColor)
{
    static CHOOSECOLOR choosecolor = { 0 };
    static COLORREF custColors[16] =
    {
        0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff,
        0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff
    };

    if (choosecolor.lStructSize == 0)
    {
        // Initializing the CHOOSECOLOR structure for ChooseColor
        ZeroMemory(&choosecolor, sizeof(choosecolor));
        choosecolor.lStructSize  = sizeof(choosecolor);
        choosecolor.hwndOwner    = m_hWnd;
        choosecolor.lpCustColors = custColors;
    }

    choosecolor.Flags = CC_RGBINIT;
    choosecolor.rgbResult = *prgbColor;
    if (!::ChooseColor(&choosecolor))
        return FALSE;

    *prgbColor = choosecolor.rgbResult;
    return TRUE;
}

HWND CMainWindow::DoCreate()
{
    ::LoadStringW(g_hinstExe, IDS_DEFAULTFILENAME, g_szFileName, _countof(g_szFileName));

    CStringW strTitle;
    strTitle.Format(IDS_WINDOWTITLE, PathFindFileName(g_szFileName));

    RECT& rc = registrySettings.WindowPlacement.rcNormalPosition;
    return Create(HWND_DESKTOP, rc, strTitle, WS_OVERLAPPEDWINDOW, WS_EX_ACCEPTFILES);
}

// A wrapper function for HtmlHelpW
static HWND DoHtmlHelpW(HWND hwndCaller, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData)
{
    WCHAR szPath[MAX_PATH];

    if (!s_hHHCTRL_OCX && (uCommand != HH_CLOSE_ALL))
    {
        // The function loads the system library, not local
        GetSystemDirectoryW(szPath, _countof(szPath));
        StringCchCatW(szPath, _countof(szPath), L"\\hhctrl.ocx");
        s_hHHCTRL_OCX = LoadLibraryW(szPath);
        if (s_hHHCTRL_OCX)
            s_pHtmlHelpW = (FN_HtmlHelpW)GetProcAddress(s_hHHCTRL_OCX, "HtmlHelpW");
    }

    if (!s_pHtmlHelpW)
        return NULL;

    return s_pHtmlHelpW(hwndCaller, pszFile, uCommand, dwData);
}

void CMainWindow::alignChildrenToMainWindow()
{
    RECT clientRect, rc;
    GetClientRect(&clientRect);
    RECT rcSpace = clientRect;
    const UINT uFlags = (SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREPOSITION | SWP_NOCOPYBITS);

    if (::IsWindowVisible(g_hStatusBar))
    {
        ::GetWindowRect(g_hStatusBar, &rc);
        rcSpace.bottom -= rc.bottom - rc.top;
    }

    HDWP hDWP = ::BeginDeferWindowPos(3);

    if (::IsWindowVisible(toolBoxContainer))
    {
        if (registrySettings.Bar2ID == BAR2ID_RIGHT)
        {
            hDWP = ::DeferWindowPos(hDWP, toolBoxContainer, NULL,
                                    rcSpace.right - CX_TOOLBAR, rcSpace.top,
                                    CX_TOOLBAR, rcSpace.bottom - rcSpace.top,
                                    uFlags);
            rcSpace.right -= CX_TOOLBAR;
        }
        else
        {
            hDWP = ::DeferWindowPos(hDWP, toolBoxContainer, NULL,
                                    rcSpace.left, rcSpace.top,
                                    CX_TOOLBAR, rcSpace.bottom - rcSpace.top,
                                    uFlags);
            rcSpace.left += CX_TOOLBAR;
        }
    }

    if (::IsWindowVisible(paletteWindow))
    {
        if (registrySettings.Bar1ID == BAR1ID_BOTTOM)
        {
            hDWP = ::DeferWindowPos(hDWP, paletteWindow, NULL,
                                    rcSpace.left, rcSpace.bottom - CY_PALETTE,
                                    rcSpace.right - rcSpace.left, CY_PALETTE,
                                    uFlags);
            rcSpace.bottom -= CY_PALETTE;
        }
        else
        {
            hDWP = ::DeferWindowPos(hDWP, paletteWindow, NULL,
                                    rcSpace.left, rcSpace.top,
                                    rcSpace.right - rcSpace.left, CY_PALETTE,
                                    uFlags);
            rcSpace.top += CY_PALETTE;
        }
    }

    if (canvasWindow.IsWindow())
    {
        hDWP = ::DeferWindowPos(hDWP, canvasWindow, NULL,
                                rcSpace.left, rcSpace.top,
                                rcSpace.right - rcSpace.left, rcSpace.bottom - rcSpace.top,
                                uFlags);
    }

    ::EndDeferWindowPos(hDWP);
}

void CMainWindow::saveImage(BOOL overwrite)
{
    canvasWindow.OnEndDraw(FALSE);

    // Is the extension not supported?
    PWCHAR pchDotExt = PathFindExtensionW(g_szFileName);
    if (pchDotExt && *pchDotExt && !CImageDx::IsExtensionSupported(pchDotExt))
    {
        // Remove the extension
        PathRemoveExtensionW(g_szFileName);
        // No overwrite
        overwrite = FALSE;
    }

    if (g_isAFile && overwrite)
    {
        imageModel.SaveImage(g_szFileName);
    }
    else if (GetSaveFileName(g_szFileName, _countof(g_szFileName)))
    {
        imageModel.SaveImage(g_szFileName);
    }
}

void CMainWindow::InsertSelectionFromHBITMAP(HBITMAP bitmap, HWND window)
{
    int width = GetDIBWidth(bitmap);
    int height = GetDIBHeight(bitmap);
    int curWidth = imageModel.GetWidth();
    int curHeight = imageModel.GetHeight();

    if (width > curWidth || height > curHeight)
    {
        BOOL shouldEnlarge = TRUE;

        if (g_askBeforeEnlarging)
        {
            WCHAR programname[20];
            WCHAR shouldEnlargePromptText[100];

            ::LoadStringW(g_hinstExe, IDS_PROGRAMNAME, programname, _countof(programname));
            ::LoadStringW(g_hinstExe, IDS_ENLARGEPROMPTTEXT, shouldEnlargePromptText, _countof(shouldEnlargePromptText));

            switch (MessageBox(shouldEnlargePromptText, programname, MB_YESNOCANCEL | MB_ICONQUESTION))
            {
                case IDYES:
                    break;
                case IDNO:
                    shouldEnlarge = FALSE;
                    break;
                case IDCANCEL:
                    return;
            }
        }

        if (shouldEnlarge)
        {
            if (width > curWidth)
                curWidth = width;

            if (height > curHeight)
                curHeight = height;

            imageModel.Crop(curWidth, curHeight, 0, 0);
        }
    }

    toolsModel.SetActiveTool(TOOL_RECTSEL);

    selectionModel.InsertFromHBITMAP(bitmap, 0, 0);
    selectionModel.m_bShow = TRUE;
    imageModel.NotifyImageChanged();
}

LRESULT CMainWindow::OnMouseWheel(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    INT zDelta = (SHORT)HIWORD(wParam);

    if (::GetKeyState(VK_CONTROL) < 0) // Ctrl+Wheel
    {
        if (zDelta < 0)
        {
            if (toolsModel.GetZoom() > MIN_ZOOM)
                canvasWindow.zoomTo(toolsModel.GetZoom() / 2);
        }
        else if (zDelta > 0)
        {
            if (toolsModel.GetZoom() < MAX_ZOOM)
                canvasWindow.zoomTo(toolsModel.GetZoom() * 2);
        }
    }
    else // Wheel only
    {
        UINT nCount = 3;
        if (::GetAsyncKeyState(VK_SHIFT) < 0)
        {
#ifndef SPI_GETWHEELSCROLLCHARS
    #define SPI_GETWHEELSCROLLCHARS 0x006C  // Needed for pre-NT6 PSDK
#endif
            SystemParametersInfoW(SPI_GETWHEELSCROLLCHARS, 0, &nCount, 0);
            for (UINT i = 0; i < nCount; ++i)
            {
                if (zDelta < 0)
                    ::PostMessageW(canvasWindow, WM_HSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), 0);
                else if (zDelta > 0)
                    ::PostMessageW(canvasWindow, WM_HSCROLL, MAKEWPARAM(SB_LINEUP, 0), 0);
            }
        }
        else
        {
            SystemParametersInfoW(SPI_GETWHEELSCROLLLINES, 0, &nCount, 0);
            for (UINT i = 0; i < nCount; ++i)
            {
                if (zDelta < 0)
                    ::PostMessageW(canvasWindow, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), 0);
                else if (zDelta > 0)
                    ::PostMessageW(canvasWindow, WM_VSCROLL, MAKEWPARAM(SB_LINEUP, 0), 0);
            }
        }
    }

    return 0;
}

LRESULT CMainWindow::OnDropFiles(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    WCHAR droppedfile[MAX_PATH];

    HDROP hDrop = (HDROP)wParam;
    DragQueryFile(hDrop, 0, droppedfile, _countof(droppedfile));
    DragFinish(hDrop);

    ConfirmSave() && DoLoadImageFile(m_hWnd, droppedfile, TRUE);

    return 0;
}

LRESULT CMainWindow::OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // Loading and setting the window menu from resource
    m_hMenu = ::LoadMenuW(g_hinstExe, MAKEINTRESOURCEW(ID_MENU));
    SetMenu(m_hMenu);

    // Create the status bar
    DWORD style = SBARS_SIZEGRIP | WS_CHILD | (registrySettings.ShowStatusBar ? WS_VISIBLE : 0);
    g_hStatusBar = ::CreateWindowExW(0, STATUSCLASSNAME, NULL, style, 0, 0, 0, 0, m_hWnd,
                                     NULL, g_hinstExe, NULL);
    ::SendMessageW(g_hStatusBar, SB_SETMINHEIGHT, 21, 0);

    // Create the tool box
    toolBoxContainer.DoCreate(m_hWnd);

    // Create the palette window
    RECT rcEmpty = { 0, 0, 0, 0 }; // Rely on WM_SIZE
    style = WS_CHILD | (registrySettings.ShowPalette ? WS_VISIBLE : 0);
    paletteWindow.Create(m_hWnd, rcEmpty, NULL, style, WS_EX_STATICEDGE);

    // Create the canvas
    style = WS_CHILD | WS_GROUP | WS_HSCROLL | WS_VSCROLL | WS_VISIBLE;
    canvasWindow.Create(m_hWnd, rcEmpty, NULL, style, WS_EX_CLIENTEDGE);

    // Create and show the miniature if necessary
    if (registrySettings.ShowThumbnail)
    {
        miniature.DoCreate(m_hWnd);
        miniature.ShowWindow(SW_SHOWNOACTIVATE);
    }

    // Set icon
    SendMessage(WM_SETICON, ICON_BIG, (LPARAM)::LoadIconW(g_hinstExe, MAKEINTRESOURCEW(IDI_APPICON)));
    SendMessage(WM_SETICON, ICON_SMALL, (LPARAM)::LoadIconW(g_hinstExe, MAKEINTRESOURCEW(IDI_APPICON)));

    return 0;
}

LRESULT CMainWindow::OnDestroy(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    registrySettings.WindowPlacement.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(&(registrySettings.WindowPlacement));

    DoHtmlHelpW(NULL, NULL, HH_CLOSE_ALL, 0);

    if (s_hHHCTRL_OCX)
    {
        FreeLibrary(s_hHHCTRL_OCX);
        s_hHHCTRL_OCX = NULL;
        s_pHtmlHelpW = NULL;
    }

    SetMenu(NULL);
    if (m_hMenu)
    {
        ::DestroyMenu(m_hMenu);
        m_hMenu = NULL;
    }

    PostQuitMessage(0); /* send a WM_QUIT to the message queue */
    return 0;
}

BOOL CMainWindow::ConfirmSave()
{
    canvasWindow.OnEndDraw(FALSE);

    if (imageModel.IsImageSaved())
        return TRUE;

    CStringW strProgramName;
    strProgramName.LoadString(IDS_PROGRAMNAME);

    CStringW strSavePromptText;
    strSavePromptText.Format(IDS_SAVEPROMPTTEXT, PathFindFileName(g_szFileName));

    switch (MessageBox(strSavePromptText, strProgramName, MB_YESNOCANCEL | MB_ICONQUESTION))
    {
        case IDYES:
            saveImage(TRUE);
            return imageModel.IsImageSaved();
        case IDNO:
            return TRUE;
        case IDCANCEL:
            return FALSE;
    }

    return TRUE;
}

LRESULT CMainWindow::OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (ConfirmSave())
    {
        DestroyWindow();
    }
    return 0;
}

void CMainWindow::ProcessFileMenu(HMENU hPopupMenu)
{
    LPCWSTR dotext = PathFindExtensionW(g_szFileName);
    BOOL isBMP = FALSE;
    if (_wcsicmp(dotext, L".bmp") == 0 ||
        _wcsicmp(dotext, L".dib") == 0 ||
        _wcsicmp(dotext, L".rle") == 0)
    {
        isBMP = TRUE;
    }

    UINT uWallpaperEnabled = ENABLED_IF(g_isAFile && isBMP && g_fileSize > 0);
    ::EnableMenuItem(hPopupMenu, IDM_FILEASWALLPAPERPLANE,     uWallpaperEnabled);
    ::EnableMenuItem(hPopupMenu, IDM_FILEASWALLPAPERCENTERED,  uWallpaperEnabled);
    ::EnableMenuItem(hPopupMenu, IDM_FILEASWALLPAPERSTRETCHED, uWallpaperEnabled);

    for (INT iItem = 0; iItem < MAX_RECENT_FILES; ++iItem)
        RemoveMenu(hPopupMenu, IDM_FILE1 + iItem, MF_BYCOMMAND);

    if (registrySettings.strFiles[0].IsEmpty())
        return;

    RemoveMenu(hPopupMenu, IDM_FILEMOSTRECENTLYUSEDFILE, MF_BYCOMMAND);

    INT cMenuItems = GetMenuItemCount(hPopupMenu);

    for (INT iItem = 0; iItem < MAX_RECENT_FILES; ++iItem)
    {
        CStringW& strFile = registrySettings.strFiles[iItem];
        if (strFile.IsEmpty())
            break;

        // Condense the lengthy pathname by using '...'
#define MAX_RECENT_PATHNAME_DISPLAY 30
        CPath pathFile(strFile);
        pathFile.CompactPathEx(MAX_RECENT_PATHNAME_DISPLAY);
        assert(wcslen((LPCWSTR)pathFile) <= MAX_RECENT_PATHNAME_DISPLAY);

        // Add an accelerator (by '&') to the item number for quick access
        WCHAR szText[4 + MAX_RECENT_PATHNAME_DISPLAY + 1];
        StringCchPrintfW(szText, _countof(szText), L"&%u %s", iItem + 1, (LPCWSTR)pathFile);

        INT iMenuItem = (cMenuItems - 2) + iItem;
        InsertMenu(hPopupMenu, iMenuItem, MF_BYPOSITION | MF_STRING, IDM_FILE1 + iItem, szText);
    }
}

BOOL CMainWindow::CanUndo() const
{
    if (toolsModel.GetActiveTool() == TOOL_TEXT && ::IsWindowVisible(textEditWindow))
        return (BOOL)textEditWindow.SendMessage(EM_CANUNDO);
    if (selectionModel.m_bShow && toolsModel.IsSelection())
        return TRUE;
    return imageModel.CanUndo();
}

BOOL CMainWindow::CanRedo() const
{
    if (toolsModel.GetActiveTool() == TOOL_TEXT && ::IsWindowVisible(textEditWindow))
        return FALSE; // There is no "WM_REDO" in EDIT control
    return imageModel.CanRedo();
}

BOOL CMainWindow::CanPaste() const
{
    if (toolsModel.GetActiveTool() == TOOL_TEXT && ::IsWindowVisible(textEditWindow))
        return ::IsClipboardFormatAvailable(CF_UNICODETEXT);

    return (::IsClipboardFormatAvailable(CF_ENHMETAFILE) ||
            ::IsClipboardFormatAvailable(CF_DIB) ||
            ::IsClipboardFormatAvailable(CF_BITMAP));
}

LRESULT CMainWindow::OnInitMenuPopup(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HMENU menu = (HMENU)wParam;
    BOOL trueSelection = (selectionModel.m_bShow && toolsModel.IsSelection());
    BOOL textShown = (toolsModel.GetActiveTool() == TOOL_TEXT && ::IsWindowVisible(textEditWindow));
    DWORD dwStart = 0, dwEnd = 0;
    if (textShown)
        textEditWindow.SendMessage(EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
    BOOL hasTextSel = (dwStart < dwEnd);

    //
    // File menu
    //
    if (::GetSubMenu(GetMenu(), 0) == menu)
    {
        ProcessFileMenu(menu);
    }

    //
    // Edit menu
    //
    EnableMenuItem(menu, IDM_EDITUNDO, ENABLED_IF(CanUndo()));
    EnableMenuItem(menu, IDM_EDITREDO, ENABLED_IF(CanRedo()));
    EnableMenuItem(menu, IDM_EDITCUT, ENABLED_IF(textShown ? hasTextSel : trueSelection));
    EnableMenuItem(menu, IDM_EDITCOPY, ENABLED_IF(textShown ? hasTextSel : trueSelection));
    EnableMenuItem(menu, IDM_EDITDELETESELECTION,
                   ENABLED_IF(textShown ? hasTextSel : trueSelection));
    EnableMenuItem(menu, IDM_EDITINVERTSELECTION, ENABLED_IF(trueSelection));
    EnableMenuItem(menu, IDM_EDITCOPYTO, ENABLED_IF(trueSelection));
    EnableMenuItem(menu, IDM_EDITPASTE, ENABLED_IF(CanPaste()));

    //
    // View menu
    //
    CheckMenuItem(menu, IDM_VIEWTOOLBOX, CHECKED_IF(::IsWindowVisible(toolBoxContainer)));
    CheckMenuItem(menu, IDM_VIEWCOLORPALETTE, CHECKED_IF(::IsWindowVisible(paletteWindow)));
    CheckMenuItem(menu, IDM_VIEWSTATUSBAR,    CHECKED_IF(::IsWindowVisible(g_hStatusBar)));
    CheckMenuItem(menu, IDM_FORMATICONBAR, CHECKED_IF(::IsWindowVisible(fontsDialog)));
    EnableMenuItem(menu, IDM_FORMATICONBAR, ENABLED_IF(toolsModel.GetActiveTool() == TOOL_TEXT));
    CheckMenuItem(menu, IDM_VIEWZOOM125, CHECKED_IF(toolsModel.GetZoom() == 125));
    CheckMenuItem(menu, IDM_VIEWZOOM25,  CHECKED_IF(toolsModel.GetZoom() == 250));
    CheckMenuItem(menu, IDM_VIEWZOOM50,  CHECKED_IF(toolsModel.GetZoom() == 500));
    CheckMenuItem(menu, IDM_VIEWZOOM100, CHECKED_IF(toolsModel.GetZoom() == 1000));
    CheckMenuItem(menu, IDM_VIEWZOOM200, CHECKED_IF(toolsModel.GetZoom() == 2000));
    CheckMenuItem(menu, IDM_VIEWZOOM400, CHECKED_IF(toolsModel.GetZoom() == 4000));
    CheckMenuItem(menu, IDM_VIEWZOOM800, CHECKED_IF(toolsModel.GetZoom() == 8000));
    CheckMenuItem(menu, IDM_VIEWSHOWGRID,      CHECKED_IF(g_showGrid));
    CheckMenuItem(menu, IDM_VIEWSHOWMINIATURE, CHECKED_IF(registrySettings.ShowThumbnail));

    //
    // Image menu
    //
    EnableMenuItem(menu, IDM_IMAGECROP, ENABLED_IF(selectionModel.m_bShow));
    EnableMenuItem(menu, IDM_IMAGEDELETEIMAGE, ENABLED_IF(!selectionModel.m_bShow));
    CheckMenuItem(menu, IDM_IMAGEDRAWOPAQUE, CHECKED_IF(!toolsModel.IsBackgroundTransparent()));

    //
    // Palette menu
    //
    CheckMenuItem(menu, IDM_COLORSMODERNPALETTE, CHECKED_IF(paletteModel.SelectedPalette() == PAL_MODERN));
    CheckMenuItem(menu, IDM_COLORSOLDPALETTE,    CHECKED_IF(paletteModel.SelectedPalette() == PAL_OLDTYPE));
    return 0;
}

LRESULT CMainWindow::OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    int test[] = { LOWORD(lParam) - 260, LOWORD(lParam) - 140, LOWORD(lParam) - 20 };
    if (::IsWindow(g_hStatusBar))
    {
        ::SendMessageW(g_hStatusBar, WM_SIZE, 0, 0);
        ::SendMessageW(g_hStatusBar, SB_SETPARTS, 3, (LPARAM)&test);
    }
    alignChildrenToMainWindow();
    return 0;
}

LRESULT CMainWindow::OnGetMinMaxInfo(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    MINMAXINFO *mm = (MINMAXINFO*)lParam;
    mm->ptMinTrackSize = { 330, 360 };
    return 0;
}

LRESULT CMainWindow::OnKeyDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    switch (wParam)
    {
        case VK_ESCAPE:
            canvasWindow.PostMessage(nMsg, wParam, lParam);
            break;
        case VK_LEFT:
            selectionModel.moveSelection(-1, 0);
            break;
        case VK_RIGHT:
            selectionModel.moveSelection(+1, 0);
            break;
        case VK_UP:
            selectionModel.moveSelection(0, -1);
            break;
        case VK_DOWN:
            selectionModel.moveSelection(0, +1);
            break;
        default:
            break;
    }
    return 0;
}

LRESULT CMainWindow::OnSysColorChange(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    /* Redirect message to common controls */
    HWND hToolbar = FindWindowEx(toolBoxContainer.m_hWnd, NULL, TOOLBARCLASSNAME, NULL);
    ::SendMessageW(hToolbar, WM_SYSCOLORCHANGE, 0, 0);
    return 0;
}

LRESULT CMainWindow::OnCommand(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // Disable commands while dragging mouse
    if (canvasWindow.m_drawing && ::GetCapture())
    {
        ATLTRACE("locking!\n");
        return 0;
    }

    BOOL textShown = (toolsModel.GetActiveTool() == TOOL_TEXT && ::IsWindowVisible(textEditWindow));
    switch (LOWORD(wParam))
    {
        case IDM_HELPINFO:
        {
            WCHAR infotitle[100], infotext[200];
            ::LoadStringW(g_hinstExe, IDS_INFOTITLE, infotitle, _countof(infotitle));
            ::LoadStringW(g_hinstExe, IDS_INFOTEXT, infotext, _countof(infotext));
            ::ShellAboutW(m_hWnd, infotitle, infotext,
                          LoadIconW(g_hinstExe, MAKEINTRESOURCEW(IDI_APPICON)));
            break;
        }
        case IDM_HELPHELPTOPICS:
            DoHtmlHelpW(m_hWnd, L"%SystemRoot%\\Help\\mspaint.chm", HH_DISPLAY_TOPIC, 0);
            break;
        case IDM_FILEEXIT:
            SendMessage(WM_CLOSE, wParam, lParam);
            break;
        case IDM_FILENEW:
            if (ConfirmSave())
            {
                InitializeImage(NULL, NULL, FALSE);
            }
            break;
        case IDM_FILEOPEN:
            {
                WCHAR szFileName[MAX_LONG_PATH] = L"";
                if (ConfirmSave() && GetOpenFileName(szFileName, _countof(szFileName)))
                {
                    DoLoadImageFile(m_hWnd, szFileName, TRUE);
                }
                break;
            }
        case IDM_FILESAVE:
            saveImage(TRUE);
            break;
        case IDM_FILESAVEAS:
            saveImage(FALSE);
            break;
        case IDM_FILEPAGESETUP:
            // DUMMY: Shows the dialog only, no functionality
            PAGESETUPDLG psd;
            ZeroMemory(&psd, sizeof(psd));
            psd.lStructSize = sizeof(psd);
            psd.hwndOwner = m_hWnd;
            PageSetupDlg(&psd);
            break;
        case IDM_FILEPRINT:
            // TODO: Test whether it actually works
            PRINTDLG pd;
            ZeroMemory(&pd, sizeof(pd));
            pd.lStructSize = sizeof(pd);
            pd.hwndOwner = m_hWnd;
            pd.hDevMode = NULL;  // freed by user
            pd.hDevNames = NULL;  // freed by user
            pd.Flags = PD_USEDEVMODECOPIESANDCOLLATE | PD_RETURNDC;
            pd.nCopies = 1;
            pd.nFromPage = 0xffff;
            pd.nToPage = 0xffff;
            pd.nMinPage = 1;
            pd.nMaxPage = 0xffff;
            if (PrintDlg(&pd) == TRUE)
            {
                ::BitBlt(pd.hDC, 0, 0, imageModel.GetWidth(), imageModel.GetHeight(), imageModel.GetDC(), 0, 0, SRCCOPY);
                DeleteDC(pd.hDC);
            }
            if (pd.hDevMode)
                GlobalFree(pd.hDevMode);
            if (pd.hDevNames)
                GlobalFree(pd.hDevNames);
            break;
        case IDM_FILESEND:
            canvasWindow.OnEndDraw(FALSE);
            if (!OpenMailer(m_hWnd, g_szFileName))
            {
                ShowError(IDS_CANTSENDMAIL);
            }
            break;
        case IDM_FILEASWALLPAPERPLANE:
            RegistrySettings::SetWallpaper(g_szFileName, RegistrySettings::TILED);
            break;
        case IDM_FILEASWALLPAPERCENTERED:
            RegistrySettings::SetWallpaper(g_szFileName, RegistrySettings::CENTERED);
            break;
        case IDM_FILEASWALLPAPERSTRETCHED:
            RegistrySettings::SetWallpaper(g_szFileName, RegistrySettings::STRETCHED);
            break;
        case IDM_FILE1:
        case IDM_FILE2:
        case IDM_FILE3:
        case IDM_FILE4:
        {
            INT iFile = LOWORD(wParam) - IDM_FILE1;
            if (ConfirmSave())
                DoLoadImageFile(m_hWnd, registrySettings.strFiles[iFile], TRUE);
            break;
        }
        case IDM_EDITUNDO:
            if (textShown)
            {
                textEditWindow.PostMessage(WM_UNDO, 0, 0);
                break;
            }
            canvasWindow.OnEndDraw(FALSE);
            imageModel.Undo();
            break;
        case IDM_EDITREDO:
            if (textShown)
            {
                // There is no "WM_REDO" in EDIT control
                break;
            }
            canvasWindow.OnEndDraw(FALSE);
            imageModel.Redo();
            break;
        case IDM_EDITCOPY:
            if (textShown)
            {
                textEditWindow.SendMessage(WM_COPY);
                break;
            }
            if (!selectionModel.m_bShow || !OpenClipboard())
                break;

            EmptyClipboard();

            selectionModel.TakeOff();

            {
                HBITMAP hbmCopy = selectionModel.GetSelectionContents();
                HGLOBAL hGlobal = BitmapToClipboardDIB(hbmCopy);
                if (hGlobal)
                    ::SetClipboardData(CF_DIB, hGlobal);
                else
                    ShowOutOfMemory();
                ::DeleteObject(hbmCopy);
            }

            CloseClipboard();
            break;
        case IDM_EDITCUT:
            if (textShown)
            {
                textEditWindow.SendMessage(WM_CUT);
                break;
            }
            /* Copy */
            SendMessage(WM_COMMAND, IDM_EDITCOPY, 0);
            /* Delete selection */
            SendMessage(WM_COMMAND, IDM_EDITDELETESELECTION, 0);
            break;
        case IDM_EDITPASTE:
            if (textShown)
            {
                textEditWindow.SendMessage(WM_PASTE);
                break;
            }

            if (!OpenClipboard())
                break;

            // In many cases, CF_ENHMETAFILE provides a better image than CF_DIB
            if (::IsClipboardFormatAvailable(CF_ENHMETAFILE))
            {
                HENHMETAFILE hEMF = (HENHMETAFILE)::GetClipboardData(CF_ENHMETAFILE);
                if (hEMF)
                {
                    HBITMAP hbm = BitmapFromHEMF(hEMF);
                    ::DeleteEnhMetaFile(hEMF);
                    if (hbm)
                    {
                        InsertSelectionFromHBITMAP(hbm, m_hWnd);
                        CloseClipboard();
                        break;
                    }
                }
            }

            // In many cases, CF_DIB provides a better image than CF_BITMAP
            if (::IsClipboardFormatAvailable(CF_DIB))
            {
                HBITMAP hbm = BitmapFromClipboardDIB(::GetClipboardData(CF_DIB));
                if (hbm)
                {
                    InsertSelectionFromHBITMAP(hbm, m_hWnd);
                    CloseClipboard();
                    break;
                }
            }

            // The last resort
            if (::IsClipboardFormatAvailable(CF_BITMAP))
            {
                HBITMAP hbm = (HBITMAP)::GetClipboardData(CF_BITMAP);
                if (hbm)
                {
                    InsertSelectionFromHBITMAP(hbm, m_hWnd);
                    CloseClipboard();
                    break;
                }
            }

            // Failed to paste
            {
                CStringW strText, strTitle;
                strText.LoadString(IDS_CANTPASTE);
                strTitle.LoadString(IDS_PROGRAMNAME);
                MessageBox(strText, strTitle, MB_ICONINFORMATION);
            }

            CloseClipboard();
            break;
        case IDM_EDITDELETESELECTION:
        {
            if (textShown)
            {
                textEditWindow.SendMessage(WM_CLEAR);
                break;
            }
            switch (toolsModel.GetActiveTool())
            {
                case TOOL_FREESEL:
                case TOOL_RECTSEL:
                    selectionModel.DeleteSelection();
                    break;

                case TOOL_TEXT:
                    canvasWindow.OnEndDraw(TRUE);
                    break;
                default:
                    break;
            }
            break;
        }
        case IDM_EDITSELECTALL:
        {
            if (textShown)
            {
                textEditWindow.SendMessage(EM_SETSEL, 0, -1);
                break;
            }
            HWND hToolbar = FindWindowEx(toolBoxContainer.m_hWnd, NULL, TOOLBARCLASSNAME, NULL);
            ::SendMessageW(hToolbar, TB_CHECKBUTTON, ID_RECTSEL, MAKELPARAM(TRUE, 0));
            toolsModel.selectAll();
            canvasWindow.Invalidate(TRUE);
            break;
        }
        case IDM_EDITCOPYTO:
        {
            WCHAR szFileName[MAX_LONG_PATH];
            ::LoadStringW(g_hinstExe, IDS_DEFAULTFILENAME, szFileName, _countof(szFileName));
            if (GetSaveFileName(szFileName, _countof(szFileName)))
            {
                HBITMAP hbmSelection = selectionModel.GetSelectionContents();
                if (!hbmSelection)
                {
                    ShowOutOfMemory();
                    break;
                }
                SaveDIBToFile(hbmSelection, szFileName, FALSE);
                DeleteObject(hbmSelection);
            }
            break;
        }
        case IDM_EDITPASTEFROM:
        {
            WCHAR szFileName[MAX_LONG_PATH] = L"";
            if (GetOpenFileName(szFileName, _countof(szFileName)))
            {
                HBITMAP hbmNew = DoLoadImageFile(m_hWnd, szFileName, FALSE);
                if (hbmNew)
                    InsertSelectionFromHBITMAP(hbmNew, m_hWnd);
            }
            break;
        }
        case IDM_COLORSEDITPALETTE:
        {
            COLORREF rgbColor = paletteModel.GetFgColor();
            if (ChooseColor(&rgbColor))
                paletteModel.SetFgColor(rgbColor);
            break;
        }
        case IDM_COLORSMODERNPALETTE:
            paletteModel.SelectPalette(PAL_MODERN);
            break;
        case IDM_COLORSOLDPALETTE:
            paletteModel.SelectPalette(PAL_OLDTYPE);
            break;
        case IDM_IMAGEINVERTCOLORS:
        {
            if (selectionModel.m_bShow)
                selectionModel.InvertSelection();
            else
                imageModel.InvertColors();
            break;
        }
        case IDM_IMAGEDELETEIMAGE:
            imageModel.PushImageForUndo();
            Rect(imageModel.GetDC(), 0, 0, imageModel.GetWidth(), imageModel.GetHeight(), paletteModel.GetBgColor(), paletteModel.GetBgColor(), 0, TRUE);
            imageModel.NotifyImageChanged();
            break;
        case IDM_IMAGEROTATEMIRROR:
            {
                CWaitCursor waitCursor;
                canvasWindow.updateScrollPos();
                switch (mirrorRotateDialog.DoModal(mainWindow.m_hWnd))
                {
                    case 1: /* flip horizontally */
                    {
                        if (selectionModel.m_bShow)
                            selectionModel.FlipHorizontally();
                        else
                            imageModel.FlipHorizontally();
                        break;
                    }
                    case 2: /* flip vertically */
                    {
                        if (selectionModel.m_bShow)
                            selectionModel.FlipVertically();
                        else
                            imageModel.FlipVertically();
                        break;
                    }
                    case 3: /* rotate 90 degrees */
                    {
                        if (selectionModel.m_bShow)
                            selectionModel.RotateNTimes90Degrees(1);
                        else
                            imageModel.RotateNTimes90Degrees(1);
                        break;
                    }
                    case 4: /* rotate 180 degrees */
                    {
                        if (selectionModel.m_bShow)
                            selectionModel.RotateNTimes90Degrees(2);
                        else
                            imageModel.RotateNTimes90Degrees(2);
                        break;
                    }
                    case 5: /* rotate 270 degrees */
                    {
                        if (selectionModel.m_bShow)
                            selectionModel.RotateNTimes90Degrees(3);
                        else
                            imageModel.RotateNTimes90Degrees(3);
                        break;
                    }
                }
            }
            break;
        case IDM_IMAGEATTRIBUTES:
        {
            if (attributesDialog.DoModal(mainWindow.m_hWnd))
            {
                CWaitCursor waitCursor;
                if (attributesDialog.m_bBlackAndWhite && !imageModel.IsBlackAndWhite())
                {
                    CStringW strText(MAKEINTRESOURCEW(IDS_LOSECOLOR));
                    CStringW strTitle(MAKEINTRESOURCEW(IDS_PROGRAMNAME));
                    INT id = MessageBox(strText, strTitle, MB_ICONINFORMATION | MB_YESNOCANCEL);
                    if (id != IDYES)
                        break;

                    imageModel.PushBlackAndWhite();
                }

                if (imageModel.GetWidth() != attributesDialog.newWidth ||
                    imageModel.GetHeight() != attributesDialog.newHeight)
                {
                    imageModel.Crop(attributesDialog.newWidth, attributesDialog.newHeight);
                }
            }
            break;
        }
        case IDM_IMAGESTRETCHSKEW:
        {
            if (stretchSkewDialog.DoModal(mainWindow.m_hWnd))
            {
                CWaitCursor waitCursor;
                if (selectionModel.m_bShow)
                {
                    selectionModel.StretchSkew(stretchSkewDialog.percentage.x, stretchSkewDialog.percentage.y,
                                               stretchSkewDialog.angle.x, stretchSkewDialog.angle.y);
                }
                else
                {
                    imageModel.StretchSkew(stretchSkewDialog.percentage.x, stretchSkewDialog.percentage.y,
                                           stretchSkewDialog.angle.x, stretchSkewDialog.angle.y);
                }
            }
            break;
        }
        case IDM_IMAGEDRAWOPAQUE:
            toolsModel.SetBackgroundTransparent(!toolsModel.IsBackgroundTransparent());
            break;
        case IDM_IMAGECROP:
        {
            HBITMAP hbmCopy = selectionModel.GetSelectionContents();
            imageModel.PushImageForUndo(hbmCopy);
            selectionModel.HideSelection();
            break;
        }
        case IDM_VIEWTOOLBOX:
            registrySettings.ShowToolBox = !toolBoxContainer.IsWindowVisible();
            toolBoxContainer.ShowWindow(registrySettings.ShowToolBox ? SW_SHOWNOACTIVATE : SW_HIDE);
            alignChildrenToMainWindow();
            break;
        case IDM_VIEWCOLORPALETTE:
            registrySettings.ShowPalette = !paletteWindow.IsWindowVisible();
            paletteWindow.ShowWindow(registrySettings.ShowPalette ? SW_SHOWNOACTIVATE : SW_HIDE);
            alignChildrenToMainWindow();
            break;
        case IDM_VIEWSTATUSBAR:
            registrySettings.ShowStatusBar = !::IsWindowVisible(g_hStatusBar);
            ::ShowWindow(g_hStatusBar, (registrySettings.ShowStatusBar ? SW_SHOWNOACTIVATE : SW_HIDE));
            alignChildrenToMainWindow();
            break;
        case IDM_FORMATICONBAR:
            if (toolsModel.GetActiveTool() == TOOL_TEXT)
            {
                if (!fontsDialog.IsWindow())
                {
                    fontsDialog.Create(mainWindow);
                }
                registrySettings.ShowTextTool = !::IsWindowVisible(fontsDialog);
                fontsDialog.ShowWindow(registrySettings.ShowTextTool ? SW_SHOW : SW_HIDE);
                fontsDialog.SendMessage(DM_REPOSITION, 0, 0);
            }
            break;
        case IDM_VIEWSHOWGRID:
            g_showGrid = !g_showGrid;
            canvasWindow.Invalidate(FALSE);
            break;
        case IDM_VIEWSHOWMINIATURE:
            registrySettings.ShowThumbnail = !::IsWindowVisible(miniature);
            miniature.DoCreate(m_hWnd);
            miniature.ShowWindow(registrySettings.ShowThumbnail ? SW_SHOWNOACTIVATE : SW_HIDE);
            break;

        case IDM_VIEWZOOM125:
            canvasWindow.zoomTo(125);
            break;
        case IDM_VIEWZOOM25:
            canvasWindow.zoomTo(250);
            break;
        case IDM_VIEWZOOM50:
            canvasWindow.zoomTo(500);
            break;
        case IDM_VIEWZOOM100:
            canvasWindow.zoomTo(1000);
            break;
        case IDM_VIEWZOOM200:
            canvasWindow.zoomTo(2000);
            break;
        case IDM_VIEWZOOM400:
            canvasWindow.zoomTo(4000);
            break;
        case IDM_VIEWZOOM800:
            canvasWindow.zoomTo(8000);
            break;

        case IDM_VIEWFULLSCREEN:
            // Create and show the fullscreen window
            fullscreenWindow.DoCreate();
            fullscreenWindow.ShowWindow(SW_SHOWMAXIMIZED);
            break;

        case IDM_CTRL_PLUS:
            toolsModel.SpecialTweak(FALSE);
            break;
        case IDM_CTRL_MINUS:
            toolsModel.SpecialTweak(TRUE);
            break;
    }
    return 0;
}

VOID CMainWindow::TrackPopupMenu(POINT ptScreen, INT iSubMenu)
{
    HMENU hMenu = ::LoadMenuW(g_hinstExe, MAKEINTRESOURCEW(ID_POPUPMENU));
    HMENU hSubMenu = ::GetSubMenu(hMenu, iSubMenu);

    ::SetForegroundWindow(m_hWnd);
    INT_PTR id = ::TrackPopupMenu(hSubMenu, TPM_RIGHTBUTTON | TPM_RETURNCMD,
                                  ptScreen.x, ptScreen.y, 0, m_hWnd, NULL);
    PostMessage(WM_NULL);
    if (id != 0)
        PostMessage(WM_COMMAND, id);

    ::DestroyMenu(hMenu);
}

// entry point
INT WINAPI
wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, INT nCmdShow)
{
    g_hinstExe = hInstance;

    // Initialize common controls library
    INITCOMMONCONTROLSEX iccx;
    iccx.dwSize = sizeof(iccx);
    iccx.dwICC = ICC_STANDARD_CLASSES | ICC_USEREX_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&iccx);

    // Load settings from registry
    registrySettings.Load(nCmdShow);

    // Create the main window
    if (!mainWindow.DoCreate())
    {
        MessageBox(NULL, L"Failed to create main window.", NULL, MB_ICONERROR);
        return 1;
    }

    // Initialize imageModel
    if (__argc < 2 || !DoLoadImageFile(mainWindow, __targv[1], TRUE))
        InitializeImage(NULL, NULL, FALSE);

    // Make the window visible on the screen
    mainWindow.ShowWindow(registrySettings.WindowPlacement.showCmd);

    // Load the access keys
    HACCEL hAccel = ::LoadAcceleratorsW(hInstance, MAKEINTRESOURCEW(800));

    // The message loop
    MSG msg;
    while (::GetMessage(&msg, NULL, 0, 0))
    {
        if (fontsDialog.IsWindow() && fontsDialog.IsDialogMessage(&msg))
            continue;

        if (::TranslateAcceleratorW(mainWindow, hAccel, &msg))
            continue;

        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }

    // Unload the access keys
    ::DestroyAcceleratorTable(hAccel);

    // Write back settings to registry
    registrySettings.Store();

    if (g_szMailTempFile[0])
        ::DeleteFileW(g_szMailTempFile);

    // Return the value that PostQuitMessage() gave
    return (INT)msg.wParam;
}
