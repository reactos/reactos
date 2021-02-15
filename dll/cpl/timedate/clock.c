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
    HWND hwnd;

    HPEN hNormalPen, hBoldPen, hHeavyPen, hGreyPen;
    HBRUSH hGreyBrush;
    HBITMAP hbmBackScreen;

    INT cxClient, cyClient;
    BOOL bTimer;
    INT Radius;
    POINT Center;

    SYSTEMTIME stCurrent, stPrevious;
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
Line(HDC hDC, POINT pt0, POINT pt1)
{
#if 1 /* FIXME: CORE-2527 workaround */
    BeginPath(hDC);
    MoveToEx(hDC, pt0.x, pt0.y, NULL);
    LineTo(hDC, pt1.x, pt1.y);
    EndPath(hDC);
    WidenPath(hDC);

    LOGPEN LogPen;
    GetObject(GetCurrentObject(hDC, OBJ_PEN), sizeof(LogPen), &LogPen);

    HBRUSH hbr = CreateSolidBrush(LogPen.lopnColor);
    HGDIOBJ hbrOld = SelectObject(hDC, hbr);

    FillPath(hDC);

    SelectObject(hDC, hbrOld);
    DeleteObject(hbr);
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
        SetClockCoodinates(&Points[0], Center, Radius - 7, iAngle);
        if (iAngle % 5 == 0)
        {
            SetClockCoodinates(&Points[1], Center, Radius - 13, iAngle);
            SelectObject(hdc, pClockData->hBoldPen);
        }
        else
        {
            SetClockCoodinates(&Points[1], Center, Radius - 10, iAngle);
            SelectObject(hdc, pClockData->hGreyPen);
        }
        Line(hdc, Points[0], Points[1]);
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
    HGDIOBJ hOldPen = SelectObject(hdc, pClockData->hHeavyPen);
    iAngle = (pst->wHour * 30) % 360 + pst->wMinute / 2;
    SetClockCoodinates(&Point, Center, Radius * 1 / 2, iAngle);
    Line(hdc, Center, Point);
    SelectObject(hdc, pClockData->hGreyPen);
    Line(hdc, Center, Point);

    /* The minute hand */
    SelectObject(hdc, pClockData->hBoldPen);
    iAngle = pst->wMinute * 6;
    SetClockCoodinates(&Point, Center, Radius * 3 / 4, iAngle);
    Line(hdc, Center, Point);
    SelectObject(hdc, pClockData->hGreyPen);
    Line(hdc, Center, Point);

    /* The second hand */
    SelectObject(hdc, pClockData->hGreyPen);
    iAngle = pst->wSecond * 6;
    SetClockCoodinates(&Point, Center, Radius * 5 / 6, iAngle);
    Line(hdc, Center, Point);

    /* The center disc */
    HGDIOBJ hOldBrush = SelectObject(hdc, GetStockObject(BLACK_BRUSH));
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
    PCLOCKDATA pClockData;
    pClockData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CLOCKDATA));
    if (!pClockData)
        return NULL;

    pClockData->hwnd = hwnd;
    pClockData->hGreyPen = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
    pClockData->hGreyBrush = CreateSolidBrush(RGB(128, 128, 128));
    pClockData->hNormalPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    pClockData->hBoldPen = CreatePen(PS_SOLID, 3, RGB(0, 0, 0));
    pClockData->hHeavyPen = CreatePen(PS_SOLID, 5, RGB(0, 0, 0));

    SetTimer(hwnd, ID_TIMER, 1000, NULL);
    pClockData->bTimer = TRUE;

    GetLocalTime(&pClockData->stCurrent);
    pClockData->stPrevious = pClockData->stCurrent;

    return pClockData;
}

static VOID
ClockWnd_DestroyData(HWND hwnd, PCLOCKDATA pClockData)
{
    pClockData->hwnd = NULL;
    DeleteObject(pClockData->hGreyPen);
    DeleteObject(pClockData->hGreyBrush);
    DeleteObject(pClockData->hbmBackScreen);
    DeleteObject(pClockData->hNormalPen);
    DeleteObject(pClockData->hBoldPen);
    DeleteObject(pClockData->hHeavyPen);

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
