/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint_new/winproc.cpp
 * PURPOSE:     Window procedure of the main window and all children apart from
 *              hPalWin, hToolSettings and hSelection
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

#include "dialogs.h"
#include "registry.h"

/* FUNCTIONS ********************************************************/

void CMainWindow::selectTool(int tool)
{
    selectionWindow.ShowWindow(SW_HIDE);
    toolsModel.SetActiveTool(tool);
    pointSP = 0;                // resets the point-buffer of the polygon and bezier functions
    toolSettingsWindow.Invalidate(TRUE);
    ::ShowWindow(hTrackbarZoom, (tool == TOOL_ZOOM) ? SW_SHOW : SW_HIDE);
    textEditWindow.ShowWindow((tool == TOOL_TEXT) ? SW_SHOW : SW_HIDE);
}

void
updateCanvasAndScrollbars()
{
    selectionWindow.ShowWindow(SW_HIDE);
    imageArea.MoveWindow(3, 3, imgXRes * toolsModel.GetZoom() / 1000, imgYRes * toolsModel.GetZoom() / 1000, FALSE);
    scrollboxWindow.Invalidate(TRUE);
    imageArea.Invalidate(FALSE);

    scrollboxWindow.SetScrollPos(SB_HORZ, 0, TRUE);
    scrollboxWindow.SetScrollPos(SB_VERT, 0, TRUE);
}

void
zoomTo(int newZoom, int mouseX, int mouseY)
{
    int tbPos = 0;
    int tempZoom = newZoom;

    RECT clientRectScrollbox;
    RECT clientRectImageArea;
    int x, y, w, h;
    scrollboxWindow.GetClientRect(&clientRectScrollbox);
    imageArea.GetClientRect(&clientRectImageArea);
    w = clientRectImageArea.right * clientRectScrollbox.right / (clientRectImageArea.right * newZoom / toolsModel.GetZoom());
    h = clientRectImageArea.bottom * clientRectScrollbox.bottom / (clientRectImageArea.bottom * newZoom / toolsModel.GetZoom());
    x = max(0, min(clientRectImageArea.right - w, mouseX - w / 2)) * newZoom / toolsModel.GetZoom();
    y = max(0, min(clientRectImageArea.bottom - h, mouseY - h / 2)) * newZoom / toolsModel.GetZoom();

    toolsModel.SetZoom(newZoom);

    selectionWindow.ShowWindow(SW_HIDE);
    imageArea.MoveWindow(3, 3, imgXRes * toolsModel.GetZoom() / 1000, imgYRes * toolsModel.GetZoom() / 1000, FALSE);
    scrollboxWindow.Invalidate(TRUE);
    imageArea.Invalidate(FALSE);

    scrollboxWindow.SendMessage(WM_HSCROLL, SB_THUMBPOSITION | (x << 16), 0);
    scrollboxWindow.SendMessage(WM_VSCROLL, SB_THUMBPOSITION | (y << 16), 0);

    while (tempZoom > 125)
    {
        tbPos++;
        tempZoom = tempZoom >> 1;
    }
    SendMessage(hTrackbarZoom, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) tbPos);
}

void CMainWindow::alignChildrenToMainWindow()
{
    int x, y, w, h;
    RECT clientRect;
    mainWindow.GetClientRect(&clientRect);

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

    scrollboxWindow.MoveWindow(x, y, w, ::IsWindowVisible(hStatusBar) ? h - 23 : h, TRUE);
    paletteWindow.MoveWindow(x, 9, 255, 32, TRUE);
}

void CMainWindow::saveImage(BOOL overwrite)
{
    if (isAFile && overwrite)
    {
        SaveDIBToFile(hBms[currInd], filepathname, hDrawingDC, &fileTime, &fileSize, fileHPPM,
                      fileVPPM);
        imageSaved = TRUE;
    }
    else if (GetSaveFileName(&sfn) != 0)
    {
        TCHAR tempstr[1000];
        TCHAR resstr[100];
        SaveDIBToFile(hBms[currInd], sfn.lpstrFile, hDrawingDC, &fileTime, &fileSize,
                      fileHPPM, fileVPPM);
        CopyMemory(filename, sfn.lpstrFileTitle, sizeof(filename));
        CopyMemory(filepathname, sfn.lpstrFile, sizeof(filepathname));
        LoadString(hProgInstance, IDS_WINDOWTITLE, resstr, SIZEOF(resstr));
        _stprintf(tempstr, resstr, filename);
        mainWindow.SetWindowText(tempstr);
        isAFile = TRUE;
        imageSaved = TRUE;
    }
}

void CMainWindow::UpdateApplicationProperties(HBITMAP bitmap, LPTSTR newfilename, LPTSTR newfilepathname)
{
    TCHAR tempstr[1000];
    TCHAR resstr[100];
    insertReversible(bitmap);
    updateCanvasAndScrollbars();
    CopyMemory(filename, newfilename, sizeof(filename));
    CopyMemory(filepathname, newfilepathname, sizeof(filepathname));
    LoadString(hProgInstance, IDS_WINDOWTITLE, resstr, SIZEOF(resstr));
    _stprintf(tempstr, resstr, filename);
    SetWindowText(tempstr);
    clearHistory();
    isAFile = TRUE;
}

void CMainWindow::InsertSelectionFromHBITMAP(HBITMAP bitmap, HWND window)
{
    HDC hTempDC;
    HBITMAP hTempMask;

    HWND hToolbar = FindWindowEx(toolBoxContainer.m_hWnd, NULL, TOOLBARCLASSNAME, NULL);
    SendMessage(hToolbar, TB_CHECKBUTTON, ID_RECTSEL, MAKELONG(TRUE, 0));
    SendMessage(window, WM_COMMAND, ID_RECTSEL, 0);

    DeleteObject(SelectObject(hSelDC, hSelBm = (HBITMAP) CopyImage(bitmap,
                                                                   IMAGE_BITMAP, 0, 0,
                                                                   LR_COPYRETURNORG)));
    newReversible();
    SetRectEmpty(&rectSel_src);
    rectSel_dest.left = rectSel_dest.top = 0;
    rectSel_dest.right = rectSel_dest.left + GetDIBWidth(hSelBm);
    rectSel_dest.bottom = rectSel_dest.top + GetDIBHeight(hSelBm);

    hTempDC = CreateCompatibleDC(hSelDC);
    hTempMask = CreateBitmap(RECT_WIDTH(rectSel_dest), RECT_HEIGHT(rectSel_dest), 1, 1, NULL);
    SelectObject(hTempDC, hTempMask);
    Rect(hTempDC, rectSel_dest.left, rectSel_dest.top, rectSel_dest.right, rectSel_dest.bottom, 0x00ffffff, 0x00ffffff, 1, 1);
    DeleteObject(hSelMask);
    hSelMask = hTempMask;
    DeleteDC(hTempDC);

    placeSelWin();
    selectionWindow.ShowWindow(SW_SHOW);
    ForceRefreshSelectionContents();
}

LRESULT CMainWindow::OnDropFiles(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HDROP drophandle;
    TCHAR droppedfile[MAX_PATH];
    HBITMAP bmNew = NULL;
    drophandle = (HDROP)wParam;
    DragQueryFile(drophandle, 0, droppedfile, SIZEOF(droppedfile));
    DragFinish(drophandle);
    LoadDIBFromFile(&bmNew, droppedfile, &fileTime, &fileSize, &fileHPPM, &fileVPPM);
    if (bmNew != NULL)
    {
        TCHAR *pathend;
        pathend = _tcsrchr(droppedfile, '\\');
        pathend++;
        UpdateApplicationProperties(bmNew, pathend, pathend);
    }
    return 0;
}

LRESULT CMainWindow::OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SendMessage(WM_SETICON, ICON_BIG, (LPARAM) LoadIcon(hProgInstance, MAKEINTRESOURCE(IDI_APPICON)));
    SendMessage(WM_SETICON, ICON_SMALL, (LPARAM) LoadIcon(hProgInstance, MAKEINTRESOURCE(IDI_APPICON)));
    ptStack = NULL;
    ptSP = 0;
    return 0;
}

LRESULT CMainWindow::OnDestroy(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    PostQuitMessage(0); /* send a WM_QUIT to the message queue */
    return 0;
}

LRESULT CMainWindow::OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (!imageSaved)
    {
        TCHAR programname[20];
        TCHAR saveprompttext[100];
        TCHAR temptext[500];
        LoadString(hProgInstance, IDS_PROGRAMNAME, programname, SIZEOF(programname));
        LoadString(hProgInstance, IDS_SAVEPROMPTTEXT, saveprompttext, SIZEOF(saveprompttext));
        _stprintf(temptext, saveprompttext, filename);
        switch (MessageBox(temptext, programname, MB_YESNOCANCEL | MB_ICONQUESTION))
        {
            case IDNO:
                DestroyWindow();
                break;
            case IDYES:
                saveImage(FALSE);
                if (imageSaved)
                    DestroyWindow();
                break;
        }
    }
    else
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
            EnableMenuItem(menu, IDM_FILEASWALLPAPERPLANE,     ENABLED_IF(isAFile));
            EnableMenuItem(menu, IDM_FILEASWALLPAPERCENTERED,  ENABLED_IF(isAFile));
            EnableMenuItem(menu, IDM_FILEASWALLPAPERSTRETCHED, ENABLED_IF(isAFile));
            break;
        case 1: /* Edit menu */
            EnableMenuItem(menu, IDM_EDITUNDO, ENABLED_IF(undoSteps > 0));
            EnableMenuItem(menu, IDM_EDITREDO, ENABLED_IF(redoSteps > 0));
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
    if (m_hWnd == mainWindow.m_hWnd)
    {
        int test[] = { LOWORD(lParam) - 260, LOWORD(lParam) - 140, LOWORD(lParam) - 20 };
        SendMessage(hStatusBar, WM_SIZE, wParam, lParam);
        SendMessage(hStatusBar, SB_SETPARTS, 3, (LPARAM)&test);
        alignChildrenToMainWindow();
        Invalidate(TRUE);
    }
    return 0;
}

LRESULT CMainWindow::OnGetMinMaxInfo(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (m_hWnd == mainWindow.m_hWnd)
    {
        MINMAXINFO *mm = (LPMINMAXINFO) lParam;
        mm->ptMinTrackSize.x = 330;
        mm->ptMinTrackSize.y = 430;
    }
    return 0;
}

LRESULT CMainWindow::OnSetCursor(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SetCursor(LoadCursor(NULL, IDC_ARROW));
    return 0;
}

LRESULT CMainWindow::OnKeyDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (wParam == VK_ESCAPE)
    {
        if (!imageArea.drawing)
        {
            /* Deselect */
            if ((toolsModel.GetActiveTool() == TOOL_RECTSEL) || (toolsModel.GetActiveTool() == TOOL_FREESEL))
            {
                startPaintingL(hDrawingDC, 0, 0, paletteModel.GetFgColor(), paletteModel.GetBgColor());
                whilePaintingL(hDrawingDC, 0, 0, paletteModel.GetFgColor(), paletteModel.GetBgColor());
                endPaintingL(hDrawingDC, 0, 0, paletteModel.GetFgColor(), paletteModel.GetBgColor());
                selectionWindow.ShowWindow(SW_HIDE);
            }
        }
        /* FIXME: also cancel current drawing underway */
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
            ShellAbout(mainWindow.m_hWnd, infotitle, infotext, paintIcon);
            DeleteObject(paintIcon);
            break;
        }
        case IDM_HELPHELPTOPICS:
            HtmlHelp(mainWindow.m_hWnd, _T("help\\Paint.chm"), 0, 0);
            break;
        case IDM_FILEEXIT:
            SendMessage(WM_CLOSE, wParam, lParam);
            break;
        case IDM_FILENEW:
        {
            BOOL reset = TRUE;
            if (!imageSaved)
            {
                TCHAR programname[20];
                TCHAR saveprompttext[100];
                TCHAR temptext[500];
                LoadString(hProgInstance, IDS_PROGRAMNAME, programname, SIZEOF(programname));
                LoadString(hProgInstance, IDS_SAVEPROMPTTEXT, saveprompttext, SIZEOF(saveprompttext));
                _stprintf(temptext, saveprompttext, filename);
                switch (MessageBox(temptext, programname, MB_YESNOCANCEL | MB_ICONQUESTION))
                {
                    case IDNO:
                        imageSaved = TRUE;
                        break;
                    case IDYES:
                        saveImage(FALSE);
                        break;
                    case IDCANCEL:
                        reset = FALSE;
                        break;
                }
            }
            if (reset && imageSaved)
            {
                Rectangle(hDrawingDC, 0 - 1, 0 - 1, imgXRes + 1, imgYRes + 1);
                imageArea.Invalidate(FALSE);
                updateCanvasAndScrollbars();
                clearHistory();
            }
            break;
        }
        case IDM_FILEOPEN:
            if (GetOpenFileName(&ofn) != 0)
            {
                HBITMAP bmNew = NULL;
                LoadDIBFromFile(&bmNew, ofn.lpstrFile, &fileTime, &fileSize, &fileHPPM, &fileVPPM);
                if (bmNew != NULL)
                {
                    UpdateApplicationProperties(bmNew, ofn.lpstrFileTitle, ofn.lpstrFileTitle);
                }
            }
            break;
        case IDM_FILESAVE:
            saveImage(TRUE);
            break;
        case IDM_FILESAVEAS:
            saveImage(FALSE);
            break;
        case IDM_FILEASWALLPAPERPLANE:
            SetWallpaper(filepathname, 1, 1);
            break;
        case IDM_FILEASWALLPAPERCENTERED:
            SetWallpaper(filepathname, 1, 0);
            break;
        case IDM_FILEASWALLPAPERSTRETCHED:
            SetWallpaper(filepathname, 2, 0);
            break;
        case IDM_EDITUNDO:
            undo();
            imageArea.Invalidate(FALSE);
            break;
        case IDM_EDITREDO:
            redo();
            imageArea.Invalidate(FALSE);
            break;
        case IDM_EDITCOPY:
            OpenClipboard();
            EmptyClipboard();
            SetClipboardData(CF_BITMAP, CopyImage(hSelBm, IMAGE_BITMAP, 0, 0, LR_COPYRETURNORG));
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
            /* remove selection window and already painted content using undo(),
            paint Rect for rectangular selections and Poly for freeform selections */
            undo();
            if (toolsModel.GetActiveTool() == TOOL_RECTSEL)
            {
                newReversible();
                Rect(hDrawingDC, rectSel_dest.left, rectSel_dest.top, rectSel_dest.right,
                     rectSel_dest.bottom, paletteModel.GetBgColor(), paletteModel.GetBgColor(), 0, TRUE);
            }
            if (toolsModel.GetActiveTool() == TOOL_FREESEL)
            {
                newReversible();
                Poly(hDrawingDC, ptStack, ptSP + 1, 0, 0, 2, 0, FALSE, TRUE);
            }
            break;
        }
        case IDM_EDITSELECTALL:
        {
            HWND hToolbar = FindWindowEx(toolBoxContainer.m_hWnd, NULL, TOOLBARCLASSNAME, NULL);
            SendMessage(hToolbar, TB_CHECKBUTTON, ID_RECTSEL, MAKELONG(TRUE, 0));
            SendMessage(WM_COMMAND, ID_RECTSEL, 0);
            startPaintingL(hDrawingDC, 0, 0, paletteModel.GetFgColor(), paletteModel.GetBgColor());
            whilePaintingL(hDrawingDC, imgXRes, imgYRes, paletteModel.GetFgColor(), paletteModel.GetBgColor());
            endPaintingL(hDrawingDC, imgXRes, imgYRes, paletteModel.GetFgColor(), paletteModel.GetBgColor());
            break;
        }
        case IDM_EDITCOPYTO:
            if (GetSaveFileName(&ofn) != 0)
                SaveDIBToFile(hSelBm, ofn.lpstrFile, hDrawingDC, NULL, NULL, fileHPPM, fileVPPM);
            break;
        case IDM_EDITPASTEFROM:
            if (GetOpenFileName(&ofn) != 0)
            {
                HBITMAP bmNew = NULL;
                LoadDIBFromFile(&bmNew, ofn.lpstrFile, &fileTime, &fileSize, &fileHPPM, &fileVPPM);
                if (bmNew != NULL)
                {
                    InsertSelectionFromHBITMAP(bmNew, m_hWnd);
                    DeleteObject(bmNew);
                }
            }
            break;
        case IDM_COLORSEDITPALETTE:
            if (ChooseColor(&choosecolor))
            {
                paletteModel.SetFgColor(choosecolor.rgbResult);
                paletteWindow.Invalidate(FALSE);
            }
            break;
        case IDM_COLORSMODERNPALETTE:
            paletteModel.SelectPalette(1);
            paletteWindow.Invalidate(FALSE);
            break;
        case IDM_COLORSOLDPALETTE:
            paletteModel.SelectPalette(2);
            paletteWindow.Invalidate(FALSE);
            break;
        case IDM_IMAGEINVERTCOLORS:
        {
            RECT tempRect;
            newReversible();
            SetRect(&tempRect, 0, 0, imgXRes, imgYRes);
            InvertRect(hDrawingDC, &tempRect);
            imageArea.Invalidate(FALSE);
            break;
        }
        case IDM_IMAGEDELETEIMAGE:
            newReversible();
            Rect(hDrawingDC, 0, 0, imgXRes, imgYRes, paletteModel.GetBgColor(), paletteModel.GetBgColor(), 0, TRUE);
            imageArea.Invalidate(FALSE);
            break;
        case IDM_IMAGEROTATEMIRROR:
            switch (mirrorRotateDlg())
            {
                case 1: /* flip horizontally */
                    if (selectionWindow.IsWindowVisible())
                    {
                        SelectObject(hSelDC, hSelMask);
                        StretchBlt(hSelDC, RECT_WIDTH(rectSel_dest) - 1, 0, -RECT_WIDTH(rectSel_dest), RECT_HEIGHT(rectSel_dest), hSelDC,
                                   0, 0, RECT_WIDTH(rectSel_dest), RECT_HEIGHT(rectSel_dest), SRCCOPY);
                        SelectObject(hSelDC, hSelBm);
                        StretchBlt(hSelDC, RECT_WIDTH(rectSel_dest) - 1, 0, -RECT_WIDTH(rectSel_dest), RECT_HEIGHT(rectSel_dest), hSelDC,
                                   0, 0, RECT_WIDTH(rectSel_dest), RECT_HEIGHT(rectSel_dest), SRCCOPY);
                        ForceRefreshSelectionContents();
                    }
                    else
                    {
                        newReversible();
                        StretchBlt(hDrawingDC, imgXRes - 1, 0, -imgXRes, imgYRes, hDrawingDC, 0, 0,
                                   imgXRes, imgYRes, SRCCOPY);
                        imageArea.Invalidate(FALSE);
                    }
                    break;
                case 2: /* flip vertically */
                    if (selectionWindow.IsWindowVisible())
                    {
                        SelectObject(hSelDC, hSelMask);
                        StretchBlt(hSelDC, 0, RECT_HEIGHT(rectSel_dest) - 1, RECT_WIDTH(rectSel_dest), -RECT_HEIGHT(rectSel_dest), hSelDC,
                                   0, 0, RECT_WIDTH(rectSel_dest), RECT_HEIGHT(rectSel_dest), SRCCOPY);
                        SelectObject(hSelDC, hSelBm);
                        StretchBlt(hSelDC, 0, RECT_HEIGHT(rectSel_dest) - 1, RECT_WIDTH(rectSel_dest), -RECT_HEIGHT(rectSel_dest), hSelDC,
                                   0, 0, RECT_WIDTH(rectSel_dest), RECT_HEIGHT(rectSel_dest), SRCCOPY);
                        ForceRefreshSelectionContents();
                    }
                    else
                    {
                        newReversible();
                        StretchBlt(hDrawingDC, 0, imgYRes - 1, imgXRes, -imgYRes, hDrawingDC, 0, 0,
                                   imgXRes, imgYRes, SRCCOPY);
                        imageArea.Invalidate(FALSE);
                    }
                    break;
                case 3: /* rotate 90 degrees */
                    break;
                case 4: /* rotate 180 degrees */
                    if (selectionWindow.IsWindowVisible())
                    {
                        SelectObject(hSelDC, hSelMask);
                        StretchBlt(hSelDC, RECT_WIDTH(rectSel_dest) - 1, RECT_HEIGHT(rectSel_dest) - 1, -RECT_WIDTH(rectSel_dest), -RECT_HEIGHT(rectSel_dest), hSelDC,
                                   0, 0, RECT_WIDTH(rectSel_dest), RECT_HEIGHT(rectSel_dest), SRCCOPY);
                        SelectObject(hSelDC, hSelBm);
                        StretchBlt(hSelDC, RECT_WIDTH(rectSel_dest) - 1, RECT_HEIGHT(rectSel_dest) - 1, -RECT_WIDTH(rectSel_dest), -RECT_HEIGHT(rectSel_dest), hSelDC,
                                   0, 0, RECT_WIDTH(rectSel_dest), RECT_HEIGHT(rectSel_dest), SRCCOPY);
                        ForceRefreshSelectionContents();
                    }
                    else
                    {
                        newReversible();
                        StretchBlt(hDrawingDC, imgXRes - 1, imgYRes - 1, -imgXRes, -imgYRes, hDrawingDC,
                                   0, 0, imgXRes, imgYRes, SRCCOPY);
                        imageArea.Invalidate(FALSE);
                    }
                    break;
                case 5: /* rotate 270 degrees */
                    break;
            }
            break;
        case IDM_IMAGEATTRIBUTES:
        {
            if (attributesDlg())
            {
                cropReversible(widthSetInDlg, heightSetInDlg, 0, 0);
                updateCanvasAndScrollbars();
            }
            break;
        }
        case IDM_IMAGESTRETCHSKEW:
        {
            if (changeSizeDlg())
            {
                insertReversible((HBITMAP) CopyImage(hBms[currInd], IMAGE_BITMAP,
                                                     imgXRes * stretchSkew.percentage.x / 100,
                                                     imgYRes * stretchSkew.percentage.y / 100, 0));
                updateCanvasAndScrollbars();
            }
            break;
        }
        case IDM_IMAGEDRAWOPAQUE:
            toolsModel.SetBackgroundTransparent(!toolsModel.IsBackgroundTransparent());
            toolSettingsWindow.Invalidate(TRUE);
            break;
        case IDM_IMAGECROP:
            insertReversible((HBITMAP) CopyImage(hSelBm, IMAGE_BITMAP, 0, 0, LR_COPYRETURNORG));
            updateCanvasAndScrollbars();
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
        case ID_FREESEL:
            selectTool(1);
            break;
        case ID_RECTSEL:
            selectTool(2);
            break;
        case ID_RUBBER:
            selectTool(3);
            break;
        case ID_FILL:
            selectTool(4);
            break;
        case ID_COLOR:
            selectTool(5);
            break;
        case ID_ZOOM:
            selectTool(6);
            break;
        case ID_PEN:
            selectTool(7);
            break;
        case ID_BRUSH:
            selectTool(8);
            break;
        case ID_AIRBRUSH:
            selectTool(9);
            break;
        case ID_TEXT:
            selectTool(10);
            break;
        case ID_LINE:
            selectTool(11);
            break;
        case ID_BEZIER:
            selectTool(12);
            break;
        case ID_RECT:
            selectTool(13);
            break;
        case ID_SHAPE:
            selectTool(14);
            break;
        case ID_ELLIPSE:
            selectTool(15);
            break;
        case ID_RRECT:
            selectTool(16);
            break;
    }
    return 0;
}
