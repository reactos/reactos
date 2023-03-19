/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/winproc.cpp
 * PURPOSE:     Window procedure of the main window and all children apart from
 *              hPalWin, hToolSettings and hSelection
 * PROGRAMMERS: Benedikt Freisen
 *              Katayama Hirofumi MZ
 *              Stanislav Motylkov
 */

#include "precomp.h"
#include <assert.h>

typedef HWND (WINAPI *FN_HtmlHelpW)(HWND, LPCWSTR, UINT, DWORD_PTR);

static HINSTANCE s_hHHCTRL_OCX = NULL; // HtmlHelpW needs "hhctrl.ocx"
static FN_HtmlHelpW s_pHtmlHelpW = NULL;

/* FUNCTIONS ********************************************************/

// A wrapper function for HtmlHelpW
static HWND DoHtmlHelpW(HWND hwndCaller, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData)
{
    WCHAR szPath[MAX_PATH];

    if (!s_hHHCTRL_OCX && (uCommand != HH_CLOSE_ALL))
    {
        // The function loads the system library, not local
        GetSystemDirectoryW(szPath, _countof(szPath));
        wcscat(szPath, L"\\hhctrl.ocx");
        s_hHHCTRL_OCX = LoadLibraryW(szPath);
        if (s_hHHCTRL_OCX)
            s_pHtmlHelpW = (FN_HtmlHelpW)GetProcAddress(s_hHHCTRL_OCX, "HtmlHelpW");
    }

    if (!s_pHtmlHelpW)
        return NULL;

    return s_pHtmlHelpW(hwndCaller, pszFile, uCommand, dwData);
}

BOOL
zoomTo(int newZoom, int mouseX, int mouseY)
{
    RECT clientRectScrollbox;
    RECT clientRectImageArea;
    int x, y, w, h;
    canvasWindow.GetClientRect(&clientRectScrollbox);
    imageArea.GetClientRect(&clientRectImageArea);
    w = clientRectImageArea.right * newZoom / toolsModel.GetZoom();
    h = clientRectImageArea.bottom * newZoom / toolsModel.GetZoom();
    if (!w || !h)
    {
        return FALSE;
    }
    w = clientRectImageArea.right * clientRectScrollbox.right / w;
    h = clientRectImageArea.bottom * clientRectScrollbox.bottom / h;
    x = max(0, min(clientRectImageArea.right - w, mouseX - w / 2)) * newZoom / toolsModel.GetZoom();
    y = max(0, min(clientRectImageArea.bottom - h, mouseY - h / 2)) * newZoom / toolsModel.GetZoom();

    toolsModel.SetZoom(newZoom);

    imageArea.MoveWindow(GRIP_SIZE, GRIP_SIZE, Zoomed(imageModel.GetWidth()), Zoomed(imageModel.GetHeight()), FALSE);
    canvasWindow.Invalidate(TRUE);
    imageArea.Invalidate(FALSE);

    canvasWindow.SendMessage(WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, x), 0);
    canvasWindow.SendMessage(WM_VSCROLL, MAKEWPARAM(SB_THUMBPOSITION, y), 0);
    return TRUE;
}

void CMainWindow::alignChildrenToMainWindow()
{
    RECT clientRect, rc;
    GetClientRect(&clientRect);
    RECT rcSpace = clientRect;

    if (::IsWindowVisible(hStatusBar))
    {
        ::GetWindowRect(hStatusBar, &rc);
        rcSpace.bottom -= rc.bottom - rc.top;
    }

    HDWP hDWP = ::BeginDeferWindowPos(3);

    if (::IsWindowVisible(toolBoxContainer))
    {
        hDWP = ::DeferWindowPos(hDWP, toolBoxContainer, NULL,
                                rcSpace.left, rcSpace.top,
                                CX_TOOLBAR, rcSpace.bottom - rcSpace.top,
                                SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREPOSITION);
        rcSpace.left += CX_TOOLBAR;
    }

    if (::IsWindowVisible(paletteWindow))
    {
        hDWP = ::DeferWindowPos(hDWP, paletteWindow, NULL,
                                rcSpace.left, rcSpace.top,
                                rcSpace.right - rcSpace.left, CY_PALETTE,
                                SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREPOSITION);
        rcSpace.top += CY_PALETTE;
    }

    if (canvasWindow.IsWindow())
    {
        hDWP = ::DeferWindowPos(hDWP, canvasWindow, NULL,
                                rcSpace.left, rcSpace.top,
                                rcSpace.right - rcSpace.left, rcSpace.bottom - rcSpace.top,
                                SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREPOSITION);
    }

    ::EndDeferWindowPos(hDWP);
}

void CMainWindow::saveImage(BOOL overwrite)
{
    imageArea.finishDrawing();

    if (isAFile && overwrite)
    {
        imageModel.SaveImage(filepathname);
    }
    else if (GetSaveFileName(&sfn) != 0)
    {
        imageModel.SaveImage(sfn.lpstrFile);
        _tcsncpy(filepathname, sfn.lpstrFile, _countof(filepathname));
        CString strTitle;
        strTitle.Format(IDS_WINDOWTITLE, (LPCTSTR)sfn.lpstrFileTitle);
        SetWindowText(strTitle);
        isAFile = TRUE;
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

        if (askBeforeEnlarging)
        {
            TCHAR programname[20];
            TCHAR shouldEnlargePromptText[100];

            LoadString(hProgInstance, IDS_PROGRAMNAME, programname, _countof(programname));
            LoadString(hProgInstance, IDS_ENLARGEPROMPTTEXT, shouldEnlargePromptText, _countof(shouldEnlargePromptText));

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

    HWND hToolbar = FindWindowEx(toolBoxContainer.m_hWnd, NULL, TOOLBARCLASSNAME, NULL);
    SendMessage(hToolbar, TB_CHECKBUTTON, ID_RECTSEL, MAKELPARAM(TRUE, 0));
    toolBoxContainer.SendMessage(WM_COMMAND, ID_RECTSEL);

    imageModel.CopyPrevious();
    selectionModel.InsertFromHBITMAP(bitmap);

    placeSelWin();
    selectionWindow.ShowWindow(SW_SHOW);
    selectionWindow.ForceRefreshSelectionContents();
}

LRESULT CMainWindow::OnMouseWheel(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    INT zDelta = (SHORT)HIWORD(wParam);

    if (::GetAsyncKeyState(VK_CONTROL) < 0)
    {
        if (zDelta < 0)
        {
            if (toolsModel.GetZoom() > MIN_ZOOM)
                zoomTo(toolsModel.GetZoom() / 2, 0, 0);
        }
        else if (zDelta > 0)
        {
            if (toolsModel.GetZoom() < MAX_ZOOM)
                zoomTo(toolsModel.GetZoom() * 2, 0, 0);
        }
    }
    else
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
    TCHAR droppedfile[MAX_PATH];

    HDROP hDrop = (HDROP)wParam;
    DragQueryFile(hDrop, 0, droppedfile, _countof(droppedfile));
    DragFinish(hDrop);

    ConfirmSave() && DoLoadImageFile(m_hWnd, droppedfile, TRUE);

    return 0;
}

LRESULT CMainWindow::OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SendMessage(WM_SETICON, ICON_BIG, (LPARAM) LoadIcon(hProgInstance, MAKEINTRESOURCE(IDI_APPICON)));
    SendMessage(WM_SETICON, ICON_SMALL, (LPARAM) LoadIcon(hProgInstance, MAKEINTRESOURCE(IDI_APPICON)));
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

    PostQuitMessage(0); /* send a WM_QUIT to the message queue */
    return 0;
}

BOOL CMainWindow::ConfirmSave()
{
    imageArea.finishDrawing();

    if (imageModel.IsImageSaved())
        return TRUE;

    CString strProgramName;
    strProgramName.LoadString(IDS_PROGRAMNAME);

    CString strSavePromptText;
    strSavePromptText.Format(IDS_SAVEPROMPTTEXT, PathFindFileName(filepathname));

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
    LPCTSTR dotext = PathFindExtensionW(filepathname);
    BOOL isBMP = FALSE;
    if (_tcsicmp(dotext, _T(".bmp")) == 0 ||
        _tcsicmp(dotext, _T(".dib")) == 0 ||
        _tcsicmp(dotext, _T(".rle")) == 0)
    {
        isBMP = TRUE;
    }

    EnableMenuItem(hPopupMenu, IDM_FILEASWALLPAPERPLANE,     ENABLED_IF(isAFile && isBMP));
    EnableMenuItem(hPopupMenu, IDM_FILEASWALLPAPERCENTERED,  ENABLED_IF(isAFile && isBMP));
    EnableMenuItem(hPopupMenu, IDM_FILEASWALLPAPERSTRETCHED, ENABLED_IF(isAFile && isBMP));

    for (INT iItem = 0; iItem < MAX_RECENT_FILES; ++iItem)
        RemoveMenu(hPopupMenu, IDM_FILE1 + iItem, MF_BYCOMMAND);

    if (registrySettings.strFiles[0].IsEmpty())
        return;

    RemoveMenu(hPopupMenu, IDM_FILEMOSTRECENTLYUSEDFILE, MF_BYCOMMAND);

    INT cMenuItems = GetMenuItemCount(hPopupMenu);

    for (INT iItem = 0; iItem < MAX_RECENT_FILES; ++iItem)
    {
        CString& strFile = registrySettings.strFiles[iItem];
        if (strFile.IsEmpty())
            break;

        // Condense the lengthy pathname by using '...'
#define MAX_RECENT_PATHNAME_DISPLAY 30
        CPath pathFile(strFile);
        pathFile.CompactPathEx(MAX_RECENT_PATHNAME_DISPLAY);
        assert(_tcslen((LPCTSTR)pathFile) <= MAX_RECENT_PATHNAME_DISPLAY);

        // Add an accelerator (by '&') to the item number for quick access
        TCHAR szText[4 + MAX_RECENT_PATHNAME_DISPLAY + 1];
        wsprintf(szText, _T("&%u %s"), iItem + 1, (LPCTSTR)pathFile);

        INT iMenuItem = (cMenuItems - 2) + iItem;
        InsertMenu(hPopupMenu, iMenuItem, MF_BYPOSITION | MF_STRING, IDM_FILE1 + iItem, szText);
    }
}

LRESULT CMainWindow::OnInitMenuPopup(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HMENU menu = GetMenu();
    BOOL trueSelection =
        (::IsWindowVisible(selectionWindow) &&
         ((toolsModel.GetActiveTool() == TOOL_FREESEL) || (toolsModel.GetActiveTool() == TOOL_RECTSEL)));

    switch (lParam)
    {
        case 0: /* File menu */
            ProcessFileMenu((HMENU)wParam);
            break;
        case 1: /* Edit menu */
            EnableMenuItem(menu, IDM_EDITUNDO, ENABLED_IF(imageModel.HasUndoSteps()));
            EnableMenuItem(menu, IDM_EDITREDO, ENABLED_IF(imageModel.HasRedoSteps()));
            EnableMenuItem(menu, IDM_EDITCUT,  ENABLED_IF(trueSelection));
            EnableMenuItem(menu, IDM_EDITCOPY, ENABLED_IF(trueSelection));
            EnableMenuItem(menu, IDM_EDITDELETESELECTION, ENABLED_IF(trueSelection));
            EnableMenuItem(menu, IDM_EDITINVERTSELECTION, ENABLED_IF(trueSelection));
            EnableMenuItem(menu, IDM_EDITCOPYTO, ENABLED_IF(trueSelection));
            OpenClipboard();
            EnableMenuItem(menu, IDM_EDITPASTE, ENABLED_IF(GetClipboardData(CF_BITMAP) != NULL));
            CloseClipboard();
            break;
        case 2: /* View menu */
            CheckMenuItem(menu, IDM_VIEWTOOLBOX, CHECKED_IF(::IsWindowVisible(toolBoxContainer)));
            CheckMenuItem(menu, IDM_VIEWCOLORPALETTE, CHECKED_IF(::IsWindowVisible(paletteWindow)));
            CheckMenuItem(menu, IDM_VIEWSTATUSBAR,    CHECKED_IF(::IsWindowVisible(hStatusBar)));
            CheckMenuItem(menu, IDM_FORMATICONBAR, CHECKED_IF(::IsWindowVisible(fontsDialog)));
            EnableMenuItem(menu, IDM_FORMATICONBAR, ENABLED_IF(toolsModel.GetActiveTool() == TOOL_TEXT));

            CheckMenuItem(menu, IDM_VIEWSHOWGRID,      CHECKED_IF(showGrid));
            CheckMenuItem(menu, IDM_VIEWSHOWMINIATURE, CHECKED_IF(showMiniature));
            break;
        case 3: /* Image menu */
            EnableMenuItem(menu, IDM_IMAGECROP, ENABLED_IF(::IsWindowVisible(selectionWindow)));
            CheckMenuItem(menu, IDM_IMAGEDRAWOPAQUE, CHECKED_IF(!toolsModel.IsBackgroundTransparent()));
            break;
    }

    CheckMenuItem(menu, IDM_VIEWZOOM125, CHECKED_IF(toolsModel.GetZoom() == 125));
    CheckMenuItem(menu, IDM_VIEWZOOM25,  CHECKED_IF(toolsModel.GetZoom() == 250));
    CheckMenuItem(menu, IDM_VIEWZOOM50,  CHECKED_IF(toolsModel.GetZoom() == 500));
    CheckMenuItem(menu, IDM_VIEWZOOM100, CHECKED_IF(toolsModel.GetZoom() == 1000));
    CheckMenuItem(menu, IDM_VIEWZOOM200, CHECKED_IF(toolsModel.GetZoom() == 2000));
    CheckMenuItem(menu, IDM_VIEWZOOM400, CHECKED_IF(toolsModel.GetZoom() == 4000));
    CheckMenuItem(menu, IDM_VIEWZOOM800, CHECKED_IF(toolsModel.GetZoom() == 8000));

    CheckMenuItem(menu, IDM_COLORSMODERNPALETTE, CHECKED_IF(paletteModel.SelectedPalette() == PAL_MODERN));
    CheckMenuItem(menu, IDM_COLORSOLDPALETTE,    CHECKED_IF(paletteModel.SelectedPalette() == PAL_OLDTYPE));
    return 0;
}

LRESULT CMainWindow::OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    int test[] = { LOWORD(lParam) - 260, LOWORD(lParam) - 140, LOWORD(lParam) - 20 };
    if (::IsWindow(hStatusBar))
    {
        ::SendMessage(hStatusBar, WM_SIZE, 0, 0);
        ::SendMessage(hStatusBar, SB_SETPARTS, 3, (LPARAM)&test);
    }
    alignChildrenToMainWindow();
    return 0;
}

LRESULT CMainWindow::OnGetMinMaxInfo(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    MINMAXINFO *mm = (LPMINMAXINFO) lParam;
    mm->ptMinTrackSize.x = 330;
    mm->ptMinTrackSize.y = 430;
    return 0;
}

LRESULT CMainWindow::OnKeyDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (wParam == VK_ESCAPE)
    {
        HWND hwndCapture = GetCapture();
        if (hwndCapture)
        {
            if (canvasWindow.m_hWnd == hwndCapture ||
                selectionWindow.m_hWnd == hwndCapture ||
                imageArea.m_hWnd == hwndCapture ||
                fullscreenWindow.m_hWnd == hwndCapture)
            {
                SendMessage(hwndCapture, nMsg, wParam, lParam);
            }
        }
        else
        {
            imageArea.SendMessage(nMsg, wParam, lParam);
        }
    }
    return 0;
}

LRESULT CMainWindow::OnSysColorChange(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    /* Redirect message to common controls */
    HWND hToolbar = FindWindowEx(toolBoxContainer.m_hWnd, NULL, TOOLBARCLASSNAME, NULL);
    SendMessage(hToolbar, WM_SYSCOLORCHANGE, 0, 0);
    return 0;
}

LRESULT CMainWindow::OnCommand(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // Disable commands while dragging mouse
    if (imageArea.drawing && ::GetCapture())
    {
        ATLTRACE("locking!\n");
        return 0;
    }

    switch (LOWORD(wParam))
    {
        case IDM_HELPINFO:
        {
            HICON paintIcon = LoadIcon(hProgInstance, MAKEINTRESOURCE(IDI_APPICON));
            TCHAR infotitle[100];
            TCHAR infotext[200];
            LoadString(hProgInstance, IDS_INFOTITLE, infotitle, _countof(infotitle));
            LoadString(hProgInstance, IDS_INFOTEXT, infotext, _countof(infotext));
            ShellAbout(m_hWnd, infotitle, infotext, paintIcon);
            DeleteObject(paintIcon);
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
                SetBitmapAndInfo(NULL, NULL, 0, FALSE);
            }
            break;
        case IDM_FILEOPEN:
            if (ConfirmSave() && GetOpenFileName(&ofn))
            {
                DoLoadImageFile(m_hWnd, ofn.lpstrFile, TRUE);
            }
            break;
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
                BitBlt(pd.hDC, 0, 0, imageModel.GetWidth(), imageModel.GetHeight(), imageModel.GetDC(), 0, 0, SRCCOPY);
                DeleteDC(pd.hDC);
            }
            if (pd.hDevMode)
                GlobalFree(pd.hDevMode);
            if (pd.hDevNames)
                GlobalFree(pd.hDevNames);
            break;
        case IDM_FILEASWALLPAPERPLANE:
            RegistrySettings::SetWallpaper(filepathname, RegistrySettings::TILED);
            break;
        case IDM_FILEASWALLPAPERCENTERED:
            RegistrySettings::SetWallpaper(filepathname, RegistrySettings::CENTERED);
            break;
        case IDM_FILEASWALLPAPERSTRETCHED:
            RegistrySettings::SetWallpaper(filepathname, RegistrySettings::STRETCHED);
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
            if (toolsModel.GetActiveTool() == TOOL_TEXT && ::IsWindowVisible(textEditWindow))
                break;
            if (selectionWindow.IsWindowVisible())
            {
                if (toolsModel.GetActiveTool() == TOOL_RECTSEL ||
                    toolsModel.GetActiveTool() == TOOL_FREESEL)
                {
                    imageArea.cancelDrawing();
                    break;
                }
            }
            if (ToolBase::pointSP != 0) // drawing something?
            {
                imageArea.cancelDrawing();
                break;
            }
            imageModel.Undo();
            imageArea.Invalidate(FALSE);
            break;
        case IDM_EDITREDO:
            if (toolsModel.GetActiveTool() == TOOL_TEXT && ::IsWindowVisible(textEditWindow))
                break;
            if (ToolBase::pointSP != 0) // drawing something?
            {
                imageArea.finishDrawing();
                break;
            }
            imageModel.Redo();
            imageArea.Invalidate(FALSE);
            break;
        case IDM_EDITCOPY:
            OpenClipboard();
            EmptyClipboard();
            SetClipboardData(CF_BITMAP, CopyDIBImage(selectionModel.GetBitmap()));
            CloseClipboard();
            break;
        case IDM_EDITCUT:
            /* Copy */
            SendMessage(WM_COMMAND, IDM_EDITCOPY, 0);
            /* Delete selection */
            SendMessage(WM_COMMAND, IDM_EDITDELETESELECTION, 0);
            break;
        case IDM_EDITPASTE:
            OpenClipboard();
            if (GetClipboardData(CF_BITMAP) != NULL)
            {
                InsertSelectionFromHBITMAP((HBITMAP) GetClipboardData(CF_BITMAP), m_hWnd);
            }
            CloseClipboard();
            break;
        case IDM_EDITDELETESELECTION:
        {
            switch (toolsModel.GetActiveTool())
            {
                case TOOL_FREESEL:
                case TOOL_RECTSEL:
                    imageModel.DeleteSelection();
                    break;

                case TOOL_TEXT:
                    imageArea.cancelDrawing();
                    break;
                default:
                    break;
            }
            break;
        }
        case IDM_EDITSELECTALL:
        {
            if (toolsModel.GetActiveTool() == TOOL_TEXT && ::IsWindowVisible(textEditWindow))
            {
                textEditWindow.SendMessage(EM_SETSEL, 0, -1);
                break;
            }
            HWND hToolbar = FindWindowEx(toolBoxContainer.m_hWnd, NULL, TOOLBARCLASSNAME, NULL);
            SendMessage(hToolbar, TB_CHECKBUTTON, ID_RECTSEL, MAKELPARAM(TRUE, 0));
            toolsModel.selectAll();
            imageArea.Invalidate(TRUE);
            break;
        }
        case IDM_EDITCOPYTO:
            if (GetSaveFileName(&ofn))
                SaveDIBToFile(selectionModel.GetBitmap(), ofn.lpstrFile, imageModel.GetDC());
            break;
        case IDM_EDITPASTEFROM:
            if (GetOpenFileName(&ofn))
            {
                HBITMAP hbmNew = DoLoadImageFile(m_hWnd, ofn.lpstrFile, FALSE);
                if (hbmNew)
                {
                    InsertSelectionFromHBITMAP(hbmNew, m_hWnd);
                    DeleteObject(hbmNew);
                }
            }
            break;
        case IDM_COLORSEDITPALETTE:
            if (ChooseColor(&choosecolor))
                paletteModel.SetFgColor(choosecolor.rgbResult);
            break;
        case IDM_COLORSMODERNPALETTE:
            paletteModel.SelectPalette(PAL_MODERN);
            break;
        case IDM_COLORSOLDPALETTE:
            paletteModel.SelectPalette(PAL_OLDTYPE);
            break;
        case IDM_IMAGEINVERTCOLORS:
        {
            imageModel.InvertColors();
            break;
        }
        case IDM_IMAGEDELETEIMAGE:
            imageModel.CopyPrevious();
            Rect(imageModel.GetDC(), 0, 0, imageModel.GetWidth(), imageModel.GetHeight(), paletteModel.GetBgColor(), paletteModel.GetBgColor(), 0, TRUE);
            imageArea.Invalidate(FALSE);
            break;
        case IDM_IMAGEROTATEMIRROR:
            switch (mirrorRotateDialog.DoModal(mainWindow.m_hWnd))
            {
                case 1: /* flip horizontally */
                    if (::IsWindowVisible(selectionWindow))
                        selectionModel.FlipHorizontally();
                    else
                        imageModel.FlipHorizontally();
                    break;
                case 2: /* flip vertically */
                    if (::IsWindowVisible(selectionWindow))
                        selectionModel.FlipVertically();
                    else
                        imageModel.FlipVertically();
                    break;
                case 3: /* rotate 90 degrees */
                    if (::IsWindowVisible(selectionWindow))
                        selectionModel.RotateNTimes90Degrees(1);
                    else
                        imageModel.RotateNTimes90Degrees(1);
                    break;
                case 4: /* rotate 180 degrees */
                    if (::IsWindowVisible(selectionWindow))
                        selectionModel.RotateNTimes90Degrees(2);
                    else
                        imageModel.RotateNTimes90Degrees(2);
                    break;
                case 5: /* rotate 270 degrees */
                    if (::IsWindowVisible(selectionWindow))
                        selectionModel.RotateNTimes90Degrees(3);
                    else
                        imageModel.RotateNTimes90Degrees(3);
                    break;
            }
            break;
        case IDM_IMAGEATTRIBUTES:
        {
            if (attributesDialog.DoModal(mainWindow.m_hWnd))
            {
                imageModel.Crop(attributesDialog.newWidth, attributesDialog.newHeight, 0, 0);
            }
            break;
        }
        case IDM_IMAGESTRETCHSKEW:
        {
            if (stretchSkewDialog.DoModal(mainWindow.m_hWnd))
            {
                if (::IsWindowVisible(selectionWindow))
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
            imageModel.Insert(CopyDIBImage(selectionModel.GetBitmap()));
            break;

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
            registrySettings.ShowStatusBar = !::IsWindowVisible(hStatusBar);
            ::ShowWindow(hStatusBar, (registrySettings.ShowStatusBar ? SW_SHOWNOACTIVATE : SW_HIDE));
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
            showGrid = !showGrid;
            imageArea.Invalidate(FALSE);
            break;
        case IDM_VIEWSHOWMINIATURE:
            showMiniature = !showMiniature;
            miniature.ShowWindow(showMiniature ? SW_SHOW : SW_HIDE);
            break;

        case IDM_VIEWZOOM125:
            zoomTo(125, 0, 0);
            break;
        case IDM_VIEWZOOM25:
            zoomTo(250, 0, 0);
            break;
        case IDM_VIEWZOOM50:
            zoomTo(500, 0, 0);
            break;
        case IDM_VIEWZOOM100:
            zoomTo(1000, 0, 0);
            break;
        case IDM_VIEWZOOM200:
            zoomTo(2000, 0, 0);
            break;
        case IDM_VIEWZOOM400:
            zoomTo(4000, 0, 0);
            break;
        case IDM_VIEWZOOM800:
            zoomTo(8000, 0, 0);
            break;

        case IDM_VIEWFULLSCREEN:
            fullscreenWindow.ShowWindow(SW_SHOW);
            ShowWindow(SW_HIDE);
            break;
    }
    return 0;
}
