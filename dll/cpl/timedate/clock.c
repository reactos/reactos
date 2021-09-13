/*
 * PROJECT:     ReactOS Timedate Control Panel
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/timedate/clock.c
 * PURPOSE:     Draws the analog clock
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *              Copyright 2007 Eric Kohl
 */

/* Code based on clock.c from Programming Windows, Charles Petzold */

#include "timedate.h"

#include <math.h>

typedef struct _CLOCKDATA
{
    HBRUSH hGreyBrush;
    HPEN hGreyPen;
    INT cxClient;
    INT cyClient;
    SYSTEMTIME stCurrent;
    SYSTEMTIME stPrevious;
    BOOL bTimer;
} CLOCKDATA, *PCLOCKDATA;


#define TWOPI (2 * 3.14159)

static const WCHAR szClockWndClass[] = L"ClockWndClass";

static VOID
RotatePoint(POINT pt[], INT iNum, INT iAngle)
{
     INT i;
     POINT ptTemp;

     for (i = 0 ; i < iNum ; i++)
     {
          ptTemp.x = (INT) (pt[i].x * cos (TWOPI * iAngle / 360) +
               pt[i].y * sin (TWOPI * iAngle / 360));

          ptTemp.y = (INT) (pt[i].y * cos (TWOPI * iAngle / 360) -
               pt[i].x * sin (TWOPI * iAngle / 360));

          pt[i] = ptTemp;
     }
}


static INT
DrawClock(HDC hdc, PCLOCKDATA pClockData)
{
     INT iAngle,Radius;
     POINT pt[3];
     HBRUSH hBrushOld;
     HPEN hPenOld = NULL;

     /* Grey brush to fill the dots */
     hBrushOld = SelectObject(hdc, pClockData->hGreyBrush);

     hPenOld = GetCurrentObject(hdc, OBJ_PEN);

     // TODO: Check if this conversion is correct resp. usable
     Radius = min(pClockData->cxClient,pClockData->cyClient) * 2;

     for (iAngle = 0; iAngle < 360; iAngle += 6)
     {
          /* Starting coords */
          pt[0].x = 0;
          pt[0].y = Radius;

          /* Rotate start coords */
          RotatePoint(pt, 1, iAngle);

          /* Determine whether it's a big dot or a little dot
           * i.e. 1-4 or 5, 6-9 or 10, 11-14 or 15 */
          if (iAngle % 5)
          {
                pt[2].x = pt[2].y = 7;
                SelectObject(hdc, pClockData->hGreyPen);
          }
          else
          {
              pt[2].x = pt[2].y = 16;
              SelectObject(hdc, GetStockObject(BLACK_PEN));
          }

          pt[0].x -= pt[2].x / 2;
          pt[0].y -= pt[2].y / 2;

          pt[1].x  = pt[0].x + pt[2].x;
          pt[1].y  = pt[0].y + pt[2].y;

          Ellipse(hdc, pt[0].x, pt[0].y, pt[1].x, pt[1].y);
     }

     SelectObject(hdc, hBrushOld);
     SelectObject(hdc, hPenOld);
     return Radius;
}


static VOID
DrawHands(HDC hdc, SYSTEMTIME * pst, BOOL fChange, INT Radius)
{
     POINT pt[3][5] = { {{0, (INT)-Radius/6}, {(INT)Radius/9, 0},
	     {0, (INT)Radius/1.8}, {(INT)-Radius/9, 0}, {0, (INT)-Radius/6}},
     {{0, (INT)-Radius/4.5}, {(INT)Radius/18, 0}, {0, (INT) Radius*0.89},
	     {(INT)-Radius/18, 0}, {0, (INT)-Radius/4.5}},
     {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, (INT) Radius*0.89}} };
     INT i, iAngle[3];
     POINT ptTemp[3][5];

     /* Black pen for outline, white brush for fill */
     SelectObject(hdc, GetStockObject(BLACK_PEN));
     SelectObject(hdc, GetStockObject(WHITE_BRUSH));

     iAngle[0] = (pst->wHour * 30) % 360 + pst->wMinute / 2;
     iAngle[1] = pst->wMinute * 6;
     iAngle[2] = pst->wSecond * 6;

     CopyMemory(ptTemp, pt, sizeof(pt));

     for (i = fChange ? 0 : 2; i < 3; i++)
     {
          RotatePoint(ptTemp[i], 5, iAngle[i]);

          Polygon(hdc, ptTemp[i], 5);
     }
}


static LRESULT CALLBACK
ClockWndProc(HWND hwnd,
             UINT uMsg,
             WPARAM wParam,
             LPARAM lParam)
{
    PCLOCKDATA pClockData;
    HDC hdc, hdcMem;
    PAINTSTRUCT ps;

    pClockData = (PCLOCKDATA)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_CREATE:
            pClockData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CLOCKDATA));
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pClockData);

            pClockData->hGreyPen = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
            pClockData->hGreyBrush = CreateSolidBrush(RGB(128, 128, 128));

            GetLocalTime(&pClockData->stCurrent);
            pClockData->stPrevious = pClockData->stCurrent;

            pClockData->bTimer = (SetTimer(hwnd, ID_TIMER, 1000 - pClockData->stCurrent.wMilliseconds, NULL) != 0);
            break;

        case WM_SIZE:
            pClockData->cxClient = LOWORD(lParam);
            pClockData->cyClient = HIWORD(lParam);
            break;

        case WM_TIMECHANGE:
        case WM_TIMER:
            GetLocalTime(&pClockData->stCurrent);
            InvalidateRect(hwnd, NULL, FALSE);
            pClockData->stPrevious = pClockData->stCurrent;

            // Reset timeout.
            if (pClockData->bTimer)
            {
                SetTimer(hwnd, ID_TIMER, 1000 - pClockData->stCurrent.wMilliseconds, NULL);
            }
            break;

        case WM_PAINT:
            hdc = BeginPaint(hwnd, &ps);

            hdcMem = CreateCompatibleDC(hdc);
            if (hdcMem)
            {
                HBITMAP hBmp, hBmpOld;

                hBmp = CreateCompatibleBitmap(hdc,
                                              pClockData->cxClient,
                                              pClockData->cyClient);
                if (hBmp)
                {
                    RECT rcParent;
                    HWND hParentWnd = GetParent(hwnd);
                    INT oldMap, Radius;
                    POINT oldOrg;

                    hBmpOld = SelectObject(hdcMem, hBmp);

                    SetRect(&rcParent, 0, 0, pClockData->cxClient, pClockData->cyClient);
                    MapWindowPoints(hwnd, hParentWnd, (POINT*)&rcParent, 2);
                    OffsetViewportOrgEx(hdcMem, -rcParent.left, -rcParent.top, &oldOrg);
                    SendMessage(hParentWnd, WM_PRINT, (WPARAM)hdcMem, PRF_ERASEBKGND | PRF_CLIENT);
                    SetViewportOrgEx(hdcMem, oldOrg.x, oldOrg.y, NULL);

                    oldMap = SetMapMode(hdcMem, MM_ISOTROPIC);
                    SetWindowExtEx(hdcMem, 3600, 2700, NULL);
                    SetViewportExtEx(hdcMem, 800, -600, NULL);
                    SetViewportOrgEx(hdcMem,
                                     pClockData->cxClient / 2,
                                     pClockData->cyClient / 2,
                                     &oldOrg);

                    Radius = DrawClock(hdcMem, pClockData);
                    DrawHands(hdcMem, &pClockData->stPrevious, TRUE, Radius);

                    SetMapMode(hdcMem, oldMap);
                    SetViewportOrgEx(hdcMem, oldOrg.x, oldOrg.y, NULL);

                    BitBlt(hdc,
                           0,
                           0,
                           pClockData->cxClient,
                           pClockData->cyClient,
                           hdcMem,
                           0,
                           0,
                           SRCCOPY);

                    SelectObject(hdcMem, hBmpOld);
                    DeleteObject(hBmp);
                }

                DeleteDC(hdcMem);
            }

            EndPaint(hwnd, &ps);
            break;

        /* No need to erase background, handled during paint */
        case WM_ERASEBKGND:
            return 1;

        case WM_DESTROY:
            DeleteObject(pClockData->hGreyPen);
            DeleteObject(pClockData->hGreyBrush);

            if (pClockData->bTimer)
                KillTimer(hwnd, ID_TIMER);

            HeapFree(GetProcessHeap(), 0, pClockData);
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
                SYSTEMTIME LocalTime;

                GetLocalTime(&LocalTime);
                pClockData->bTimer = (SetTimer(hwnd, ID_TIMER, 1000 - LocalTime.wMilliseconds, NULL) != 0);
            }
            break;

        default:
            DefWindowProcW(hwnd,
                           uMsg,
                           wParam,
                           lParam);
    }

    return TRUE;
}


BOOL
RegisterClockControl(VOID)
{
    WNDCLASSEXW wc = {0};

    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = ClockWndProc;
    wc.hInstance = hApplet;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = szClockWndClass;

    return RegisterClassExW(&wc) != (ATOM)0;
}


VOID
UnregisterClockControl(VOID)
{
    UnregisterClassW(szClockWndClass,
                     hApplet);
}
