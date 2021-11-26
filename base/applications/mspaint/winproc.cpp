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

/* INCLUDES *********************************************************/

#include "precomp.h"

#include "dialogs.h"

/* FUNCTIONS ********************************************************/

BOOL
zoomTo(int newZoom, int mouseX, int mouseY)
{
    RECT clientRectScrollbox;
    RECT clientRectImageArea;
    int x, y, w, h;
    scrollboxWindow.GetClientRect(&clientRectScrollbox);
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

    selectionWindow.ShowWindow(SW_HIDE);
    imageArea.MoveWindow(3, 3, imageModel.GetWidth() * toolsModel.GetZoom() / 1000, imageModel.GetHeight() * toolsModel.GetZoom() / 1000, FALSE);
    scrollboxWindow.Invalidate(TRUE);
    imageArea.Invalidate(FALSE);

    scrollboxWindow.SendMessage(WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, x), 0);
    scrollboxWindow.SendMessage(WM_VSCROLL, MAKEWPARAM(SB_THUMBPOSITION, y), 0);
    return TRUE;
}

void CMainWindow::alignChildrenToMainWindow()
{
    int x, y, w, h;
    RECT clientRect;
    GetClientRect(&clientRect);

    if (toolBoxContainer.IsWindowVisible())
    {
        x = 56;
        w = clientRect.right - 56;
    }
    else
    {
        x = 0;
        w = clientRect.right;
    }
    if (paletteWindow.IsWindowVisible())
    {
        y = 49;
        h = clientRect.bottom - 49;
    }
    else
    {
        y = 3;
        h = clientRect.bottom - 3;
    }

    RECT statusBarRect0;
    SendMessage(hStatusBar, SB_GETRECT, 0, (LPARAM)&statusBarRect0);
    int statusBarBorders[3];
    SendMessage(hStatusBar, SB_GETBORDERS, 0, (LPARAM)&statusBarBorders);
    int statusBarHeight = statusBarRect0.bottom - statusBarRect0.top + statusBarBorders[1];

    scrollboxWindow.MoveWindow(x, y, w, ::IsWindowVisible(hStatusBar) ? h - statusBarHeight : h, TRUE);
    paletteWindow.MoveWindow(x, 9, 255, 32, TRUE);
}

void CMainWindow::saveImage(BOOL overwrite)
{
    if (isAFile && overwrite)
    {
        imageModel.SaveImage(filepathname);
    }
    else if (GetSaveFileName(&sfn) != 0)
    {
        imageModel.SaveImage(sfn.lpstrFile);
        _tcsncpy(filepathname, sfn.lpstrFile, SIZEOF(filepathname));
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

            LoadString(hProgInstance, IDS_PROGRAMNAME, programname, SIZEOF(programname));
            LoadString(hProgInstance, IDS_ENLARGEPROMPTTEXT, shouldEnlargePromptText, SIZEOF(shouldEnlargePromptText));

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
    ForceRefreshSelectionContents();
}

LRESULT CMainWindow::OnDropFiles(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TCHAR droppedfile[MAX_PATH];

    HDROP hDrop = (HDROP)wParam;
    DragQueryFile(hDrop, 0, droppedfile, SIZEOF(droppedfile));
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
    GetWindowPlacement(&(registrySettings.WindowPlacement));
    PostQuitMessage(0); /* send a WM_QUIT to the message queue */
    return 0;
}

BOOL CMainWindow::ConfirmSave()
{
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

LRESULT CMainWindow::OnInitMenuPopup(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HMENU menu = GetMenu();
    BOOL trueSelection = (selectionWindow.IsWindowVisible() && ((toolsModel.GetActiveTool() == TOOL_FREESEL) || (toolsModel.GetActiveTool() == TOOL_RECTSEL)));
    switch (lParam)
    {
        case 0: /* File menu */
            if ((HMENU)wParam != GetSubMenu(menu, 0))
                break;
            EnableMenuItem(menu, IDM_FILEASWALLPAPERPLANE,     ENABLED_IF(isAFile));
            EnableMenuItem(menu, IDM_FILEASWALLPAPERCENTERED,  ENABLED_IF(isAFile));
            EnableMenuItem(menu, IDM_FILEASWALLPAPERSTRETCHED, ENABLED_IF(isAFile));
            RemoveMenu(menu, IDM_FILE1, MF_BYCOMMAND);
            RemoveMenu(menu, IDM_FILE2, MF_BYCOMMAND);
            RemoveMenu(menu, IDM_FILE3, MF_BYCOMMAND);
            RemoveMenu(menu, IDM_FILE4, MF_BYCOMMAND);
            if (!registrySettings.strFile1.IsEmpty())
            {
                RemoveMenu(menu, IDM_FILEMOSTRECENTLYUSEDFILE, MF_BYCOMMAND);
                CPath pathFile1(registrySettings.strFile1);
                pathFile1.CompactPathEx(30);
                if (!registrySettings.strFile2.IsEmpty())
                {
                    CPath pathFile2(registrySettings.strFile2);
                    pathFile2.CompactPathEx(30);
                    if (!registrySettings.strFile3.IsEmpty())
                    {
                        CPath pathFile3(registrySettings.strFile3);
                        pathFile3.CompactPathEx(30);
                        if (!registrySettings.strFile4.IsEmpty())
                        {
                            CPath pathFile4(registrySettings.strFile4);
                            pathFile4.CompactPathEx(30);
                            InsertMenu((HMENU)wParam, 17, MF_BYPOSITION | MF_STRING, IDM_FILE4, _T("4 ") + pathFile4);
                        }
                        InsertMenu((HMENU)wParam, 17, MF_BYPOSITION | MF_STRING, IDM_FILE3, _T("3 ") + pathFile3);
                    }
                    InsertMenu((HMENU)wParam, 17, MF_BYPOSITION | MF_STRING, IDM_FILE2, _T("2 ") + pathFile2);
                }
                InsertMenu((HMENU)wParam, 17, MF_BYPOSITION | MF_STRING, IDM_FILE1, _T("1 ") + pathFile1);
            }
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
                CheckMenuItem(menu, IDM_VIEWTOOLBOX,      CHECKED_IF(toolBoxContainer.IsWindowVisible()));
                CheckMenuItem(menu, IDM_VIEWCOLORPALETTE, CHECKED_IF(paletteWindow.IsWindowVisible()));
                CheckMenuItem(menu, IDM_VIEWSTATUSBAR,    CHECKED_IF(::IsWindowVisible(hStatusBar)));
                CheckMenuItem(menu, IDM_FORMATICONBAR,    CHECKED_IF(textEditWindow.IsWindowVisible()));
                EnableMenuItem(menu, IDM_FORMATICONBAR, ENABLED_IF(toolsModel.GetActiveTool() == TOOL_TEXT));

                CheckMenuItem(menu, IDM_VIEWSHOWGRID,      CHECKED_IF(showGrid));
                CheckMenuItem(menu, IDM_VIEWSHOWMINIATURE, CHECKED_IF(showMiniature));
            break;
        case 3: /* Image menu */
            EnableMenuItem(menu, IDM_IMAGECROP, ENABLED_IF(selectionWindow.IsWindowVisible()));
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

    CheckMenuItem(menu, IDM_COLORSMODERNPALETTE, CHECKED_IF(paletteModel.SelectedPalette() == 1));
    CheckMenuItem(menu, IDM_COLORSOLDPALETTE,    CHECKED_IF(paletteModel.SelectedPalette() == 2));
    return 0;
}

LRESULT CMainWindow::OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    int test[] = { LOWORD(lParam) - 260, LOWORD(lParam) - 140, LOWORD(lParam) - 20 };
    SendMessage(hStatusBar, WM_SIZE, wParam, lParam);
    SendMessage(hStatusBar, SB_SETPARTS, 3, (LPARAM)&test);
    alignChildrenToMainWindow();
    Invalidate(TRUE);
    return 0;
}

LRESULT CMainWindow::OnGetMinMaxInfo(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    MINMAXINFO *mm = (LPMINMAXINFO) lParam;
    mm->ptMinTrackSize.x = 330;
    mm->ptMinTrackSize.y = 430;
    return 0;
}

LRESULT CMainWindow::OnSetCursor(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SetCursor(LoadCursor(NULL, IDC_ARROW));
    bHandled = FALSE;
    return 0;
}

LRESULT CMainWindow::OnKeyDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (wParam == VK_ESCAPE)
    {
        HWND hwndCapture = GetCapture();
        if (hwndCapture)
        {
            if (selectionWindow.m_hWnd == hwndCapture ||
                imageArea.m_hWnd == hwndCapture ||
                fullscreenWindow.m_hWnd == hwndCapture ||
                sizeboxLeftTop.m_hWnd == hwndCapture ||
                sizeboxCenterTop.m_hWnd == hwndCapture ||
                sizeboxRightTop.m_hWnd == hwndCapture ||
                sizeboxLeftCenter.m_hWnd == hwndCapture ||
                sizeboxRightCenter.m_hWnd == hwndCapture ||
                sizeboxLeftBottom.m_hWnd == hwndCapture ||
                sizeboxCenterBottom.m_hWnd == hwndCapture ||
                sizeboxRightBottom.m_hWnd == hwndCapture)
            {
                SendMessage(hwndCapture, nMsg, wParam, lParam);
            }
        }
        else
        {
            switch (toolsModel.GetActiveTool())
            {
                case TOOL_SHAPE: case TOOL_BEZIER:
                    imageArea.SendMessage(nMsg, wParam, lParam);
                    break;
            }
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
    switch (LOWORD(wParam))
    {
        case IDM_HELPINFO:
        {
            HICON paintIcon = LoadIcon(hProgInstance, MAKEINTRESOURCE(IDI_APPICON));
            TCHAR infotitle[100];
            TCHAR infotext[200];
            LoadString(hProgInstance, IDS_INFOTITLE, infotitle, SIZEOF(infotitle));
            LoadString(hProgInstance, IDS_INFOTEXT, infotext, SIZEOF(infotext));
            ShellAbout(m_hWnd, infotitle, infotext, paintIcon);
            DeleteObject(paintIcon);
            break;
        }
        case IDM_HELPHELPTOPICS:
            HtmlHelp(m_hWnd, _T("help\\Paint.chm"), 0, 0);
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
        {
            ConfirmSave() && DoLoadImageFile(m_hWnd, registrySettings.strFile1, TRUE);
            break;
        }
        case IDM_FILE2:
        {
            ConfirmSave() && DoLoadImageFile(m_hWnd, registrySettings.strFile2, TRUE);
            break;
        }
        case IDM_FILE3:
        {
            ConfirmSave() && DoLoadImageFile(m_hWnd, registrySettings.strFile3, TRUE);
            break;
        }
        case IDM_FILE4:
        {
            ConfirmSave() && DoLoadImageFile(m_hWnd, registrySettings.strFile4, TRUE);
            break;
        }
        case IDM_EDITUNDO:
            imageModel.Undo();
            imageArea.Invalidate(FALSE);
            break;
        case IDM_EDITREDO:
            imageModel.Redo();
            imageArea.Invalidate(FALSE);
            break;
        case IDM_EDITCOPY:
            OpenClipboard();
            EmptyClipboard();
            SetClipboardData(CF_BITMAP, CopyImage(selectionModel.GetBitmap(), IMAGE_BITMAP, 0, 0, LR_COPYRETURNORG));
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
            /* remove selection window and already painted content using undo */
            imageModel.Undo();
            break;
        }
        case IDM_EDITSELECTALL:
        {
            HWND hToolbar = FindWindowEx(toolBoxContainer.m_hWnd, NULL, TOOLBARCLASSNAME, NULL);
            SendMessage(hToolbar, TB_CHECKBUTTON, ID_RECTSEL, MAKELPARAM(TRUE, 0));
            toolBoxContainer.SendMessage(WM_COMMAND, ID_RECTSEL);
            //TODO: do this properly
            startPaintingL(imageModel.GetDC(), 0, 0, paletteModel.GetFgColor(), paletteModel.GetBgColor());
            whilePaintingL(imageModel.GetDC(), imageModel.GetWidth(), imageModel.GetHeight(), paletteModel.GetFgColor(), paletteModel.GetBgColor());
            endPaintingL(imageModel.GetDC(), imageModel.GetWidth(), imageModel.GetHeight(), paletteModel.GetFgColor(), paletteModel.GetBgColor());
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
            paletteModel.SelectPalette(1);
            break;
        case IDM_COLORSOLDPALETTE:
            paletteModel.SelectPalette(2);
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
                    if (selectionWindow.IsWindowVisible())
                        selectionModel.FlipHorizontally();
                    else
                        imageModel.FlipHorizontally();
                    break;
                case 2: /* flip vertically */
                    if (selectionWindow.IsWindowVisible())
                        selectionModel.FlipVertically();
                    else
                        imageModel.FlipVertically();
                    break;
                case 3: /* rotate 90 degrees */
                    break;
                case 4: /* rotate 180 degrees */
                    if (selectionWindow.IsWindowVisible())
                        selectionModel.RotateNTimes90Degrees(2);
                    else
                        imageModel.RotateNTimes90Degrees(2);
                    break;
                case 5: /* rotate 270 degrees */
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
                imageModel.StretchSkew(stretchSkewDialog.percentage.x, stretchSkewDialog.percentage.y,
                                       stretchSkewDialog.angle.x, stretchSkewDialog.angle.y);
            }
            break;
        }
        case IDM_IMAGEDRAWOPAQUE:
            toolsModel.SetBackgroundTransparent(!toolsModel.IsBackgroundTransparent());
            break;
        case IDM_IMAGECROP:
            imageModel.Insert((HBITMAP) CopyImage(selectionModel.GetBitmap(), IMAGE_BITMAP, 0, 0, LR_COPYRETURNORG));
            break;

        case IDM_VIEWTOOLBOX:
            toolBoxContainer.ShowWindow(toolBoxContainer.IsWindowVisible() ? SW_HIDE : SW_SHOW);
            alignChildrenToMainWindow();
            break;
        case IDM_VIEWCOLORPALETTE:
            paletteWindow.ShowWindow(paletteWindow.IsWindowVisible() ? SW_HIDE : SW_SHOW);
            alignChildrenToMainWindow();
            break;
        case IDM_VIEWSTATUSBAR:
            ::ShowWindow(hStatusBar, ::IsWindowVisible(hStatusBar) ? SW_HIDE : SW_SHOW);
            alignChildrenToMainWindow();
            break;
        case IDM_FORMATICONBAR:
            textEditWindow.ShowWindow(textEditWindow.IsWindowVisible() ? SW_HIDE : SW_SHOW);
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
