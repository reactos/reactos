/* Core functions lifted from Programming Windows, Charles Petzold */

#include "timedate.h"

#define TWOPI (2 * 3.14159)

static const TCHAR szClockWndClass[] = TEXT("ClockWndClass");


VOID SetIsotropic(HDC hdc, INT cxClient, INT cyClient)
{
     SetMapMode (hdc, MM_ISOTROPIC);
     SetWindowExtEx (hdc, 1000, 1000, NULL);
     SetViewportExtEx (hdc, cxClient / 2, -cyClient / 2, NULL);
     SetViewportOrgEx (hdc, cxClient / 2,  cyClient / 2, NULL);
}

VOID RotatePoint(POINT pt[], INT iNum, INT iAngle)
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

VOID DrawClock(HDC hdc)
{
     INT iAngle;
     POINT pt[3];

     for (iAngle = 0; iAngle < 360; iAngle += 6)
     {
          pt[0].x = 0;
          pt[0].y = 900;
          
          RotatePoint(pt, 1, iAngle);
          
          pt[2].x = pt[2].y = iAngle % 5 ? 33 : 100;
          
          pt[0].x -= pt[2].x / 2;
          pt[0].y -= pt[2].y / 2;
          
          pt[1].x  = pt[0].x + pt[2].x;
          pt[1].y  = pt[0].y + pt[2].y;
          
          SelectObject(hdc, GetStockObject (BLACK_BRUSH));
          
          Ellipse(hdc, pt[0].x, pt[0].y, pt[1].x, pt[1].y);
     }
}

VOID DrawHands(HDC hdc, SYSTEMTIME * pst, BOOL fChange)
{
    static POINT pt[3][5] = { {{0, -150}, {100, 0}, {0, 600}, {-100, 0}, {0, -150}},
                              {{0, -200}, { 50, 0}, {0, 800}, { -50, 0}, {0, -200}},
                              {{0,    0}, {  0, 0}, {0,   0}, {   0, 0}, {0,  800}} };
     INT i, iAngle[3];
     POINT ptTemp[3][5];
     
     iAngle[0] = (pst->wHour * 30) % 360 + pst->wMinute / 2;
     iAngle[1] =  pst->wMinute  *  6;
     iAngle[2] =  pst->wSecond  *  6;
     
     memcpy(ptTemp, pt, sizeof(pt));
     
     for (i = fChange ? 0 : 2 ; i < 3 ; i++)
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
    static INT cxClient, cyClient;
    static SYSTEMTIME stPrevious;
    BOOL fChange;
    HDC hdc;
    PAINTSTRUCT ps;
    SYSTEMTIME st;

    switch (uMsg)
    {
        case WM_CREATE:
            SetTimer(hwnd, ID_TIMER, 1000, NULL);
            GetLocalTime(&st);
            stPrevious = st;
        break;

        case WM_SIZE:
            cxClient = LOWORD(lParam);
            cyClient = HIWORD(lParam);
        break;

        case WM_TIMER:
            GetLocalTime(&st);

            fChange = st.wHour   != stPrevious.wHour ||
                    st.wMinute != stPrevious.wMinute;

            hdc = GetDC(hwnd);

            SetIsotropic(hdc, cxClient, cyClient);

            InvalidateRect(hwnd, NULL, TRUE);

            SelectObject(hdc, GetStockObject(BLACK_PEN));
            DrawHands(hdc, &st, TRUE);

            ReleaseDC(hwnd, hdc);

            stPrevious = st;
        break;

        case WM_PAINT:
            hdc = BeginPaint(hwnd, &ps);

            SetIsotropic(hdc, cxClient, cyClient);

            DrawClock(hdc);
            SelectObject(hdc, GetStockObject(WHITE_BRUSH));
            DrawHands(hdc, &stPrevious, TRUE);

            EndPaint(hwnd, &ps);
        break;

        case WM_DESTROY:
            KillTimer(hwnd, ID_TIMER);
        break;

        default:
            DefWindowProc(hwnd, 
                          uMsg, 
                          wParam, 
                          lParam);
    }

    return TRUE;
}



BOOL
InitClockWindowClass(VOID)
{
    WNDCLASSEX wc = {0};

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = ClockWndProc;
    wc.hInstance = hApplet;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = szClockWndClass;
    wc.hIconSm = NULL;

    return RegisterClassEx(&wc) != (ATOM)0;
}
