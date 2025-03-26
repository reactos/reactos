#include <windows.h>
#include <uxtheme.h>
#include <stdlib.h>
#include <tchar.h>
#include "resource.h"

LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

HICON hico;
HBITMAP hbmp;
HIMAGELIST himl;
POINT scPos;

HBRUSH hbrNULL;
HBRUSH hbrRed;
HBRUSH hbrGreen;
HBRUSH hbrBlue;
HBRUSH hbrCyan;
HBRUSH hbrYellow;

HBRUSH hbrCtlColorStatic;
HBRUSH hbrCtlColorBtn;
HBRUSH hbrPrintClientClear;
HBRUSH hbrErase;

BOOL bSkipErase;
BOOL bSkipPaint;

static void RegisterMyClass(HINSTANCE hInst)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInst;
	wcex.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_WIN32PROJECT1));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = NULL;
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_MENU);
	wcex.lpszClassName = L"ButtonTests";
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	RegisterClassExW(&wcex);
}

#define TOP_MARGIN 50
#define LEFT_MARGIN 160
#define X_GAP 10
#define Y_GAP 10
#define Y_HEIGHT 40
#define X_WIDTH 150

static HWND CreateWnd(HWND hWnd, LPCWSTR Class, LPCWSTR Text, LONG style, int i, int j)
{
	return 	CreateWindowW(Class,
						  Text,
						  style | WS_CHILD | WS_VISIBLE,
						  LEFT_MARGIN + (X_GAP + X_WIDTH) * i,
						  TOP_MARGIN + ((Y_GAP + Y_HEIGHT)  *j),
						  X_WIDTH,
						  Y_HEIGHT,
						  hWnd, NULL, NULL, NULL);
}

static HWND CreateBtn(HWND hWnd, LPCWSTR Text, LONG style, int i, int j)
{
    WCHAR buffer[100];
    SIZE s;

	HWND ret = CreateWnd(hWnd, L"Button", Text, style, i, j);
    if (GetWindowLongW(ret, GWL_STYLE) != (style | WS_CHILD | WS_VISIBLE))
    {
        swprintf(buffer, L"expected 0x%x got 0x%x", (style | WS_CHILD | WS_VISIBLE), GetWindowLongW(ret, GWL_STYLE));
        MessageBox(0, buffer, L"error", MB_OK);
    }

    if (SendMessageW(ret, BCM_GETIDEALSIZE, 0, (LPARAM)&s))
    {
        swprintf(buffer, L"%s (%d, %d)", Text, s.cx, s.cy);
        SendMessageW(ret, WM_SETTEXT, 0, (LPARAM)buffer);
    }
    return ret;
}

static void CreateButtonSet(HWND hwndParent, HWND *ahwnd, int i, int j, DWORD style)
{
	ahwnd[0] = CreateBtn(hwndParent, L"TestButton", style, i, j + 0);
	ahwnd[1] = CreateBtn(hwndParent, L"TestButton1", style, i, j + 1);
	ahwnd[2] = CreateBtn(hwndParent, L"TestButton2", style, i, j + 2);
	ahwnd[3] = CreateBtn(hwndParent, L"TestButton3", style | BS_BITMAP, i, j + 3);
	ahwnd[4] = CreateBtn(hwndParent, L"TestButton4", style | BS_ICON, i, j + 4);

	SendMessageW(ahwnd[1], BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbmp);
	SendMessageW(ahwnd[2], BM_SETIMAGE, IMAGE_ICON, (LPARAM)hico);

	SendMessageW(ahwnd[3], BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbmp);
	SendMessageW(ahwnd[4], BM_SETIMAGE, IMAGE_ICON, (LPARAM)hico);
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	HWND hwnd[200];
	HANDLE hActCtx;
	ULONG_PTR cookie;
	BOOL bActivated;

	ACTCTXW actctx = { sizeof(actctx) };
    actctx.hModule = hInstance;
    actctx.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID| ACTCTX_FLAG_HMODULE_VALID;
    actctx.lpResourceName = MAKEINTRESOURCEW(500);
	hActCtx = CreateActCtxW(&actctx);

	hico = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_WIN32PROJECT1));
	hbmp = LoadBitmapW(hInstance, MAKEINTRESOURCEW(IDB_BITMAP1));

    scPos.x = 0;
    scPos.y = 0;

    hbrNULL = (HBRUSH)GetStockObject (NULL_BRUSH);
    hbrRed = CreateSolidBrush(0x00000FF);
    hbrGreen = CreateSolidBrush(0x0000FF00);
    hbrBlue = CreateSolidBrush(0x00FF0000);
    hbrCyan = CreateSolidBrush(0x00FFFF00);
    hbrYellow = CreateSolidBrush(0x0000FFFF);

    hbrCtlColorStatic = hbrRed;
    hbrCtlColorBtn = hbrCyan;
    hbrPrintClientClear = hbrYellow;
    hbrErase = hbrGreen;

    bSkipErase = FALSE;
    bSkipPaint = FALSE;

	RegisterMyClass(hInstance);

	HWND hWnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"ButtonTests", L"Button tests", WS_OVERLAPPEDWINDOW| WS_HSCROLL| WS_VSCROLL,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	CreateWnd(hWnd, L"Static", L"no images", 0, -1, 0);
	CreateWnd(hWnd, L"Static", L"with BM_SETIMAGE", 0, -1, 1);
	CreateWnd(hWnd, L"Static", L"with BM_SETIMAGE", 0, -1, 2);
	CreateWnd(hWnd, L"Static", L"with BM_SETIMAGE and BS_BITMAP", 0, -1, 3);
	CreateWnd(hWnd, L"Static", L"with BM_SETIMAGE and BS_ICON", 0, -1, 4);

	CreateWnd(hWnd, L"Static", L"Button V5", 0, 0, -1);
	CreateButtonSet(hWnd, &hwnd[0],  0, 0,  BS_PUSHBUTTON);
	CreateButtonSet(hWnd, &hwnd[5],  1, 0,  BS_DEFPUSHBUTTON);
    CreateButtonSet(hWnd, &hwnd[10], 0, 5,  BS_PUSHBUTTON|WS_DISABLED);
	CreateButtonSet(hWnd, &hwnd[15], 1, 5,  BS_GROUPBOX);
	CreateButtonSet(hWnd, &hwnd[20], 0, 10, BS_CHECKBOX);
	CreateButtonSet(hWnd, &hwnd[25], 1, 10, BS_RADIOBUTTON);

	bActivated = ActivateActCtx(hActCtx, &cookie);
    LoadLibraryW(L"comctl32.dll");

    himl = ImageList_LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_BITMAP2), 16, 0, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION);
    BUTTON_IMAGELIST btniml = {himl, {1,1,1,1}, BUTTON_IMAGELIST_ALIGN_LEFT};

	CreateWnd(hWnd, L"Static", L"Button V6 without themes", 0, 2, -1);
	CreateButtonSet(hWnd, &hwnd[30], 2, 0,  BS_PUSHBUTTON);
	CreateButtonSet(hWnd, &hwnd[35], 3, 0,  BS_DEFPUSHBUTTON);
    CreateButtonSet(hWnd, &hwnd[40], 2, 5,  BS_PUSHBUTTON|WS_DISABLED);
	CreateButtonSet(hWnd, &hwnd[55], 3, 5,  BS_GROUPBOX);
	CreateButtonSet(hWnd, &hwnd[50], 2, 10, BS_CHECKBOX);
	CreateButtonSet(hWnd, &hwnd[55], 3, 10, BS_RADIOBUTTON);

	CreateWnd(hWnd, L"Static", L"Button V6 with imagelist and no themes", 0, 4, -1);
	CreateButtonSet(hWnd, &hwnd[60], 4, 0,  BS_PUSHBUTTON);
	CreateButtonSet(hWnd, &hwnd[65], 5, 0,  BS_DEFPUSHBUTTON);
	CreateButtonSet(hWnd, &hwnd[70], 4, 5,  BS_PUSHBUTTON|WS_DISABLED);
	CreateButtonSet(hWnd, &hwnd[75], 5, 5,  BS_GROUPBOX);
	CreateButtonSet(hWnd, &hwnd[80], 4, 10, BS_CHECKBOX);
	CreateButtonSet(hWnd, &hwnd[85], 5, 10, BS_RADIOBUTTON);

	for (int i = 30; i < 90; i++)
		SetWindowTheme(hwnd[i], L"", L"");

	for (int i = 60; i< 90; i++)
		SendMessageW(hwnd[i], BCM_SETIMAGELIST, 0, (LPARAM)&btniml);

	CreateWnd(hWnd, L"Static", L"Button V6 with themes and imagelist", 0, 6, -1);
	CreateButtonSet(hWnd, &hwnd[120], 6, 0,  BS_PUSHBUTTON);
	CreateButtonSet(hWnd, &hwnd[125], 7, 0,  BS_DEFPUSHBUTTON);
    CreateButtonSet(hWnd, &hwnd[130], 6, 5,  BS_PUSHBUTTON|WS_DISABLED);
	CreateButtonSet(hWnd, &hwnd[135], 7, 5,  BS_GROUPBOX);
	CreateButtonSet(hWnd, &hwnd[140], 6, 10, BS_CHECKBOX);
	CreateButtonSet(hWnd, &hwnd[145], 7, 10, BS_RADIOBUTTON);

	CreateWnd(hWnd, L"Static", L"Button V6 with themes", 0, 8, -1);
	CreateButtonSet(hWnd, &hwnd[90],  8, 0,  BS_PUSHBUTTON);
	CreateButtonSet(hWnd, &hwnd[95],  9, 0,  BS_DEFPUSHBUTTON);
    CreateButtonSet(hWnd, &hwnd[100], 8, 5,  BS_PUSHBUTTON|WS_DISABLED);
	CreateButtonSet(hWnd, &hwnd[105], 9, 5,  BS_GROUPBOX);
	CreateButtonSet(hWnd, &hwnd[110], 8, 10, BS_CHECKBOX);
	CreateButtonSet(hWnd, &hwnd[115], 9, 10, BS_RADIOBUTTON);

	for (int i = 120; i< 150; i++)
		SendMessageW(hwnd[i], BCM_SETIMAGELIST, 0, (LPARAM)&btniml);

	if (bActivated) DeactivateActCtx(0, cookie);

    SCROLLINFO vsi = {sizeof(SCROLLINFO), SIF_ALL, 0, 2 * TOP_MARGIN + 10 * (Y_GAP + Y_HEIGHT), Y_HEIGHT, 0, 0};
    SCROLLINFO hsi = {sizeof(SCROLLINFO), SIF_ALL, 0, 2 * LEFT_MARGIN + 13 * (X_GAP + X_WIDTH), X_WIDTH, 0, 0};

    SetScrollInfo(hWnd, SB_HORZ, &hsi, FALSE);
    SetScrollInfo(hWnd, SB_VERT, &vsi, FALSE);

    ShowScrollBar(hWnd, SB_HORZ, TRUE);
    ShowScrollBar(hWnd, SB_VERT, TRUE);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int) msg.wParam;
}

static VOID
OnScroll(HWND hwnd, INT nBar, WORD sbCode)
{
    RECT rect;

    SCROLLINFO sInfo;
    INT oldPos, Maximum;
    PLONG pOriginXY;

    //ASSERT(nBar == SB_HORZ || nBar == SB_VERT);

    GetClientRect(hwnd, &rect);

    if (nBar == SB_HORZ)
    {
        Maximum =1000;//pData->cxMin - (rect.right-rect.left) /* pData->cxOld */;
        pOriginXY = &scPos.x;
    }
    else // if (nBar == SB_VERT)
    {
        Maximum = 1000;//pData->cyMin - (rect.bottom-rect.top) /* pData->cyOld */;
        pOriginXY = &scPos.y;
    }

    /* Set scrollbar sizes */
    sInfo.cbSize = sizeof(sInfo);
    sInfo.fMask = SIF_RANGE | SIF_POS | SIF_PAGE | SIF_TRACKPOS;

    if (!GetScrollInfo(hwnd, nBar, &sInfo))
        return;

    oldPos = sInfo.nPos;

    switch (sbCode)
    {
        case SB_LINEUP:   // SB_LINELEFT:
            sInfo.nPos--;
            break;

        case SB_LINEDOWN: // SB_LINERIGHT:
            sInfo.nPos++;
            break;

        case SB_PAGEUP:   // SB_PAGELEFT:
            sInfo.nPos -= sInfo.nPage;
            break;

        case SB_PAGEDOWN: // SB_PAGERIGHT:
            sInfo.nPos += sInfo.nPage;
            break;

        case SB_THUMBTRACK:
            sInfo.nPos = sInfo.nTrackPos;
            break;

        case SB_THUMBPOSITION:
            sInfo.nPos = sInfo.nTrackPos;
            break;

        case SB_TOP:    // SB_LEFT:
            sInfo.nPos = sInfo.nMin;
            break;

        case SB_BOTTOM: // SB_RIGHT:
            sInfo.nPos = sInfo.nMax;
            break;

        default:
            break;
    }

    sInfo.nPos = min(max(sInfo.nPos, 0), Maximum);

    if (oldPos != sInfo.nPos)
    {
        POINT scOldPos = scPos;

        /* We now modify scPos */
        *pOriginXY = sInfo.nPos;

        ScrollWindowEx(hwnd,
                       (scOldPos.x - scPos.x),
                       (scOldPos.y - scPos.y),
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       SW_INVALIDATE | SW_ERASE | SW_SCROLLCHILDREN);

        sInfo.fMask = SIF_POS;
        SetScrollInfo(hwnd, nBar, &sInfo, TRUE);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT rc = {0,0,5000,5000};
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_CTLCOLORSTATIC:
        return (LRESULT)hbrCtlColorStatic;
    case WM_CTLCOLORBTN:
        return (LRESULT)hbrCtlColorBtn;
    case WM_ERASEBKGND:
        FillRect((HDC)wParam, &rc, hbrErase);
        return TRUE;
    case WM_PRINTCLIENT:
        FillRect((HDC)wParam, &rc, hbrPrintClientClear);
        break;
    case WM_HSCROLL:
        OnScroll(hWnd, SB_HORZ, LOWORD(wParam));
        break;
    case WM_VSCROLL:
        OnScroll(hWnd, SB_VERT, LOWORD(wParam));
        break;
    case WM_DRAWITEM :
    {
        DRAWITEMSTRUCT* di = (DRAWITEMSTRUCT*)lParam;
        INT oldBkMode;
        INT state = (di->itemState == ODS_SELECTED) ? DFCS_BUTTONPUSH|DFCS_PUSHED : DFCS_BUTTONPUSH;
        DrawFrameControl( di->hDC, &di->rcItem, DFC_BUTTON, state );
        oldBkMode = SetBkMode(di->hDC, TRANSPARENT);
        DrawTextW(di->hDC, L"Ownder drawn text", -1, &di->rcItem, DT_VCENTER | DT_CENTER);
        SetBkMode(di->hDC, oldBkMode);
        break;
    }
    case WM_NOTIFY:
    {
        NMHDR* phdr = (NMHDR*)lParam;
        if (phdr->code == NM_CUSTOMDRAW)
        {
            LPNMCUSTOMDRAW lpNMCustomDraw = (LPNMCUSTOMDRAW) lParam;
            if (lpNMCustomDraw->dwDrawStage == CDDS_PREERASE && bSkipErase)
                return CDRF_SKIPDEFAULT;
            else if (lpNMCustomDraw->dwDrawStage == CDDS_PREPAINT && bSkipPaint)
                return CDRF_SKIPDEFAULT;
            return CDRF_DODEFAULT;
        }
    }
    case WM_COMMAND:
    {
        UINT id = LOWORD(wParam);
        switch(id)
        {
            case IDM_NULL_WIN:   hbrErase = hbrNULL;  break;
            case IDM_RED_WIN:    hbrErase = hbrRed;  break;
            case IDM_GREEN_WIN:  hbrErase = hbrGreen;  break;
            case IDM_BLUE_WIN:   hbrErase = hbrBlue;  break;
            case IDM_YELLOW_WIN: hbrErase = hbrYellow;  break;
            case IDM_CYAN_WIN:   hbrErase = hbrCyan;  break;

            case IDM_NULL_STATIC:   hbrCtlColorStatic = hbrNULL;  break;
            case IDM_RED_STATIC:    hbrCtlColorStatic = hbrRed;  break;
            case IDM_GREEN_STATIC:  hbrCtlColorStatic = hbrGreen;  break;
            case IDM_BLUE_STATIC:   hbrCtlColorStatic = hbrBlue;  break;
            case IDM_YELLOW_STATIC: hbrCtlColorStatic = hbrYellow;  break;
            case IDM_CYAN_STATIC:   hbrCtlColorStatic = hbrCyan;  break;

            case IDM_NULL_BTN:      hbrCtlColorBtn = hbrNULL;  break;
            case IDM_RED_BTN:       hbrCtlColorBtn = hbrRed;  break;
            case IDM_GREEN_BTN:     hbrCtlColorBtn = hbrGreen;  break;
            case IDM_BLUE_BTN:      hbrCtlColorBtn = hbrBlue;  break;
            case IDM_YELLOW_BTN:    hbrCtlColorBtn = hbrYellow;  break;
            case IDM_CYAN_BTN:      hbrCtlColorBtn = hbrCyan;  break;

            case IDM_NULL_PRINTCLNT:    hbrPrintClientClear = hbrNULL;  break;
            case IDM_RED_PRINTCLNT:     hbrPrintClientClear = hbrRed;  break;
            case IDM_GREEN_PRINTCLNT:   hbrPrintClientClear = hbrGreen;  break;
            case IDM_BLUE_PRINTCLNT:    hbrPrintClientClear = hbrBlue;  break;
            case IDM_YELLOW_PRINTCLNT:  hbrPrintClientClear = hbrYellow;  break;
            case IDM_CYAN_PRINTCLNT:    hbrPrintClientClear = hbrCyan;  break;

            case IDM_SKIP_ERASE:    bSkipErase = !bSkipErase; break;
            case IDM_SKIP_PAINT:    bSkipPaint = !bSkipPaint; break;
            default:
                return 0;
        }

        InvalidateRect(hWnd, NULL, TRUE);
        break;
    }
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
