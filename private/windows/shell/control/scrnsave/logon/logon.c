
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntexapi.h>

#include <windows.h>
#include <scrnsave.h>
#include <logon.h>

// For IsOS()
#include <shlwapi.h>
#include <shlwapip.h>


HANDLE hInst;

int cxhwndLogon;
int cyhwndLogon;
int xScreen;        // the top-left corner of the screen, might be different than (0,0)
int yScreen;
int cxScreen;
int cyScreen;
HBRUSH hbrBlack;
HDC hdcLogon;
HWND hwndLogon;
HBITMAP hbmLogon = NULL;
HICON ghiconLogon = NULL;
HICON   hMovingIcon = NULL;
HPALETTE ghpal = NULL;

#define APPCLASS "LOGON"

#define MAX_CAPTION_LENGTH  128

DWORD FAR lRandom(VOID)
{
    static DWORD glSeed = (DWORD)-365387184;

    glSeed *= 69069;
    return(++glSeed);
}

HPALETTE GetPalette(HBITMAP hbm)
{
    DIBSECTION ds;
    int i;
    HANDLE hmem;
    HDC hdc, hdcMem;
    LOGPALETTE *ppal;
    HPALETTE hpal;
    RGBQUAD rgbquad[256];
    USHORT nColors;

    GetObject(hbm, sizeof(DIBSECTION), &ds);
    if (ds.dsBmih.biBitCount > 8)
        return NULL;

    nColors = (ds.dsBmih.biBitCount < 16) ? (1 << ds.dsBmih.biBitCount) : 0xffff;

    hmem = GlobalAlloc(GHND, sizeof (LOGPALETTE) + sizeof (PALETTEENTRY) * nColors);
    if (hmem == NULL)
        return NULL;

    ppal = (LPLOGPALETTE) GlobalLock(hmem);

    hdc = GetDC(NULL);
    hdcMem = CreateCompatibleDC(hdc);
    SelectObject(hdcMem, hbm);

    ppal->palVersion = 0x300;
    ppal->palNumEntries = nColors;
    GetDIBColorTable(hdcMem, 0, nColors, rgbquad);

    for (i = 0; i < nColors; i++) {
        ppal->palPalEntry[i].peRed = rgbquad[i].rgbRed;
        ppal->palPalEntry[i].peGreen = rgbquad[i].rgbGreen;
        ppal->palPalEntry[i].peBlue = rgbquad[i].rgbBlue;
    }

    hpal = CreatePalette(ppal);

    GlobalUnlock(hmem);
    GlobalFree(hmem);

    DeleteObject(hdcMem);
    ReleaseDC(NULL, hdc);

    return hpal;
}

BOOL APIENTRY
WndProc(
    HWND    hwnd,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam)
{
    int x, y;
    int nColorsChanged;

    switch (message) {
    PAINTSTRUCT ps;

    case WM_PALETTECHANGED:
        if ((HWND)wParam == hwnd)
            break;

    case WM_QUERYNEWPALETTE:
    {
        HDC hdc = GetDC(hwnd);
        SelectPalette(hdc, ghpal, FALSE);
        nColorsChanged = RealizePalette(hdc);
        ReleaseDC(hwnd, hdc);

        if (nColorsChanged != 0) {
            InvalidateRect(hwnd, NULL, TRUE);
        }
    }
    break;

    case WM_PAINT:
        BeginPaint(hwnd, &ps);
        SelectPalette(ps.hdc, ghpal, FALSE);
        BitBlt(ps.hdc, 0, 0, cxhwndLogon, cyhwndLogon, hdcLogon, 0, 0, SRCCOPY);
        EndPaint(hwnd, &ps);
        break;

    case WM_TIMER:
        /*
         * Pick a new place on the screen to put the dialog.
         */
        x = lRandom() % (cxScreen - cxhwndLogon) + xScreen;
        y = lRandom() % (cyScreen - cyhwndLogon) + yScreen;

        SetWindowPos(hwndLogon, NULL, x, y, 0, 0,
                SWP_NOSIZE | SWP_NOZORDER);
        break;

    case WM_CLOSE:
        ExitProcess(0);
        break;

    case WM_SETFOCUS:
        /*
         * Don't allow DefDlgProc() to do default processing on this
         * message because it'll set the focus to the first control and
         * we want it set to the main dialog so that DefScreenSaverProc()
         * will see the key input and cancel the screen saver.
         */
        return TRUE;
        break;

    /*
     * Call DefScreenSaverProc() so we get its default processing (so it
     * can detect key and mouse input).
     */
    default:
        return DefScreenSaverProc(hwnd, message, wParam, lParam) ? TRUE : FALSE;
    }

    return 0;
}

LRESULT APIENTRY
ScreenSaverProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT rc;
    HDC hdc;
    PAINTSTRUCT ps;
    static int sx;
    static int sy;
    BITMAP bm;

    switch (message) {
    case WM_CREATE:
        /*
         * Background window is black
         */

        /*
         * Make sure we use the entire virtual desktop size for multiple
         * displays:
         */
        cxScreen =  ((LPCREATESTRUCT)lParam)->cx;
        cyScreen =  ((LPCREATESTRUCT)lParam)->cy;
        xScreen =  ((LPCREATESTRUCT)lParam)->x;
        yScreen =  ((LPCREATESTRUCT)lParam)->y;

        hbrBlack = GetStockObject(BLACK_BRUSH);

        if (!fChildPreview) {
            WNDCLASS wndClass;


            if (hbmLogon == NULL) {
                LPTSTR res;

                if (IsOS(OS_WIN2000DATACENTER))
                {
                    res = MAKEINTRESOURCE(IDB_DATACENTER);
                }
                else if (IsOS(OS_SERVERAPPLIANCE))
                {
                    res = MAKEINTRESOURCE(IDB_SERVERAPP);
                }
                else if (IsOS(OS_WIN2000ADVSERVER))
                {
                    res = MAKEINTRESOURCE(IDB_ADVANCED);
                }
                else if (IsOS(OS_WIN2000SERVER))
                {
                    res = MAKEINTRESOURCE(IDB_SERVER);
                }
                else
                {
                    res = MAKEINTRESOURCE(IDB_WORKSTA);
                }

                hbmLogon = LoadImage(hMainInstance, res, IMAGE_BITMAP, 0, 0,
                        LR_CREATEDIBSECTION);

                ghpal = GetPalette(hbmLogon);
            }

            GetObject(hbmLogon, sizeof(bm), &bm);
            cxhwndLogon = bm.bmWidth;
            cyhwndLogon = bm.bmHeight;

            hdcLogon = CreateCompatibleDC(NULL);
            SelectObject(hdcLogon, hbmLogon);


            wndClass.style         = CS_HREDRAW | CS_VREDRAW;
            wndClass.lpfnWndProc   = (WNDPROC)WndProc;
            wndClass.cbClsExtra    = 0;
            wndClass.cbWndExtra    = sizeof(LONG);
            wndClass.hInstance     = hInst;
            wndClass.hIcon         = NULL;
            wndClass.hCursor       = NULL;
            wndClass.hbrBackground = NULL;
            wndClass.lpszMenuName  = NULL;
            wndClass.lpszClassName = TEXT("LOGON");

            RegisterClass(&wndClass);


            /*
             * Create the window we'll move around every 10 seconds.
             */
            hwndLogon = CreateWindowEx( WS_EX_TOPMOST, TEXT("LOGON"), NULL, WS_VISIBLE | WS_POPUP,
                    50, 50, cxhwndLogon, cyhwndLogon, hMainWindow, NULL, hInst, NULL);

            SetTimer(hwndLogon, 1, 10 * 1000, 0);


            /*
             * Post this message so we activate after this window is created.
             */
            PostMessage(hwnd, WM_USER, 0, 0);
        } else {
            SetTimer(hwnd, 1, 10 * 1000, 0);

            cxhwndLogon = GetSystemMetrics(SM_CXICON);
            cyhwndLogon = GetSystemMetrics(SM_CYICON);

            ghiconLogon = LoadIcon(hMainInstance, MAKEINTRESOURCE(1));

            sx = lRandom() % (cxScreen - cxhwndLogon) + xScreen;
            sy = lRandom() % (cyScreen - cyhwndLogon) + yScreen;
        }

        break;

    case WM_SIZE:
        cxScreen = LOWORD(lParam);
        cyScreen = HIWORD(lParam);
        break;

    case WM_WINDOWPOSCHANGING:
        /*
         * Take down hwndLogon if this window is going invisible.
         */
        if (hwndLogon == NULL)
            break;

        if (((LPWINDOWPOS)lParam)->flags & SWP_HIDEWINDOW) {
            ShowWindow(hwndLogon, SW_HIDE);
        }
        break;

    case WM_USER:
        /*
         * Now show and activate this window.
         */
        if (hwndLogon == NULL)
            break;

        SetWindowPos(hwndLogon, NULL, 0, 0, 0, 0, SWP_SHOWWINDOW |
                SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER);
        break;

    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        SetRect(&rc, xScreen, yScreen, cxScreen, cyScreen);
        FillRect(hdc, &rc, hbrBlack);

        if (fChildPreview) {
            DrawIcon(hdc, sx, sy, ghiconLogon);
        }

        EndPaint(hwnd, &ps);
        break;

    case WM_NCACTIVATE:
        /*
         * Case out WM_NCACTIVATE so the dialog activates: DefScreenSaverProc
         * returns FALSE for this message, not allowing activation.
         */
        if (!fChildPreview)
            return DefWindowProc(hwnd, message, wParam, lParam);
        break;

    case WM_TIMER:
        /*
         * Pick a new place on the screen to put the dialog.
         */
        sx = lRandom() % (cxScreen - cxhwndLogon) + xScreen;
        sy = lRandom() % (cyScreen - cyhwndLogon) + yScreen;
        InvalidateRect(hwnd, NULL, TRUE);
        break;
    }

    return DefScreenSaverProc(hwnd, message, wParam, lParam);
}

BOOL APIENTRY
ScreenSaverConfigureDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    TCHAR ach1[256];
    TCHAR ach2[256];

    switch (message) {
    case WM_INITDIALOG:
        /*
         * This is hack-o-rama, but fast and cheap.
         */
        LoadString(hMainInstance, IDS_DESCRIPTION, ach1, sizeof(ach1));
        LoadString(hMainInstance, 2, ach2, sizeof(ach2));

        MessageBox(hDlg, ach2, ach1, MB_OK | MB_ICONEXCLAMATION);

        EndDialog(hDlg, TRUE);
        break;
    }
    return FALSE;
}

BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{
    return TRUE;
}
