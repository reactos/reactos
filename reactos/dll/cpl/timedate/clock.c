/* Code based on clock.c from Programming Windows, Charles Petzold */

#include <timedate.h>

#define TWOPI (2 * 3.14159)

static const TCHAR szClockWndClass[] = TEXT("ClockWndClass");
static HBRUSH hGreyBrush = NULL;
static HPEN hGreyPen = NULL;

static VOID 
SetIsotropic(HDC hdc, INT cxClient, INT cyClient)
{
    /* set isotropic mode */
     SetMapMode(hdc, MM_ISOTROPIC);
     /* position axis in centre of window */
     SetViewportOrgEx(hdc, cxClient / 2,  cyClient / 2, NULL);
}

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

static VOID 
DrawClock(HDC hdc)
{
     INT   iAngle;
     POINT pt[3];
     HBRUSH hBrushOld;
     HPEN hPenOld = NULL;

     /* grey brush to fill the dots */     
     hBrushOld = SelectObject(hdc, hGreyBrush);

     hPenOld = GetCurrentObject(hdc, OBJ_PEN);

     for(iAngle = 0; iAngle < 360; iAngle += 6)
     {
          /* starting coords */
          pt[0].x = 0;
          pt[0].y = 180;

          /* rotate start coords */
          RotatePoint(pt, 1, iAngle);

          /* determine whether it's a big dot or a little dot
           * i.e. 1-4 or 5, 6-9 or 10, 11-14 or 15 */
          if (iAngle % 5)
          {
                pt[2].x = pt[2].y = 7;                
                SelectObject(hdc, hGreyPen);
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
}

static VOID 
DrawHands(HDC hdc, SYSTEMTIME * pst, BOOL fChange)
{
     static POINT pt[3][5] = { {{0, -30}, {20, 0}, {0, 100}, {-20, 0}, {0, -30}},
                               {{0, -40}, {10, 0}, {0, 160}, {-10, 0}, {0, -40}},
                               {{0,   0}, { 0, 0}, {0,   0}, {  0, 0}, {0, 160}} };
     INT i, iAngle[3];
     POINT ptTemp[3][5];

     /* Black pen for outline, white brush for fill */
     SelectObject(hdc, GetStockObject(BLACK_PEN));
     SelectObject(hdc, GetStockObject(WHITE_BRUSH));

     iAngle[0] = (pst->wHour * 30) % 360 + pst->wMinute / 2;
     iAngle[1] =  pst->wMinute  *  6;
     iAngle[2] =  pst->wSecond  *  6;

     memcpy(ptTemp, pt, sizeof(pt));

     for(i = fChange ? 0 : 2; i < 3; i++)
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
    HDC hdc;
    PAINTSTRUCT ps;
    SYSTEMTIME st;

    switch (uMsg)
    {
        case WM_CREATE:
            hGreyPen = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
            hGreyBrush = CreateSolidBrush(RGB(128, 128, 128));
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

            InvalidateRect(hwnd, NULL, TRUE);

            stPrevious = st;
        break;

        case WM_PAINT:
            hdc = BeginPaint(hwnd, &ps);

            SetIsotropic(hdc, cxClient, cyClient);

            DrawClock(hdc);
            DrawHands(hdc, &stPrevious, TRUE);

            EndPaint(hwnd, &ps);
        break;

        case WM_DESTROY:
            DeleteObject(hGreyPen);
            DeleteObject(hGreyBrush);
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
