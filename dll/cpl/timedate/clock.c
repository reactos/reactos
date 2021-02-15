/*
 * PROJECT:     ReactOS Timedate Control Panel
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/timedate/clock.c
 * PURPOSE:     Draws the analog clock
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *              Copyright 2007 Eric Kohl
 *              Copyright 2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

/* Code based on clock.c from Programming Windows, Charles Petzold */
/* Modified by Katayama Hirofumi MZ */

#include "timedate.h"
#include <math.h>

typedef struct _CLOCKDATA
{
    INT cxClient;
    INT cyClient;
    INT Radius;
    POINT Center;
    HPEN hHourPen, hMinutePen, hSecondPen, hBoldPen;
    HBRUSH hBlueBrush, hGreyBrush;
    HBITMAP hbmBackScreen;
    SYSTEMTIME stCurrent;
    SYSTEMTIME stPrevious;
    BOOL bTimer;
} CLOCKDATA, *PCLOCKDATA;

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

static const WCHAR szClockWndClass[] = L"ClockWndClass";

static inline VOID
SetPolarCoodinates(LPPOINT pPoint, POINT Center, INT Radius, INT Angle)
{
    pPoint->x = round(Center.x + Radius * cos(Angle * (M_PI / 180)));
    pPoint->y = round(Center.y - Radius * sin(Angle * (M_PI / 180)));
}

static inline VOID
SetClockCoodinates(LPPOINT pPoint, POINT Center, INT Radius, INT Angle)
{
    SetPolarCoodinates(pPoint, Center, Radius, 90 - Angle);
}

static inline VOID
Line(HDC hDC, POINT pt0, POINT pt1, BOOL Stroke)
{
#if 1 /* FIXME: CORE-2527 workaround */
    BeginPath(hDC);
    MoveToEx(hDC, pt0.x, pt0.y, NULL);
    LineTo(hDC, pt1.x, pt1.y);
    EndPath(hDC);
    WidenPath(hDC);
    HRGN hRgn = PathToRegion(hDC);

    LOGPEN LogPen;
    GetObject(GetCurrentObject(hDC, OBJ_PEN), sizeof(LogPen), &LogPen);

    HBRUSH hbr = CreateSolidBrush(LogPen.lopnColor);
    FillRgn(hDC, hRgn, hbr);
    DeleteObject(hbr);

    if (Stroke)
    {
        FrameRgn(hDC, hRgn, GetStockObject(BLACK_BRUSH), 1, 1);
    }

    DeleteObject(hRgn);
#else
    MoveToEx(hDC, pt0.x, pt0.y, NULL);
    LineTo(hDC, pt1.x, pt1.y);
#endif
}

static VOID
ClockWnd_Resize(HWND hwnd, PCLOCKDATA pClockData, INT cxNew, INT cyNew)
{
    if (pClockData->cxClient >= cxNew && pClockData->cyClient >= cyNew)
        return;

    pClockData->cxClient = cxNew;
    pClockData->cyClient = cyNew;
    pClockData->Center.x = cxNew / 2;
    pClockData->Center.y = cyNew / 2;
    pClockData->Radius = min(cxNew, cyNew) / 2;

    HDC hDC = GetDC(hwnd);
    if (!hDC)
        return;

    HBITMAP hbmOld = pClockData->hbmBackScreen;
    pClockData->hbmBackScreen = CreateCompatibleBitmap(hDC, cxNew, cyNew);
    DeleteObject(hbmOld);

    ReleaseDC(hwnd, hDC);
}

static VOID
DrawClock(HDC hdc, PCLOCKDATA pClockData)
{
    POINT Points[2], Center = pClockData->Center;

    /* Fill background */
    RECT rc = { 0, 0, pClockData->cxClient, pClockData->cyClient };
    FillRect(hdc, &rc, (HBRUSH)(COLOR_3DFACE + 1));

    /* Draw a circle board */
    HGDIOBJ hOldPen = SelectObject(hdc, pClockData->hBoldPen);
    HGDIOBJ hOldBrush = SelectObject(hdc, GetStockObject(WHITE_BRUSH));
    INT Radius = pClockData->Radius, Margin = 3;
    Ellipse(hdc, Center.x - Radius + Margin, Center.y - Radius + Margin,
                 Center.x + Radius - Margin, Center.y + Radius - Margin);

    /* Draw dots */
    for (INT iAngle = 0; iAngle < 360; iAngle += 6)
    {
        if (iAngle % 5 == 0)
        {
            SetClockCoodinates(&Points[0], Center, Radius - 7, iAngle);
            SetClockCoodinates(&Points[1], Center, Radius - 13, iAngle);
            SelectObject(hdc, pClockData->hBoldPen);
            Line(hdc, Points[0], Points[1], FALSE);
        }
        else
        {
            SetClockCoodinates(&Points[1], Center, Radius - 8, iAngle);
            SelectObject(hdc, GetStockObject(NULL_PEN));
            SelectObject(hdc, pClockData->hGreyBrush);
            const INT cxy = 2;
            Ellipse(hdc, Points[1].x - cxy, Points[1].y - cxy, Points[1].x + cxy, Points[1].y + cxy);
        }
    }

    SelectObject(hdc, hOldPen);
    SelectObject(hdc, hOldBrush);
}

static VOID
DrawHands(HDC hdc, PCLOCKDATA pClockData)
{
    INT iAngle, Radius = pClockData->Radius;
    LPSYSTEMTIME pst = &pClockData->stPrevious;
    POINT Point, Center = pClockData->Center;

    /* The hour hand */
    HGDIOBJ hOldPen = SelectObject(hdc, pClockData->hHourPen);
    iAngle = (pst->wHour * 30) % 360 + pst->wMinute / 2;
    SetClockCoodinates(&Point, Center, Radius * 1 / 2, iAngle);
    Line(hdc, Center, Point, TRUE);

    /* The minute hand */
    SelectObject(hdc, pClockData->hMinutePen);
    iAngle = pst->wMinute * 6;
    SetClockCoodinates(&Point, Center, Radius * 3 / 4, iAngle);
    Line(hdc, Center, Point, TRUE);

    /* The second hand */
    SelectObject(hdc, pClockData->hSecondPen);
    iAngle = pst->wSecond * 6;
    SetClockCoodinates(&Point, Center, Radius * 5 / 6, iAngle);
    Line(hdc, Center, Point, FALSE);

    /* The center disc */
    HGDIOBJ hOldBrush = SelectObject(hdc, pClockData->hBlueBrush);
    const INT cxy = 5;
    Ellipse(hdc, Center.x - cxy, Center.y - cxy, Center.x + cxy, Center.y + cxy);
    SelectObject(hdc, hOldBrush);

    SelectObject(hdc, hOldPen);
}

static VOID
ClockWnd_OnDraw(HWND hwnd, HDC hdc, PCLOCKDATA pClockData)
{
    HDC hdcMem = CreateCompatibleDC(hdc);
    if (!hdcMem)
        return;

    HGDIOBJ hBmpOld = SelectObject(hdcMem, pClockData->hbmBackScreen);

    DrawClock(hdcMem, pClockData);
    DrawHands(hdcMem, pClockData);

    BitBlt(hdc, 0, 0, pClockData->cxClient, pClockData->cyClient,
           hdcMem, 0, 0, SRCCOPY);

    SelectObject(hdcMem, hBmpOld);
    DeleteDC(hdcMem);
}

static PCLOCKDATA
ClockWnd_CreateData(HWND hwnd)
{
    PCLOCKDATA pClockData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CLOCKDATA));
    if (!pClockData)
        return NULL;

    pClockData->hHourPen = CreatePen(PS_SOLID, 12, RGB(255, 0, 0)); /* Red */
    pClockData->hMinutePen = CreatePen(PS_SOLID, 8, RGB(0, 255, 0)); /* Green */
    pClockData->hSecondPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 255)); /* Blue */
    pClockData->hBoldPen = CreatePen(PS_SOLID, 3, RGB(0, 0, 0)); /* Black */
    pClockData->hBlueBrush = CreateSolidBrush(RGB(0, 0, 255)); /* Blue */
    pClockData->hGreyBrush = CreateSolidBrush(RGB(128, 128, 128)); /* Gray */

    SetTimer(hwnd, ID_TIMER, 1000, NULL);
    pClockData->bTimer = TRUE;

    GetLocalTime(&pClockData->stCurrent);
    pClockData->stPrevious = pClockData->stCurrent;

    return pClockData;
}

static VOID
ClockWnd_DestroyData(HWND hwnd, PCLOCKDATA pClockData)
{
    DeleteObject(pClockData->hHourPen);
    DeleteObject(pClockData->hMinutePen);
    DeleteObject(pClockData->hSecondPen);
    DeleteObject(pClockData->hBoldPen);
    DeleteObject(pClockData->hBlueBrush);
    DeleteObject(pClockData->hGreyBrush);
    DeleteObject(pClockData->hbmBackScreen);

    if (pClockData->bTimer)
        KillTimer(hwnd, ID_TIMER);

    HeapFree(GetProcessHeap(), 0, pClockData);
}

static LRESULT CALLBACK
ClockWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PCLOCKDATA pClockData = (PCLOCKDATA)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    HDC hdc;
    PAINTSTRUCT ps;

    switch (uMsg)
    {
        case WM_CREATE:
            pClockData = ClockWnd_CreateData(hwnd);
            if (!pClockData)
                return -1;

            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pClockData);
            break;

        case WM_SIZE:
            ClockWnd_Resize(hwnd, pClockData, LOWORD(lParam), HIWORD(lParam));
            break;

        case WM_TIMECHANGE:
        case WM_TIMER:
            GetLocalTime(&pClockData->stCurrent);
            InvalidateRect(hwnd, NULL, FALSE);
            pClockData->stPrevious = pClockData->stCurrent;
            break;

        case WM_PAINT:
            hdc = BeginPaint(hwnd, &ps);
            if (hdc)
            {
                ClockWnd_OnDraw(hwnd, hdc, pClockData);
                EndPaint(hwnd, &ps);
            }
            break;

        /* No need to erase background, handled during paint */
        case WM_ERASEBKGND:
            return 1;

        case WM_DESTROY:
            ClockWnd_DestroyData(hwnd, pClockData);
            break;

        case CLM_STOPCLOCK:
            if (pClockData->bTimer)
            {
                KillTimer(hwnd, ID_TIMER);
                pClockData->bTimer = FALSE;
            }
            break;

        case CLM_STARTCLOCK:
            if (!pClockData->bTimer)
            {
                SetTimer(hwnd, ID_TIMER, 1000, NULL);
                pClockData->bTimer = TRUE;
            }
            break;

        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

BOOL
RegisterClockControl(VOID)
{
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = ClockWndProc;
    wc.hInstance = hApplet;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.lpszClassName = szClockWndClass;

    return RegisterClassExW(&wc) != 0;
}

VOID
UnregisterClockControl(VOID)
{
    UnregisterClassW(szClockWndClass, hApplet);
}
