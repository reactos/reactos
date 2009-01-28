/*
 *
 * PROJECT:         ReactOS Picture and Fax Viewer
 * FILE:            dll/win32/shimgvw/shimgvw.c
 * PURPOSE:         shimgvw.dll
 * PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
 *
 * UPDATE HISTORY:
 *      28/05/2008  Created
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <windows.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <tchar.h>
#include <debug.h>

#include "shimgvw.h"


HINSTANCE hInstance;
SHIMGVW_SETTINGS shiSettings;
GpImage *image;
WNDPROC PrevProc = NULL;

HWND hDispWnd, hToolBar;

/* ToolBar Buttons */
static const TBBUTTON Buttons [] =
{   /* iBitmap,     idCommand,   fsState,         fsStyle,     bReserved[2], dwData, iString */
    {TBICON_PREV,   IDC_PREV,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
    {TBICON_NEXT,   IDC_NEXT,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
    {15,            0,           TBSTATE_ENABLED, BTNS_SEP,    {0}, 0, 0},
    {TBICON_ZOOMP,  IDC_ZOOMP,   TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
    {TBICON_ZOOMM,  IDC_ZOOMM,   TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
    {15,            0,           TBSTATE_ENABLED, BTNS_SEP,    {0}, 0, 0},
    {TBICON_ROT1,   IDC_ROT1,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
    {TBICON_ROT2,   IDC_ROT2,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
    {15,            0,           TBSTATE_ENABLED, BTNS_SEP,    {0}, 0, 0},
    {TBICON_SAVE,   IDC_SAVE,    TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
    {TBICON_PRINT,  IDC_PRINT,   TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
};

static void pLoadImage(LPWSTR szOpenFileName)
{
    if (GetFileAttributesW(szOpenFileName) == 0xFFFFFFFF)
    {
        DPRINT1("File %s not found!\n", szOpenFileName);
        return;
    }

    GdipLoadImageFromFile(szOpenFileName, &image);
    if (!image)
    {
        DPRINT1("GdipLoadImageFromFile() failed\n");
    }
}

static VOID
ImageView_DrawImage(HWND hwnd)
{
    GpGraphics *graphics;
    UINT uImgWidth, uImgHeight;
    UINT height = 0, width = 0, x = 0, y = 0;
    PAINTSTRUCT ps;
    RECT rect;
    HDC hdc;

    hdc = BeginPaint(hwnd, &ps);
    if (!hdc)
    {
        DPRINT1("BeginPaint() failed\n");
        return;
    }

    GdipCreateFromHDC(hdc, &graphics);
    if (!graphics)
    {
        DPRINT1("GdipCreateFromHDC() failed\n");
        return;
    }
  
    GdipGetImageWidth(image, &uImgWidth);
    GdipGetImageHeight(image, &uImgHeight);

    if (GetClientRect(hwnd, &rect))
    {
        FillRect(hdc, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));

        if ((rect.right == uImgWidth)&&(rect.bottom == uImgHeight))
        {
            x = 0, y = 0, width = rect.right, height = rect.bottom;
        }
        else if ((rect.right >= uImgWidth)&&(rect.bottom >= uImgHeight))
        {
            x = (rect.right/2)-(uImgWidth/2);
            y = (rect.bottom/2)-(uImgHeight/2);
            width = uImgWidth;
            height = uImgHeight;
        }
        else if ((rect.right < uImgWidth)||(rect.bottom < uImgHeight))
        {
            if (rect.bottom < uImgHeight)
            {
                height = rect.bottom;
                width = uImgWidth*(UINT)rect.bottom/uImgHeight;
                x = (rect.right/2)-(width/2);
                y = (rect.bottom/2)-(height/2);
            }
            if (rect.right < uImgWidth)
            {
                width = rect.right;
                height = uImgHeight*(UINT)rect.right/uImgWidth;
                x = (rect.right/2)-(width/2);
                y = (rect.bottom/2)-(height/2);
            }
            if ((height > rect.bottom)||(width > rect.right))
            {
                for (;;)
                {
                    if (((int)width - 1 < 0)||((int)height - 1 < 0)) break;
                    width -= 1;
                    height -= 1;
                    y = (rect.bottom/2)-(height/2);
                    x = (rect.right/2)-(width/2);
                    if ((height < rect.bottom)&&(width < rect.right)) break;
                }
            }
        }
        else if ((rect.right <= uImgWidth)&&(rect.bottom <= uImgHeight))
        {
            height = uImgHeight*(UINT)rect.right/uImgWidth;
            y = (rect.bottom/2)-(height/2);
            width = rect.right;

            if ((height > rect.bottom)||(width > rect.right))
            {
                for (;;)
                {
                    if (((int)width - 1 < 0)||((int)height - 1 < 0)) break;
                    width -= 1;
                    height -= 1;
                    y = (rect.bottom/2)-(height/2);
                    x = (rect.right/2)-(width/2);
                    if ((height < rect.bottom)&&(width < rect.right)) break;
                }
            }
        }

        DPRINT("x = %d\ny = %d\nWidth = %d\nHeight = %d\n\nrect.right = %d\nrect.bottom = %d\n\nuImgWidth = %d\nuImgHeight = %d\n", x, y, width, height, rect.right, rect.bottom, uImgWidth, uImgHeight);
        Rectangle(hdc, x - 1, y - 1, x + width + 1, y + height + 1);
        GdipDrawImageRect(graphics, image, x, y, width, height);
    }
    GdipDeleteGraphics(graphics);
    EndPaint(hwnd, &ps);
}

static BOOL
ImageView_LoadSettings()
{
    HKEY hKey;
    DWORD dwSize;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\ReactOS\\shimgvw"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(SHIMGVW_SETTINGS);
        if (RegQueryValueEx(hKey, _T("Settings"), NULL, NULL, (LPBYTE)&shiSettings, &dwSize) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return TRUE;
        }

        RegCloseKey(hKey);
    }

    return FALSE;
}

static VOID
ImageView_SaveSettings(HWND hwnd)
{
    WINDOWPLACEMENT wp;
    HKEY hKey;

    ShowWindow(hwnd, SW_HIDE);
    wp.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(hwnd, &wp);

    shiSettings.Left = wp.rcNormalPosition.left;
    shiSettings.Top  = wp.rcNormalPosition.top;
    shiSettings.Right  = wp.rcNormalPosition.right;
    shiSettings.Bottom = wp.rcNormalPosition.bottom;
    shiSettings.Maximized = (IsZoomed(hwnd) || (wp.flags & WPF_RESTORETOMAXIMIZED));

    if (RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\ReactOS\\shimgvw"), 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        RegSetValueEx(hKey, _T("Settings"), 0, REG_BINARY, (LPBYTE)&shiSettings, sizeof(SHIMGVW_SETTINGS));
        RegCloseKey(hKey);
    }
}

static BOOL
ImageView_CreateToolBar(HWND hwnd)
{
    INT numButtons = sizeof(Buttons) / sizeof(Buttons[0]);

    hToolBar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
                              WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | CCS_BOTTOM | TBSTYLE_TOOLTIPS,
                              0, 0, 0, 0, hwnd,
                              0, hInstance, NULL);
    if(hToolBar != NULL)
    {
        HIMAGELIST hImageList;

        SendMessage(hToolBar, TB_SETEXTENDEDSTYLE,
                    0, TBSTYLE_EX_HIDECLIPPEDBUTTONS);

        SendMessage(hToolBar, TB_BUTTONSTRUCTSIZE,
                    sizeof(Buttons[0]), 0);

        hImageList = ImageList_Create(TB_IMAGE_WIDTH, TB_IMAGE_HEIGHT, ILC_MASK | ILC_COLOR24, 1, 1);

        ImageList_AddMasked(hImageList, LoadImage(hInstance, MAKEINTRESOURCE(IDB_PREVICON), IMAGE_BITMAP,
                      TB_IMAGE_WIDTH, TB_IMAGE_HEIGHT, LR_DEFAULTCOLOR), RGB(255, 255, 255));

        ImageList_AddMasked(hImageList, LoadImage(hInstance, MAKEINTRESOURCE(IDB_NEXTICON), IMAGE_BITMAP,
                      TB_IMAGE_WIDTH, TB_IMAGE_HEIGHT, LR_DEFAULTCOLOR), RGB(255, 255, 255));

        ImageList_AddMasked(hImageList, LoadImage(hInstance, MAKEINTRESOURCE(IDB_ZOOMPICON), IMAGE_BITMAP,
                      TB_IMAGE_WIDTH, TB_IMAGE_HEIGHT, LR_DEFAULTCOLOR), RGB(255, 255, 255));

        ImageList_AddMasked(hImageList, LoadImage(hInstance, MAKEINTRESOURCE(IDB_ZOOMMICON), IMAGE_BITMAP,
                      TB_IMAGE_WIDTH, TB_IMAGE_HEIGHT, LR_DEFAULTCOLOR), RGB(255, 255, 255));

        ImageList_AddMasked(hImageList, LoadImage(hInstance, MAKEINTRESOURCE(IDB_SAVEICON), IMAGE_BITMAP,
                      TB_IMAGE_WIDTH, TB_IMAGE_HEIGHT, LR_DEFAULTCOLOR), RGB(255, 255, 255));

        ImageList_AddMasked(hImageList, LoadImage(hInstance, MAKEINTRESOURCE(IDB_PRINTICON), IMAGE_BITMAP,
                      TB_IMAGE_WIDTH, TB_IMAGE_HEIGHT, LR_DEFAULTCOLOR), RGB(255, 255, 255));

        ImageList_AddMasked(hImageList, LoadImage(hInstance, MAKEINTRESOURCE(IDB_ROT1ICON), IMAGE_BITMAP,
                      TB_IMAGE_WIDTH, TB_IMAGE_HEIGHT, LR_DEFAULTCOLOR), RGB(255, 255, 255));

        ImageList_AddMasked(hImageList, LoadImage(hInstance, MAKEINTRESOURCE(IDB_ROT2ICON), IMAGE_BITMAP,
                      TB_IMAGE_WIDTH, TB_IMAGE_HEIGHT, LR_DEFAULTCOLOR), RGB(255, 255, 255));

        if (hImageList == NULL) return FALSE;

        ImageList_Destroy((HIMAGELIST)SendMessage(hToolBar, TB_SETIMAGELIST,
                                                  0, (LPARAM)hImageList));

        SendMessage(hToolBar, TB_ADDBUTTONS,
                    numButtons, (LPARAM)Buttons);

        return TRUE;
    }

    return FALSE;
}

LRESULT CALLBACK
ImageView_DispWndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message)
    {
        case WM_PAINT:
        {
            ImageView_DrawImage(hwnd);
            return 0L;
        }
	}
    return CallWindowProc(PrevProc, hwnd, Message, wParam, lParam);
}

static VOID
ImageView_InitControls(HWND hwnd)
{
    MoveWindow(hwnd, shiSettings.Left, shiSettings.Top,
               shiSettings.Right - shiSettings.Left,
               shiSettings.Bottom - shiSettings.Top, TRUE);

    if (shiSettings.Maximized) ShowWindow(hwnd, SW_MAXIMIZE);

    hDispWnd = CreateWindowEx(0, _T("STATIC"), _T(""),
                              WS_CHILD | WS_VISIBLE,
                              0, 0, 0, 0, hwnd, NULL, hInstance, NULL);

    SetClassLong(hDispWnd, GCL_STYLE, CS_HREDRAW | CS_VREDRAW);
    PrevProc = (WNDPROC) SetWindowLong(hDispWnd, GWL_WNDPROC, (LPARAM) ImageView_DispWndProc);

    ImageView_CreateToolBar(hwnd);
}

LRESULT CALLBACK
ImageView_WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message)
    {
        case WM_CREATE:
        {
            ImageView_InitControls(hwnd);
            return 0L;
        }
        case WM_COMMAND:
        {
            switch (wParam)
            {
                case IDC_PREV:

                break;
                case IDC_NEXT:

                break;
                case IDC_ZOOMP:

                break;
                case IDC_ZOOMM:

                break;
                case IDC_SAVE:

                break;
                case IDC_PRINT:

                break;
                case IDC_ROT1:

                break;
                case IDC_ROT2:

                break;
            }
        }
        break;

        case WM_NOTIFY:
        {
            LPNMHDR pnmhdr = (LPNMHDR)lParam;

            switch (pnmhdr->code)
            {
                case TTN_GETDISPINFO:
                {
                    LPTOOLTIPTEXT lpttt;
                    UINT idButton;

                    lpttt = (LPTOOLTIPTEXT)lParam;
                    idButton = (UINT)lpttt->hdr.idFrom;

                    switch (idButton)
                    {
                        case IDC_PREV:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_PREV_PIC);
                        break;
                        case IDC_NEXT:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_NEXT_PIC);
                        break;
                        case IDC_ZOOMP:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_ZOOM_IN);
                        break;
                        case IDC_ZOOMM:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_ZOOM_OUT);
                        break;
                        case IDC_SAVE:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_SAVEAS);
                        break;
                        case IDC_PRINT:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_PRINT);
                        break;
                        case IDC_ROT1:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_ROT_COUNCW);
                        break;
                        case IDC_ROT2:
                            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_ROT_CLOCKW);
                        break;
                    }
                }
            }
            return TRUE;
        }
        case WM_SIZING:
        {
            LPRECT pRect = (LPRECT)lParam;
            if (pRect->right-pRect->left < 350)
                pRect->right = pRect->left + 350;

            if (pRect->bottom-pRect->top < 290)
                pRect->bottom = pRect->top + 290;
            return TRUE;
        }
        case WM_SIZE:
        {
            MoveWindow(hDispWnd, 1, 1, LOWORD(lParam)-1, HIWORD(lParam)-35, TRUE);
            SendMessage(hToolBar, TB_AUTOSIZE, 0, 0);
            return 0L;
        }
        case WM_DESTROY:
        {
            ImageView_SaveSettings(hwnd);
            SetWindowLong(hDispWnd, GWL_WNDPROC, (LPARAM) PrevProc);
            PostQuitMessage(0);
            break;
        }
    }

    return DefWindowProc(hwnd, Message, wParam, lParam);
}

LONG WINAPI
ImageView_CreateWindow(HWND hwnd, LPWSTR szFileName)
{
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    WNDCLASS WndClass = {0};
    TCHAR szBuf[512];
    HWND hMainWnd;
    MSG msg;

    if (!ImageView_LoadSettings())
    {
        shiSettings.Maximized = FALSE;
        shiSettings.Left      = 0;
        shiSettings.Top       = 0;
        shiSettings.Right     = 520;
        shiSettings.Bottom    = 400;
    }

    // Initialize GDI+
    gdiplusStartupInput.GdiplusVersion              = 1;
    gdiplusStartupInput.DebugEventCallback          = NULL;
    gdiplusStartupInput.SuppressBackgroundThread    = FALSE;
    gdiplusStartupInput.SuppressExternalCodecs      = FALSE;

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    pLoadImage(szFileName);

    // Create the window
    WndClass.lpszClassName  = _T("shimgvw_window");
    WndClass.lpfnWndProc    = (WNDPROC)ImageView_WndProc;
    WndClass.hInstance      = hInstance;
    WndClass.style          = CS_HREDRAW | CS_VREDRAW;
    WndClass.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));
    WndClass.hCursor        = LoadCursor(hInstance, IDC_ARROW);
    WndClass.hbrBackground  = (HBRUSH)COLOR_WINDOW;

    if (!RegisterClass(&WndClass)) return -1;

    LoadString(hInstance, IDS_APPTITLE, szBuf, sizeof(szBuf) / sizeof(TCHAR));
    hMainWnd = CreateWindow(_T("shimgvw_window"), szBuf,
                            WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CAPTION,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            0, 0, NULL, NULL, hInstance, NULL); 

    // Show it
    ShowWindow(hMainWnd, SW_SHOW);
    UpdateWindow(hMainWnd);

    // Message Loop
    while(GetMessage(&msg,NULL,0,0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (image)
        GdipDisposeImage(image);
    GdiplusShutdown(gdiplusToken);
    return -1;
}

VOID WINAPI
ImageView_FullscreenW(HWND hwnd, HINSTANCE hInst, LPCWSTR path, int nShow)
{
    ImageView_CreateWindow(hwnd, (LPWSTR)path);
}

VOID WINAPI
ImageView_Fullscreen(HWND hwnd, HINSTANCE hInst, LPCWSTR path, int nShow)
{
    ImageView_CreateWindow(hwnd, (LPWSTR)path);
}

VOID WINAPI
ImageView_FullscreenA(HWND hwnd, HINSTANCE hInst, LPCSTR path, int nShow)
{
    WCHAR szFile[MAX_PATH];

    if (MultiByteToWideChar(CP_ACP, 0, (char*)path, strlen((char*)path)+1, szFile, MAX_PATH))
    {
        ImageView_CreateWindow(hwnd, (LPWSTR)szFile);
    }
}

VOID WINAPI
ImageView_PrintTo(HWND hwnd, HINSTANCE hInst, LPCWSTR path, int nShow)
{
    DPRINT("ImageView_PrintTo() not implemented\n");
}

VOID WINAPI
ImageView_PrintToA(HWND hwnd, HINSTANCE hInst, LPCSTR path, int nShow)
{
    DPRINT("ImageView_PrintToA() not implemented\n");
}

VOID WINAPI
ImageView_PrintToW(HWND hwnd, HINSTANCE hInst, LPCWSTR path, int nShow)
{
    DPRINT("ImageView_PrintToW() not implemented\n");
}

VOID WINAPI
imageview_fullscreenW(HWND hwnd, HINSTANCE hInst, LPCWSTR path, int nShow)
{
    DPRINT("ImageView_fullscreenW() not implemented\n");
}

BOOL WINAPI
DllMain(IN HINSTANCE hinstDLL,
        IN DWORD dwReason,
        IN LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
            hInstance = hinstDLL;
            break;
    }

    return TRUE;
}

