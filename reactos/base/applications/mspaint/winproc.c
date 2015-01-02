/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/paint/winproc.c
 * PURPOSE:     Window procedure of the main window and all children apart from
 *              hPalWin, hToolSettings and hSelection
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

#include "winproc.h"
#include "dialogs.h"
#include "registry.h"
#include "scrollbox.h"

/* FUNCTIONS ********************************************************/

void
RegisterWclMain()
{
    WNDCLASSEX wclMain;
    /* initializing and registering the window class used for the main window */
    wclMain.hInstance         = hProgInstance;
    wclMain.lpszClassName     = _T("MainWindow");
    wclMain.lpfnWndProc       = MainWindowProcedure;
    wclMain.style             = CS_DBLCLKS;
    wclMain.cbSize            = sizeof(WNDCLASSEX);
    wclMain.hIcon             = LoadIcon(hProgInstance, MAKEINTRESOURCE(IDI_APPICON));
    wclMain.hIconSm           = LoadIcon(hProgInstance, MAKEINTRESOURCE(IDI_APPICON));
    wclMain.hCursor           = LoadCursor(NULL, IDC_ARROW);
    wclMain.lpszMenuName      = NULL;
    wclMain.cbClsExtra        = 0;
    wclMain.cbWndExtra        = 0;
    wclMain.hbrBackground     = GetSysColorBrush(COLOR_BTNFACE);
    RegisterClassEx (&wclMain);
}

void
selectTool(int tool)
{
    ShowWindow(hSelection, SW_HIDE);
    activeTool = tool;
    pointSP = 0;                // resets the point-buffer of the polygon and bezier functions
    InvalidateRect(hToolSettings, NULL, TRUE);
    ShowWindow(hTrackbarZoom, (tool == TOOL_ZOOM) ? SW_SHOW : SW_HIDE);
    ShowWindow(hwndTextEdit, (tool == TOOL_TEXT) ? SW_SHOW : SW_HIDE);
}

void
updateCanvasAndScrollbars()
{
    ShowWindow(hSelection, SW_HIDE);
    MoveWindow(hImageArea, 3, 3, imgXRes * zoom / 1000, imgYRes * zoom / 1000, FALSE);
    InvalidateRect(hScrollbox, NULL, TRUE);
    InvalidateRect(hImageArea, NULL, FALSE);

    SetScrollPos(hScrollbox, SB_HORZ, 0, TRUE);
    SetScrollPos(hScrollbox, SB_VERT, 0, TRUE);
}

void
zoomTo(int newZoom, int mouseX, int mouseY)
{
    int tbPos = 0;
    int tempZoom = newZoom;

    RECT clientRectScrollbox;
    RECT clientRectImageArea;
    int x, y, w, h;
    GetClientRect(hScrollbox, &clientRectScrollbox);
    GetClientRect(hImageArea, &clientRectImageArea);
    w = clientRectImageArea.right * clientRectScrollbox.right / (clientRectImageArea.right * newZoom / zoom);
    h = clientRectImageArea.bottom * clientRectScrollbox.bottom / (clientRectImageArea.bottom * newZoom / zoom);
    x = max(0, min(clientRectImageArea.right - w, mouseX - w / 2)) * newZoom / zoom;
    y = max(0, min(clientRectImageArea.bottom - h, mouseY - h / 2)) * newZoom / zoom;

    zoom = newZoom;

    ShowWindow(hSelection, SW_HIDE);
    MoveWindow(hImageArea, 3, 3, imgXRes * zoom / 1000, imgYRes * zoom / 1000, FALSE);
    InvalidateRect(hScrollbox, NULL, TRUE);
    InvalidateRect(hImageArea, NULL, FALSE);

    SendMessage(hScrollbox, WM_HSCROLL, SB_THUMBPOSITION | (x << 16), 0);
    SendMessage(hScrollbox, WM_VSCROLL, SB_THUMBPOSITION | (y << 16), 0);

    while (tempZoom > 125)
    {
        tbPos++;
        tempZoom = tempZoom >> 1;
    }
    SendMessage(hTrackbarZoom, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) tbPos);
}

void
drawZoomFrame(int mouseX, int mouseY)
{
    HDC hdc;
    HPEN oldPen;
    HBRUSH oldBrush;
    LOGBRUSH logbrush;
    int rop;

    RECT clientRectScrollbox;
    RECT clientRectImageArea;
    int x, y, w, h;
    GetClientRect(hScrollbox, &clientRectScrollbox);
    GetClientRect(hImageArea, &clientRectImageArea);
    w = clientRectImageArea.right * clientRectScrollbox.right / (clientRectImageArea.right * 2);
    h = clientRectImageArea.bottom * clientRectScrollbox.bottom / (clientRectImageArea.bottom * 2);
    x = max(0, min(clientRectImageArea.right - w, mouseX - w / 2));
    y = max(0, min(clientRectImageArea.bottom - h, mouseY - h / 2));

    hdc = GetDC(hImageArea);
    oldPen = SelectObject(hdc, CreatePen(PS_SOLID, 0, 0));
    logbrush.lbStyle = BS_HOLLOW;
    oldBrush = SelectObject(hdc, CreateBrushIndirect(&logbrush));
    rop = SetROP2(hdc, R2_NOT);
    Rectangle(hdc, x, y, x + w, y + h);
    SetROP2(hdc, rop);
    DeleteObject(SelectObject(hdc, oldBrush));
    DeleteObject(SelectObject(hdc, oldPen));
    ReleaseDC(hImageArea, hdc);
}

void
alignChildrenToMainWindow()
{
    int x, y, w, h;
    RECT clientRect;
    GetClientRect(hMainWnd, &clientRect);

    if (IsWindowVisible(hToolBoxContainer))
    {
        x = 56;
        w = clientRect.right - 56;
    }
    else
    {
        x = 0;
        w = clientRect.right;
    }
    if (IsWindowVisible(hPalWin))
    {
        y = 49;
        h = clientRect.bottom - 49;
    }
    else
    {
        y = 3;
        h = clientRect.bottom - 3;
    }

    MoveWindow(hScrollbox, x, y, w, IsWindowVisible(hStatusBar) ? h - 23 : h, TRUE);
    MoveWindow(hPalWin, x, 9, 255, 32, TRUE);
}

void
saveImage(BOOL overwrite)
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
        SetWindowText(hMainWnd, tempstr);
        isAFile = TRUE;
        imageSaved = TRUE;
    }
}

void
UpdateApplicationProperties(HBITMAP bitmap, LPTSTR newfilename, LPTSTR newfilepathname)
{
    TCHAR tempstr[1000];
    TCHAR resstr[100];
    insertReversible(bitmap);
    updateCanvasAndScrollbars();
    CopyMemory(filename, newfilename, sizeof(filename));
    CopyMemory(filepathname, newfilepathname, sizeof(filepathname));
    LoadString(hProgInstance, IDS_WINDOWTITLE, resstr, SIZEOF(resstr));
    _stprintf(tempstr, resstr, filename);
    SetWindowText(hMainWnd, tempstr);
    clearHistory();
    isAFile = TRUE;
}

void
InsertSelectionFromHBITMAP(HBITMAP bitmap, HWND window)
{
    HDC hTempDC;
    HBITMAP hTempMask;

    HWND hToolbar = FindWindowEx(hToolBoxContainer, NULL, TOOLBARCLASSNAME, NULL);
    SendMessage(hToolbar, TB_CHECKBUTTON, ID_RECTSEL, MAKELONG(TRUE, 0));
    SendMessage(window, WM_COMMAND, ID_RECTSEL, 0);

    DeleteObject(SelectObject(hSelDC, hSelBm = CopyImage(bitmap,
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
    ShowWindow(hSelection, SW_SHOW);
    ForceRefreshSelectionContents();
}

BOOL drawing;

LRESULT CALLBACK
MainWindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)            /* handle the messages */
    {
        case WM_DROPFILES:
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
            break;
        }

        case WM_CREATE:
            ptStack = NULL;
            ptSP = 0;
            break;

        case WM_DESTROY:
            PostQuitMessage(0); /* send a WM_QUIT to the message queue */
            break;

        case WM_CLOSE:
            if (hwnd == hwndMiniature)
            {
                ShowWindow(hwndMiniature, SW_HIDE);
                showMiniature = FALSE;
                break;
            }
            if (!imageSaved)
            {
                TCHAR programname[20];
                TCHAR saveprompttext[100];
                TCHAR temptext[500];
                LoadString(hProgInstance, IDS_PROGRAMNAME, programname, SIZEOF(programname));
                LoadString(hProgInstance, IDS_SAVEPROMPTTEXT, saveprompttext, SIZEOF(saveprompttext));
                _stprintf(temptext, saveprompttext, filename);
                switch (MessageBox(hwnd, temptext, programname, MB_YESNOCANCEL | MB_ICONQUESTION))
                {
                    case IDNO:
                        DestroyWindow(hwnd);
                        break;
                    case IDYES:
                        saveImage(FALSE);
                        if (imageSaved)
                            DestroyWindow(hwnd);
                        break;
                }
            }
            else
            {
                DestroyWindow(hwnd);
            }
            break;

        case WM_INITMENUPOPUP:
        {
            HMENU menu = GetMenu(hMainWnd);
            BOOL trueSelection = (IsWindowVisible(hSelection) && ((activeTool == TOOL_FREESEL) || (activeTool == TOOL_RECTSEL)));
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
                    OpenClipboard(hMainWnd);
                    EnableMenuItem(menu, IDM_EDITPASTE, ENABLED_IF(GetClipboardData(CF_BITMAP) != NULL));
                    CloseClipboard();
                    break;
                case 2: /* View menu */
                        CheckMenuItem(menu, IDM_VIEWTOOLBOX,      CHECKED_IF(IsWindowVisible(hToolBoxContainer)));
                        CheckMenuItem(menu, IDM_VIEWCOLORPALETTE, CHECKED_IF(IsWindowVisible(hPalWin)));
                        CheckMenuItem(menu, IDM_VIEWSTATUSBAR,    CHECKED_IF(IsWindowVisible(hStatusBar)));
                        CheckMenuItem(menu, IDM_FORMATICONBAR,    CHECKED_IF(IsWindowVisible(hwndTextEdit)));
                        EnableMenuItem(menu, IDM_FORMATICONBAR, ENABLED_IF(activeTool == TOOL_TEXT));

                        CheckMenuItem(menu, IDM_VIEWSHOWGRID,      CHECKED_IF(showGrid));
                        CheckMenuItem(menu, IDM_VIEWSHOWMINIATURE, CHECKED_IF(showMiniature));
                    break;
                case 3: /* Image menu */
                    EnableMenuItem(menu, IDM_IMAGECROP, ENABLED_IF(IsWindowVisible(hSelection)));
                    CheckMenuItem(menu, IDM_IMAGEDRAWOPAQUE, CHECKED_IF(transpBg == 0));
                    break;
            }

            CheckMenuItem(menu, IDM_VIEWZOOM125, CHECKED_IF(zoom == 125));
            CheckMenuItem(menu, IDM_VIEWZOOM25,  CHECKED_IF(zoom == 250));
            CheckMenuItem(menu, IDM_VIEWZOOM50,  CHECKED_IF(zoom == 500));
            CheckMenuItem(menu, IDM_VIEWZOOM100, CHECKED_IF(zoom == 1000));
            CheckMenuItem(menu, IDM_VIEWZOOM200, CHECKED_IF(zoom == 2000));
            CheckMenuItem(menu, IDM_VIEWZOOM400, CHECKED_IF(zoom == 4000));
            CheckMenuItem(menu, IDM_VIEWZOOM800, CHECKED_IF(zoom == 8000));

            CheckMenuItem(menu, IDM_COLORSMODERNPALETTE, CHECKED_IF(selectedPalette == 1));
            CheckMenuItem(menu, IDM_COLORSOLDPALETTE,    CHECKED_IF(selectedPalette == 2));
            break;
        }

        case WM_SIZE:
            if (hwnd == hMainWnd)
            {
                int test[] = { LOWORD(lParam) - 260, LOWORD(lParam) - 140, LOWORD(lParam) - 20 };
                SendMessage(hStatusBar, WM_SIZE, wParam, lParam);
                SendMessage(hStatusBar, SB_SETPARTS, 3, (LPARAM)&test);
                alignChildrenToMainWindow();
                InvalidateRect(hwnd, NULL, TRUE);
            }
            if (hwnd == hImageArea)
            {
                MoveWindow(hSizeboxLeftTop,
                           0,
                           0, 3, 3, TRUE);
                MoveWindow(hSizeboxCenterTop,
                           imgXRes * zoom / 2000 + 3 * 3 / 4,
                           0, 3, 3, TRUE);
                MoveWindow(hSizeboxRightTop,
                           imgXRes * zoom / 1000 + 3,
                           0, 3, 3, TRUE);
                MoveWindow(hSizeboxLeftCenter,
                           0,
                           imgYRes * zoom / 2000 + 3 * 3 / 4, 3, 3, TRUE);
                MoveWindow(hSizeboxRightCenter,
                           imgXRes * zoom / 1000 + 3,
                           imgYRes * zoom / 2000 + 3 * 3 / 4, 3, 3, TRUE);
                MoveWindow(hSizeboxLeftBottom,
                           0,
                           imgYRes * zoom / 1000 + 3, 3, 3, TRUE);
                MoveWindow(hSizeboxCenterBottom,
                           imgXRes * zoom / 2000 + 3 * 3 / 4,
                           imgYRes * zoom / 1000 + 3, 3, 3, TRUE);
                MoveWindow(hSizeboxRightBottom,
                           imgXRes * zoom / 1000 + 3,
                           imgYRes * zoom / 1000 + 3, 3, 3, TRUE);
            }
            if (hwnd == hImageArea)
            {
                UpdateScrollbox();
            }
            break;

        case WM_GETMINMAXINFO:
            if (hwnd == hMainWnd)
            {
                MINMAXINFO *mm = (LPMINMAXINFO) lParam;
                mm->ptMinTrackSize.x = 330;
                mm->ptMinTrackSize.y = 430;
            }
            break;

        case WM_PAINT:
            DefWindowProc(hwnd, message, wParam, lParam);
            if (hwnd == hImageArea)
            {
                HDC hdc = GetDC(hImageArea);
                StretchBlt(hdc, 0, 0, imgXRes * zoom / 1000, imgYRes * zoom / 1000, hDrawingDC, 0, 0, imgXRes,
                           imgYRes, SRCCOPY);
                if (showGrid && (zoom >= 4000))
                {
                    HPEN oldPen = SelectObject(hdc, CreatePen(PS_SOLID, 1, 0x00a0a0a0));
                    int counter;
                    for(counter = 0; counter <= imgYRes; counter++)
                    {
                        MoveToEx(hdc, 0, counter * zoom / 1000, NULL);
                        LineTo(hdc, imgXRes * zoom / 1000, counter * zoom / 1000);
                    }
                    for(counter = 0; counter <= imgXRes; counter++)
                    {
                        MoveToEx(hdc, counter * zoom / 1000, 0, NULL);
                        LineTo(hdc, counter * zoom / 1000, imgYRes * zoom / 1000);
                    }
                    DeleteObject(SelectObject(hdc, oldPen));
                }
                ReleaseDC(hImageArea, hdc);
                InvalidateRect(hSelection, NULL, FALSE);
                InvalidateRect(hwndMiniature, NULL, FALSE);
            }
            else if (hwnd == hwndMiniature)
            {
                RECT mclient;
                HDC hdc;
                GetClientRect(hwndMiniature, &mclient);
                hdc = GetDC(hwndMiniature);
                StretchBlt(hdc, 0, 0, mclient.right, mclient.bottom, hDrawingDC, 0, 0, imgXRes, imgYRes, SRCCOPY);
                ReleaseDC(hwndMiniature, hdc);
            }
            break;

            // mouse events used for drawing

        case WM_SETCURSOR:
            if (hwnd == hImageArea)
            {
                switch (activeTool)
                {
                    case TOOL_FILL:
                        SetCursor(hCurFill);
                        break;
                    case TOOL_COLOR:
                        SetCursor(hCurColor);
                        break;
                    case TOOL_ZOOM:
                        SetCursor(hCurZoom);
                        break;
                    case TOOL_PEN:
                        SetCursor(hCurPen);
                        break;
                    case TOOL_AIRBRUSH:
                        SetCursor(hCurAirbrush);
                        break;
                    default:
                        SetCursor(LoadCursor(NULL, IDC_CROSS));
                }
            }
            else
                DefWindowProc(hwnd, message, wParam, lParam);
            break;

        case WM_LBUTTONDOWN:
            if (hwnd == hImageArea)
            {
                if ((!drawing) || (activeTool == TOOL_COLOR))
                {
                    SetCapture(hImageArea);
                    drawing = TRUE;
                    startPaintingL(hDrawingDC, GET_X_LPARAM(lParam) * 1000 / zoom, GET_Y_LPARAM(lParam) * 1000 / zoom,
                                   fgColor, bgColor);
                }
                else
                {
                    SendMessage(hwnd, WM_LBUTTONUP, wParam, lParam);
                    undo();
                }
                InvalidateRect(hImageArea, NULL, FALSE);
                if ((activeTool == TOOL_ZOOM) && (zoom < 8000))
                    zoomTo(zoom * 2, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            }
            break;

        case WM_RBUTTONDOWN:
            if (hwnd == hImageArea)
            {
                if ((!drawing) || (activeTool == TOOL_COLOR))
                {
                    SetCapture(hImageArea);
                    drawing = TRUE;
                    startPaintingR(hDrawingDC, GET_X_LPARAM(lParam) * 1000 / zoom, GET_Y_LPARAM(lParam) * 1000 / zoom,
                                   fgColor, bgColor);
                }
                else
                {
                    SendMessage(hwnd, WM_RBUTTONUP, wParam, lParam);
                    undo();
                }
                InvalidateRect(hImageArea, NULL, FALSE);
                if ((activeTool == TOOL_ZOOM) && (zoom > 125))
                    zoomTo(zoom / 2, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            }
            break;

        case WM_LBUTTONUP:
            if ((hwnd == hImageArea) && drawing)
            {
                ReleaseCapture();
                drawing = FALSE;
                endPaintingL(hDrawingDC, GET_X_LPARAM(lParam) * 1000 / zoom, GET_Y_LPARAM(lParam) * 1000 / zoom, fgColor,
                             bgColor);
                InvalidateRect(hImageArea, NULL, FALSE);
                if (activeTool == TOOL_COLOR)
                {
                    int tempColor =
                        GetPixel(hDrawingDC, GET_X_LPARAM(lParam) * 1000 / zoom, GET_Y_LPARAM(lParam) * 1000 / zoom);
                    if (tempColor != CLR_INVALID)
                        fgColor = tempColor;
                    InvalidateRect(hPalWin, NULL, FALSE);
                }
                SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) "");
            }
            break;

        case WM_RBUTTONUP:
            if ((hwnd == hImageArea) && drawing)
            {
                ReleaseCapture();
                drawing = FALSE;
                endPaintingR(hDrawingDC, GET_X_LPARAM(lParam) * 1000 / zoom, GET_Y_LPARAM(lParam) * 1000 / zoom, fgColor,
                             bgColor);
                InvalidateRect(hImageArea, NULL, FALSE);
                if (activeTool == TOOL_COLOR)
                {
                    int tempColor =
                        GetPixel(hDrawingDC, GET_X_LPARAM(lParam) * 1000 / zoom, GET_Y_LPARAM(lParam) * 1000 / zoom);
                    if (tempColor != CLR_INVALID)
                        bgColor = tempColor;
                    InvalidateRect(hPalWin, NULL, FALSE);
                }
                SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) "");
            }
            break;

        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE)
            {
                if (!drawing)
                {
                    /* Deselect */
                    if ((activeTool == TOOL_RECTSEL) || (activeTool == TOOL_FREESEL))
                    {
                        startPaintingL(hDrawingDC, 0, 0, fgColor, bgColor);
                        whilePaintingL(hDrawingDC, 0, 0, fgColor, bgColor);
                        endPaintingL(hDrawingDC, 0, 0, fgColor, bgColor);
                        ShowWindow(hSelection, SW_HIDE);
                    }
                }
                /* FIXME: also cancel current drawing underway */
            }
            break;

        case WM_MOUSEMOVE:
            if (hwnd == hImageArea)
            {
                LONG xNow = GET_X_LPARAM(lParam) * 1000 / zoom;
                LONG yNow = GET_Y_LPARAM(lParam) * 1000 / zoom;
                if ((!drawing) || (activeTool <= TOOL_AIRBRUSH))
                {
                    TRACKMOUSEEVENT tme;

                    if (activeTool == TOOL_ZOOM)
                    {
                        InvalidateRect(hImageArea, NULL, FALSE);
                        UpdateWindow(hImageArea);
                        drawZoomFrame(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
                    }

                    tme.cbSize = sizeof(TRACKMOUSEEVENT);
                    tme.dwFlags = TME_LEAVE;
                    tme.hwndTrack = hImageArea;
                    tme.dwHoverTime = 0;
                    TrackMouseEvent(&tme);

                    if (!drawing)
                    {
                        TCHAR coordStr[100];
                        _stprintf(coordStr, _T("%ld, %ld"), xNow, yNow);
                        SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM) coordStr);
                    }
                }
                if (drawing)
                {
                    /* values displayed in statusbar */
                    LONG xRel = xNow - start.x;
                    LONG yRel = yNow - start.y;
                    /* freesel, rectsel and text tools always show numbers limited to fit into image area */
                    if ((activeTool == TOOL_FREESEL) || (activeTool == TOOL_RECTSEL) || (activeTool == TOOL_TEXT))
                    {
                        if (xRel < 0)
                            xRel = (xNow < 0) ? -start.x : xRel;
                        else if (xNow > imgXRes)
                            xRel = imgXRes-start.x;
                        if (yRel < 0)
                            yRel = (yNow < 0) ? -start.y : yRel;
                        else if (yNow > imgYRes)
                             yRel = imgYRes-start.y;
                    }
                    /* rectsel and shape tools always show non-negative numbers when drawing */
                    if ((activeTool == TOOL_RECTSEL) || (activeTool == TOOL_SHAPE))
                    {
                        if (xRel < 0)
                            xRel = -xRel;
                        if (yRel < 0)
                            yRel =  -yRel;
                    }
                    /* while drawing, update cursor coordinates only for tools 3, 7, 8, 9, 14 */
                    switch(activeTool)
                    {
                        case TOOL_RUBBER:
                        case TOOL_PEN:
                        case TOOL_BRUSH:
                        case TOOL_AIRBRUSH:
                        case TOOL_SHAPE:
                        {
                            TCHAR coordStr[100];
                            _stprintf(coordStr, _T("%ld, %ld"), xNow, yNow);
                            SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM) coordStr);
                            break;
                        }
                    }
                    if ((wParam & MK_LBUTTON) != 0)
                    {
                        whilePaintingL(hDrawingDC, xNow, yNow, fgColor, bgColor);
                        InvalidateRect(hImageArea, NULL, FALSE);
                        if ((activeTool >= TOOL_TEXT) || (activeTool == TOOL_RECTSEL) || (activeTool == TOOL_FREESEL))
                        {
                            TCHAR sizeStr[100];
                            if ((activeTool >= TOOL_LINE) && (GetAsyncKeyState(VK_SHIFT) < 0))
                                yRel = xRel;
                            _stprintf(sizeStr, _T("%ld x %ld"), xRel, yRel);
                            SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) sizeStr);
                        }
                    }
                    if ((wParam & MK_RBUTTON) != 0)
                    {
                        whilePaintingR(hDrawingDC, xNow, yNow, fgColor, bgColor);
                        InvalidateRect(hImageArea, NULL, FALSE);
                        if (activeTool >= TOOL_TEXT)
                        {
                            TCHAR sizeStr[100];
                            if ((activeTool >= TOOL_LINE) && (GetAsyncKeyState(VK_SHIFT) < 0))
                                yRel = xRel;
                            _stprintf(sizeStr, _T("%ld x %ld"), xRel, yRel);
                            SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) sizeStr);
                        }
                    }
                }
            }
            break;

        case WM_MOUSELEAVE:
            SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM) _T(""));
            if (activeTool == TOOL_ZOOM)
                InvalidateRect(hImageArea, NULL, FALSE);
            break;

        // menu and button events

        case WM_SYSCOLORCHANGE:
        {
            /* Redirect message to common controls */
            HWND hToolbar = FindWindowEx(hToolBoxContainer, NULL, TOOLBARCLASSNAME, NULL);
            SendMessage(hToolbar, WM_SYSCOLORCHANGE, 0, 0);
            break;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDM_HELPINFO:
                {
                    HICON paintIcon = LoadIcon(hProgInstance, MAKEINTRESOURCE(IDI_APPICON));
                    TCHAR infotitle[100];
                    TCHAR infotext[200];
                    LoadString(hProgInstance, IDS_INFOTITLE, infotitle, SIZEOF(infotitle));
                    LoadString(hProgInstance, IDS_INFOTEXT, infotext, SIZEOF(infotext));
                    ShellAbout(hMainWnd, infotitle, infotext, paintIcon);
                    DeleteObject(paintIcon);
                    break;
                }
                case IDM_HELPHELPTOPICS:
                    //HtmlHelp(hMainWnd, "help\\Paint.chm", 0, 0);
                    break;
                case IDM_FILEEXIT:
                    SendMessage(hwnd, WM_CLOSE, wParam, lParam);
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
                        switch (MessageBox(hwnd, temptext, programname, MB_YESNOCANCEL | MB_ICONQUESTION))
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
                        InvalidateRect(hImageArea, NULL, FALSE);
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
                    InvalidateRect(hImageArea, NULL, FALSE);
                    break;
                case IDM_EDITREDO:
                    redo();
                    InvalidateRect(hImageArea, NULL, FALSE);
                    break;
                case IDM_EDITCOPY:
                    OpenClipboard(hMainWnd);
                    EmptyClipboard();
                    SetClipboardData(CF_BITMAP, CopyImage(hSelBm, IMAGE_BITMAP, 0, 0, LR_COPYRETURNORG));
                    CloseClipboard();
                    break;
                case IDM_EDITCUT:
                    /* Copy */
                    SendMessage(hwnd, WM_COMMAND, IDM_EDITCOPY, 0);
                    /* Delete selection */
                    SendMessage(hwnd, WM_COMMAND, IDM_EDITDELETESELECTION, 0);
                    break;
                case IDM_EDITPASTE:
                    OpenClipboard(hMainWnd);
                    if (GetClipboardData(CF_BITMAP) != NULL)
                    {
                        InsertSelectionFromHBITMAP(GetClipboardData(CF_BITMAP), hwnd);
                    }
                    CloseClipboard();
                    break;
                case IDM_EDITDELETESELECTION:
                {
                    /* remove selection window and already painted content using undo(),
                    paint Rect for rectangular selections and Poly for freeform selections */
                    undo();
                    if (activeTool == TOOL_RECTSEL)
                    {
                        newReversible();
                        Rect(hDrawingDC, rectSel_dest.left, rectSel_dest.top, rectSel_dest.right,
                             rectSel_dest.bottom, bgColor, bgColor, 0, TRUE);
                    }
                    if (activeTool == TOOL_FREESEL)
                    {
                        newReversible();
                        Poly(hDrawingDC, ptStack, ptSP + 1, 0, 0, 2, 0, FALSE, TRUE);
                    }
                    break;
                }
                case IDM_EDITSELECTALL:
                {
                    HWND hToolbar = FindWindowEx(hToolBoxContainer, NULL, TOOLBARCLASSNAME, NULL);
                    SendMessage(hToolbar, TB_CHECKBUTTON, ID_RECTSEL, MAKELONG(TRUE, 0));
                    SendMessage(hwnd, WM_COMMAND, ID_RECTSEL, 0);
                    startPaintingL(hDrawingDC, 0, 0, fgColor, bgColor);
                    whilePaintingL(hDrawingDC, imgXRes, imgYRes, fgColor, bgColor);
                    endPaintingL(hDrawingDC, imgXRes, imgYRes, fgColor, bgColor);
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
                            InsertSelectionFromHBITMAP(bmNew, hwnd);
                            DeleteObject(bmNew);
                        }
                    }
                    break;
                case IDM_COLORSEDITPALETTE:
                    if (ChooseColor(&choosecolor))
                    {
                        fgColor = choosecolor.rgbResult;
                        InvalidateRect(hPalWin, NULL, FALSE);
                    }
                    break;
                case IDM_COLORSMODERNPALETTE:
                    selectedPalette = 1;
                    CopyMemory(palColors, modernPalColors, sizeof(palColors));
                    InvalidateRect(hPalWin, NULL, FALSE);
                    break;
                case IDM_COLORSOLDPALETTE:
                    selectedPalette = 2;
                    CopyMemory(palColors, oldPalColors, sizeof(palColors));
                    InvalidateRect(hPalWin, NULL, FALSE);
                    break;
                case IDM_IMAGEINVERTCOLORS:
                {
                    RECT tempRect;
                    newReversible();
                    SetRect(&tempRect, 0, 0, imgXRes, imgYRes);
                    InvertRect(hDrawingDC, &tempRect);
                    InvalidateRect(hImageArea, NULL, FALSE);
                    break;
                }
                case IDM_IMAGEDELETEIMAGE:
                    newReversible();
                    Rect(hDrawingDC, 0, 0, imgXRes, imgYRes, bgColor, bgColor, 0, TRUE);
                    InvalidateRect(hImageArea, NULL, FALSE);
                    break;
                case IDM_IMAGEROTATEMIRROR:
                    switch (mirrorRotateDlg())
                    {
                        case 1: /* flip horizontally */
                            if (IsWindowVisible(hSelection))
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
                                InvalidateRect(hImageArea, NULL, FALSE);
                            }
                            break;
                        case 2: /* flip vertically */
                            if (IsWindowVisible(hSelection))
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
                                InvalidateRect(hImageArea, NULL, FALSE);
                            }
                            break;
                        case 3: /* rotate 90 degrees */
                            break;
                        case 4: /* rotate 180 degrees */
                            if (IsWindowVisible(hSelection))
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
                                InvalidateRect(hImageArea, NULL, FALSE);
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
                        insertReversible(CopyImage(hBms[currInd], IMAGE_BITMAP,
                                                   imgXRes * stretchSkew.percentage.x / 100,
                                                   imgYRes * stretchSkew.percentage.y / 100, 0));
                        updateCanvasAndScrollbars();
                    }
                    break;
                }
                case IDM_IMAGEDRAWOPAQUE:
                    transpBg = 1 - transpBg;
                    InvalidateRect(hToolSettings, NULL, TRUE);
                    break;
                case IDM_IMAGECROP:
                    insertReversible(CopyImage(hSelBm, IMAGE_BITMAP, 0, 0, LR_COPYRETURNORG));
                    updateCanvasAndScrollbars();
                    break;

                case IDM_VIEWTOOLBOX:
                    ShowWindow(hToolBoxContainer, IsWindowVisible(hToolBoxContainer) ? SW_HIDE : SW_SHOW);
                    alignChildrenToMainWindow();
                    break;
                case IDM_VIEWCOLORPALETTE:
                    ShowWindow(hPalWin, IsWindowVisible(hPalWin) ? SW_HIDE : SW_SHOW);
                    alignChildrenToMainWindow();
                    break;
                case IDM_VIEWSTATUSBAR:
                    ShowWindow(hStatusBar, IsWindowVisible(hStatusBar) ? SW_HIDE : SW_SHOW);
                    alignChildrenToMainWindow();
                    break;
                case IDM_FORMATICONBAR:
                    ShowWindow(hwndTextEdit, IsWindowVisible(hwndTextEdit) ? SW_HIDE : SW_SHOW);

                case IDM_VIEWSHOWGRID:
                    showGrid = !showGrid;
                    InvalidateRect(hImageArea, NULL, FALSE);
                    break;
                case IDM_VIEWSHOWMINIATURE:
                    showMiniature = !showMiniature;
                    ShowWindow(hwndMiniature, showMiniature ? SW_SHOW : SW_HIDE);
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
            break;
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0;
}
